#include "../cglp.h"

int* liftupTitle = "LIFT UP";
int* liftupDescription = "[Hold]\n Go up fast";

int[5][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] liftupCharacters = {
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
        "l   l ",
        "llll  ",
        "lll   ",
        "llll  ",
        "l   l ",
    },
    {
        "  ll  ",
        "   ll ",
        "llllll",
        "   ll ",
        "  ll  ",
    },
    {
        " llll ",
        "l llll",
        "llll l",
        "llll l",
        "ll   l",
        " llll ",
    },
};
int liftupCharactersCount = 5;

Options liftupOptions = {100, 100, 90, false};

struct LiftupWall {
  Vector pos;
  float width;
};
#define LIFTUP_WALL_COUNT 19
LiftupWall[LIFTUP_WALL_COUNT] liftupWalls;
float liftupWallVx;
float liftupWallVw;

struct LiftupPlayer {
  Vector pos;
  float vx;
  float ty;
};
LiftupPlayer liftupPlayer;
float liftupPressRatio;

#define LIFTUP_ITEM_TYPE_TURN 0
#define LIFTUP_ITEM_TYPE_BONUS 1
struct LiftupItem {
  Vector pos;
  int type;
  bool isAlive;
};
#define LIFTUP_MAX_ITEM_COUNT 64
LiftupItem[LIFTUP_MAX_ITEM_COUNT] liftupItems;
int liftupItemIndex;
float liftupNextItemDist;
int liftupBonusItemCount;
float liftupBonusItemX;
int liftupNextItemSide;
float liftupTopWallX;
float liftupTopWallW;
int liftupMultiplier;

void liftupUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(LIFTUP_WALL_COUNT, i) {
      vectorSet(&liftupWalls[i].pos, 50, i * 6 - 3);
      liftupWalls[i].width = 60;
    }
    liftupWallVx = 0;
    liftupWallVw = 0;
    vectorSet(&liftupPlayer.pos, 40, 90);
    liftupPlayer.vx = 1;
    liftupPlayer.ty = 90;
    liftupPressRatio = 0;
    INIT_UNALIVED_ARRAY_FAST(liftupItems);
    liftupItemIndex = 0;
    liftupNextItemDist = 0;
    liftupBonusItemCount = 0;
    liftupBonusItemX = 0;
    liftupNextItemSide = 1;
    liftupTopWallX = 50;
    liftupTopWallW = 60;
    liftupMultiplier = 1;
  }
  float pr;
  if (input.isPressed) {
    pr = 1;
  } else {
    pr = 0;
  }
  liftupPressRatio += (pr - liftupPressRatio) * 0.1;
  float scr = difficulty * (1 + liftupPressRatio * 3) * 0.2;
  TIMES(LIFTUP_WALL_COUNT, i) {
    LiftupWall* w = &liftupWalls[i];
    w->pos.y += scr;
    if (w->pos.y > 110) {
      w->pos.y -= LIFTUP_WALL_COUNT * 6;
      LiftupWall* pw = &liftupWalls[(int)cgl_wrap(i - 1, 0, LIFTUP_WALL_COUNT)];
      liftupTopWallX = pw->pos.x + liftupWallVx;
      w->pos.x = liftupTopWallX;
      liftupTopWallW = pw->width + liftupWallVw;
      w->width = liftupTopWallW;
      liftupWallVx = clamp(liftupWallVx + rnd(0, 1) * RNDPM() * sqrt(difficulty), -5, 5);
      liftupWallVw = clamp(liftupWallVw + rnd(0, 1) * RNDPM() * sqrt(difficulty), -3, 3);
      liftupWallVx *= 0.95;
      liftupWallVw *= 0.95;
      if ((w->pos.x + w->width / 2 > 95 && liftupWallVx > 0) ||
          (w->pos.x - w->width / 2 < 5 && liftupWallVx < 0)) {
        liftupWallVx *= -0.5;
      }
      if ((w->width > 80 && liftupWallVw > 0) || (w->width < 40 && liftupWallVw < 0)) {
        liftupWallVw *= -0.5;
      }
    }
    color = RED;
    character("c", w->pos.x - w->width / 2, w->pos.y, &scratch);
    characterOptions.isMirrorX = true;
    character("c", w->pos.x + w->width / 2, w->pos.y, &scratch);
    characterOptions.isMirrorX = false;
    color = LIGHT_RED;
    rect(w->pos.x - w->width / 2 - 2, w->pos.y - 2, -70, 5, &scratch);
    rect(w->pos.x + w->width / 2 + 2, w->pos.y - 2, 70, 5, &scratch);
  }
  liftupNextItemDist -= scr;
  if (liftupNextItemDist < 0) {
    if (liftupBonusItemCount > 0) {
      ASSIGN_ARRAY_ITEM(liftupItems, liftupItemIndex, LiftupItem, ni);
      vectorSet(&ni->pos, liftupBonusItemX, -3);
      ni->type = LIFTUP_ITEM_TYPE_BONUS;
      ni->isAlive = true;
      liftupItemIndex = cgl_wrap(liftupItemIndex + 1, 0, LIFTUP_MAX_ITEM_COUNT);
      liftupNextItemDist = 6;
    } else {
      if (liftupBonusItemCount < 0 && rnd(0, 1) < 0.5) {
        liftupBonusItemCount = rndi(2, 6);
        liftupBonusItemX = liftupTopWallX + rnd(0, liftupTopWallW * 0.25) * RNDPM();
        liftupNextItemDist = 0;
      } else {
        float x = liftupTopWallX + rnd(0.1, 0.4) * liftupNextItemSide * liftupTopWallW;
        if (rnd(0, 1) < 0.8) {
          liftupNextItemSide *= -1;
        }
        ASSIGN_ARRAY_ITEM(liftupItems, liftupItemIndex, LiftupItem, nt);
        vectorSet(&nt->pos, x, -3);
        nt->type = LIFTUP_ITEM_TYPE_TURN;
        nt->isAlive = true;
        liftupItemIndex = cgl_wrap(liftupItemIndex + 1, 0, LIFTUP_MAX_ITEM_COUNT);
        liftupNextItemDist = rnd(10, 20);
      }
    }
    liftupBonusItemCount--;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  liftupPlayer.pos.x += liftupPlayer.vx * 0.05 * difficulty * (4 - liftupPressRatio * 3);
  liftupPlayer.ty -= sqrt(difficulty) * 0.04;
  liftupPlayer.pos.y += (liftupPlayer.ty - liftupPlayer.pos.y) * 0.2;
  color = BLACK;
  int[2] pc;
  pc[0] = 'a' + (int)floor(ticks / 15) % 2;
  pc[1] = 0;
  if (liftupPlayer.vx < 0) {
    characterOptions.isMirrorX = true;
  } else {
    characterOptions.isMirrorX = false;
  }
  Collision c;
  character(pc, liftupPlayer.pos.x, liftupPlayer.pos.y, &c);
  characterOptions.isMirrorX = false;
  if (c.isColliding.rect[LIGHT_RED] || c.isColliding.character['c']) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(liftupItems, i) {
    ASSIGN_ARRAY_ITEM(liftupItems, i, LiftupItem, it);
    SKIP_IS_NOT_ALIVE(it);
    it->pos.y += scr;
    if (it->type == LIFTUP_ITEM_TYPE_TURN) {
      color = CYAN;
      if (liftupPlayer.vx > 0) {
        characterOptions.isMirrorX = true;
      } else {
        characterOptions.isMirrorX = false;
      }
      Collision ic;
      character("d", it->pos.x, it->pos.y, &ic);
      characterOptions.isMirrorX = false;
      if (ic.isColliding.rect[LIGHT_RED] || ic.isColliding.character['c']) {
        if (it->pos.x > liftupTopWallX) {
          it->pos.x -= 1;
        } else {
          it->pos.x += 1;
        }
      }
      if (ic.isColliding.character['a'] || ic.isColliding.character['b']) {
        play(LASER);
        liftupPlayer.vx *= -1;
        it->isAlive = false;
        continue;
      }
    } else {
      color = YELLOW;
      Collision ic2;
      character("e", it->pos.x, it->pos.y, &ic2);
      if (ic2.isColliding.rect[LIGHT_RED] || ic2.isColliding.character['c']) {
        if (it->pos.x > liftupTopWallX) {
          it->pos.x -= 1;
        } else {
          it->pos.x += 1;
        }
      }
      if (ic2.isColliding.character['a'] || ic2.isColliding.character['b']) {
        play(COIN);
        addScore(liftupMultiplier, it->pos.x, it->pos.y);
        liftupMultiplier = (int)clamp(liftupMultiplier + 1, 1, 99);
        liftupPlayer.ty += (99 - liftupPlayer.ty) * 0.1;
        it->isAlive = false;
        continue;
      }
    }
    if (it->pos.y > 103) {
      if (it->type == LIFTUP_ITEM_TYPE_BONUS) {
        liftupMultiplier = (int)clamp(liftupMultiplier - 1, 1, 99);
      }
      it->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  rect(0, liftupPlayer.pos.y + 3, 100, 6, &scratch);
  if (liftupPlayer.pos.y < 0) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameLiftup() {
  addGame(liftupTitle, liftupDescription, liftupCharacters,
          liftupCharactersCount, &liftupOptions, false, &liftupUpdate);
}
