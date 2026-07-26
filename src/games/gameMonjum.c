#include "../cglp.h"

int* monjumTitle = "MONJUM";
int* monjumDescription = "[Hold] Back\n[Release] Jump";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] monjumCharacters = {
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
        "      ",
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
};
int monjumCharactersCount = 4;

Options monjumOptions = {100, 100, 0, false};

#define MONJUM_ITEM_HOLE 1
#define MONJUM_ITEM_MON 2
#define MONJUM_ITEM_COIN 4

struct MonjumFloor {
  float x;
  float w;
  bool isAlive;
};
#define MONJUM_MAX_FLOOR_COUNT 16
MonjumFloor[MONJUM_MAX_FLOOR_COUNT] monjumFloors;
int monjumFloorIndex;
float monjumFlt;

struct MonjumMon {
  Vector pos;
  Vector vel;
  bool jump;
  bool isAlive;
};
#define MONJUM_MAX_MON_COUNT 16
MonjumMon[MONJUM_MAX_MON_COUNT] monjumMons;
int monjumMonIndex;
float monjumMnt;

struct MonjumCoin {
  Vector pos;
  bool isAlive;
};
#define MONJUM_MAX_COIN_COUNT 16
MonjumCoin[MONJUM_MAX_COIN_COUNT] monjumCoins;
int monjumCoinIndex;
float monjumCnt;

Vector monjumP;
Vector monjumV;
bool monjumJump;
bool monjumFall;
int monjumItem;

void monjumUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(monjumFloors);
    monjumFloors[0].x = 0;
    monjumFloors[0].w = 99;
    monjumFloors[0].isAlive = true;
    monjumFloorIndex = 1;
    INIT_UNALIVED_ARRAY_FAST(monjumMons);
    monjumMonIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(monjumCoins);
    monjumCoinIndex = 0;
    monjumFlt = 0;
    monjumMnt = 0;
    monjumCnt = 0;
    vectorSet(&monjumP, 50, 87);
    vectorSet(&monjumV, 0, 0);
    monjumJump = false;
    monjumFall = false;
  }
  color = BLACK;
  int[2] pc;
  pc[0] = 'a' + ((int)(ticks / 30) % 2);
  pc[1] = 0;
  character(pc, monjumP.x, monjumP.y, &scratch);
  float fvy = difficulty * 0.5;
  monjumFlt -= fvy;
  if (monjumFlt < 0) {
    float w = rndi(50, 90);
    ASSIGN_ARRAY_ITEM(monjumFloors, monjumFloorIndex, MonjumFloor, nf);
    nf->x = 110;
    nf->w = w;
    nf->isAlive = true;
    monjumFloorIndex = cgl_wrap(monjumFloorIndex + 1, 0, MONJUM_MAX_FLOOR_COUNT);
    monjumFlt += 10 + w;
  }
  color = BLACK;
  FOR_EACH(monjumFloors, i) {
    ASSIGN_ARRAY_ITEM(monjumFloors, i, MonjumFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->x -= fvy;
    rect(f->x, 90, f->w, 9, &scratch);
    if (f->x + f->w <= 0) {
      f->isAlive = false;
    }
  }
  monjumMnt -= difficulty;
  if (monjumMnt < 0) {
    ASSIGN_ARRAY_ITEM(monjumMons, monjumMonIndex, MonjumMon, nm);
    vectorSet(&nm->pos, 99, 87);
    vectorSet(&nm->vel, -difficulty * 0.75, 0);
    nm->jump = false;
    nm->isAlive = true;
    monjumMonIndex = cgl_wrap(monjumMonIndex + 1, 0, MONJUM_MAX_MON_COUNT);
    monjumMnt += rnd(200, 300);
  }
  FOR_EACH(monjumMons, i) {
    ASSIGN_ARRAY_ITEM(monjumMons, i, MonjumMon, m);
    SKIP_IS_NOT_ALIVE(m);
    vectorAdd(&m->pos, m->vel.x, m->vel.y);
    if (m->jump) {
      m->vel.y += difficulty * 0.09;
      if (m->pos.y > 87) {
        m->pos.y = 87;
        m->vel.y = 0;
        m->jump = false;
      }
    } else {
      color = TRANSPARENT;
      box(m->pos.x - 4, m->pos.y + 6, 6, 6, &scratch);
      if (!scratch.isColliding.rect[BLACK]) {
        m->jump = true;
        m->vel.y = -1.4 * sqrt(difficulty);
      }
    }
    color = PURPLE;
    characterOptions.isMirrorX = true;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    int[2] mc;
    mc[0] = 'c' + ((int)(ticks / 30) % 2);
    mc[1] = 0;
    character(mc, m->pos.x, m->pos.y, &scratch);
    if (scratch.isColliding.character['a'] || scratch.isColliding.character['b']) {
      play(EXPLOSION);
      gameOver();
    }
    if (m->pos.x <= -3) {
      m->isAlive = false;
    }
  }
  characterOptions.isMirrorX = false;
  monjumCnt -= difficulty;
  if (monjumCnt < 0) {
    ASSIGN_ARRAY_ITEM(monjumCoins, monjumCoinIndex, MonjumCoin, nc);
    vectorSet(&nc->pos, 99, 60);
    nc->isAlive = true;
    monjumCoinIndex = cgl_wrap(monjumCoinIndex + 1, 0, MONJUM_MAX_COIN_COUNT);
    monjumCnt += rnd(50, 100);
  }
  color = YELLOW;
  FOR_EACH(monjumCoins, i) {
    ASSIGN_ARRAY_ITEM(monjumCoins, i, MonjumCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= difficulty;
    text("$", c->pos.x, c->pos.y, &scratch);
    if (scratch.isColliding.character['a'] || scratch.isColliding.character['b']) {
      play(LASER);
      monjumItem |= MONJUM_ITEM_COIN;
      c->isAlive = false;
      continue;
    }
    if (c->pos.x <= -3) {
      c->isAlive = false;
    }
  }
  color = TRANSPARENT;
  box(monjumP.x, monjumP.y, 6, 50, &scratch);
  if (scratch.isColliding.character['c'] || scratch.isColliding.character['d']) {
    monjumItem |= MONJUM_ITEM_MON;
  }
  Collision holeCheck;
  box(monjumP.x, 93, 6, 6, &holeCheck);
  bool hasHole = !holeCheck.isColliding.rect[BLACK];
  if (hasHole) {
    monjumItem |= MONJUM_ITEM_HOLE;
  }
  if (monjumFall) {
    monjumV.y += 0.14 * difficulty;
    if (monjumP.y > 99) {
      play(EXPLOSION);
      gameOver();
    }
  } else if (monjumJump) {
    monjumV.y += 0.12 * difficulty;
    if (monjumP.y > 87) {
      monjumP.y = 87;
      monjumV.y = 0;
      if (hasHole) {
        monjumFall = true;
      } else {
        monjumJump = false;
        int bc = (monjumItem & 1) + ((monjumItem >> 1) & 1) + ((monjumItem >> 2) & 1);
        int[4] scoreTable = {0, 1, 5, 20};
        int sc = scoreTable[bc];
        if (sc > 0) {
          play(COIN);
          addScore(sc, monjumP.x, monjumP.y);
        }
      }
    }
  } else {
    if (hasHole) {
      monjumFall = true;
    }
    if (input.isPressed) {
      monjumV.x = -0.5 * difficulty;
    } else {
      monjumV.x = 0;
      monjumP.x += (50 - monjumP.x) * 0.02;
    }
    if (input.isJustReleased && ticks > 5) {
      play(JUMP);
      monjumV.y = -2.5 * sqrt(difficulty);
      monjumJump = true;
      monjumItem = 0;
    }
  }
  vectorAdd(&monjumP, monjumV.x, monjumV.y);
  monjumP.x = clamp(monjumP.x, 0, 50);
}

void addGameMonjum() {
  addGame(monjumTitle, monjumDescription, monjumCharacters,
          monjumCharactersCount, &monjumOptions, false, &monjumUpdate);
}
