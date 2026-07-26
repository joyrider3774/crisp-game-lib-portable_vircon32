#include "../cglp.h"

int* zipfallTitle = "ZIP FALL";
int* zipfallDescription = "[Tap]\n Fall";

// Vircon32 port note: character rows shorter than CHARACTER_WIDTH(6) are
// padded with trailing spaces to exactly 6 characters (string-literal array
// initializers are checked strictly for size in this dialect - see
// VIRCON32_C_DIALECT.md 17.1).
int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] zipfallCharacters = {
    {
        "b bb b",
        " bbbb ",
        "bbbbbb",
        " b  b ",
        " b  b ",
    },
    {
        "r rr r",
        "rrrrrr",
        " r  r ",
    },
    {
        "  yy  ",
        " yyyy ",
        "y yy y",
        "y yy y",
        " yyyy ",
        "  yy  ",
    },
};
int zipfallCharactersCount = 3;

Options zipfallOptions = {100, 100, 3, false};

struct ZipfallZipLine {
  float y;
  bool isAlive;
};
// Vircon32 port note: upstream's backpacker.currentZip is a live reference
// to one of the (growable) zipLines array's objects, read every frame while
// attached (it keeps scrolling with the rest of the line until either the
// player detaches by tapping again, or the line itself scrolls off the top
// of the screen). There are no live references across this port's fixed
// arrays, so the backpacker instead stores which zip-line slot to keep
// reading (index + a "currently attached at all" flag) - same pattern as
// gameAccelb.c's AccelbPlayerMissile target. Zip lines are spawned roughly
// every 20-40 scrolled units and live for ~100 scrolled units (much longer
// than the gap between spawns), so at most a handful are ever alive at
// once; ZIPFALL_MAX_ZIPLINE_COUNT is sized generously above that so the
// ring buffer never recycles a slot the backpacker is still attached to.
#define ZIPFALL_MAX_ZIPLINE_COUNT 32
ZipfallZipLine[ZIPFALL_MAX_ZIPLINE_COUNT] zipfallZipLines;
int zipfallZipLineIndex;

struct ZipfallBackpacker {
  Vector pos;
  Vector vel;
  bool hasZip;
  int zipIndex;
};
ZipfallBackpacker zipfallBackpacker;

#define ZIPFALL_TYPE_OBSTACLE 0
#define ZIPFALL_TYPE_ITEM 1
struct ZipfallObject {
  Vector pos;
  float vx;
  int type;
  bool isAlive;
};
#define ZIPFALL_MAX_OBJECT_COUNT 32
ZipfallObject[ZIPFALL_MAX_OBJECT_COUNT] zipfallObjects;
int zipfallObjectIndex;

float zipfallScrollSpeed;
float zipfallNextZipDist;
float zipfallNextObjectTicks;
float zipfallMultiplier;

void zipfallAddZipLine(float y) {
  ASSIGN_ARRAY_ITEM(zipfallZipLines, zipfallZipLineIndex, ZipfallZipLine, z);
  z->y = y;
  z->isAlive = true;
  zipfallZipLineIndex = cgl_wrap(zipfallZipLineIndex + 1, 0, ZIPFALL_MAX_ZIPLINE_COUNT);
}

void zipfallUpdateBackpacker() {
  Collision scratch;
  if (input.isJustPressed) {
    play(POWER_UP);
    zipfallBackpacker.pos.y += 5;
    zipfallBackpacker.vel.y = 1;
    zipfallBackpacker.hasZip = false;
  }
  if (!zipfallBackpacker.hasZip) {
    zipfallBackpacker.vel.y += 0.1;
    zipfallBackpacker.pos.y += zipfallBackpacker.vel.y * difficulty - zipfallScrollSpeed;
  } else {
    zipfallBackpacker.pos.y = zipfallZipLines[zipfallBackpacker.zipIndex].y + 3;
    zipfallBackpacker.pos.x += zipfallBackpacker.vel.x * difficulty;
  }
  if ((zipfallBackpacker.pos.x < 20 && zipfallBackpacker.vel.x < 0) ||
      (zipfallBackpacker.pos.x > 80 && zipfallBackpacker.vel.x > 0)) {
    zipfallBackpacker.vel.x = -zipfallBackpacker.vel.x;
    if (zipfallMultiplier >= 2) {
      zipfallMultiplier--;
    }
  }
  if (zipfallBackpacker.pos.y < 3) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLUE;
  character("a", zipfallBackpacker.pos.x, zipfallBackpacker.pos.y, &scratch);
}

