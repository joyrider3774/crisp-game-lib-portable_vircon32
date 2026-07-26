#include "../cglp.h"

int* laserfortressTitle = "LASER FORTRESS";
int* laserfortressDescription = "[Hold]\n Laser irradiation";

int[6][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] laserfortressCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        "  l   ",
        "  l   ",
        "ll ll ",
        "  l   ",
        "  l   ",
    },
    {
        " l    ",
        "l  l  ",
        "lllll ",
        "l  l  ",
        " l    ",
    },
};
int laserfortressCharactersCount = 6;

Options laserfortressOptions = {160, 60, 1, false};

#define LASERFORTRESS_TYPE_ALLY 0
#define LASERFORTRESS_TYPE_ENEMY 1

struct LaserfortressObj {
  float x;
  float vx;
  float ticks;
  int type;
  bool isAlive;
};
#define LASERFORTRESS_MAX_OBJ_COUNT 32
LaserfortressObj[LASERFORTRESS_MAX_OBJ_COUNT] laserfortressObjs;
int laserfortressObjIndex;
float laserfortressNextObjTicks;
int laserfortressNextObjCount;
int laserfortressNextObjType;
int laserfortressHighSpeedIndex;
float laserfortressSightX;
bool laserfortressLaserActive;
float laserfortressLaserX;
int laserfortressMultiplier;

void laserfortressUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(laserfortressObjs);
    laserfortressObjIndex = 0;
    laserfortressNextObjTicks = 0;
    laserfortressNextObjCount = 5;
    laserfortressHighSpeedIndex = -1;
    laserfortressNextObjType = LASERFORTRESS_TYPE_ENEMY;
    laserfortressSightX = 40;
    laserfortressLaserActive = false;
    laserfortressMultiplier = 1;
  }
  color = BLUE;
  rect(0, 50, 200, 10, &scratch);
  color = LIGHT_CYAN;
  rect(0, 7, 14, 22, &scratch);
  character("f", 17, 8, &scratch);
  if (input.isJustPressed) {
    play(SELECT);
    laserfortressLaserX = laserfortressSightX;
    laserfortressLaserActive = true;
    laserfortressMultiplier = 1;
    color = BLUE;
    particle(20, 8, 9, 3, 0, CGLP_PI / 8);
  }
  if (laserfortressLaserActive && input.isPressed) {
    play(LASER);
    color = BLUE;
    thickness = 2;
    line(laserfortressLaserX, 50, 20, 8, &scratch);
    laserfortressLaserX += difficulty * 2;
    particle(laserfortressLaserX, 50, 1, 2, -CGLP_PI / 2, CGLP_PI / 6);
    color = PURPLE;
    box(laserfortressLaserX - 2, 50, 5, 1, &scratch);
  }
  laserfortressNextObjTicks--;
  if (laserfortressNextObjTicks < 0) {
    ASSIGN_ARRAY_ITEM(laserfortressObjs, laserfortressObjIndex, LaserfortressObj, no);
    no->x = 163;
    float vxMul;
    if (laserfortressNextObjCount == laserfortressHighSpeedIndex) {
      vxMul = 2;
    } else {
      vxMul = 1;
    }
    no->vx = difficulty * vxMul;
    no->ticks = rndi(0, 99);
    no->type = laserfortressNextObjType;
    no->isAlive = true;
    laserfortressObjIndex = cgl_wrap(laserfortressObjIndex + 1, 0, LASERFORTRESS_MAX_OBJ_COUNT);
    laserfortressNextObjCount--;
    if (laserfortressNextObjCount < 0) {
      laserfortressNextObjCount = 9 - floor(sqrt(rnd(0, 50)));
      laserfortressNextObjTicks = rnd(20, 30) / difficulty;
      if (laserfortressNextObjType == LASERFORTRESS_TYPE_ALLY) {
        laserfortressNextObjType = LASERFORTRESS_TYPE_ENEMY;
      } else if (rnd(0, 1) < 0.3) {
        laserfortressNextObjType = LASERFORTRESS_TYPE_ALLY;
      }
      if (rnd(0, 1) < 0.5) {
        laserfortressHighSpeedIndex = -1;
      } else {
        laserfortressHighSpeedIndex = rndi(0, 2);
      }
    } else {
      laserfortressNextObjTicks = rnd(5, 8) / difficulty;
    }
  }
  float minX = 200;
  FOR_EACH(laserfortressObjs, i) {
    ASSIGN_ARRAY_ITEM(laserfortressObjs, i, LaserfortressObj, o);
    SKIP_IS_NOT_ALIVE(o);
    o->x -= o->vx;
    if (o->type == LASERFORTRESS_TYPE_ENEMY && o->x < minX) {
      minX = o->x;
    }
    o->ticks++;
    if (o->type == LASERFORTRESS_TYPE_ALLY) {
      color = BLUE;
    } else {
      color = RED;
    }
    int[2] oc;
    if (o->type == LASERFORTRESS_TYPE_ALLY) {
      oc[0] = 'a' + (int)floor(o->ticks / 12) % 2;
    } else {
      oc[0] = 'c' + (int)floor(o->ticks / 12) % 2;
    }
    oc[1] = 0;
    characterOptions.isMirrorX = true;
    Collision ocoll;
    character(oc, o->x, 47, &ocoll);
    characterOptions.isMirrorX = false;
    if (ocoll.isColliding.rect[PURPLE]) {
      if (o->type == LASERFORTRESS_TYPE_ALLY) {
        play(EXPLOSION);
        gameOver();
      } else {
        play(HIT);
        particle(o->x, 47, 16, 1, 0, CGLP_PI * 2);
        addScore(laserfortressMultiplier, o->x, 47);
        laserfortressMultiplier++;
      }
      o->isAlive = false;
      continue;
    }
    if (o->x < 0) {
      if (o->type == LASERFORTRESS_TYPE_ENEMY) {
        play(EXPLOSION);
        color = RED;
        text("X", 3, 47, &scratch);
        gameOver();
      }
      o->isAlive = false;
      continue;
    }
  }
  if (minX < 200) {
    laserfortressSightX += (minX - difficulty * 3 - 5 - laserfortressSightX) * 0.3;
  }
  color = BLACK;
  character("e", laserfortressSightX, 47, &scratch);
}

void addGameLaserfortress() {
  addGame(laserfortressTitle, laserfortressDescription, laserfortressCharacters,
          laserfortressCharactersCount, &laserfortressOptions, false,
          &laserfortressUpdate);
}
