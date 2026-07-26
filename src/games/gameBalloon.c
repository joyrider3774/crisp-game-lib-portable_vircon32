#include "../cglp.h"

int* balloonTitle = "BALLOON";
int* balloonDescription = "[Slide] Wind";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] balloonCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int balloonCharactersCount = 0;

Options balloonOptions = {100, 100, 27, false};

struct BalloonBalloon {
  Vector pos;
  Vector vel;
  float r;
  float t;
  bool isAlive;
};
#define BALLOON_MAX_BALLOON_COUNT 8
BalloonBalloon[BALLOON_MAX_BALLOON_COUNT] balloonBalloons;
int balloonBalloonIndex;
float balloonAddBalloonTicks;

struct BalloonBonus {
  Vector pos;
  Vector vel;
  int balloonIndex;
  bool hasBalloon;
  bool isAlive;
};
// Raised from 32: once attached a bonus rides its balloon for the balloon's
// whole ~330-tick life (difficulty-independent), while bonus spawn interval
// is 50/difficulty ticks, so concurrent count grows ~6.6*difficulty unboundedly.
#define BALLOON_MAX_BONUS_COUNT 256
BalloonBonus[BALLOON_MAX_BONUS_COUNT] balloonBonuses;
int balloonBonusIndex;
float balloonAddBonusTicks;

Vector balloonPrevInputPos;
Vector balloonWind;
int balloonMultiplier;
float balloonScoreTotal;
Vector balloonScorePos;

void balloonUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - balloon/bonus
  // pickup and popping are all direct distanceTo() checks (see the
  // "< bl->r * 1.3" and "< b->r" comparisons below), so the engine's own
  // O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure waste here.
  // Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(balloonBalloons);
    balloonBalloonIndex = 0;
    balloonAddBalloonTicks = 0;
    vectorSet(&balloonPrevInputPos, input.pos.x, input.pos.y);
    vectorSet(&balloonWind, 0, 0);
    INIT_UNALIVED_ARRAY_FAST(balloonBonuses);
    balloonBonusIndex = 0;
    balloonAddBonusTicks = 0;
  }
  balloonMultiplier = 1;
  balloonScoreTotal = 0;
  balloonAddBonusTicks--;
  if (balloonAddBonusTicks < 0) {
    ASSIGN_ARRAY_ITEM(balloonBonuses, balloonBonusIndex, BalloonBonus, nb);
    vectorSet(&nb->pos, rnd(10, 80), -5);
    vectorSet(&nb->vel, rnd(0, 0.2) * RNDPM(), rnd(0.1, 0.25 + difficulty * 0.05));
    nb->hasBalloon = false;
    nb->isAlive = true;
    balloonBonusIndex = cgl_wrap(balloonBonusIndex + 1, 0, BALLOON_MAX_BONUS_COUNT);
    balloonAddBonusTicks += 50 / difficulty;
  }
  FOR_EACH(balloonBonuses, i) {
    ASSIGN_ARRAY_ITEM(balloonBonuses, i, BalloonBonus, b);
    SKIP_IS_NOT_ALIVE(b);
    if (b->hasBalloon) {
      color = GREEN;
      BalloonBalloon* bl = &balloonBalloons[b->balloonIndex];
      Vector d;
      d.x = bl->pos.x - b->pos.x;
      d.y = bl->pos.y - b->pos.y;
      vectorMul(&d, 0.1);
      vectorAdd(&b->pos, d.x, d.y);
      Vector d2;
      d2.x = bl->pos.x - b->pos.x;
      d2.y = bl->pos.y - b->pos.y;
      vectorMul(&d2, 0.02);
      vectorAdd(&b->vel, d2.x, d2.y);
      vectorMul(&b->vel, 0.95);
    } else {
      color = YELLOW;
      FOR_EACH(balloonBalloons, j) {
        ASSIGN_ARRAY_ITEM(balloonBalloons, j, BalloonBalloon, bl);
        SKIP_IS_NOT_ALIVE(bl);
        if (distanceTo(&bl->pos, b->pos.x, b->pos.y) < bl->r * 1.3) {
          play(SELECT);
          b->hasBalloon = true;
          b->balloonIndex = j;
        }
      }
    }
    if ((b->pos.x < 0 && b->vel.x < 0) || (b->pos.x > 99 && b->vel.x > 0)) {
      b->vel.x *= -1;
    }
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    text("$", b->pos.x, b->pos.y, &scratch);
    if (!b->hasBalloon && b->pos.y > 99) {
      play(EXPLOSION);
      gameOver();
    }
    if (b->hasBalloon && !balloonBalloons[b->balloonIndex].isAlive) {
      balloonScoreTotal += balloonMultiplier;
      balloonScorePos = b->pos;
      balloonMultiplier++;
      b->isAlive = false;
      continue;
    }
  }
  if (balloonScoreTotal > 0) {
    play(COIN);
    addScore(balloonScoreTotal, balloonScorePos.x, balloonScorePos.y + 9);
  }
  color = CYAN;
  Vector o;
  o.x = input.pos.x - balloonPrevInputPos.x;
  o.y = input.pos.y - balloonPrevInputPos.y;
  if (vectorLength(&o) < 9) {
    vectorMul(&o, 0.5);
    vectorAdd(&balloonWind, o.x, o.y);
    vectorMul(&balloonWind, 0.5);
  }
  vectorSet(&balloonPrevInputPos, input.pos.x, input.pos.y);
  particle(input.pos.x, input.pos.y, 3, vectorLength(&balloonWind), vectorAngle(&balloonWind), 0.1);
  balloonAddBalloonTicks--;
  if (balloonAddBalloonTicks < 0) {
    float r = 20;
    ASSIGN_ARRAY_ITEM(balloonBalloons, balloonBalloonIndex, BalloonBalloon, nbl);
    vectorSet(&nbl->pos, rnd(r, 99 - r), 99 + r);
    vectorSet(&nbl->vel, rnd(0, 1) * RNDPM(), 0);
    nbl->r = r;
    nbl->t = 0;
    nbl->isAlive = true;
    balloonBalloonIndex = cgl_wrap(balloonBalloonIndex + 1, 0, BALLOON_MAX_BALLOON_COUNT);
    balloonAddBalloonTicks += 200;
  }
  color = GREEN;
  FOR_EACH(balloonBalloons, i) {
    ASSIGN_ARRAY_ITEM(balloonBalloons, i, BalloonBalloon, b);
    SKIP_IS_NOT_ALIVE(b);
    if (distanceTo(&input.pos, b->pos.x, b->pos.y) < b->r) {
      vectorAdd(&b->vel, balloonWind.x, balloonWind.y);
    }
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    if ((b->pos.x < b->r && b->vel.x < 0) || (b->pos.x > 99 - b->r && b->vel.x > 0)) {
      b->vel.x *= -0.7;
    }
    if (b->pos.y > 99 - b->r && b->vel.y > 0) {
      b->vel.y *= -0.7;
    }
    b->vel.x *= 0.9;
    b->vel.y += (-0.3 - b->vel.y) * 0.1;
    b->r *= 1 - vectorLength(&b->vel) * 0.003;
    b->t++;
    float a = b->t * 0.03;
    thickness = 3;
    TIMES(7, k) {
      Vector p;
      vectorSet(&p, b->r, 0);
      rotate(&p, a);
      vectorAdd(&p, b->pos.x, b->pos.y);
      bar(p.x, p.y, b->r * 0.3, a + CGLP_PI_2, &scratch);
      a += CGLP_PI * 2 / 7;
    }
    if (b->pos.y < b->r || b->r < 9) {
      b->isAlive = false;
      particle(b->pos.x, b->pos.y, 9, 1, 0, CGLP_PI * 2);
      continue;
    }
  }
}

void addGameBalloon() {
  addGame(balloonTitle, balloonDescription, balloonCharacters,
          balloonCharactersCount, &balloonOptions, true, &balloonUpdate);
}
