#include "../cglp.h"

int* wiperTitle = "WIPER";
int* wiperDescription = "[Tap]\n Swing";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] wiperCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int wiperCharactersCount = 1;

Options wiperOptions = {100, 100, 8, true};

struct WiperRain {
  Vector pos;
  float size;
  bool isAlive;
};
#define WIPER_MAX_RAIN_COUNT 64
WiperRain[WIPER_MAX_RAIN_COUNT] wiperRains;
int wiperRainIndex;
int wiperNextRainTicks;
float wiperAngle;
float wiperVa;

void wiperUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(wiperRains);
    wiperRainIndex = 0;
    wiperNextRainTicks = 0;
    wiperAngle = (-CGLP_PI / 5) * 4;
    wiperVa = 1;
  }
  wiperAngle += wiperVa * 0.05 * sqrt(difficulty);
  if ((wiperAngle > -CGLP_PI / 5 && wiperVa > 0) ||
      (wiperAngle < (-CGLP_PI / 5) * 4 && wiperVa < 0)) {
    play(HIT);
    wiperVa *= -1;
  }
  float ta;
  if (wiperVa > 0) {
    ta = -CGLP_PI / 5;
  } else {
    ta = (-CGLP_PI / 5) * 4;
  }
  if (wiperAngle > -CGLP_PI / 3) {
    ta = (-CGLP_PI / 5) * 4;
  }
  if (wiperAngle < (-CGLP_PI / 3) * 2) {
    ta = -CGLP_PI / 5;
  }
  color = LIGHT_BLACK;
  thickness = 2;
  barCenterPosRatio = -0.7;
  bar(50, 110, 36, ta, &scratch);
  if (input.isJustPressed) {
    play(SELECT);
    wiperAngle = ta;
    wiperVa *= -1;
  }
  color = BLACK;
  thickness = 6;
  barCenterPosRatio = -0.7;
  bar(50, 110, 36, wiperAngle, &scratch);
  wiperNextRainTicks--;
  if (wiperNextRainTicks < 0) {
    ASSIGN_ARRAY_ITEM(wiperRains, wiperRainIndex, WiperRain, r);
    vectorSet(&r->pos, rnd(10, 90), rnd(0, 20));
    r->size = 2;
    r->isAlive = true;
    wiperRainIndex = cgl_wrap(wiperRainIndex + 1, 0, WIPER_MAX_RAIN_COUNT);
    wiperNextRainTicks = 10 / difficulty;
  }
  FOR_EACH(wiperRains, i) {
    ASSIGN_ARRAY_ITEM(wiperRains, i, WiperRain, r);
    SKIP_IS_NOT_ALIVE(r);
    color = TRANSPARENT;
    box(r->pos.x, r->pos.y, sqrt(r->size) * 2, sqrt(r->size) * 2, &scratch);
    bool isRemoved = scratch.isColliding.rect[BLACK];
    bool isAbsorbed = false;
    FOR_EACH(wiperRains, j) {
      ASSIGN_ARRAY_ITEM(wiperRains, j, WiperRain, other);
      SKIP_IS_NOT_ALIVE(other);
      if (r->pos.y > other->pos.y &&
          distanceTo(&r->pos, other->pos.x, other->pos.y) < sqrt(r->size + other->size) * 2) {
        other->size += r->size;
        other->pos.x = (other->pos.x + r->pos.x) / 2;
        isAbsorbed = true;
      }
    }
    if (isAbsorbed) {
      play(LASER);
      r->isAlive = false;
      continue;
    }
    r->pos.y += sqrt(r->size - 1.9) * 0.3 * sqrt(difficulty);
    color = CYAN;
    box(r->pos.x, r->pos.y, sqrt(r->size), sqrt(r->size), &scratch);
    if (isRemoved || scratch.isColliding.rect[BLACK]) {
      play(POWER_UP);
      addScore(ceil(r->size - 1), r->pos.x, r->pos.y);
      r->isAlive = false;
      continue;
    }
    if (r->pos.y > 99) {
      play(EXPLOSION);
      color = RED;
      text("X", r->pos.x, 97, &scratch);
      gameOver();
    }
  }
}

void addGameWiper() {
  addGame(wiperTitle, wiperDescription, wiperCharacters, wiperCharactersCount,
          &wiperOptions, false, &wiperUpdate);
}
