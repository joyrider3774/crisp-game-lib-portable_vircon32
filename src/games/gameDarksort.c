#include "../cglp.h"

int* darksortTitle = "DARK SORT";
int* darksortDescription = "[Hold]\n Light gate";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] darksortCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int darksortCharactersCount = 0;

Options darksortOptions = {100, 100, 0, false};

struct DarksortGate {
  float x;
  float y;
  float vx;
  bool isLight;
};
DarksortGate darksortGate;

struct DarksortOrb {
  float x;
  float y;
  bool isWhite;
  float speed;
  bool isAlive;
};
// Spawns every 8-30 ticks (shrinks toward 8 as difficulty rises), 1-2 orbs
// per spawn, each falling across ~105px at 0.8*sqrt(difficulty) px/tick -
// worked out to roughly 5 concurrent orbs at worst, sized generously above.
#define DARKSORT_MAX_ORB_COUNT 32
DarksortOrb[DARKSORT_MAX_ORB_COUNT] darksortOrbs;
int darksortOrbIndex;

int darksortSpawnTimer;
float darksortVisionRadius;
int darksortCombo;

void darksortUpdate() {
  Collision scratch;
  // Never reads a Collision result - catches are decided by distance math, not hitboxes.
  hasCollision = false;
  if (!ticks) {
    darksortGate.x = 50;
    darksortGate.y = 92;
    darksortGate.vx = 1;
    darksortGate.isLight = false;
    INIT_UNALIVED_ARRAY_FAST(darksortOrbs);
    darksortOrbIndex = 0;
    darksortSpawnTimer = 0;
    darksortVisionRadius = 55;
    darksortCombo = 0;
  }

  darksortGate.isLight = input.isPressed;

  darksortSpawnTimer--;
  if (darksortSpawnTimer <= 0) {
    int spawnInterval = 30 - (int)floor(difficulty);
    if (spawnInterval < 8) {
      spawnInterval = 8;
    }
    darksortSpawnTimer = spawnInterval;

    int numOrbs = 1;
    if (difficulty > 5) {
      if (rnd(0, 1) < 0.3) {
        numOrbs = 2;
      } else {
        numOrbs = 1;
      }
    }

    TIMES(numOrbs, i) {
      ASSIGN_ARRAY_ITEM(darksortOrbs, darksortOrbIndex, DarksortOrb, o);
      o->x = rnd(12, 88);
      o->y = -5 - i * 15;
      o->isWhite = rnd(0, 1) < 0.5;
      o->speed = 0.8 * sqrt(difficulty);
      o->isAlive = true;
      darksortOrbIndex = cgl_wrap(darksortOrbIndex + 1, 0, DARKSORT_MAX_ORB_COUNT);
    }
  }

  color = LIGHT_BLACK;
  rect(0, 0, 100, 100, &scratch);

  color = WHITE;
  thickness = 6;
  arc(darksortGate.x, darksortGate.y - 25, darksortVisionRadius, 0, CGLP_PI * 2, &scratch);

  darksortGate.x += darksortGate.vx * sqrt(difficulty);
  if ((darksortGate.x > 90 && darksortGate.vx > 0) ||
      (darksortGate.x < 10 && darksortGate.vx < 0)) {
    darksortGate.vx *= -1;
  }
  if (darksortGate.isLight) {
    color = WHITE;
  } else {
    color = BLACK;
  }
  box(darksortGate.x, darksortGate.y, 24, 5, &scratch);

  bool shouldEnd = false;

  FOR_EACH(darksortOrbs, i) {
    ASSIGN_ARRAY_ITEM(darksortOrbs, i, DarksortOrb, o);
    SKIP_IS_NOT_ALIVE(o);
    o->y += o->speed;

    float dx = o->x - darksortGate.x;
    float dy = o->y - (darksortGate.y - 25);
    bool inVision = sqrt(dx * dx + dy * dy) < darksortVisionRadius;

    if (inVision) {
      if (o->isWhite) {
        color = WHITE;
      } else {
        color = BLACK;
      }
      box(o->x, o->y, 6, 6, &scratch);
    }

    if (o->y >= darksortGate.y - 2 && o->y <= darksortGate.y + 3 &&
        fabs(o->x - darksortGate.x) < 14) {
      if ((o->isWhite && darksortGate.isLight) ||
          (!o->isWhite && !darksortGate.isLight)) {
        play(COIN);
        darksortCombo++;
        addScore(darksortCombo, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        o->isAlive = false;
        continue;
      } else {
        play(EXPLOSION);
        shouldEnd = true;
      }
    }

    if (o->y > 100) {
      play(HIT);
      darksortCombo = 0;
      o->isAlive = false;
      continue;
    }
  }

  if (shouldEnd) {
    gameOver();
  }
}

void addGameDarksort() {
  addGame(darksortTitle, darksortDescription, darksortCharacters,
          darksortCharactersCount, &darksortOptions, false, &darksortUpdate);
}
