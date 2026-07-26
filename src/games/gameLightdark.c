#include "../cglp.h"

int* lightdarkTitle = "LIGHT DARK";
int* lightdarkDescription = "[Tap] Jump\n[Hold] Fly";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] lightdarkCharacters = {
    {
        "    ll",
        "   l  ",
        "  l   ",
        "  ll  ",
        " l  l ",
        "l    l",
    },
    {
        "  ll  ",
        "  l   ",
        "  l   ",
        "  ll  ",
        "  ll  ",
        " l  l ",
    },
    {
        "  ll  ",
        "  ll  ",
        " llll ",
        " llll ",
        "llllll",
        "llllll",
    },
};
int lightdarkCharactersCount = 3;

Options lightdarkOptions = {200, 100, 6, false};

#define LIGHTDARK_TYPE_SPIKE 0
#define LIGHTDARK_TYPE_COIN 1
#define LIGHTDARK_SIDE_LIGHT 0
#define LIGHTDARK_SIDE_DARK 1
#define LIGHTDARK_STATE_GROUND 0
#define LIGHTDARK_STATE_JUMP 1

struct LightdarkObj {
  float x;
  int type;
  int side;
  bool isAlive;
};
#define LIGHTDARK_MAX_OBJ_COUNT 64
LightdarkObj[LIGHTDARK_MAX_OBJ_COUNT] lightdarkObjs;
int lightdarkObjIndex;

float[2] lightdarkObjDists;
int[2] lightdarkObjTypes;
Vector lightdarkPos;
int lightdarkSide;
Vector lightdarkVel;
int lightdarkState;
float lightdarkMultiplier;
float lightdarkScx;

void lightdarkUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(lightdarkObjs);
    lightdarkObjIndex = 0;
    TIMES(10, i) {
      ASSIGN_ARRAY_ITEM(lightdarkObjs, lightdarkObjIndex, LightdarkObj, o);
      if (i < 5) {
        o->x = 25 + i * 9;
        o->side = LIGHTDARK_SIDE_LIGHT;
      } else {
        o->x = 99 + i * 9;
        o->side = LIGHTDARK_SIDE_DARK;
      }
      o->type = LIGHTDARK_TYPE_COIN;
      o->isAlive = true;
      lightdarkObjIndex = cgl_wrap(lightdarkObjIndex + 1, 0, LIGHTDARK_MAX_OBJ_COUNT);
    }
    lightdarkObjDists[0] = 200;
    lightdarkObjDists[1] = 300;
    lightdarkObjTypes[0] = LIGHTDARK_TYPE_COIN;
    lightdarkObjTypes[1] = LIGHTDARK_TYPE_SPIKE;
    vectorSet(&lightdarkPos, 9, 0);
    lightdarkSide = LIGHTDARK_SIDE_LIGHT;
    vectorSet(&lightdarkVel, 2, 0);
    lightdarkState = LIGHTDARK_STATE_GROUND;
    lightdarkMultiplier = 1;
    lightdarkScx = 0;
  }
  float scr = lightdarkVel.x * difficulty;
  color = LIGHT_BLACK;
  rect(0, 50, 200, 50, &scratch);
  if (lightdarkState == LIGHTDARK_STATE_GROUND) {
    if (input.isJustPressed) {
      if (lightdarkSide == LIGHTDARK_SIDE_LIGHT) {
        lightdarkSide = LIGHTDARK_SIDE_DARK;
      } else {
        lightdarkSide = LIGHTDARK_SIDE_LIGHT;
      }
      if (lightdarkSide == LIGHTDARK_SIDE_LIGHT) {
        play(JUMP);
      } else {
        play(LASER);
      }
      lightdarkVel.y = 3 * sqrt(difficulty);
      lightdarkPos.y = 7;
      lightdarkState = LIGHTDARK_STATE_JUMP;
      lightdarkScx = 0;
    }
  } else {
    if (input.isJustPressed) {
      play(HIT);
      if (lightdarkSide == LIGHTDARK_SIDE_LIGHT) {
        lightdarkSide = LIGHTDARK_SIDE_DARK;
      } else {
        lightdarkSide = LIGHTDARK_SIDE_LIGHT;
      }
    }
    float dvy;
    if (input.isPressed) {
      dvy = 0.1;
    } else {
      dvy = 0.5;
    }
    lightdarkVel.y -= dvy * difficulty;
    lightdarkPos.y += lightdarkVel.y;
    if (lightdarkPos.y < 0) {
      lightdarkPos.y = 0;
      lightdarkState = LIGHTDARK_STATE_GROUND;
    }
  }
  float y;
  if (lightdarkSide == LIGHTDARK_SIDE_LIGHT) {
    y = 47 - lightdarkPos.y;
  } else {
    y = 53 + lightdarkPos.y;
  }
  if (lightdarkSide == LIGHTDARK_SIDE_LIGHT) {
    color = BLACK;
  } else {
    color = WHITE;
  }
  int[2] ch;
  if (lightdarkState == LIGHTDARK_STATE_JUMP) {
    ch[0] = 'b';
  } else {
    ch[0] = 'a' + (int)floor((ticks * difficulty) / 10) % 2;
  }
  ch[1] = 0;
  if (lightdarkSide == LIGHTDARK_SIDE_DARK) {
    characterOptions.isMirrorY = true;
  }
  character(ch, lightdarkPos.x, y, &scratch);
  characterOptions.isMirrorY = false;
  FOR_EACH(lightdarkObjs, i) {
    ASSIGN_ARRAY_ITEM(lightdarkObjs, i, LightdarkObj, o);
    SKIP_IS_NOT_ALIVE(o);
    float oy;
    if (o->side == LIGHTDARK_SIDE_LIGHT) {
      oy = 46;
    } else {
      oy = 54;
    }
    if (o->side == LIGHTDARK_SIDE_LIGHT) {
      color = BLACK;
    } else {
      color = WHITE;
    }
    Collision oc;
    if (o->side == LIGHTDARK_SIDE_DARK) {
      characterOptions.isMirrorY = true;
    }
    if (o->type == LIGHTDARK_TYPE_SPIKE) {
      character("c", o->x, oy, &oc);
    } else {
      text("o", o->x, oy, &oc);
    }
    characterOptions.isMirrorY = false;
    if (oc.isColliding.character['a'] || oc.isColliding.character['b']) {
      if (o->type == LIGHTDARK_TYPE_SPIKE) {
        play(EXPLOSION);
        gameOver();
      } else {
        if (o->side == LIGHTDARK_SIDE_LIGHT) {
          play(COIN);
        } else {
          play(SELECT);
        }
        addScore(lightdarkMultiplier, o->x + lightdarkScx * 7, oy);
        lightdarkMultiplier++;
        lightdarkScx++;
        o->isAlive = false;
        continue;
      }
    }
    o->x -= scr;
    if (o->x < -3) {
      if (o->type == LIGHTDARK_TYPE_COIN && lightdarkMultiplier > 1) {
        lightdarkMultiplier--;
      }
      o->isAlive = false;
      continue;
    }
  }
  TIMES(2, i) {
    lightdarkObjDists[i] -= scr;
    int sideVal;
    if (i == 0) {
      sideVal = LIGHTDARK_SIDE_LIGHT;
    } else {
      sideVal = LIGHTDARK_SIDE_DARK;
    }
    float o;
    if (lightdarkObjTypes[i] == LIGHTDARK_TYPE_COIN) {
      o = 9;
    } else {
      o = 6;
    }
    int c;
    if (lightdarkObjTypes[i] == LIGHTDARK_TYPE_COIN) {
      c = rndi(4, 8);
    } else {
      c = rndi(5, 15);
    }
    if (lightdarkObjDists[i] < 0) {
      TIMES(c, j) {
        ASSIGN_ARRAY_ITEM(lightdarkObjs, lightdarkObjIndex, LightdarkObj, no);
        no->x = 200 + j * o;
        no->type = lightdarkObjTypes[i];
        no->side = sideVal;
        no->isAlive = true;
        lightdarkObjIndex = cgl_wrap(lightdarkObjIndex + 1, 0, LIGHTDARK_MAX_OBJ_COUNT);
      }
      lightdarkObjDists[i] = c * o + rnd(40, 120);
      if (lightdarkObjTypes[i] == LIGHTDARK_TYPE_COIN) {
        lightdarkObjTypes[i] = LIGHTDARK_TYPE_SPIKE;
      } else {
        lightdarkObjTypes[i] = LIGHTDARK_TYPE_COIN;
      }
    }
  }
}

void addGameLightdark() {
  addGame(lightdarkTitle, lightdarkDescription, lightdarkCharacters,
          lightdarkCharactersCount, &lightdarkOptions, false,
          &lightdarkUpdate);
}
