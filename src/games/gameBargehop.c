#include "../cglp.h"

int* bargehopTitle = "BARGE HOP";
int* bargehopDescription = "[Tap]\n Jump\n[Hold]\n Reduce gravity";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bargehopCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int bargehopCharactersCount = 1;

Options bargehopOptions = {150, 100, 1, false};

#define BARGEHOP_WORLD_W 150
#define BARGEHOP_WORLD_H 100
#define BARGEHOP_WATER_Y 78
#define BARGEHOP_WATER_TOP_Y 71
#define BARGEHOP_DOCK_W 14
#define BARGEHOP_DOCK_H 12
#define BARGEHOP_BARGE_H 8
#define BARGEHOP_BARGE_CLEARANCE 5
#define BARGEHOP_PLAYER_W 6
#define BARGEHOP_PLAYER_H 8
#define BARGEHOP_GRAVITY 0.15
#define BARGEHOP_JUMP_VY -2.1
#define BARGEHOP_JUMP_VX 0.7
#define BARGEHOP_TIME_LIMIT 1000
#define BARGEHOP_MAX_TIME_BONUS 120
#define BARGEHOP_JUMP_DUR 16
#define BARGEHOP_LAND_DUR 12
#define BARGEHOP_JUMP_STRETCH_SX 0.8
#define BARGEHOP_JUMP_STRETCH_SY 1.35
#define BARGEHOP_LAND_SQUASH_SX 1.35
#define BARGEHOP_LAND_SQUASH_SY 0.8
#define BARGEHOP_BARGE_COUNT 3

struct BargehopDock {
  float x;
  float y;
  float w;
  float h;
};
BargehopDock bargehopLeftDock;
BargehopDock bargehopRightDock;

struct BargehopBarge {
  float x;
  float y;
  float w;
  float h;
  float vx;
  float amp;
  float freq;
  float phase;
};
BargehopBarge[BARGEHOP_BARGE_COUNT] bargehopBarges;

struct BargehopPlayer {
  float x;
  float y;
  float vx;
  float vy;
  bool onGround;
  int onBargeIndex;
};
BargehopPlayer bargehopPlayer;

int bargehopRoundLevel;
float bargehopJumpAnimT;
float bargehopLandAnimT;
float bargehopRemain;

float bargehopLerp(float a, float b, float t) {
  return a + (b - a) * t;
}

void bargehopInitRound() {
  float dockRaise = 3;
  bargehopLeftDock.x = BARGEHOP_DOCK_W / 2 + 1;
  bargehopLeftDock.y = BARGEHOP_WATER_Y - BARGEHOP_DOCK_H / 2 - dockRaise;
  bargehopLeftDock.w = BARGEHOP_DOCK_W;
  bargehopLeftDock.h = BARGEHOP_DOCK_H;
  bargehopRightDock.x = BARGEHOP_WORLD_W - (BARGEHOP_DOCK_W / 2 + 1);
  bargehopRightDock.y = BARGEHOP_WATER_Y - BARGEHOP_DOCK_H / 2 - dockRaise;
  bargehopRightDock.w = BARGEHOP_DOCK_W;
  bargehopRightDock.h = BARGEHOP_DOCK_H;

  float roundDifficulty = 1 + sqrt(bargehopRoundLevel * 0.3);
  float speedScale = 0.5 * roundDifficulty;
  float speedBase = 0.35 * speedScale;
  float baseY = BARGEHOP_WATER_Y - BARGEHOP_BARGE_H / 2;

  TIMES(BARGEHOP_BARGE_COUNT, bgi) {
    float w = clamp(rndi(20, 30), 12, 28);
    float dir;
    if (rnd(0, 1) < 0.5) {
      dir = -1;
    } else {
      dir = 1;
    }
    float sp = speedBase * rnd(0.9, 1.5) * dir;
    float leftEdge2 = bargehopLeftDock.x + bargehopLeftDock.w / 2 + w / 2 + 2;
    float rightEdge2 = bargehopRightDock.x - bargehopRightDock.w / 2 - w / 2 - 2;
    float x;
    int tries = 0;
    bool overlap;
    do {
      x = rnd(leftEdge2, rightEdge2);
      tries++;
      if (tries > 50) break;
      overlap = false;
      int checkI;
      for (checkI = 0; checkI < bgi; checkI++) {
        if (fabs(bargehopBarges[checkI].x - x) < (bargehopBarges[checkI].w + w) * 0.55) {
          overlap = true;
        }
      }
    } while (overlap);

    BargehopBarge* bg = &bargehopBarges[bgi];
    bg->x = x;
    bg->y = baseY;
    bg->w = w;
    bg->h = BARGEHOP_BARGE_H;
    bg->vx = sp;
    bg->amp = rnd(1, 2);
    bg->freq = rnd(0.02, 0.05) * roundDifficulty;
    bg->phase = rnd(0, CGLP_PI * 2);
  }

  bargehopPlayer.x = bargehopLeftDock.x;
  bargehopPlayer.y = bargehopLeftDock.y - bargehopLeftDock.h / 2 - BARGEHOP_PLAYER_H / 2;
  bargehopPlayer.vx = 0;
  bargehopPlayer.vy = 0;
  bargehopPlayer.onGround = true;
  bargehopPlayer.onBargeIndex = -1;
  bargehopRemain = clamp(bargehopRemain + BARGEHOP_TIME_LIMIT / 2, 0, BARGEHOP_TIME_LIMIT);
}

