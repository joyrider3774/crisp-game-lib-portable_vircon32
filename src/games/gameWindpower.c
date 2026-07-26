#include "../cglp.h"

int* windpowerTitle = "WIND POWER";
int* windpowerDescription = "[Hold] Blow air";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] windpowerCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int windpowerCharactersCount = 0;

Options windpowerOptions = {100, 100, 1, false};

// Vircon32 port note: upstream initializes `player` (a Vector) but never
// draws or reads it anywhere else in update() - a vestigial leftover in
// the original source (dead code, not a translation gap). Kept here,
// initialized but otherwise unused, to stay a faithful line-for-line port
// rather than second-guessing upstream.
Vector windpowerPlayer;

struct WindpowerWindmill {
  Vector pos;
  float speed;
  float angle;
};
WindpowerWindmill windpowerPlayerWindmill;
WindpowerWindmill windpowerWindmill;

struct WindpowerObstacle {
  Vector pos;
  float size;
  float vx;
  bool isAlive;
};
#define WINDPOWER_MAX_OBSTACLE_COUNT 16
WindpowerObstacle[WINDPOWER_MAX_OBSTACLE_COUNT] windpowerObstacles;
int windpowerObstacleIndex;

float windpowerNextObstacleTicks;
float windpowerWindPower;

void windpowerUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&windpowerPlayer, 0, 50);
    vectorSet(&windpowerPlayerWindmill.pos, 10, 60);
    windpowerPlayerWindmill.speed = 0;
    windpowerPlayerWindmill.angle = 0;
    vectorSet(&windpowerWindmill.pos, 85, 60);
    windpowerWindmill.speed = 0;
    windpowerWindmill.angle = 0;
    INIT_UNALIVED_ARRAY_FAST(windpowerObstacles);
    windpowerObstacleIndex = 0;
    windpowerNextObstacleTicks = 0;
    windpowerWindPower = 0;
  }
  if (input.isPressed) {
    windpowerWindPower += difficulty * 0.02;
    windpowerWindPower = clamp(windpowerWindPower, 0, difficulty);
  } else {
    windpowerWindPower *= 0.9;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isJustReleased) {
    play(CLICK);
  }
  windpowerPlayerWindmill.speed = windpowerWindPower * 2;
  windpowerPlayerWindmill.angle += windpowerPlayerWindmill.speed * 0.2;
  color = CYAN;
  box(windpowerPlayerWindmill.pos.x, windpowerPlayerWindmill.pos.y, 5,
      sin(windpowerPlayerWindmill.angle) * 20, &scratch);
  windpowerWindmill.speed += windpowerWindPower * 0.02;
  windpowerWindmill.speed = fmin(windpowerWindmill.speed, 10);
  windpowerWindmill.speed *= 0.98;
  windpowerWindmill.angle += windpowerWindmill.speed * 0.2;
  color = LIGHT_BLACK;
  thickness = 4;
  barCenterPosRatio = 0;
  TIMES(3, i) {
    bar(windpowerWindmill.pos.x, windpowerWindmill.pos.y, 10,
        windpowerWindmill.angle + CGLP_PI * 2 / 3 * i, &scratch);
  }
  if (windpowerWindmill.angle > CGLP_PI * 2) {
    play(COIN);
    windpowerWindmill.angle = fmod(windpowerWindmill.angle, CGLP_PI * 2);
  }
  addScore(windpowerWindmill.speed, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  windpowerNextObstacleTicks -= difficulty;
  if (windpowerNextObstacleTicks < 0) {
    play(HIT);
    float size = rnd(5, 10);
    ASSIGN_ARRAY_ITEM(windpowerObstacles, windpowerObstacleIndex, WindpowerObstacle, no);
    vectorSet(&no->pos, rnd(20, 99), -size);
    no->size = size;
    no->vx = 0;
    no->isAlive = true;
    windpowerObstacleIndex = cgl_wrap(windpowerObstacleIndex + 1, 0, WINDPOWER_MAX_OBSTACLE_COUNT);
    windpowerNextObstacleTicks = rnd(75, 250);
  }
  FOR_EACH(windpowerObstacles, oi) {
    ASSIGN_ARRAY_ITEM(windpowerObstacles, oi, WindpowerObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.y += difficulty * 0.5;
    if (o->pos.y > 20 && o->pos.y < 100) {
      o->vx += windpowerWindPower / o->size;
    }
    o->vx *= 0.9;
    o->pos.x += o->vx;
    color = BLACK;
    box(o->pos.x, o->pos.y, o->size, o->size, &scratch);
    // Vircon32 port note: upstream really does call box() twice with
    // identical arguments here - once to draw, once more just to read its
    // collision result. Kept as two separate calls (not deduped to one)
    // to stay a faithful port: this engine's collision detection uses
    // draw order as part of its semantics (see "Collision detection" in
    // PORTING.md / checkHitBox() in cglp.c), so the duplicate
    // registration is part of the original observable behavior, not just
    // an accident of translation.
    Collision oc;
    box(o->pos.x, o->pos.y, o->size, o->size, &oc);
    if (oc.isColliding.rect[LIGHT_BLACK]) {
      play(EXPLOSION);
      gameOver();
    }
    if (o->pos.y > 100) {
      o->isAlive = false;
      continue;
    }
  }
}

void addGameWindpower() {
  addGame(windpowerTitle, windpowerDescription, windpowerCharacters,
          windpowerCharactersCount, &windpowerOptions, false, &windpowerUpdate);
}