void zipfallUpdateZipLines() {
  Collision scratch;
  color = BLACK;
  FOR_EACH(zipfallZipLines, zi) {
    ASSIGN_ARRAY_ITEM(zipfallZipLines, zi, ZipfallZipLine, z);
    SKIP_IS_NOT_ALIVE(z);
    box(50, z->y, 60, 2, &scratch);
    if (scratch.isColliding.character['a'] && !zipfallBackpacker.hasZip) {
      play(CLICK);
      zipfallBackpacker.hasZip = true;
      zipfallBackpacker.zipIndex = zi;
    }
    z->y -= zipfallScrollSpeed;
    z->isAlive = z->y >= 0;
  }

  zipfallNextZipDist -= zipfallScrollSpeed;
  if (zipfallNextZipDist < 0) {
    zipfallAddZipLine(100);
    zipfallNextZipDist = rnd(20, 40);
  }
}

void zipfallUpdateObjects() {
  zipfallNextObjectTicks -= difficulty;
  if (zipfallNextObjectTicks <= 0) {
    int type;
    if (rnd(0, 1) < 0.6) {
      type = ZIPFALL_TYPE_OBSTACLE;
    } else {
      type = ZIPFALL_TYPE_ITEM;
    }
    float vx = rnd(0.5, 1) * RNDPM();
    Vector pos;
    if (vx > 0) {
      vectorSet(&pos, -3, rnd(0, 120));
    } else {
      vectorSet(&pos, 103, rnd(0, 120));
    }
    ASSIGN_ARRAY_ITEM(zipfallObjects, zipfallObjectIndex, ZipfallObject, no);
    no->pos = pos;
    no->vx = vx;
    no->type = type;
    no->isAlive = true;
    zipfallObjectIndex = cgl_wrap(zipfallObjectIndex + 1, 0, ZIPFALL_MAX_OBJECT_COUNT);
    zipfallNextObjectTicks = rnd(20, 30);
  }

  FOR_EACH(zipfallObjects, oi) {
    ASSIGN_ARRAY_ITEM(zipfallObjects, oi, ZipfallObject, obj);
    SKIP_IS_NOT_ALIVE(obj);
    obj->pos.y -= zipfallScrollSpeed;
    obj->pos.x += obj->vx * difficulty;
    if (obj->type == ZIPFALL_TYPE_OBSTACLE) {
      color = RED;
      Collision oc;
      character("b", obj->pos.x, obj->pos.y, &oc);
      if (oc.isColliding.character['a']) {
        play(EXPLOSION);
        gameOver();
      }
    } else {
      color = YELLOW;
      Collision ic;
      character("c", obj->pos.x, obj->pos.y, &ic);
      if (ic.isColliding.character['a']) {
        play(COIN);
        addScore(zipfallMultiplier, obj->pos.x, obj->pos.y);
        zipfallMultiplier += difficulty * 3;
        obj->isAlive = false;
        continue;
      }
    }
    obj->isAlive = !(obj->pos.x < -5 || obj->pos.x > 105);
  }
}

void zipfallUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(zipfallZipLines);
    zipfallZipLineIndex = 0;
    zipfallAddZipLine(20);
    zipfallAddZipLine(50);
    zipfallAddZipLine(75);
    vectorSet(&zipfallBackpacker.pos, 20, 20);
    vectorSet(&zipfallBackpacker.vel, 1, 0);
    zipfallBackpacker.hasZip = true;
    zipfallBackpacker.zipIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(zipfallObjects);
    zipfallObjectIndex = 0;
    zipfallScrollSpeed = 1;
    zipfallNextZipDist = 0;
    zipfallNextObjectTicks = 0;
    zipfallMultiplier = 1;
  }

  zipfallScrollSpeed = 0.05 * difficulty;
  if (zipfallBackpacker.pos.y > 30) {
    zipfallScrollSpeed -= (30 - zipfallBackpacker.pos.y) * 0.1;
  }
  zipfallUpdateBackpacker();
  zipfallUpdateZipLines();
  zipfallUpdateObjects();

  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(zipfallMultiplier)));
  text(multText, 2, 10, &scratch);
}

void addGameZipfall() {
  addGame(zipfallTitle, zipfallDescription, zipfallCharacters, zipfallCharactersCount,
          &zipfallOptions, false, &zipfallUpdate);
}
