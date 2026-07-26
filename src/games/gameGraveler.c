#include "../cglp.h"

int* gravelerTitle = "GRAVELER";
int* gravelerDescription = "[Hold]\n Reverse gravity";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] gravelerCharacters = {{
    "l     ",
    " lll  ",
    "ll  ll",
    " ll   ",
}};
int gravelerCharactersCount = 1;

Options gravelerOptions = {100, 100, 29, true};

#define GRAVELER_SX 10

struct GravelerWall {
  Vector pos;
  float width;
};
#define GRAVELER_WALL_COUNT 11
GravelerWall[GRAVELER_WALL_COUNT] gravelerWalls;

struct GravelerNextWall {
  float y;
  float vy;
  float w;
  float wy;
};
GravelerNextWall gravelerNextWall;
float gravelerNextWallDist;
int gravelerNextWallIndex;
float gravelerGrvOfs;

float gravelerSy;
float gravelerSvy;

#define GRAVELER_OBJ_TYPE_COIN 0
#define GRAVELER_OBJ_TYPE_SPIKE 1
struct GravelerObj {
  Vector pos;
  int type;
  bool isAlive;
};
#define GRAVELER_MAX_OBJ_COUNT 32
GravelerObj[GRAVELER_MAX_OBJ_COUNT] gravelerObjs;
int gravelerObjIndex;
float gravelerNextObjDist;
int gravelerCoinCount;
float gravelerCoinY;
int gravelerMultiplier;

void gravelerUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(GRAVELER_WALL_COUNT, i) {
      vectorSet(&gravelerWalls[i].pos, i * 10, 50);
      gravelerWalls[i].width = 60;
    }
    gravelerNextWall.y = 50;
    gravelerNextWall.vy = 0;
    gravelerNextWall.w = 60;
    gravelerNextWall.wy = 0;
    gravelerNextWallDist = 10;
    gravelerNextWallIndex = 0;
    gravelerGrvOfs = 0;
    gravelerSy = 65;
    gravelerSvy = 0;
    INIT_UNALIVED_ARRAY_FAST(gravelerObjs);
    gravelerObjIndex = 0;
    TIMES(5, i) {
      ASSIGN_ARRAY_ITEM(gravelerObjs, gravelerObjIndex, GravelerObj, no);
      vectorSet(&no->pos, i * 7 + 15, 65);
      no->type = GRAVELER_OBJ_TYPE_COIN;
      no->isAlive = true;
      gravelerObjIndex = cgl_wrap(gravelerObjIndex + 1, 0, GRAVELER_MAX_OBJ_COUNT);
    }
    gravelerNextObjDist = 10;
    gravelerCoinCount = 0;
    gravelerMultiplier = 1;
  }
  // BGM tempo escalation every 600 ticks omitted: the JS sss.setTempo() /
  // stopBgm() / playBgm() dynamic-tempo API has no equivalent here - this
  // port's BGM plays at a single fixed tempo for the whole session.
  float scr = difficulty * 0.5;
  TIMES(GRAVELER_WALL_COUNT, i) {
    GravelerWall* w = &gravelerWalls[i];
    color = BLUE;
    rect(w->pos.x, 0, 11, w->pos.y - w->width / 2, &scratch);
    rect(w->pos.x, w->pos.y + w->width / 2, 11, 101 - w->pos.y - w->width / 2, &scratch);
    if (input.isPressed) {
      color = PURPLE;
    } else {
      color = CYAN;
    }
    for (float y = w->pos.y - w->width / 2 + gravelerGrvOfs; y < w->pos.y; y += 10) {
      rect(w->pos.x, y, 10, 1, &scratch);
    }
    for (float y = w->pos.y + w->width / 2 - gravelerGrvOfs; y > w->pos.y; y -= 10) {
      rect(w->pos.x, y, 10, 1, &scratch);
    }
    if (GRAVELER_SX >= w->pos.x && GRAVELER_SX < w->pos.x + 10) {
      float f;
      if (gravelerSy < w->pos.y) {
        f = -1;
      } else {
        f = 1;
      }
      if (input.isPressed) {
        f *= -1.5;
      }
      gravelerSvy += sqrt(difficulty) * f * 0.015;
    }
    w->pos.x -= scr;
  }
  float grvDelta;
  if (input.isPressed) {
    grvDelta = 0.25;
  } else {
    grvDelta = -0.16;
  }
  gravelerGrvOfs = cgl_wrap(gravelerGrvOfs + difficulty * grvDelta, 0, 10);
  gravelerNextWallDist -= scr;
  if (gravelerNextWallDist <= 0) {
    GravelerWall* w = &gravelerWalls[gravelerNextWallIndex];
    gravelerNextWallIndex = (int)cgl_wrap(gravelerNextWallIndex + 1, 0, GRAVELER_WALL_COUNT);
    gravelerNextWall.vy += rnd(0, 0.2) * RNDPM();
    gravelerNextWall.y += gravelerNextWall.vy;
    gravelerNextWall.wy += rnd(0, 0.2) * RNDPM();
    gravelerNextWall.w += gravelerNextWall.wy;
    if (gravelerNextWall.y - gravelerNextWall.w / 2 < 20 && gravelerNextWall.vy < 0) {
      gravelerNextWall.vy += 1;
    }
    if (gravelerNextWall.y + gravelerNextWall.w / 2 > 80 && gravelerNextWall.vy > 0) {
      gravelerNextWall.vy -= 1;
    }
    if (gravelerNextWall.w < 32 && gravelerNextWall.wy < 0) {
      gravelerNextWall.wy += 1;
    }
    if (gravelerNextWall.w > 60 && gravelerNextWall.wy > 0) {
      gravelerNextWall.wy -= 1;
    }
    vectorSet(&w->pos, 100 + gravelerNextWallDist, gravelerNextWall.y);
    w->width = gravelerNextWall.w;
    gravelerNextWallDist += 10;
  }
  color = BLACK;
  gravelerSy += gravelerSvy;
  gravelerSvy *= 0.99;
  Collision sc;
  character("a", GRAVELER_SX, gravelerSy, &sc);
  if (sc.isColliding.rect[BLUE]) {
    play(EXPLOSION);
    gameOver();
  }
  if (input.isJustPressed) {
    play(LASER);
  } else if (input.isJustReleased) {
    play(HIT);
  }
  FOR_EACH(gravelerObjs, i) {
    ASSIGN_ARRAY_ITEM(gravelerObjs, i, GravelerObj, o);
    SKIP_IS_NOT_ALIVE(o);
    if (o->type == GRAVELER_OBJ_TYPE_COIN) {
      color = YELLOW;
      Collision oc;
      text("o", o->pos.x, o->pos.y, &oc);
      if (oc.isColliding.character['a']) {
        play(POWER_UP);
        addScore(gravelerMultiplier, o->pos.x, o->pos.y);
        gravelerMultiplier++;
        o->isAlive = false;
        continue;
      }
      if (oc.isColliding.rect[BLUE]) {
        if (gravelerCoinY < 50) {
          gravelerCoinY += 10;
        } else {
          gravelerCoinY -= 10;
        }
        o->isAlive = false;
        continue;
      }
    } else {
      color = RED;
      Collision oc2;
      text("x", o->pos.x, o->pos.y, &oc2);
      if (oc2.isColliding.character['a']) {
        play(EXPLOSION);
        gameOver();
      }
    }
    o->pos.x -= scr;
    if (o->pos.x < -3) {
      if (o->type == GRAVELER_OBJ_TYPE_COIN && gravelerMultiplier > 1) {
        gravelerMultiplier--;
      }
      o->isAlive = false;
      continue;
    }
  }
  gravelerNextObjDist -= scr;
  if (gravelerNextObjDist < 0) {
    GravelerWall* w =
        &gravelerWalls[(int)cgl_wrap(gravelerNextWallIndex - 1, 0, GRAVELER_WALL_COUNT)];
    if (gravelerCoinCount == 0) {
      ASSIGN_ARRAY_ITEM(gravelerObjs, gravelerObjIndex, GravelerObj, no);
      vectorSet(&no->pos, 103, rnd(w->pos.y - 10, w->pos.y + 10));
      no->type = GRAVELER_OBJ_TYPE_SPIKE;
      no->isAlive = true;
      gravelerObjIndex = cgl_wrap(gravelerObjIndex + 1, 0, GRAVELER_MAX_OBJ_COUNT);
      gravelerCoinCount = rndi(3, 7);
      gravelerNextObjDist = rnd(40, 60) * sqrt(difficulty);
      if (rnd(0, 1) < 0.5) {
        gravelerCoinY = rnd(w->pos.y - w->width * 0.36, w->pos.y - w->width * 0.2);
      } else {
        gravelerCoinY = rnd(w->pos.y + w->width * 0.36, w->pos.y + w->width * 0.2);
      }
    } else {
      ASSIGN_ARRAY_ITEM(gravelerObjs, gravelerObjIndex, GravelerObj, no2);
      vectorSet(&no2->pos, 103, gravelerCoinY);
      no2->type = GRAVELER_OBJ_TYPE_COIN;
      no2->isAlive = true;
      gravelerObjIndex = cgl_wrap(gravelerObjIndex + 1, 0, GRAVELER_MAX_OBJ_COUNT);
      gravelerCoinCount--;
      if (gravelerCoinCount == 0) {
        gravelerNextObjDist = rnd(40, 60) * sqrt(difficulty);
      } else {
        gravelerNextObjDist = 7;
      }
    }
  }
}

void addGameGraveler() {
  addGame(gravelerTitle, gravelerDescription, gravelerCharacters,
          gravelerCharactersCount, &gravelerOptions, false, &gravelerUpdate);
}
