#include "random.h"
#include "time.h"

// Vircon32 port note: this int pattern is really an unsigned 32-bit value
// (per the xorshift algorithm) squeezed into a signed 32-bit word. ">>" on
// Vircon32 is always a logical (zero-fill) shift no matter the type, so the
// xorshift math below is bit-for-bit identical to the original uint32_t
// version. The only place the sign actually matters is converting the final
// value to a float in [0,1) - a raw (float) cast would treat a negative bit
// pattern as a negative number, so uintToFloat() below corrects for that by
// adding 2^32 when the top bit is set.

float uintToFloat(int value) {
  float f = (float)value;
  if (value < 0) {
    f += 4294967296.0;
  }
  return f;
}

int nextRandom(Random* random) {
  int t = random->x ^ (random->x << 11);
  random->x = random->y;
  random->y = random->z;
  random->z = random->w;
  random->w = random->w ^ (random->w >> 19) ^ (t ^ (t >> 8));
  return random->w;
}

float getRandom(Random* random, float low, float high) {
  return uintToFloat(nextRandom(random)) / 4294967295.0 * (high - low) + low;
}

int getIntRandom(Random* random, int low, int high) {
  if (low == high) {
    return low;
  }
  // nextRandom() is treated as unsigned for the modulo too, otherwise a
  // negative bit pattern would make the result negative.
  int r = nextRandom(random);
  int m = high - low;
  int rem;
  if (r >= 0) {
    rem = r % m;
  } else {
    // r's bit pattern represents the unsigned value U = r + 2^32, and we
    // want U mod m. Neither 2^31 nor 2^32 fit in a signed 32-bit int
    // (2^31 = 2147483648 is already one past INT_MAX), so they can't be
    // written as int literals or produced by an int cast - the previous
    // version tried "(int)(2147483648.0)", which is itself an out-of-range
    // cast (undefined/overflow behaviour) and was corrupting the result.
    // Instead, build everything from 2^30 (1073741824), which *does* fit:
    //   lowPart = r & 0x7FFFFFFF   -- always >= 0, equals r + 2^31
    //   U = lowPart + 2^31
    //   2^31 mod m = ((2^30 mod m) * 2) mod m -- computed with only small,
    //                                             non-negative values
    int lowPart = r & 0x7FFFFFFF;
    int half = 1073741824; // 2^30
    int twoPow31ModM = ((half % m) * 2) % m;
    rem = ((lowPart % m) + twoPow31ModM) % m;
  }
  return rem + low;
}

int getPlusOrMinusRandom(Random* random) {
  return getIntRandom(random, 0, 2) * 2 - 1;
}

void setRandomSeed(Random* random, int w) {
  int loopCount = 32;
  random->x = 123456789;
  random->y = 362436069;
  random->z = 521288629;
  random->w = w;
  for (int i = 0; i < loopCount; i++) {
    nextRandom(random);
  }
}

void setRandomSeedWithTime(Random* random) {
  // Vircon32 has no wall-clock time(NULL); combine the calendar time with
  // the free-running cycle counter so seeds differ between runs/frames.
  setRandomSeed(random, get_time() ^ get_cycle_counter());
}
