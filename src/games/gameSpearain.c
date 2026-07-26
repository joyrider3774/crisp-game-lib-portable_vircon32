#include "../cglp.h"

int* spearainTitle = "SPEARAIN";
int* spearainDescription = "[Hold] Slow";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] spearainCharacters = {
    {
        " l    ",
        " l    ",
        " l    ",
        " l    ",
        "lll   ",
        " l    ",
    },
    {
        "llll  ",
        "llll  ",
        " l    ",
        "lll   ",
        " lll  ",
        "l     ",
    },
    {
        "llll  ",
        "llll  ",
        "  l   ",
        " lll  ",
        "lll   ",
        "   l  ",
    },
};
int spearainCharactersCount = 3;

Options spearainOptions = {100, 100, 6, false};

struct SpearainObj {
  Vector pos;
  float speed;
  bool isBonus;
  bool isAlive;
};
// While holding, appRatio grows 5%/tick (faster spawns) while speedRatio only decays 2%/tick
// (slower fall) for ~140 ticks until the <0.03 rebound kicks in - a single ordinary "hold to slow"
// can pile up ~100+ concurrent slow-falling objects before they clear.
#define SPEARAIN_MAX_OBJ_COUNT 256
SpearainObj[SPEARAIN_MAX_OBJ_COUNT] spearainObjs;
int spearainObjIndex;

float spearainAppTicks;
float spearainAppRatio;
int spearainBonusAppCount;
float spearainSpeedRatio;
float spearainX;
float spearainVx;
float spearainAnim;

void spearainUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(spearainObjs);
    spearainObjIndex = 0;
    spearainAppTicks = 0;
    spearainAppRatio = 1;
    spearainBonusAppCount = 0;
    spearainSpeedRatio = 1;
    spearainX = 50;
    spearainVx = 1;
    spearainAnim = 0;
  }
  color = LIGHT_BLUE;
  rect(0, 0, 9, 99, &scratch);
  rect(90, 0, 9, 99, &scratch);
  rect(0, 90, 99, 9, &scratch);
  spearainAppTicks -= difficulty;
  if (spearainAppTicks < 0) {
    play(LASER);
    spearainBonusAppCount--;
    ASSIGN_ARRAY_ITEM(spearainObjs, spearainObjIndex, SpearainObj, o);
    vectorSet(&o->pos, rnd(12, 88), 2);
    float speedDivisor;
    if (spearainBonusAppCount < 0) {
      speedDivisor = 3;
    } else {
      speedDivisor = 1;
    }
    o->speed = (1 + rnd(0, sqrt(sqrt(difficulty)))) / speedDivisor;
    o->isBonus = spearainBonusAppCount < 0;
    o->isAlive = true;
    spearainObjIndex = cgl_wrap(spearainObjIndex + 1, 0, SPEARAIN_MAX_OBJ_COUNT);
    if (spearainBonusAppCount < 0) {
      spearainBonusAppCount = 12;
    }
    spearainAppTicks += rnd(40, 60) / difficulty / spearainAppRatio;
  }
  if (input.isJustPressed) {
    spearainSpeedRatio *= 0.5;
  }
  if (spearainSpeedRatio < 1 && input.isPressed) {
    spearainSpeedRatio *= 0.98;
    spearainAppRatio *= 1.05;
  } else {
    spearainSpeedRatio += (1 - spearainSpeedRatio) * 0.05;
    spearainAppRatio = 1;
  }
  if (spearainSpeedRatio < 0.03 || input.isJustReleased) {
    spearainSpeedRatio = 1 / sqrt(spearainSpeedRatio);
  }
  FOR_EACH(spearainObjs, i) {
    ASSIGN_ARRAY_ITEM(spearainObjs, i, SpearainObj, o);
    SKIP_IS_NOT_ALIVE(o);
    float s = o->speed * spearainSpeedRatio * difficulty;
    if (o->isBonus) {
      color = YELLOW;
      text("$", o->pos.x, o->pos.y, &scratch);
      if (s > 78) {
        s *= 0.2;
      }
    } else {
      color = RED;
      character("a", o->pos.x, o->pos.y, &scratch);
      if (s > 74) {
        s *= 1.5;
      }
    }
    o->pos.y += s;
    if (o->pos.y >= 87) {
      play(HIT);
    }
    o->isAlive = o->pos.y < 87;
  }
  float sVal = (spearainVx / sqrt(spearainSpeedRatio)) * difficulty;
  spearainX += sVal;
  if ((spearainX < 12 && spearainVx < 0) || (spearainX > 88 && spearainVx > 0)) {
    addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    play(SELECT);
    spearainVx *= -1;
  }
  spearainAnim += fabs(sVal) * 0.1;
  color = BLACK;
  int[2] pc;
  pc[0] = 'b' + (int)floor(fmod(spearainAnim, 2));
  pc[1] = 0;
  Collision cl;
  character(pc, spearainX, 87, &cl);
  if (cl.isColliding.text['$']) {
    play(COIN);
    COUNT_IS_ALIVE(spearainObjs, aliveObjCount);
    addScore(aliveObjCount, spearainX, 87);
    color = BLACK;
    FOR_EACH(spearainObjs, i) {
      ASSIGN_ARRAY_ITEM(spearainObjs, i, SpearainObj, o);
      SKIP_IS_NOT_ALIVE(o);
      particle(o->pos.x, o->pos.y, 5, 1, 0, CGLP_PI * 2);
    }
    INIT_UNALIVED_ARRAY_FAST(spearainObjs);
    spearainAppRatio = 1;
    spearainAppTicks = 60;
  } else if (cl.isColliding.character['a']) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameSpearain() {
  addGame(spearainTitle, spearainDescription, spearainCharacters,
          spearainCharactersCount, &spearainOptions, false, &spearainUpdate);
}
