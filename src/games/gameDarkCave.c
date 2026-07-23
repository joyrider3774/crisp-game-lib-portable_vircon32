#include "../cglp.h"

char* darkcaveTitle = "DARK CAVE";
char* darkcaveDescription = "[Slide] Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] darkcaveCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int darkcaveCharactersCount = 0;

Options darkcaveOptions = {100, 100, 6, true};

struct DarkcaveObject {
  Vector pos;
  int type; // 's' for spike, 'c' for coin
  bool isAlive;
};

#define DARKCAVE_MAX_OBJ_COUNT 64
DarkcaveObject[DARKCAVE_MAX_OBJ_COUNT] darkcaveObjs;
int darkcaveObjIndex;
float darkcaveNextSpikeDist;
float darkcaveNextCoinDist;

struct DarkcavePlayer {
  Vector pos;
  float angle;
  float range;
};

DarkcavePlayer darkcavePlayer;
int darkcavePressedTicks;
float darkcaveAngleWidth = M_PI / 6;

void darkcaveUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(darkcaveObjs);
    darkcaveObjIndex = 0;
    darkcaveNextSpikeDist = 0;
    darkcaveNextCoinDist = 10;
    darkcavePressedTicks = 0;
    darkcavePlayer.pos.x = 50;
    darkcavePlayer.pos.y = 90;
    darkcavePlayer.angle = 0;
    darkcavePlayer.range = 40;
  }

  float scr = difficulty * (0.1 + darkcavePlayer.range * 0.01);

  // Update spike generation
  darkcaveNextSpikeDist -= scr;
  if (darkcaveNextSpikeDist < 0) {
    ASSIGN_ARRAY_ITEM(darkcaveObjs, darkcaveObjIndex, DarkcaveObject, o);
    o->pos.x = rnd(5, 95);
    o->pos.y = 0;
    o->type = 's';
    o->isAlive = true;
    darkcaveObjIndex = cgl_wrap(darkcaveObjIndex + 1, 0, DARKCAVE_MAX_OBJ_COUNT);
    darkcaveNextSpikeDist = rnd(20, 30);
  }

  // Update coin generation
  darkcaveNextCoinDist -= scr;
  if (darkcaveNextCoinDist < 0) {
    ASSIGN_ARRAY_ITEM(darkcaveObjs, darkcaveObjIndex, DarkcaveObject, o);
    o->pos.x = rnd(10, 90);
    o->pos.y = 0;
    o->type = 'c';
    o->isAlive = true;
    darkcaveObjIndex = cgl_wrap(darkcaveObjIndex + 1, 0, DARKCAVE_MAX_OBJ_COUNT);
    darkcaveNextCoinDist = rnd(30, 40);
  }

  // Update player movement
  float pp = darkcavePlayer.pos.x;
  darkcavePlayer.pos.x = clamp(input.pos.x, 3, 97);
  float vx = clamp(darkcavePlayer.pos.x - pp, -0.1, 0.1);
  darkcavePlayer.angle = clamp(darkcavePlayer.angle + vx * sqrt(difficulty), -M_PI / 8, M_PI / 8);
  darkcavePlayer.range *= 1 - 0.001 * sqrt(difficulty);

  // Draw player
  color = WHITE;
  box(darkcavePlayer.pos.x, darkcavePlayer.pos.y, 7, 7, &scratch);

  // Update and draw objects
  FOR_EACH(darkcaveObjs, i) {
    ASSIGN_ARRAY_ITEM(darkcaveObjs, i, DarkcaveObject, o);
    SKIP_IS_NOT_ALIVE(o);

    o->pos.y += scr;
    float d = distanceTo(&darkcavePlayer.pos, o->pos.x, o->pos.y);
    float a = angleTo(&darkcavePlayer.pos, o->pos.x, o->pos.y) + M_PI / 2;

    int* symbol;
    if (o->type == 'c') {
      symbol = "$";
    } else {
      symbol = "*";
    }

    if (fabs(a - darkcavePlayer.angle) < darkcaveAngleWidth) {
      if (o->type == 'c') {
        color = YELLOW;
      } else {
        color = PURPLE;
      }
      if (d < darkcavePlayer.range) {
        text(symbol, o->pos.x, o->pos.y, &scratch);
      } else {
        particle(o->pos.x, o->pos.y, 0.05, 1, 0, M_PI * 2);
      }
    }

    color = TRANSPARENT;
    Collision tCl;
    text(symbol, o->pos.x, o->pos.y, &tCl);
    if (tCl.isColliding.rect[WHITE]) {
      if (o->type == 'c') {
        play(POWER_UP);
        addScore(floor(darkcavePlayer.range), o->pos.x, o->pos.y);
        darkcavePlayer.range = clamp(darkcavePlayer.range + 9, 9, 99);
      } else {
        play(EXPLOSION);
        color = RED;
        text("*", o->pos.x, o->pos.y, &scratch);
        particle(o->pos.x, o->pos.y, 5, 1, 0, M_PI * 2);
        darkcavePlayer.range /= 3;
      }
      o->isAlive = false;
    }
  }

  // Draw player's vision cone
  if (darkcavePlayer.range < 9) {
    color = RED;
  } else {
    color = BLACK;
  }
  thickness = 2;

  float a1 = darkcavePlayer.angle - darkcaveAngleWidth - M_PI / 2;
  float a2 = a1 + darkcaveAngleWidth * 2;

  Vector p1, p2;
  vectorSet(&p1, darkcavePlayer.pos.x, darkcavePlayer.pos.y);
  vectorSet(&p2, darkcavePlayer.pos.x, darkcavePlayer.pos.y);
  addWithAngle(&p1, a1, darkcavePlayer.range);
  addWithAngle(&p2, a2, darkcavePlayer.range);

  line(darkcavePlayer.pos.x, darkcavePlayer.pos.y, p1.x, p1.y, &scratch);
  line(darkcavePlayer.pos.x, darkcavePlayer.pos.y, p2.x, p2.y, &scratch);
  arc(darkcavePlayer.pos.x, darkcavePlayer.pos.y, darkcavePlayer.range, a1, a2, &scratch);

  // Check game over condition
  if (darkcavePlayer.range < 9) {
    play(RANDOM);
    gameOver();
  }
}

void addGameDarkCave() {
  addGame(darkcaveTitle, darkcaveDescription, darkcaveCharacters, darkcaveCharactersCount,
          &darkcaveOptions, true, &darkcaveUpdate);
}
