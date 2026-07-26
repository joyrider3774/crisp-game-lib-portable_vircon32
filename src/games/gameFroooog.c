#include "../cglp.h"

char* froooogTitle = "FROOOOG";
char* froooogDescription = "[Hold]    Bend\n[Release] Jump";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] froooogCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int froooogCharactersCount = 1;

Options froooogOptions = {100, 100, 4, false};

struct FroooogLane {
  float y;
  float vx;
  float width;
  int color;
  float interval;
  float ticks;
  bool isAlive;
};
#define FROOOOG_MAX_LANE_COUNT 16
FroooogLane[FROOOOG_MAX_LANE_COUNT] froooogLanes;
int froooogLaneIndex;
int froooogNextEmptyLaneCount;
struct FroooogCar {
  Vector pos;
  float vx;
  float width;
  int color;
  bool isAlive;
};
#define FROOOOG_MAX_CAR_COUNT 32
FroooogCar[FROOOOG_MAX_CAR_COUNT] froooogCars;
int froooogCarIndex;
float froooogNextLaneY;
struct FroooogFrog {
  float y;
  float py;
  float targetY;
  bool isSafe;
  int state;
};
FroooogFrog froooogFrog;

void froooogAddLane(bool isEmpty) {
  froooogNextEmptyLaneCount--;
  float vr = sqrt(difficulty) + 0.1;
  float vx = (rnd(0, vr) * rnd(0, vr) * rnd(0, vr) + 1) * 0.3 * RNDPM();
  float width = rndi(9, 19);
  float interval = width + rnd(40, 90) / fabs(vx);
  ASSIGN_ARRAY_ITEM(froooogLanes, froooogLaneIndex, FroooogLane, l);
  l->y = froooogNextLaneY;
  l->vx = vx;
  l->width = width;
  int[5] cs = {RED, PURPLE, YELLOW, BLUE, CYAN};
  l->color = cs[rndi(0, 5)];
  l->interval = interval;
  if (froooogNextEmptyLaneCount < 0 || isEmpty) {
    l->ticks = 9999999;
  } else {
    l->ticks = rnd(0, interval / 2);
  }
  l->isAlive = true;
  froooogLaneIndex = cgl_wrap(froooogLaneIndex + 1, 0, FROOOOG_MAX_LANE_COUNT);
  froooogNextLaneY -= 10;
  if (froooogNextEmptyLaneCount < 0) {
    froooogNextEmptyLaneCount = rndi(9, 16);
  }
}

