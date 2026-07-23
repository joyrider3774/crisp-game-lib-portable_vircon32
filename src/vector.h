#ifndef VECTOR_H
#define VECTOR_H

// Vircon32 port note: "typedef struct { ... } Vector;" (anonymous struct +
// typedef) is not accepted by the Vircon32 C parser. A named struct works
// directly as a type (no "struct" keyword needed at use sites), so we
// declare it that way instead.
struct Vector {
  float x;
  float y;
};

Vector* vectorSet(Vector* vec, float x, float y);
Vector* vectorAdd(Vector* vec, float x, float y);
Vector* vectorMul(Vector* vec, float v);
Vector* rotate(Vector* vec, float angle);
Vector* addWithAngle(Vector* vec, float angle, float length);
float angleTo(Vector* vec, float x, float y);
float distanceTo(Vector* vec, float x, float y);
float vectorAngle(Vector* vec);
float vectorLength(Vector* vec);
// Vircon32's atan2 crashes when both arguments are exactly 0 - use this
// instead of atan2() directly anywhere a (0,0) case is plausible.
float cgl_atan2(float y, float x);

// Expand 'v' to 'v.x, v.y'
#define VEC_XY(v) v.x, v.y

#endif
