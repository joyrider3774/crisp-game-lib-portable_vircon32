#include "../cglp.h"

int* scaffoldTitle = "SCAFFOLD";
int* scaffoldDescription = "[Tap]\n Change angle\n[Hold]\n Scaffold";

int[5][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] scaffoldCharacters = {
    {
        "    ll",
        "   lll",
        "  ll  ",
        " ll   ",
        "ll    ",
        "l     ",
    },
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "llllll",
        "llllll",
    },
    {
        "l     ",
        "ll    ",
        " ll   ",
        "  ll  ",
        "   lll",
        "    ll",
    },
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
};
int scaffoldCharactersCount = 5;

Options scaffoldOptions = {100, 100, 100, false};

#define SCAFFOLD_OBJ_TYPE_GOLD 0
#define SCAFFOLD_OBJ_TYPE_SPIKE 1

struct ScaffoldFloor {
  Vector pos;
  int type;
  bool isAlive;
};
#define SCAFFOLD_MAX_FLOOR_COUNT 64
ScaffoldFloor[SCAFFOLD_MAX_FLOOR_COUNT] scaffoldFloors;
int scaffoldFloorIndex;

Vector scaffoldNextFloorPos;
int scaffoldNextFloorType;
int scaffoldTv;
int scaffoldPressedCount;
Vector scaffoldWall;

struct ScaffoldObj {
  Vector pos;
  float vy;
  float d;
  float distance;
  int type;
  bool isAlive;
};
#define SCAFFOLD_MAX_OBJ_COUNT 64
ScaffoldObj[SCAFFOLD_MAX_OBJ_COUNT] scaffoldObjs;
int scaffoldObjIndex;
float scaffoldNextObjDist;

Vector scaffoldPlayer;
int scaffoldMultiplier;

void scaffoldUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(scaffoldFloors);
    scaffoldFloorIndex = 0;
    TIMES(9, fi) {
      ASSIGN_ARRAY_ITEM(scaffoldFloors, scaffoldFloorIndex, ScaffoldFloor, f0);
      vectorSet(&f0->pos, fi * 6 + 3, 52);
      f0->type = 1;
      f0->isAlive = true;
      scaffoldFloorIndex = cgl_wrap(scaffoldFloorIndex + 1, 0, SCAFFOLD_MAX_FLOOR_COUNT);
    }
    vectorSet(&scaffoldNextFloorPos, 9 * 6 + 3, 52);
    scaffoldNextFloorType = 0;
    scaffoldTv = 1;
    scaffoldPressedCount = 0;
    vectorSet(&scaffoldWall, -9, 0);
    INIT_UNALIVED_ARRAY_FAST(scaffoldObjs);
    scaffoldObjIndex = 0;
    scaffoldNextObjDist = 0;
    vectorSet(&scaffoldPlayer, 5, 50);
    scaffoldMultiplier = 1;
  }
  float scr = difficulty * 0.05;
  if (scaffoldNextFloorPos.x > 40) {
    scr += (scaffoldNextFloorPos.x - 40) * 0.1;
  }
  scaffoldWall.x -= scr;
  color = RED;
  if (scaffoldWall.x < -6) {
    float wy = 47;
    TIMES(9, wi) {
      float y = 47 + rndi(-9, 9) * 4;
      if (fabs(y - scaffoldNextFloorPos.y) < 25) {
        wy = y;
        break;
      }
    }
    vectorSet(&scaffoldWall, 6 - fmod(scaffoldNextFloorPos.x, 6) + 120, wy);
    ASSIGN_ARRAY_ITEM(scaffoldObjs, scaffoldObjIndex, ScaffoldObj, ngo);
    vectorSet(&ngo->pos, scaffoldWall.x + 4, scaffoldWall.y + 4);
    ngo->vy = 0.1;
    ngo->d = 2;
    ngo->distance = 4;
    ngo->type = SCAFFOLD_OBJ_TYPE_GOLD;
    ngo->isAlive = true;
    scaffoldObjIndex = cgl_wrap(scaffoldObjIndex + 1, 0, SCAFFOLD_MAX_OBJ_COUNT);
    color = PURPLE;
    rect(100, 0, 40, 100, &scratch);
    color = RED;
    scaffoldNextObjDist += 30;
  }
  rect(scaffoldWall.x + 3, 0, 2, scaffoldWall.y - 12, &scratch);
  rect(scaffoldWall.x + 3, scaffoldWall.y + 18, 2, 100 - 18 - scaffoldWall.y, &scratch);
  rect(0, -7, 100, 9, &scratch);
  rect(0, 98, 100, 9, &scratch);
  if (input.isJustReleased) {
    play(LASER);
    if ((scaffoldNextFloorType == 0 && scaffoldTv == -1) ||
        (scaffoldNextFloorType == 2 && scaffoldTv == 1)) {
      scaffoldTv *= -1;
    }
    scaffoldNextFloorType += scaffoldTv;
  }
  if (input.isPressed) {
    scaffoldPressedCount++;
    if (scaffoldPressedCount > 15 / sqrt(difficulty)) {
      play(SELECT);
      ASSIGN_ARRAY_ITEM(scaffoldFloors, scaffoldFloorIndex, ScaffoldFloor, nf);
      float fy;
      if (scaffoldNextFloorType == 2) {
        fy = scaffoldNextFloorPos.y + 4;
      } else {
        fy = scaffoldNextFloorPos.y;
      }
      vectorSet(&nf->pos, scaffoldNextFloorPos.x, fy);
      nf->type = scaffoldNextFloorType;
      nf->isAlive = true;
      scaffoldFloorIndex = cgl_wrap(scaffoldFloorIndex + 1, 0, SCAFFOLD_MAX_FLOOR_COUNT);
      vectorAdd(&scaffoldNextFloorPos, 6, scaffoldNextFloorType * 4 - 4);
      scaffoldPressedCount = 0;
    }
  } else {
    scaffoldPressedCount = 0;
  }
  color = BLACK;
  FOR_EACH(scaffoldFloors, fi2) {
    ASSIGN_ARRAY_ITEM(scaffoldFloors, fi2, ScaffoldFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->pos.x -= scr;
    int[2] fc;
    fc[0] = 'a' + f->type;
    fc[1] = 0;
    character(fc, f->pos.x, f->pos.y, &scratch);
    if (f->pos.x < -3) {
      f->isAlive = false;
      continue;
    }
  }
  color = CYAN;
  scaffoldNextFloorPos.x -= scr;
  int[2] nfc;
  nfc[0] = 'a' + scaffoldNextFloorType;
  nfc[1] = 0;
  float nfy;
  if (scaffoldNextFloorType == 2) {
    nfy = scaffoldNextFloorPos.y + 4;
  } else {
    nfy = scaffoldNextFloorPos.y;
  }
  character(nfc, scaffoldNextFloorPos.x, nfy, &scratch);
  float vx = 0;
  if (scaffoldPlayer.x < 20) {
    vx += (20 - scaffoldPlayer.x) * 0.2;
  }
  scaffoldPlayer.x += vx - scr;
  int[2] pc;
  pc[0] = 'd' + (ticks / 15) % 2;
  pc[1] = 0;
  Collision pcoll;
  character(pc, scaffoldPlayer.x, scaffoldPlayer.y, &pcoll);
  if (pcoll.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  color = TRANSPARENT;
  int type = -1;
  TIMES(9, ti) {
    Collision cc;
    box(scaffoldPlayer.x + 4, scaffoldPlayer.y, 1, 6, &cc);
    if (cc.isColliding.character['a']) {
      type = 0;
      scaffoldPlayer.y--;
    } else if (cc.isColliding.character['b']) {
      type = 1;
      scaffoldPlayer.y--;
    } else if (cc.isColliding.character['c']) {
      type = 2;
      scaffoldPlayer.y--;
    } else {
      if (type != -1) {
        break;
      }
      scaffoldPlayer.y++;
    }
  }
  if (type == 0) {
    scaffoldPlayer.y += 4;
  }
  scaffoldNextObjDist -= scr;
  if (scaffoldNextObjDist < 0) {
    int otype;
    if (rnd(0, 1) < 0.5) {
      otype = SCAFFOLD_OBJ_TYPE_GOLD;
    } else {
      otype = SCAFFOLD_OBJ_TYPE_SPIKE;
    }
    float odiv;
    if (otype == SCAFFOLD_OBJ_TYPE_GOLD) {
      odiv = 4;
    } else {
      odiv = 1.5;
    }
    float odistance = rnd(20, 60) / odiv;
    ASSIGN_ARRAY_ITEM(scaffoldObjs, scaffoldObjIndex, ScaffoldObj, no);
    vectorSet(&no->pos, 120, clamp(scaffoldNextFloorPos.y + rnd(0, 20) * RNDPM(), 10, 90));
    no->d = odistance / 2;
    no->distance = odistance;
    no->vy = rnd(1, sqrt(difficulty)) * RNDPM() * 0.3;
    no->type = otype;
    no->isAlive = true;
    scaffoldObjIndex = cgl_wrap(scaffoldObjIndex + 1, 0, SCAFFOLD_MAX_OBJ_COUNT);
    scaffoldNextObjDist = rnd(15, 25);
  }
  FOR_EACH(scaffoldObjs, oi2) {
    ASSIGN_ARRAY_ITEM(scaffoldObjs, oi2, ScaffoldObj, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.x -= scr;
    o->pos.y += o->vy;
    o->d -= fabs(o->vy);
    if (o->d < 0) {
      o->d = o->distance;
      o->vy *= -1;
    }
    if (o->type == SCAFFOLD_OBJ_TYPE_GOLD) {
      color = YELLOW;
    } else {
      color = RED;
    }
    Collision oc;
    if (o->type == SCAFFOLD_OBJ_TYPE_GOLD) {
      text("$", o->pos.x, o->pos.y, &oc);
    } else {
      text("x", o->pos.x, o->pos.y, &oc);
    }
    if (o->distance > 4 && oc.isColliding.rect[PURPLE]) {
      o->isAlive = false;
      continue;
    }
    if (oc.isColliding.character['d'] || oc.isColliding.character['e']) {
      if (o->type == SCAFFOLD_OBJ_TYPE_GOLD) {
        play(COIN);
        addScore(scaffoldMultiplier, o->pos.x, o->pos.y);
        scaffoldMultiplier++;
        o->isAlive = false;
        continue;
      } else {
        play(EXPLOSION);
        gameOver();
      }
    }
    if (o->pos.x < -3) {
      if (o->type == SCAFFOLD_OBJ_TYPE_GOLD && scaffoldMultiplier > 1) {
        play(HIT);
        scaffoldMultiplier--;
      }
      o->isAlive = false;
      continue;
    }
  }
}

void addGameScaffold() {
  addGame(scaffoldTitle, scaffoldDescription, scaffoldCharacters, scaffoldCharactersCount,
          &scaffoldOptions, false, &scaffoldUpdate);
}
