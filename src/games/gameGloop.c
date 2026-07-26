#include "../cglp.h"

int* gloopTitle = "GLOOP";
int* gloopDescription = "[Slide] Move";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] gloopCharacters = {
    {
        " llll ",
        "llllll",
        "ll  ll",
        "ll  ll",
    },
    {
        " lll  ",
        "l   l ",
        "l l l ",
        "l   l ",
        " lll  ",
    },
    {
        "l l   ",
        " l    ",
        "l l   ",
    },
    {
        " l l l",
        "l   l ",
        " l   l",
        "l   l ",
        " l   l",
        "l l l ",
    },
};
int gloopCharactersCount = 4;

Options gloopOptions = {100, 100, 0, false};

Vector gloopP;
Vector gloopV;
float gloopVya;

struct GloopPi {
  Vector pos;
  bool isAlive;
};
#define GLOOP_MAX_PI_COUNT 7
GloopPi[GLOOP_MAX_PI_COUNT] gloopPis;

struct GloopSp {
  Vector pos;
  bool isCleared;
  bool slotUsed;
};
#define GLOOP_MAX_SP_COUNT 32
GloopSp[GLOOP_MAX_SP_COUNT] gloopSps;
int gloopSpIndex;

void gloopUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&gloopP, 50, 50);
    vectorSet(&gloopV, 0, 0);
    gloopVya = -1;
    INIT_UNALIVED_ARRAY_FAST(gloopPis);
    INIT_UNALIVED_ARRAY_FAST(gloopSps);
    gloopSpIndex = 0;
  }
  color = LIGHT_CYAN;
  for (int x = 0; x < 15; x++) {
    for (int y = 0; y < 18; y++) {
      if ((x + y) % 2 == 0) {
        character("d", x * 6 + 8, y * 6, &scratch);
      }
    }
  }
  gloopV.y += 0.02 * difficulty * gloopVya;
  vectorMul(&gloopV, 0.99);
  vectorAdd(&gloopP, gloopV.x, gloopV.y);
  gloopP.x = clamp(input.pos.x, 8, 92);
  gloopP.y = cgl_wrap(gloopP.y, 0, 99);
  color = BLACK;
  character("a", gloopP.x, gloopP.y, &scratch);
  TIMES(GLOOP_MAX_PI_COUNT, i) {
    if (!gloopPis[i].isAlive) {
      vectorSet(&gloopPis[i].pos, rnd(10, 90), rnd(10, 90));
      gloopPis[i].isAlive = true;
    }
  }
  if (rnd(0, 1) < 0.02 * difficulty) {
    Vector pp;
    vectorSet(&pp, rnd(10, 90), rnd(10, 90));
    if (fabs(pp.x - gloopP.x) + fabs(cgl_wrap(pp.y - gloopP.y, -50, 50)) > 25) {
      ASSIGN_ARRAY_ITEM(gloopSps, gloopSpIndex, GloopSp, nsp);
      nsp->pos = pp;
      nsp->isCleared = false;
      nsp->slotUsed = true;
      gloopSpIndex = cgl_wrap(gloopSpIndex + 1, 0, GLOOP_MAX_SP_COUNT);
    }
  }
  TIMES(GLOOP_MAX_PI_COUNT, i) {
    GloopPi* bumper = &gloopPis[i];
    if (!bumper->isAlive) {
      continue;
    }
    character("b", bumper->pos.x, bumper->pos.y, &scratch);
    if (scratch.isColliding.character['a']) {
      if (fabs(gloopV.y) > 1) {
        play(SELECT);
        bumper->isAlive = false;
        FOR_EACH(gloopSps, k) {
          ASSIGN_ARRAY_ITEM(gloopSps, k, GloopSp, sp);
          if (!sp->slotUsed || sp->isCleared) {
            continue;
          }
          if (distanceTo(&sp->pos, bumper->pos.x, bumper->pos.y) < 20) {
            play(COIN);
            sp->isCleared = true;
          }
        }
      } else {
        play(HIT);
      }
      gloopV.y *= -0.3;
      gloopVya *= -1;
      gloopP.y = bumper->pos.y + gloopVya * 5;
    }
  }
  int sc = 1;
  color = RED;
  FOR_EACH(gloopSps, i) {
    ASSIGN_ARRAY_ITEM(gloopSps, i, GloopSp, sp);
    if (!sp->slotUsed) {
      continue;
    }
    if (!sp->isCleared) {
      character("c", sp->pos.x, sp->pos.y, &scratch);
      if (scratch.isColliding.character['a']) {
        play(EXPLOSION);
        gameOver();
      }
    } else {
      addScore(sc, sp->pos.x, sp->pos.y);
      sc++;
      sp->slotUsed = false;
    }
  }
}

void addGameGloop() {
  addGame(gloopTitle, gloopDescription, gloopCharacters, gloopCharactersCount,
          &gloopOptions, true, &gloopUpdate);
}
