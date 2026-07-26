#include "../cglp.h"

int* thrustlrTitle = "THRUST LR";
int* thrustlrDescription = "[Slide]\n Thrust";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] thrustlrCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int thrustlrCharactersCount = 1;

Options thrustlrOptions = {100, 100, 80, true};

#define THRUSTLR_SHIP_Y 85

struct ThrustlrRect {
  Vector pos;
  float vx;
  float size;
  bool isAlive;
};
#define THRUSTLR_MAX_RECT_COUNT 32
ThrustlrRect[THRUSTLR_MAX_RECT_COUNT] thrustlrRects;
int thrustlrRectIndex;
float thrustlrNextRectDist;

struct ThrustlrShip {
  float x;
  float vx;
};
ThrustlrShip thrustlrShip;

float thrustlrAddRect(float y) {
  bool t = rnd(0, 1) < 0.3;
  float size;
  float x;
  if (t) {
    size = rnd(7, 12);
    x = rnd(10, 90);
  } else {
    size = rnd(20, 30);
    x = 50 + rnd(40, 60) * RNDPM();
  }
  ASSIGN_ARRAY_ITEM(thrustlrRects, thrustlrRectIndex, ThrustlrRect, r);
  vectorSet(&r->pos, x, y - size);
  r->vx = 0;
  r->size = size;
  r->isAlive = true;
  thrustlrRectIndex = cgl_wrap(thrustlrRectIndex + 1, 0, THRUSTLR_MAX_RECT_COUNT);
  return size;
}

void thrustlrUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(thrustlrRects);
    thrustlrRectIndex = 0;
    thrustlrNextRectDist = 0;
    thrustlrShip.x = 50;
    thrustlrShip.vx = 0;
    thrustlrAddRect(60);
    thrustlrAddRect(40);
    thrustlrAddRect(20);
  }
  float scr = difficulty * 0.2;
  thrustlrNextRectDist -= scr;
  if (thrustlrNextRectDist < 0) {
    float size = thrustlrAddRect(0);
    thrustlrNextRectDist = rnd(10, size);
  }
  float tx = -(clamp(input.pos.x, 0, 100) - 50);
  FOR_EACH(thrustlrRects, i) {
    ASSIGN_ARRAY_ITEM(thrustlrRects, i, ThrustlrRect, r);
    SKIP_IS_NOT_ALIVE(r);
    r->pos.y += scr;
    if (r->pos.y - r->size < THRUSTLR_SHIP_Y && r->pos.y + r->size > THRUSTLR_SHIP_Y) {
      float d = r->pos.x - thrustlrShip.x;
      if (d * tx < 0) {
        r->vx -= (((tx / (fabs(d) - (r->size - fabs(r->pos.y - THRUSTLR_SHIP_Y)))) *
                   0.2) /
                  r->size) *
                 difficulty;
      }
      r->vx *= 0.98;
    }
    r->pos.x += r->vx;
    if (r->pos.x < -r->size || r->pos.x > 99 + r->size) {
      play(POWER_UP);
      float scoreX;
      if (r->pos.x < 50) {
        scoreX = 10;
      } else {
        scoreX = 90;
      }
      addScore(floor(r->size), scoreX, clamp(r->pos.y, 0, 95));
      r->isAlive = false;
      continue;
    }
    thickness = 3;
    line(r->pos.x + r->size, r->pos.y, r->pos.x, r->pos.y + r->size, &scratch);
    thickness = 3;
    line(r->pos.x, r->pos.y + r->size, r->pos.x - r->size, r->pos.y, &scratch);
    thickness = 3;
    line(r->pos.x - r->size, r->pos.y, r->pos.x, r->pos.y - r->size, &scratch);
    thickness = 3;
    line(r->pos.x, r->pos.y - r->size, r->pos.x + r->size, r->pos.y, &scratch);
    r->isAlive = r->pos.y - r->size * 2 <= 99;
  }
  color = LIGHT_BLACK;
  rect(50, 93, 1, 7, &scratch);
  rect(50, 95, -tx, 3, &scratch);
  color = BLACK;
  thrustlrShip.vx += tx * 0.001 * sqrt(difficulty);
  thrustlrShip.x = clamp(thrustlrShip.x + thrustlrShip.vx, 4, 96);
  if ((thrustlrShip.x < 5 && thrustlrShip.vx < 0) ||
      (thrustlrShip.x > 95 && thrustlrShip.vx > 0)) {
    thrustlrShip.vx *= -0.7;
  }
  thrustlrShip.vx *= 0.95;
  particle(thrustlrShip.x, THRUSTLR_SHIP_Y, fabs(tx) * 0.05, -tx * 0.05, 0, 0.5);
  thickness = 2;
  Collision c1;
  line(thrustlrShip.x - 1, THRUSTLR_SHIP_Y - 2, thrustlrShip.x - 2,
       THRUSTLR_SHIP_Y + 2, &c1);
  thickness = 2;
  Collision c2;
  line(thrustlrShip.x + 1, THRUSTLR_SHIP_Y - 2, thrustlrShip.x + 2,
       THRUSTLR_SHIP_Y + 2, &c2);
  if (c1.isColliding.rect[BLACK] || c2.isColliding.rect[BLACK]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameThrustlr() {
  addGame(thrustlrTitle, thrustlrDescription, thrustlrCharacters,
          thrustlrCharactersCount, &thrustlrOptions, true, &thrustlrUpdate);
}
