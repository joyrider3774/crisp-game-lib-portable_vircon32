#include "vector.h"
#include "math.h"

// Vircon32 port note: math.h only provides sin/cos/sqrt/atan2 (no "f" suffix
// versions - float is the only floating type in this dialect anyway).

// Vircon32's atan2 crashes when both arguments are exactly 0 (e.g. asking
// for the angle between two objects at the exact same position, or of a
// zero-length velocity vector - both routine in these games). Standard
// atan2(0, 0) conventionally returns 0 anyway, so guarding it here keeps
// the normal semantics and just avoids the crash.
float cgl_atan2(float y, float x) {
  if (y == 0 && x == 0) {
    return 0;
  }
  return atan2(y, x);
}

Vector* vectorSet(Vector* vec, float x, float y) {
  vec->x = x;
  vec->y = y;
  return vec;
}

Vector* vectorAdd(Vector* vec, float x, float y) {
  vec->x += x;
  vec->y += y;
  return vec;
}

Vector* vectorMul(Vector* vec, float v) {
  vec->x *= v;
  vec->y *= v;
  return vec;
}

Vector* rotate(Vector* vec, float angle) {
  float tx = vec->x;
  vec->x = tx * cos(angle) - vec->y * sin(angle);
  vec->y = tx * sin(angle) + vec->y * cos(angle);
  return vec;
}

Vector* addWithAngle(Vector* vec, float angle, float length) {
  vec->x += cos(angle) * length;
  vec->y += sin(angle) * length;
  return vec;
}

float angleTo(Vector* vec, float x, float y) {
  return cgl_atan2(y - vec->y, x - vec->x);
}

float distanceTo(Vector* vec, float x, float y) {
  float ox = x - vec->x;
  float oy = y - vec->y;
  return sqrt(ox * ox + oy * oy);
}

float vectorAngle(Vector* vec) { return cgl_atan2(vec->y, vec->x); }

float vectorLength(Vector* vec) {
  return sqrt(vec->x * vec->x + vec->y * vec->y);
}
