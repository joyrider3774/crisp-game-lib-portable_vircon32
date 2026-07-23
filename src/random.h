#ifndef RANDOM_H
#define RANDOM_H

// Vircon32 port note: no uint32_t / unsigned int in this dialect - every int
// is a 32-bit signed word. The xorshift generator below still works
// correctly because Vircon32's ">>" is ALWAYS a logical (zero-fill) shift
// regardless of type (see dialect notes in random.c), which is exactly the
// behaviour this generator relies on.
struct Random {
  int x;
  int y;
  int z;
  int w;
};

int nextRandom(Random* random);
float getRandom(Random* random, float low, float high);
int getIntRandom(Random* random, int low, int high);
int getPlusOrMinusRandom(Random* random);
void setRandomSeed(Random* random, int w);
void setRandomSeedWithTime(Random* random);

#endif
