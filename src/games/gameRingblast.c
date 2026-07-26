#include "../cglp.h"

int* ringblastTitle = "RING BLAST";
int* ringblastDescription = "[D-Pad]\n Change angle/speed";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] ringblastCharacters = {
    {
        " bbbb ",
        "bBBBBb",
        "bBBBBb",
        "bBBBBb",
        "bBBBBb",
        " bbbb ",
    },
    {
        " rrrr ",
        "rRRRRr",
        "rRRRRr",
        "rRRRRr",
        "rRRRRr",
        " rrrr ",
    },
};
int ringblastCharactersCount = 2;

Options ringblastOptions = {100, 150, 3, false};

#define RINGBLAST_ANGLE_STEP 0.03
#define RINGBLAST_SPEED_STEP 0.05

struct RingblastStone {
  Vector pos;
  Vector vel;
  int side; // 0 = player, 1 = enemy, 2 = blasting (about to become a ring)
  int blastTicks;
  bool isAlive;
};
#define RINGBLAST_MAX_STONE_COUNT 64
RingblastStone[RINGBLAST_MAX_STONE_COUNT] ringblastStones;
int ringblastStoneIndex;

struct RingblastRing {
  float radius;
  float angle;
  float angleWidth;
  float angleWidthVel;
  bool isAlive;
};
#define RINGBLAST_MAX_RING_COUNT 32
RingblastRing[RINGBLAST_MAX_RING_COUNT] ringblastRings;
int ringblastRingIndex;

float ringblastNextStoneTime;
int ringblastStoneCount;
float ringblastStoneAngle;
float ringblastStoneSpeed;
float ringblastEnemyStoneAngle;
float ringblastEnemyStoneAngleVel;
float ringblastEnemyStoneSpeed;
float ringblastEnemyStoneSpeedVel;
Vector ringblastCenterPos = {50, 60};
int ringblastMultiplier;

void ringblastAddCollidingVelocity(RingblastStone* s, RingblastStone* as, Vector* v) {
  float a = angleTo(&s->pos, as->pos.x, as->pos.y);
  float pr = fabs(cos(vectorAngle(v) - a)) * 0.7;
  addWithAngle(&as->vel, a, vectorLength(v) * pr);
  addWithAngle(&s->vel, a, -vectorLength(v) * pr);
}

void ringblastUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(ringblastStones);
    ringblastStoneIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(ringblastRings);
    ringblastRingIndex = 0;
    ringblastNextStoneTime = 1;
    ringblastStoneCount = 10;
    ringblastStoneAngle = -CGLP_PI / 2;
    ringblastStoneSpeed = 1;
    ringblastEnemyStoneAngle = CGLP_PI / 2;
    ringblastEnemyStoneAngleVel = 1;
    ringblastEnemyStoneSpeed = 1;
    ringblastEnemyStoneSpeedVel = 1;
    ringblastMultiplier = 1;
  }
  float sd = sqrt(difficulty);
  color = LIGHT_PURPLE;
  rect(0, 0, 10, 150, &scratch);
  rect(90, 0, 10, 150, &scratch);
  color = LIGHT_BLACK;
  rect(10, 119, 80, 1, &scratch);
  box(50, 60, 3, 3, &scratch);
  rect(0, 144, (ringblastStoneCount + ringblastNextStoneTime) * 6, 6, &scratch);
  color = BLACK;
  if (ticks < 99) {
    text("Use D-pad", 22, 130, &scratch);
  }
  TIMES(ringblastStoneCount, ii) {
    character("a", ii * 6 + 3, 147, &scratch);
  }

  // Upstream aims via mouse-drag; redesigned to d-pad (left/right = angle, up/down = speed).
  if (input.left.isPressed) {
    ringblastStoneAngle -= RINGBLAST_ANGLE_STEP;
  }
  if (input.right.isPressed) {
    ringblastStoneAngle += RINGBLAST_ANGLE_STEP;
  }
  ringblastStoneAngle = clamp(ringblastStoneAngle, -CGLP_PI / 2 - CGLP_PI / 3, -CGLP_PI / 2 + CGLP_PI / 3);
  if (input.up.isPressed) {
    ringblastStoneSpeed += RINGBLAST_SPEED_STEP;
  }
  if (input.down.isPressed) {
    ringblastStoneSpeed -= RINGBLAST_SPEED_STEP;
  }
  ringblastStoneSpeed = clamp(ringblastStoneSpeed, 1, 4);
  thickness = 2;
  barCenterPosRatio = 0;
  bar(50, 120, ringblastStoneSpeed * 5, ringblastStoneAngle, &scratch);
  if (ringblastStoneCount > 0) {
    character("a", 50, 120, &scratch);
  }

  ringblastEnemyStoneAngle += ringblastEnemyStoneAngleVel * rnd(0.02, 0.03) * sd;
  if ((ringblastEnemyStoneAngle > CGLP_PI / 2 + CGLP_PI / 3 && ringblastEnemyStoneAngleVel > 0) ||
      (ringblastEnemyStoneAngle < CGLP_PI / 2 - CGLP_PI / 3 && ringblastEnemyStoneAngleVel < 0)) {
    ringblastEnemyStoneAngleVel *= -1;
  }
  ringblastEnemyStoneSpeed += ringblastEnemyStoneSpeedVel * rnd(0.03, 0.04) * sd;
  if ((ringblastEnemyStoneSpeed > 4 && ringblastEnemyStoneSpeedVel > 0) ||
      (ringblastEnemyStoneSpeed < 1 && ringblastEnemyStoneSpeedVel < 0)) {
    ringblastEnemyStoneSpeedVel *= -1;
  }
  thickness = 2;
  barCenterPosRatio = 0;
  bar(50, 0, ringblastEnemyStoneSpeed * 5, ringblastEnemyStoneAngle, &scratch);
  character("b", 50, 0, &scratch);

  int blastStoneCount = 0;
  FOR_EACH(ringblastStones, bi) {
    ASSIGN_ARRAY_ITEM(ringblastStones, bi, RingblastStone, bs);
    SKIP_IS_NOT_ALIVE(bs);
    if (bs->side == 2) {
      blastStoneCount++;
    }
  }

  ringblastNextStoneTime -= 0.01 * sd;
  if (ringblastNextStoneTime <= 0) {
    if (ringblastStoneCount == 0) {
      COUNT_IS_ALIVE(ringblastRings, ringsAliveCount);
      if (ringsAliveCount == 0 && blastStoneCount == 0) {
        play(EXPLOSION);
        gameOver();
      } else {
        ringblastNextStoneTime = 0;
      }
    } else {
      play(LASER);
      ringblastNextStoneTime = 1;
      ringblastStoneCount--;
      ASSIGN_ARRAY_ITEM(ringblastStones, ringblastStoneIndex, RingblastStone, ps);
      vectorSet(&ps->pos, 50, 120);
      vectorSet(&ps->vel, ringblastStoneSpeed, 0);
      rotate(&ps->vel, ringblastStoneAngle);
      ps->side = 0;
      ps->blastTicks = 60;
      ps->isAlive = true;
      ringblastStoneIndex = cgl_wrap(ringblastStoneIndex + 1, 0, RINGBLAST_MAX_STONE_COUNT);
      ASSIGN_ARRAY_ITEM(ringblastStones, ringblastStoneIndex, RingblastStone, es);
      vectorSet(&es->pos, 50, 0);
      vectorSet(&es->vel, ringblastEnemyStoneSpeed, 0);
      rotate(&es->vel, ringblastEnemyStoneAngle);
      es->side = 1;
      es->blastTicks = 60;
      es->isAlive = true;
      ringblastStoneIndex = cgl_wrap(ringblastStoneIndex + 1, 0, RINGBLAST_MAX_STONE_COUNT);
    }
  }

  color = PURPLE;
  FOR_EACH(ringblastRings, ri) {
    ASSIGN_ARRAY_ITEM(ringblastRings, ri, RingblastRing, r);
    SKIP_IS_NOT_ALIVE(r);
    r->angleWidth += r->angleWidthVel;
    thickness = 3;
    arc(ringblastCenterPos.x, ringblastCenterPos.y, r->radius, r->angle - r->angleWidth, r->angle + r->angleWidth, &scratch);
    r->radius += 2;
    if (r->radius > 99) {
      r->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  FOR_EACH(ringblastStones, si) {
    ASSIGN_ARRAY_ITEM(ringblastStones, si, RingblastStone, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    vectorMul(&s->vel, 0.98);
    if ((s->pos.x < 13 && s->vel.x < 0) || (s->pos.x > 87 && s->vel.x > 0)) {
      s->vel.x *= -1;
    }
    if ((s->pos.y < 3 && s->vel.y < 0) || (s->pos.y > 117 && s->vel.y > 0)) {
      s->vel.y *= -1;
    }
    int ss = s->side;
    if (ss == 2) {
      s->blastTicks--;
      if (s->blastTicks < 1) {
        play(COIN);
        float radius = distanceTo(&s->pos, ringblastCenterPos.x, ringblastCenterPos.y);
        ASSIGN_ARRAY_ITEM(ringblastRings, ringblastRingIndex, RingblastRing, nr);
        nr->radius = radius;
        nr->angle = angleTo(&ringblastCenterPos, s->pos.x, s->pos.y);
        nr->angleWidth = 0;
        nr->angleWidthVel = 1.0 / (radius + 1);
        nr->isAlive = true;
        ringblastRingIndex = cgl_wrap(ringblastRingIndex + 1, 0, RINGBLAST_MAX_RING_COUNT);
        s->isAlive = false;
        continue;
      }
      ss = (ticks / s->blastTicks) % 2;
    }
    int[2] stoneChar;
    stoneChar[0] = 'a' + ss;
    stoneChar[1] = 0;
    character(stoneChar, s->pos.x, s->pos.y, &scratch);
    if (scratch.isColliding.rect[PURPLE] && s->side < 2) {
      play(POWER_UP);
      if (s->side == 0) {
        ringblastStoneCount++;
      }
      if (s->side == 1) {
        color = RED;
      } else {
        color = BLUE;
      }
      particle(s->pos.x, s->pos.y, 16, 1, 0, CGLP_PI * 2);
      addScore(ringblastMultiplier, s->pos.x, s->pos.y);
      ringblastMultiplier++;
      s->isAlive = false;
      continue;
    }
    FOR_EACH(ringblastStones, sj) {
      if (sj == si) {
        continue;
      }
      ASSIGN_ARRAY_ITEM(ringblastStones, sj, RingblastStone, as);
      SKIP_IS_NOT_ALIVE(as);
      if (distanceTo(&s->pos, as->pos.x, as->pos.y) < 6) {
        Vector v;
        v = s->vel;
        Vector asVel;
        asVel = as->vel;
        ringblastAddCollidingVelocity(s, as, &asVel);
        ringblastAddCollidingVelocity(as, s, &v);
        vectorAdd(&s->pos, s->vel.x, s->vel.y);
        vectorAdd(&s->pos, s->vel.x, s->vel.y);
        vectorAdd(&as->pos, as->vel.x, as->vel.y);
        vectorAdd(&as->pos, as->vel.x, as->vel.y);
        if (s->side == 1 && as->side == 0) {
          play(CLICK);
          s->side = 2;
        } else if (as->side == 1 && s->side == 0) {
          play(CLICK);
          as->side = 2;
        } else {
          play(HIT);
        }
      }
    }
  }
  COUNT_IS_ALIVE(ringblastRings, ringsAliveEnd);
  if (ringsAliveEnd == 0) {
    ringblastMultiplier = 1;
  }
}

void addGameRingblast() {
  addGame(ringblastTitle, ringblastDescription, ringblastCharacters,
          ringblastCharactersCount, &ringblastOptions, false, &ringblastUpdate);
}
