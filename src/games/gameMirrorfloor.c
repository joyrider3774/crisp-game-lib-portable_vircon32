#include "../cglp.h"

int* mirrorfloorTitle = "MIRROR FLOOR";
int* mirrorfloorDescription = "[Tap]\n Jump\n[Hold]\n Speed up";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] mirrorfloorCharacters = {
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
        " llll ",
        "ll lll",
        "ll lll",
        "ll lll",
        "ll lll",
        " llll ",
    },
};
int mirrorfloorCharactersCount = 3;

Options mirrorfloorOptions = {100, 100, 5, true};

#define MIRRORFLOOR_STATE_RUN 0
#define MIRRORFLOOR_STATE_JUMP 1
#define MIRRORFLOOR_PLAYER_X 9

struct MirrorfloorFloor {
  Vector pos;
  float width;
  bool isAlive;
};
#define MIRRORFLOOR_MAX_FLOOR_COUNT 32
MirrorfloorFloor[MIRRORFLOOR_MAX_FLOOR_COUNT] mirrorfloorFloors;
int mirrorfloorFloorIndex;
float mirrorfloorNextFloorDist;

struct MirrorfloorCoin {
  Vector pos;
  bool isAlive;
};
#define MIRRORFLOOR_MAX_COIN_COUNT 64
MirrorfloorCoin[MIRRORFLOOR_MAX_COIN_COUNT] mirrorfloorCoins;
int mirrorfloorCoinIndex;

struct MirrorfloorPlayer {
  float y;
  float my;
  float vy;
  float speed;
  int side;
  int state;
};
MirrorfloorPlayer mirrorfloorPlayer;
int mirrorfloorMultiplier;

void mirrorfloorUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(mirrorfloorFloors);
    mirrorfloorFloorIndex = 0;
    ASSIGN_ARRAY_ITEM(mirrorfloorFloors, mirrorfloorFloorIndex, MirrorfloorFloor, f0);
    vectorSet(&f0->pos, 10, 50);
    f0->width = 80;
    f0->isAlive = true;
    mirrorfloorFloorIndex = cgl_wrap(mirrorfloorFloorIndex + 1, 0, MIRRORFLOOR_MAX_FLOOR_COUNT);
    mirrorfloorNextFloorDist = 0;
    INIT_UNALIVED_ARRAY_FAST(mirrorfloorCoins);
    mirrorfloorCoinIndex = 0;
    ASSIGN_ARRAY_ITEM(mirrorfloorCoins, mirrorfloorCoinIndex, MirrorfloorCoin, c0);
    vectorSet(&c0->pos, 60, 47);
    c0->isAlive = true;
    mirrorfloorCoinIndex = cgl_wrap(mirrorfloorCoinIndex + 1, 0, MIRRORFLOOR_MAX_COIN_COUNT);
    mirrorfloorPlayer.y = 10;
    mirrorfloorPlayer.my = 0;
    mirrorfloorPlayer.vy = 0;
    mirrorfloorPlayer.speed = 1;
    mirrorfloorPlayer.side = 1;
    mirrorfloorPlayer.state = MIRRORFLOOR_STATE_JUMP;
    mirrorfloorMultiplier = 1;
  }
  float scr = difficulty * 0.5 * mirrorfloorPlayer.speed;
  if (mirrorfloorPlayer.state == MIRRORFLOOR_STATE_RUN) {
    if (input.isJustPressed) {
      play(POWER_UP);
      mirrorfloorPlayer.vy = -1.5 * sqrt(difficulty) * mirrorfloorPlayer.side;
      mirrorfloorPlayer.state = MIRRORFLOOR_STATE_JUMP;
    }
  }
  if (mirrorfloorPlayer.state == MIRRORFLOOR_STATE_JUMP) {
    mirrorfloorPlayer.vy += 0.07 * difficulty * mirrorfloorPlayer.side;
    mirrorfloorPlayer.y += mirrorfloorPlayer.vy;
  }
  if (input.isPressed) {
    mirrorfloorPlayer.speed += (2 - mirrorfloorPlayer.speed) * 0.05;
  } else {
    mirrorfloorPlayer.speed += (1 - mirrorfloorPlayer.speed) * 0.2;
  }
  if ((mirrorfloorPlayer.y < 0 && mirrorfloorPlayer.side == -1) ||
      (mirrorfloorPlayer.y > 99 && mirrorfloorPlayer.side == 1)) {
    play(RANDOM);
    gameOver();
  }
  color = BLACK;
  int[2] pc;
  pc[0] = 'a' + (int)floor(ticks / 15) % 2;
  pc[1] = 0;
  characterOptions.isMirrorY = mirrorfloorPlayer.side == -1;
  character(pc, MIRRORFLOOR_PLAYER_X, mirrorfloorPlayer.y, &scratch);
  characterOptions.isMirrorY = false;
  mirrorfloorNextFloorDist -= scr;
  if (mirrorfloorNextFloorDist < 0) {
    ASSIGN_ARRAY_ITEM(mirrorfloorFloors, mirrorfloorFloorIndex, MirrorfloorFloor, nf);
    vectorSet(&nf->pos, 100, rnd(10, 90));
    nf->width = rnd(45, 75);
    nf->isAlive = true;
    mirrorfloorFloorIndex = cgl_wrap(mirrorfloorFloorIndex + 1, 0, MIRRORFLOOR_MAX_FLOOR_COUNT);
    float cx = rnd(20, 25);
    while (cx < nf->width - 20) {
      ASSIGN_ARRAY_ITEM(mirrorfloorCoins, mirrorfloorCoinIndex, MirrorfloorCoin, nc);
      vectorSet(&nc->pos, 100 + cx, nf->pos.y - 3);
      nc->isAlive = true;
      mirrorfloorCoinIndex = cgl_wrap(mirrorfloorCoinIndex + 1, 0, MIRRORFLOOR_MAX_COIN_COUNT);
      cx += rnd(15, 30);
    }
    mirrorfloorNextFloorDist += nf->width + rnd(10, 20);
  }
  bool isOnFloor = false;
  FOR_EACH(mirrorfloorFloors, i) {
    ASSIGN_ARRAY_ITEM(mirrorfloorFloors, i, MirrorfloorFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->pos.x -= scr;
    if (mirrorfloorPlayer.side == 1) {
      color = CYAN;
    } else {
      color = LIGHT_CYAN;
    }
    Collision c1;
    rect(f->pos.x, f->pos.y, f->width, 1, &c1);
    if (mirrorfloorPlayer.side == -1) {
      color = CYAN;
    } else {
      color = LIGHT_CYAN;
    }
    Collision c2;
    rect(f->pos.x, f->pos.y + 1, f->width, 1, &c2);
    if ((c1.isColliding.character['a'] || c1.isColliding.character['b'] ||
         c2.isColliding.character['a'] || c2.isColliding.character['b']) &&
        mirrorfloorPlayer.vy * mirrorfloorPlayer.side > 0) {
      play(LASER);
      mirrorfloorPlayer.state = MIRRORFLOOR_STATE_RUN;
      float ofs;
      if (mirrorfloorPlayer.side == 1) {
        ofs = -3;
      } else {
        ofs = 5;
      }
      mirrorfloorPlayer.y = f->pos.y + ofs;
    }
    if (f->pos.x - 3 < MIRRORFLOOR_PLAYER_X &&
        MIRRORFLOOR_PLAYER_X < f->pos.x + f->width + 3) {
      mirrorfloorPlayer.my = f->pos.y - (mirrorfloorPlayer.y - f->pos.y) + 2;
      color = LIGHT_BLACK;
      int[2] pc2;
      pc2[0] = 'a' + (int)floor(ticks / 15) % 2;
      pc2[1] = 0;
      characterOptions.isMirrorY = mirrorfloorPlayer.side == 1;
      character(pc2, MIRRORFLOOR_PLAYER_X, mirrorfloorPlayer.my, &scratch);
      characterOptions.isMirrorY = false;
      isOnFloor = true;
    }
    f->isAlive = f->pos.x >= -f->width;
  }
  if (!isOnFloor) {
    mirrorfloorPlayer.state = MIRRORFLOOR_STATE_JUMP;
  }
  FOR_EACH(mirrorfloorCoins, i) {
    ASSIGN_ARRAY_ITEM(mirrorfloorCoins, i, MirrorfloorCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= scr;
    if (mirrorfloorPlayer.side == 1) {
      color = YELLOW;
    } else {
      color = LIGHT_YELLOW;
    }
    Collision cl;
    character("c", c->pos.x, c->pos.y, &cl);
    if (mirrorfloorPlayer.side == 1) {
      color = LIGHT_YELLOW;
    } else {
      color = YELLOW;
    }
    character("c", c->pos.x, c->pos.y + 8, &scratch);
    if (cl.isColliding.character['a'] || cl.isColliding.character['b']) {
      play(COIN);
      addScore(mirrorfloorMultiplier, c->pos.x, c->pos.y);
      mirrorfloorMultiplier++;
      mirrorfloorPlayer.side *= -1;
      mirrorfloorPlayer.vy *= -1;
      mirrorfloorPlayer.y = mirrorfloorPlayer.my;
      c->isAlive = false;
      continue;
    }
    if (c->pos.x < -3) {
      if (mirrorfloorMultiplier > 1) {
        mirrorfloorMultiplier--;
      }
      c->isAlive = false;
      continue;
    }
  }
}

void addGameMirrorfloor() {
  addGame(mirrorfloorTitle, mirrorfloorDescription, mirrorfloorCharacters,
          mirrorfloorCharactersCount, &mirrorfloorOptions, false,
          &mirrorfloorUpdate);
}
