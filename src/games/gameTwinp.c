#include "../cglp.h"

int* twinpTitle = "TWIN P";
int* twinpDescription = "[Hold]\n Stretch\n[Release]\n Pin";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] twinpCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int twinpCharactersCount = 1;

Options twinpOptions = {100, 100, 16, true};

struct TwinpObj {
  Vector pos;
  Vector vel;
  bool isCross;
  bool isIn;
  bool isAlive;
};
#define TWINP_MAX_OBJ_COUNT 32
TwinpObj[TWINP_MAX_OBJ_COUNT] twinpObjs;
int twinpObjIndex;

Vector twinpPos;
float twinpAngle;
float twinpLen;
float twinpTargetLen;
bool twinpColorFlag;
float twinpObjAddTicks;
int twinpCrossAddCount;
int twinpMultiplier;

void twinpUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&twinpPos, 40, 50);
    twinpAngle = 0;
    twinpLen = 15;
    twinpTargetLen = 15;
    twinpColorFlag = true;
    INIT_UNALIVED_ARRAY_FAST(twinpObjs);
    twinpObjIndex = 0;
    twinpObjAddTicks = 0;
    twinpCrossAddCount = 9;
    twinpMultiplier = 1;
  }
  Vector np;
  vectorSet(&np, twinpPos.x, twinpPos.y);
  addWithAngle(&np, twinpAngle, twinpLen);
  color = BLACK;
  thickness = 2;
  line(twinpPos.x, twinpPos.y, np.x, np.y, &scratch);
  if (twinpColorFlag) {
    color = RED;
  } else {
    color = BLUE;
  }
  text("P", twinpPos.x + 3, twinpPos.y - 3, &scratch);
  if (twinpColorFlag) {
    color = BLUE;
  } else {
    color = RED;
  }
  text("P", np.x + 3, np.y - 3, &scratch);
  bool posInRect =
      twinpPos.x >= 0 && twinpPos.x < 99 && twinpPos.y >= 0 && twinpPos.y < 99;
  bool npInRect = np.x >= 0 && np.x < 99 && np.y >= 0 && np.y < 99;
  if (!posInRect && !npInRect) {
    play(RANDOM);
    color = RED;
    text("X", twinpPos.x, twinpPos.y, &scratch);
    text("X", np.x, np.y, &scratch);
    gameOver();
  }
  if (input.isJustReleased) {
    play(SELECT);
    twinpColorFlag = !twinpColorFlag;
    twinpPos = np;
    twinpAngle += CGLP_PI;
    twinpTargetLen = 15;
  } else if (input.isPressed) {
    play(LASER);
    if (twinpTargetLen < 99) {
      twinpTargetLen += difficulty * 0.2;
    }
  }
  twinpAngle += difficulty * 0.05;
  twinpLen += (twinpTargetLen - twinpLen) * 0.2;
  twinpObjAddTicks--;
  if (twinpObjAddTicks < 0) {
    bool isCrossType = twinpCrossAddCount == 0;
    ASSIGN_ARRAY_ITEM(twinpObjs, twinpObjIndex, TwinpObj, o);
    vectorSet(&o->pos, 50, 50);
    addWithAngle(&o->pos, rnd(0, CGLP_PI * 2), 80);
    float velAngle = angleTo(&o->pos, rnd(20, 80), rnd(20, 80));
    vectorSet(&o->vel, 0, 0);
    addWithAngle(&o->vel, velAngle, rnd(1, difficulty) * 0.2);
    o->isCross = isCrossType;
    o->isIn = false;
    o->isAlive = true;
    twinpObjIndex = cgl_wrap(twinpObjIndex + 1, 0, TWINP_MAX_OBJ_COUNT);
    twinpCrossAddCount--;
    if (twinpCrossAddCount < 0) {
      twinpCrossAddCount = rndi(8, 12);
    }
    twinpObjAddTicks = rnd(60, 90) / difficulty;
  }
  FOR_EACH(twinpObjs, i) {
    ASSIGN_ARRAY_ITEM(twinpObjs, i, TwinpObj, o);
    SKIP_IS_NOT_ALIVE(o);
    vectorAdd(&o->pos, o->vel.x, o->vel.y);
    bool oInRect =
        o->pos.x >= -5 && o->pos.x < 105 && o->pos.y >= -5 && o->pos.y < 105;
    if (oInRect) {
      o->isIn = true;
    } else if (o->isIn) {
      if (!o->isCross) {
        if (twinpMultiplier > 1) {
          play(HIT);
          twinpMultiplier--;
        }
      }
      o->isAlive = false;
      continue;
    }
    if (!o->isCross) {
      color = YELLOW;
      Collision oc;
      box(o->pos.x, o->pos.y, 4, 4, &oc);
      if (oc.isColliding.rect[BLACK]) {
        play(COIN);
        addScore(twinpMultiplier, o->pos.x, o->pos.y);
        twinpMultiplier++;
        o->isAlive = false;
        continue;
      }
    } else {
      color = PURPLE;
      thickness = 2;
      Collision c1;
      bar(o->pos.x, o->pos.y, 4, CGLP_PI / 4, &c1);
      thickness = 2;
      Collision c2;
      bar(o->pos.x, o->pos.y, 4, -CGLP_PI / 4, &c2);
      if (c1.isColliding.rect[BLACK] || c2.isColliding.rect[BLACK]) {
        play(RANDOM);
        gameOver();
      }
    }
  }
}

void addGameTwinp() {
  addGame(twinpTitle, twinpDescription, twinpCharacters, twinpCharactersCount,
          &twinpOptions, false, &twinpUpdate);
}