void froooogUpdateLanes() {
  Collision scratch;
  FOR_EACH(froooogLanes, i) {
    ASSIGN_ARRAY_ITEM(froooogLanes, i, FroooogLane, l);
    SKIP_IS_NOT_ALIVE(l);
    if (l->ticks > 999) {
      color = LIGHT_GREEN;
      box(50, l->y + 5, 100, 9, &scratch);
    } else {
      color = LIGHT_BLACK;
      box(50, l->y, 100, 1, &scratch);
    }
    l->ticks--;
    if (l->ticks < 0) {
      ASSIGN_ARRAY_ITEM(froooogCars, froooogCarIndex, FroooogCar, c);
      float startX;
      if (l->vx < 0) {
        startX = 99 + l->width / 2;
      } else {
        startX = -l->width / 2;
      }
      vectorSet(&c->pos, startX, l->y + 5);
      c->vx = l->vx;
      c->width = l->width;
      c->color = l->color;
      c->isAlive = true;
      froooogCarIndex = cgl_wrap(froooogCarIndex + 1, 0, FROOOOG_MAX_CAR_COUNT);
      l->ticks = l->interval * rnd(0.9, 1.6);
    }
    l->isAlive = l->y <= 99;
  }
  FOR_EACH(froooogCars, i) {
    ASSIGN_ARRAY_ITEM(froooogCars, i, FroooogCar, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x += c->vx;
    color = c->color;
    box(c->pos.x, c->pos.y, c->width, 7, &scratch);
    c->isAlive =
        !(c->pos.x < -c->width || c->pos.x > 99 + c->width || c->pos.y > 103);
  }
}

void froooogDrawFrog(float scale) {
  Collision cl;
  float y = froooogFrog.y;
  color = GREEN;
  box(50, y, 3 * scale, 5 * scale, &cl);
  bool* c = cl.isColliding.rect;
  if (scale == 1 && (c[RED] || c[YELLOW] || c[PURPLE] || c[BLUE] || c[CYAN])) {
    play(EXPLOSION);
    gameOver();
  }
  float ox = 2 * scale;
  float oy = 2 * scale * scale;
  float w = 2 * scale;
  float h = 3 * scale;
  Collision scratch;
  box(50 - ox, y - oy, w, h, &scratch);
  box(49 + ox, y - oy, w, h, &scratch);
  box(50 - ox, y + oy, w, h, &scratch);
  box(49 + ox, y + oy, w, h, &scratch);
}

void froooogUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(froooogLanes);
    froooogLaneIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(froooogCars);
    froooogCarIndex = 0;
    froooogNextLaneY = 90;
    froooogNextEmptyLaneCount = 0;
    TIMES(3, i) { froooogAddLane(true); }
    TIMES(7, i) { froooogAddLane(false); }
    TIMES(99, i) { froooogUpdateLanes(); }
    froooogFrog.y = 95;
    froooogFrog.py = 0;
    froooogFrog.targetY = 0;
    froooogFrog.state = 0;
    froooogFrog.isSafe = true;
  }
  float scr = difficulty * 0.02;
  if (froooogFrog.y < 90) {
    scr += (90 - froooogFrog.y) * 0.1;
  }
  if (froooogFrog.y > 103) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(froooogLanes, i) {
    ASSIGN_ARRAY_ITEM(froooogLanes, i, FroooogLane, l);
    SKIP_IS_NOT_ALIVE(l);
    l->y += scr;
  }
  FOR_EACH(froooogCars, i) {
    ASSIGN_ARRAY_ITEM(froooogCars, i, FroooogCar, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.y += scr;
  }
  froooogNextLaneY += scr;
  if (froooogNextLaneY > -50) {
    froooogAddLane(false);
  }
  froooogFrog.y += scr;
  froooogFrog.py += scr;
  froooogFrog.targetY += scr;
  froooogUpdateLanes();
  if (froooogFrog.state == 0) {
    froooogDrawFrog(1);
    if (input.isPressed) {
      play(SELECT);
      froooogFrog.state = 1;
      froooogFrog.targetY = froooogFrog.y - 3;
    }
  }
  if (froooogFrog.state == 1) {
    froooogFrog.targetY -= sqrt(difficulty) * 0.7;
    color = LIGHT_BLACK;
    rect(49, froooogFrog.targetY, 1, froooogFrog.y - froooogFrog.targetY, &scratch);
    froooogDrawFrog(1);
    float ty = froooogFrog.targetY;
    int li = froooogLaneIndex;
    TIMES(FROOOOG_MAX_LANE_COUNT, i) {
      li = cgl_wrap(li + 1, 0, FROOOOG_MAX_LANE_COUNT);
      ASSIGN_ARRAY_ITEM(froooogLanes, li, FroooogLane, l);
      SKIP_IS_NOT_ALIVE(l);
      if (l->y > froooogFrog.targetY) {
        ty = l->y - 5;
      }
    }
    color = LIGHT_BLACK;
    box(50, ty, 3, 5, &scratch);
    if (input.isJustReleased || froooogFrog.targetY < 9) {
      play(POWER_UP);
      froooogFrog.state = 2;
      froooogFrog.targetY = ty;
      froooogFrog.py = froooogFrog.y;
    }
  }
  if (froooogFrog.state == 2) {
    froooogFrog.y -= sqrt(difficulty) * 1.5;
    float scale;
    float jumpSpan = froooogFrog.py - froooogFrog.targetY;
    if (jumpSpan == 0) {
      // Jump start and target Y coincide (can happen if the frog was
      // already sitting at its computed target when the jump began) -
      // there's no meaningful "how far through the jump" fraction to
      // compute, and Vircon32's FDIV hard-traps on a zero divisor
      // (unlike most platforms, which would just produce Infinity/NaN
      // here and carry on). scale=1 matches what the formula tends
      // toward at both endpoints of a normal jump anyway.
      scale = 1;
    } else {
      scale = sin(((froooogFrog.y - froooogFrog.targetY) / jumpSpan) * M_PI) + 1;
    }
    froooogDrawFrog(scale);
    if (froooogFrog.y < froooogFrog.targetY) {
      play(HIT);
      color = TRANSPARENT;
      froooogFrog.y = froooogFrog.targetY;
      Collision cl;
      box(50, froooogFrog.y, 1, 1, &cl);
      bool isf = cl.isColliding.rect[LIGHT_GREEN];
      int lc;
      if (isf || froooogFrog.isSafe) {
        lc = 0;
      } else {
        lc = ceil((froooogFrog.py - froooogFrog.y - 1) / 10);
      }
      addScore(lc * lc, 50, froooogFrog.y);
      froooogFrog.state = 0;
      froooogFrog.isSafe = isf;
    }
  }
}

void addGameFroooog() {
  addGame(froooogTitle, froooogDescription, froooogCharacters,
          froooogCharactersCount, &froooogOptions, false, &froooogUpdate);
}