void bargehopNextRound() {
  bargehopRoundLevel++;
  bargehopInitRound();
}

void bargehopUpdate() {
  Collision scratch;
  if (!ticks) {
    bargehopRoundLevel = 0;
    bargehopRemain = BARGEHOP_TIME_LIMIT;
    bargehopInitRound();
  }

  float roundDifficulty = 1 + bargehopRoundLevel / 5.0;
  bargehopRemain -= sqrt(roundDifficulty);
  float remainRatio = bargehopRemain / BARGEHOP_TIME_LIMIT;
  if (bargehopRemain <= 0) {
    gameOver();
  }

  color = LIGHT_BLUE;
  box(BARGEHOP_WORLD_W / 2, BARGEHOP_WATER_Y + 11, BARGEHOP_WORLD_W, 44, &scratch);

  float barH = 4;
  float barPad = 3;
  float barWMax = BARGEHOP_WORLD_W - barPad * 2;
  float barX = barPad + barWMax * 0.5;
  float barY = BARGEHOP_WORLD_H - barPad - barH * 0.5;
  color = LIGHT_BLACK;
  box(barX, barY, barWMax, barH, &scratch);
  float barW = barWMax * remainRatio;
  float barCX = barPad + barW * 0.5;
  int blinkPeriod = 8;
  bool isWarning = remainRatio <= 0.25;
  int fillColor;
  if (isWarning && (ticks / blinkPeriod) % 2 == 0) {
    fillColor = RED;
  } else {
    fillColor = YELLOW;
  }
  color = fillColor;
  if (barW > 0) {
    box(barCX, barY, barW, barH - 1, &scratch);
  }

  color = BLACK;
  box(bargehopLeftDock.x, bargehopLeftDock.y, bargehopLeftDock.w, bargehopLeftDock.h, &scratch);
  box(bargehopRightDock.x, bargehopRightDock.y, bargehopRightDock.w, bargehopRightDock.h, &scratch);

  color = GREEN;
  TIMES(BARGEHOP_BARGE_COUNT, bgi2) {
    BargehopBarge* b = &bargehopBarges[bgi2];
    b->x += b->vx;
    b->y = BARGEHOP_WATER_Y - BARGEHOP_BARGE_H / 2 - b->amp - BARGEHOP_BARGE_CLEARANCE +
           sin(ticks * b->freq + b->phase) * b->amp;
    float leftEdge = bargehopLeftDock.x + bargehopLeftDock.w / 2;
    float rightEdge = bargehopRightDock.x - bargehopRightDock.w / 2;
    if (b->x - b->w / 2 <= leftEdge) {
      b->x = leftEdge + b->w / 2;
      b->vx = fabs(b->vx);
    }
    if (b->x + b->w / 2 >= rightEdge) {
      b->x = rightEdge - b->w / 2;
      b->vx = -fabs(b->vx);
    }
  }

  int bi;
  for (bi = 0; bi < BARGEHOP_BARGE_COUNT; bi++) {
    int bj;
    for (bj = bi + 1; bj < BARGEHOP_BARGE_COUNT; bj++) {
      BargehopBarge* a = &bargehopBarges[bi];
      BargehopBarge* b = &bargehopBarges[bj];
      if (fabs(a->y - b->y) < (a->h + b->h) * 0.5) {
        float dx = b->x - a->x;
        float overlap2 = a->w * 0.5 + b->w * 0.5 - fabs(dx);
        if (overlap2 > 0) {
          float push = overlap2 / 2 + 0.01;
          if (dx > 0) {
            a->x -= push;
            b->x += push;
          } else {
            a->x += push;
            b->x -= push;
          }
          int sA;
          if (a->vx >= 0) {
            sA = 1;
          } else {
            sA = -1;
          }
          int sB;
          if (b->vx >= 0) {
            sB = 1;
          } else {
            sB = -1;
          }
          if (sA == sB) {
            bool movingRight = sA > 0;
            BargehopBarge* trailing;
            if (movingRight) {
              if (a->x < b->x) {
                trailing = a;
              } else {
                trailing = b;
              }
            } else {
              if (a->x > b->x) {
                trailing = a;
              } else {
                trailing = b;
              }
            }
            trailing->vx = -trailing->vx;
          } else {
            a->vx = -a->vx;
            b->vx = -b->vx;
          }
          play(HIT);
        }
      }
    }
  }

  float[BARGEHOP_BARGE_COUNT] bargeTops;
  TIMES(BARGEHOP_BARGE_COUNT, bgi3) {
    BargehopBarge* b = &bargehopBarges[bgi3];
    box(b->x, b->y, b->w, b->h, &scratch);
    bargeTops[bgi3] = b->y - b->h / 2;
  }

  if (bargehopJumpAnimT > 0) {
    bargehopJumpAnimT--;
  }
  if (bargehopLandAnimT > 0) {
    bargehopLandAnimT--;
  }
  float wj = bargehopJumpAnimT / BARGEHOP_JUMP_DUR;
  float wl = bargehopLandAnimT / BARGEHOP_LAND_DUR;
  float scaleX = bargehopLerp(1, BARGEHOP_JUMP_STRETCH_SX, wj) * bargehopLerp(1, BARGEHOP_LAND_SQUASH_SX, wl);
  float scaleY = bargehopLerp(1, BARGEHOP_JUMP_STRETCH_SY, wj) * bargehopLerp(1, BARGEHOP_LAND_SQUASH_SY, wl);
  float drawW = BARGEHOP_PLAYER_W * scaleX;
  float drawH = BARGEHOP_PLAYER_H * scaleY;

  if (bargehopPlayer.onGround) {
    bargehopPlayer.vx = 0;
    bargehopPlayer.vy = 0;
    if (bargehopPlayer.onBargeIndex >= 0) {
      int bIdx = bargehopPlayer.onBargeIndex;
      bargehopPlayer.x += bargehopBarges[bIdx].vx;
      bargehopPlayer.y = bargeTops[bIdx] - BARGEHOP_PLAYER_H / 2;
    }
  } else {
    float gMul;
    if (input.isPressed) {
      gMul = 0.5;
    } else {
      gMul = 1.5;
    }
    bargehopPlayer.vy += BARGEHOP_GRAVITY * gMul * roundDifficulty;
    bargehopPlayer.x += bargehopPlayer.vx;
    bargehopPlayer.y += bargehopPlayer.vy;
  }

  if (input.isJustPressed) {
    if (bargehopPlayer.onGround) {
      bargehopPlayer.onGround = false;
      bargehopPlayer.onBargeIndex = -1;
      bargehopPlayer.vx = BARGEHOP_JUMP_VX * sqrt(roundDifficulty);
      bargehopPlayer.vy = BARGEHOP_JUMP_VY * sqrt(roundDifficulty);
      play(JUMP);
      bargehopJumpAnimT = BARGEHOP_JUMP_DUR;
    }
  }

  color = YELLOW;
  float footBottom = bargehopPlayer.y + drawH * 0.5;
  float footBoxY = footBottom - 1;
  box(bargehopPlayer.x, bargehopPlayer.y, drawW, drawH, &scratch);
  box(bargehopPlayer.x, footBoxY, BARGEHOP_PLAYER_W * 0.6, 5, &scratch);
  color = TRANSPARENT;
  Collision pc;
  box(bargehopPlayer.x, footBoxY - 48, drawW, 100, &pc);

  if (!bargehopPlayer.onGround) {
    bool inRightDockX = bargehopPlayer.x >= bargehopRightDock.x - bargehopRightDock.w * 0.6 &&
                         bargehopPlayer.x <= bargehopRightDock.x + bargehopRightDock.w * 0.6;
    bool landedOnRightDock = pc.isColliding.rect[BLACK] && bargehopPlayer.vy >= 0 && inRightDockX;
    bool landedOnBarge = false;
    int landedIndex = -1;
    if (pc.isColliding.rect[GREEN] && bargehopPlayer.vy >= 0) {
      int bi2;
      for (bi2 = BARGEHOP_BARGE_COUNT - 1; bi2 >= 0; bi2--) {
        BargehopBarge* b = &bargehopBarges[bi2];
        if (fabs(bargehopPlayer.x - b->x) <= BARGEHOP_PLAYER_W * 0.6 + b->w * 0.6) {
          landedOnBarge = true;
          landedIndex = bi2;
          break;
        }
      }
    }

    if (landedOnRightDock) {
      int bonus = (int)floor(BARGEHOP_MAX_TIME_BONUS * remainRatio);
      addScore(bonus, bargehopPlayer.x, bargehopPlayer.y - 6);
      play(COIN);
      bargehopNextRound();
      return;
    } else if (landedOnBarge) {
      float top = bargeTops[landedIndex];
      bargehopPlayer.onGround = true;
      bargehopPlayer.onBargeIndex = landedIndex;
      bargehopPlayer.y = top - BARGEHOP_PLAYER_H / 2;
      bargehopPlayer.vx = 0;
      bargehopPlayer.vy = 0;
      addScore(1, bargehopPlayer.x, bargehopPlayer.y - 6);
      play(HIT);
      bargehopLandAnimT = BARGEHOP_LAND_DUR;
    }
  }

  if (!bargehopPlayer.onGround && bargehopPlayer.vy >= 0 && footBottom >= BARGEHOP_WATER_TOP_Y) {
    color = CYAN;
    particle(bargehopPlayer.x, BARGEHOP_WATER_TOP_Y, 20, 2.0, -CGLP_PI_2, CGLP_PI / 1.3);
    color = LIGHT_BLUE;
    particle(bargehopPlayer.x, BARGEHOP_WATER_TOP_Y, 14, 1.2, -CGLP_PI_2, CGLP_PI);
    color = LIGHT_CYAN;
    particle(bargehopPlayer.x, BARGEHOP_WATER_TOP_Y - 1, 8, 1.4, 0, CGLP_PI / 6);
    particle(bargehopPlayer.x, BARGEHOP_WATER_TOP_Y - 1, 8, 1.4, CGLP_PI, CGLP_PI / 6);
    play(EXPLOSION);
    bargehopPlayer.onGround = true;
    bargehopPlayer.onBargeIndex = -1;
    bargehopPlayer.x = bargehopLeftDock.x;
    bargehopPlayer.y = bargehopLeftDock.y - bargehopLeftDock.h / 2 - BARGEHOP_PLAYER_H / 2;
    bargehopPlayer.vx = 0;
    bargehopPlayer.vy = 0;
    bargehopLandAnimT = BARGEHOP_LAND_DUR;
  }
}

void addGameBargehop() {
  addGame(bargehopTitle, bargehopDescription, bargehopCharacters,
          bargehopCharactersCount, &bargehopOptions, false, &bargehopUpdate);
}
