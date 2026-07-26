#include "../cglp.h"

int* slalomTitle = "SLALOM";
int* slalomDescription = "[Hold] Turn";

// Vircon32 port note: upstream's characters array is empty (only
// bar()/box()/line() are used, no character() calls) - blank dummy entry,
// charactersCount = 0, matching the convention in gamePinClimb.c etc.
int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] slalomCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int slalomCharactersCount = 0;

Options slalomOptions = {100, 100, 14, false};

struct SlalomWall {
  float y;
  int side;
  float length;
  bool isAlive;
};
#define SLALOM_MAX_WALL_COUNT 16
SlalomWall[SLALOM_MAX_WALL_COUNT] slalomWalls;
int slalomWallIndex;

struct SlalomRock {
  Vector p;
  Vector s;
  bool isAlive;
};
#define SLALOM_MAX_ROCK_COUNT 32
SlalomRock[SLALOM_MAX_ROCK_COUNT] slalomRocks;
int slalomRockIndex;

int slalomWallSide;
float slalomWallAppDist;
float slalomScrolling;
Vector slalomPos;
Vector slalomVel;
float slalomAngle;
float slalomMinDist;
// Vircon32 port note: upstream's targetWall is a live reference to one of
// the objects in the (growable) walls array, which keeps being the same
// object across frames until either a newer/closer wall takes over or it
// scrolls away. There are no live references across this port's fixed
// arrays, so it's tracked as an index + a "do we have one at all" flag
// instead (same pattern as gameAccelb.c's AccelbPlayerMissile target). Each
// frame the candidate target is recomputed as the alive wall with the
// largest `y` that is still above the player (< pos.y + 9) - since every
// alive wall advances by the same `scrolling` amount every frame, age order
// and descending-y order are identical, so "the oldest surviving wall that
// hasn't yet reached the player" (upstream's first-match-in-array-order
// semantics) is exactly "the largest y under the threshold" - a selection
// that doesn't depend on the walls array's physical (ring-buffer) order at
// all, sidestepping any worry about index reuse across a long play session.
bool slalomHasTargetWall;
int slalomTargetWallIndex;
float slalomRockAppDist;

void slalomUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(slalomWalls);
    slalomWallIndex = 0;
    slalomWallSide = 1;
    slalomWallAppDist = 0;
    slalomScrolling = 0;
    vectorSet(&slalomPos, 90, 90);
    slalomAngle = -CGLP_PI / 4 * 3;
    vectorSet(&slalomVel, 1, 0);
    rotate(&slalomVel, slalomAngle);
    slalomMinDist = 99;
    slalomHasTargetWall = false;
    slalomTargetWallIndex = -1;
    INIT_UNALIVED_ARRAY_FAST(slalomRocks);
    slalomRockIndex = 0;
    slalomRockAppDist = 0;
  }

  slalomRockAppDist -= slalomScrolling;
  if (slalomRockAppDist < 0) {
    float x;
    if (rnd(0, 1) < 0.5) {
      x = rndi(-5, 5);
    } else {
      x = rndi(95, 105);
    }
    ASSIGN_ARRAY_ITEM(slalomRocks, slalomRockIndex, SlalomRock, nr);
    vectorSet(&nr->p, x, -9);
    vectorSet(&nr->s, rndi(9, 19), rndi(5, 9) * 2);
    nr->isAlive = true;
    slalomRockIndex = cgl_wrap(slalomRockIndex + 1, 0, SLALOM_MAX_ROCK_COUNT);
    slalomRockAppDist += 10;
  }
  color = PURPLE;
  FOR_EACH(slalomRocks, ri) {
    ASSIGN_ARRAY_ITEM(slalomRocks, ri, SlalomRock, r);
    SKIP_IS_NOT_ALIVE(r);
    r->p.y += slalomScrolling;
    box(r->p.x, r->p.y, r->s.x, r->s.y, &scratch);
    r->isAlive = r->p.y < 109;
  }

  slalomWallAppDist -= slalomScrolling;
  if (slalomWallAppDist < 0) {
    play(SELECT);
    ASSIGN_ARRAY_ITEM(slalomWalls, slalomWallIndex, SlalomWall, nw);
    nw->y = -5;
    nw->side = slalomWallSide;
    nw->length = rndi(40, 60);
    nw->isAlive = true;
    slalomWallIndex = cgl_wrap(slalomWallIndex + 1, 0, SLALOM_MAX_WALL_COUNT);
    slalomWallSide *= -1;
    slalomWallAppDist += rndi(80, 90);
  }
  color = RED;
  int candidateIndex = -1;
  float candidateY = -1000000;
  FOR_EACH(slalomWalls, wi) {
    ASSIGN_ARRAY_ITEM(slalomWalls, wi, SlalomWall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->y += slalomScrolling;
    float bx;
    if (w->side == 1) {
      bx = 99 - w->length / 2;
    } else {
      bx = w->length / 2;
    }
    box(bx, w->y, w->length, 4, &scratch);
    if (w->y < slalomPos.y + 9 && w->y > candidateY) {
      candidateY = w->y;
      candidateIndex = wi;
    }
    w->isAlive = w->y < 105;
  }
  bool newHasTarget = candidateIndex != -1;
  bool targetChanged;
  if (newHasTarget != slalomHasTargetWall) {
    targetChanged = true;
  } else if (newHasTarget && candidateIndex != slalomTargetWallIndex) {
    targetChanged = true;
  } else {
    targetChanged = false;
  }
  if (targetChanged) {
    slalomHasTargetWall = newHasTarget;
    slalomTargetWallIndex = candidateIndex;
    float s = floor(100 / (sqrt(slalomMinDist) + 1)) - 15;
    if (s > 0) {
      play(COIN);
      addScore(s, slalomPos.x, slalomPos.y);
    }
    slalomMinDist = 99;
  }

  color = BLACK;
  float targetSide;
  if (slalomHasTargetWall) {
    targetSide = slalomWalls[slalomTargetWallIndex].side;
  } else {
    targetSide = 0;
  }
  if (input.isPressed) {
    slalomAngle += targetSide * 0.07 * difficulty;
    particle(slalomPos.x, slalomPos.y, 1, vectorLength(&slalomVel), slalomAngle + CGLP_PI, 0.2);
  }
  vectorMul(&slalomVel, 1 - 0.02 / difficulty);
  Vector accel;
  vectorSet(&accel, 0.03, 0);
  rotate(&accel, slalomAngle);
  vectorAdd(&slalomVel, accel.x, accel.y);
  vectorAdd(&slalomPos, slalomVel.x, slalomVel.y);
  slalomScrolling = 0;
  if (slalomPos.y < 88) {
    slalomScrolling += (88 - slalomPos.y) * 0.5;
  }
  slalomPos.y += slalomScrolling;

  thickness = 2;
  Vector corner;
  vectorSet(&corner, 2, 2);
  rotate(&corner, slalomAngle);
  bar(slalomPos.x + corner.x, slalomPos.y + corner.y, 1, slalomAngle, &scratch);
  vectorSet(&corner, -2, 2);
  rotate(&corner, slalomAngle);
  bar(slalomPos.x + corner.x, slalomPos.y + corner.y, 1, slalomAngle, &scratch);
  vectorSet(&corner, 2, -2);
  rotate(&corner, slalomAngle);
  bar(slalomPos.x + corner.x, slalomPos.y + corner.y, 1, slalomAngle, &scratch);
  vectorSet(&corner, -2, -2);
  rotate(&corner, slalomAngle);
  bar(slalomPos.x + corner.x, slalomPos.y + corner.y, 1, slalomAngle, &scratch);

  color = BLUE;
  thickness = 3;
  bar(slalomPos.x, slalomPos.y, 4, slalomAngle, &scratch);
  bool outOfBounds = !(slalomPos.x >= 0 && slalomPos.x <= 99 && slalomPos.y >= 0 && slalomPos.y <= 99);
  if (scratch.isColliding.rect[RED] || scratch.isColliding.rect[PURPLE] || outOfBounds) {
    play(EXPLOSION);
    gameOver();
  }

  float wy;
  float wlength;
  int wside;
  if (slalomHasTargetWall) {
    wy = slalomWalls[slalomTargetWallIndex].y;
    wlength = slalomWalls[slalomTargetWallIndex].length;
    wside = slalomWalls[slalomTargetWallIndex].side;
  } else {
    wy = 0;
    wlength = 0;
    wside = 1;
  }
  float wpx;
  if (wside == 1) {
    wpx = 99 - wlength;
  } else {
    wpx = wlength;
  }
  Vector wp;
  vectorSet(&wp, wpx, wy);
  float d = distanceTo(&wp, slalomPos.x, slalomPos.y);
  if (d < slalomMinDist) {
    slalomMinDist = d;
  }
}

void addGameSlalom() {
  addGame(slalomTitle, slalomDescription, slalomCharacters, slalomCharactersCount,
          &slalomOptions, false, &slalomUpdate);
}
