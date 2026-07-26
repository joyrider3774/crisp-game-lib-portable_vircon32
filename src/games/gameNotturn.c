#include "../cglp.h"

int* notturnTitle = "NOT TURN";
int* notturnDescription = "[Hold]\n Turn & Speed up";

int[9][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] notturnCharacters = {
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
        "llllll",
        "l ll l",
        "l ll l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "l ll l",
        "l ll l",
        "llllll",
        " l  l ",
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
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
    },
    {
        " llll ",
        "l ll l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        " llll ",
        "l ll l",
        " llll ",
        "  ll  ",
        " l  l ",
    },
    {
        "  ll  ",
        " l ll ",
        " llll ",
        "  ll  ",
    },
};
int notturnCharactersCount = 9;

Options notturnOptions = {100, 100, 80, false};

#define NOTTURN_X_BAR_COUNT 5
#define NOTTURN_X_BAR_INTERVAL 20
#define NOTTURN_X_BAR_PADDING 10.0

float notturnCalcBarX(int i) {
  return i * NOTTURN_X_BAR_INTERVAL + NOTTURN_X_BAR_PADDING;
}

struct NotturnBar {
  Vector pos;
  bool isAlive;
};
#define NOTTURN_MAX_BAR_COUNT 64
NotturnBar[NOTTURN_MAX_BAR_COUNT] notturnBars;
int notturnBarIndex;
float notturnNextBarDist;

struct NotturnEnemy {
  Vector pos;
  Vector vel;
  int xIndex;
  float noTurnDist;
  float dotDist;
  bool isAlive;
};
#define NOTTURN_MAX_ENEMY_COUNT 64
NotturnEnemy[NOTTURN_MAX_ENEMY_COUNT] notturnEnemies;
int notturnEnemyIndex;
float notturnNextEnemyTicks;

struct NotturnPlayer {
  Vector pos;
  Vector vel;
  int xIndex;
  float noTurnDist;
  float speed;
};
NotturnPlayer notturnPlayer;

struct NotturnDot {
  Vector pos;
  bool isAlive;
};
#define NOTTURN_MAX_DOT_COUNT 64
NotturnDot[NOTTURN_MAX_DOT_COUNT] notturnDots;
int notturnDotIndex;

int notturnMultiplier;

void notturnUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(notturnBars);
    notturnBarIndex = 0;
    notturnNextBarDist = 0;
    INIT_UNALIVED_ARRAY_FAST(notturnEnemies);
    notturnEnemyIndex = 0;
    notturnNextEnemyTicks = 20;
    int xIndex = 2;
    vectorSet(&notturnPlayer.pos, notturnCalcBarX(xIndex), 99);
    vectorSet(&notturnPlayer.vel, 0, 1);
    notturnPlayer.xIndex = xIndex;
    notturnPlayer.noTurnDist = 5;
    notturnPlayer.speed = 1;
    INIT_UNALIVED_ARRAY_FAST(notturnDots);
    notturnDotIndex = 0;
    notturnMultiplier = 1;
  }
  float scr = 0;
  if (notturnPlayer.pos.y > 20) {
    scr = (notturnPlayer.pos.y - 20) * 0.1;
  }
  notturnNextBarDist -= scr;
  if (notturnNextBarDist < 0) {
    int i1 = rndi(0, NOTTURN_X_BAR_COUNT - 1);
    int i2 = rndi(0, NOTTURN_X_BAR_COUNT - 1);
    ASSIGN_ARRAY_ITEM(notturnBars, notturnBarIndex, NotturnBar, nb1);
    vectorSet(&nb1->pos, notturnCalcBarX(i1) + NOTTURN_X_BAR_INTERVAL / 2.0, 102);
    nb1->isAlive = true;
    notturnBarIndex = cgl_wrap(notturnBarIndex + 1, 0, NOTTURN_MAX_BAR_COUNT);
    if (fabs((float)(i2 - i1)) > 1) {
      ASSIGN_ARRAY_ITEM(notturnBars, notturnBarIndex, NotturnBar, nb2);
      vectorSet(&nb2->pos, notturnCalcBarX(i2) + NOTTURN_X_BAR_INTERVAL / 2.0, 102);
      nb2->isAlive = true;
      notturnBarIndex = cgl_wrap(notturnBarIndex + 1, 0, NOTTURN_MAX_BAR_COUNT);
    }
    notturnNextBarDist += rnd(9, 12);
  }
  color = LIGHT_BLACK;
  TIMES(NOTTURN_X_BAR_COUNT, i) { box(notturnCalcBarX(i), 50, 3, 100, &scratch); }
  color = LIGHT_BLUE;
  FOR_EACH(notturnBars, i) {
    ASSIGN_ARRAY_ITEM(notturnBars, i, NotturnBar, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y -= scr;
    box(b->pos.x, b->pos.y, NOTTURN_X_BAR_INTERVAL - 3, 3, &scratch);
    if (b->pos.y < -1) {
      b->isAlive = false;
      continue;
    }
  }
  float sp = clamp(sqrt(difficulty) * notturnPlayer.speed * 0.2, 0, 3);
  vectorAdd(&notturnPlayer.pos, notturnPlayer.vel.x * sp, notturnPlayer.vel.y * sp);
  notturnPlayer.noTurnDist -= notturnPlayer.vel.y * sp;
  int[2] pc;
  if (notturnPlayer.vel.x != 0) {
    float bx = notturnCalcBarX(notturnPlayer.xIndex);
    if ((notturnPlayer.pos.x - bx) * notturnPlayer.vel.x > 0) {
      notturnPlayer.pos.x = bx;
      vectorSet(&notturnPlayer.vel, 0, 1);
    }
    int divisor;
    if (input.isPressed) {
      divisor = 10;
    } else {
      divisor = 20;
    }
    pc[0] = 'a' + (int)floor(ticks / divisor) % 2;
  } else {
    if (notturnPlayer.noTurnDist < 0 && input.isPressed) {
      color = TRANSPARENT;
      Collision c1;
      box(notturnPlayer.pos.x + 6, notturnPlayer.pos.y + 3, 1, 1, &c1);
      if (c1.isColliding.rect[LIGHT_BLUE]) {
        play(SELECT);
        vectorSet(&notturnPlayer.vel, 1, 0);
        notturnPlayer.xIndex++;
        notturnPlayer.noTurnDist = 5;
      } else {
        Collision c2;
        box(notturnPlayer.pos.x - 6, notturnPlayer.pos.y + 3, 1, 1, &c2);
        if (c2.isColliding.rect[LIGHT_BLUE]) {
          play(SELECT);
          vectorSet(&notturnPlayer.vel, -1, 0);
          notturnPlayer.xIndex--;
          notturnPlayer.noTurnDist = 5;
        }
      }
    }
    int divisor2;
    if (input.isPressed) {
      divisor2 = 10;
    } else {
      divisor2 = 20;
    }
    pc[0] = 'c' + (int)floor(ticks / divisor2) % 2;
  }
  pc[1] = 0;
  if (input.isJustPressed) {
    play(LASER);
  }
  if (input.isJustReleased) {
    play(HIT);
  }
  float speedTarget;
  if (input.isPressed) {
    speedTarget = 4;
  } else {
    speedTarget = 1;
  }
  notturnPlayer.speed += (speedTarget - notturnPlayer.speed) * 0.2;
  color = BLACK;
  characterOptions.isMirrorX = notturnPlayer.vel.x <= 0;
  character(pc, notturnPlayer.pos.x, notturnPlayer.pos.y, &scratch);
  characterOptions.isMirrorX = false;
  notturnPlayer.pos.y -= scr;
  notturnNextEnemyTicks--;
  if (notturnNextEnemyTicks < 0) {
    int xIndex = rndi(0, NOTTURN_X_BAR_COUNT);
    ASSIGN_ARRAY_ITEM(notturnEnemies, notturnEnemyIndex, NotturnEnemy, ne);
    vectorSet(&ne->pos, notturnCalcBarX(xIndex), 103);
    vectorSet(&ne->vel, 0, -1);
    ne->xIndex = xIndex;
    ne->noTurnDist = 0;
    ne->dotDist = 0;
    ne->isAlive = true;
    notturnEnemyIndex = cgl_wrap(notturnEnemyIndex + 1, 0, NOTTURN_MAX_ENEMY_COUNT);
    notturnNextEnemyTicks = rnd(100, 150) / difficulty;
  }
  FOR_EACH(notturnEnemies, i) {
    ASSIGN_ARRAY_ITEM(notturnEnemies, i, NotturnEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float esp = clamp(sqrt(difficulty) * 0.3, 0, 3);
    vectorAdd(&e->pos, e->vel.x * esp, e->vel.y * esp);
    e->noTurnDist += e->vel.y * esp;
    if (e->vel.x != 0) {
      float ebx = notturnCalcBarX(e->xIndex);
      if ((e->pos.x - ebx) * e->vel.x > 0) {
        e->pos.x = ebx;
        vectorSet(&e->vel, 0, -1);
      }
    } else if (e->noTurnDist < 0) {
      color = TRANSPARENT;
      Collision ec1;
      box(e->pos.x + 6, e->pos.y + 3, 1, 1, &ec1);
      if (ec1.isColliding.rect[LIGHT_BLUE]) {
        vectorSet(&e->vel, 1, 0);
        e->xIndex++;
        e->noTurnDist = 5;
      } else {
        Collision ec2;
        box(e->pos.x - 6, e->pos.y + 3, 1, 1, &ec2);
        if (ec2.isColliding.rect[LIGHT_BLUE]) {
          vectorSet(&e->vel, -1, 0);
          e->xIndex--;
          e->noTurnDist = 5;
        }
      }
    }
    color = RED;
    Collision ecc;
    character("e", e->pos.x, e->pos.y, &ecc);
    if (ecc.isColliding.character['a'] || ecc.isColliding.character['b'] ||
        ecc.isColliding.character['c'] || ecc.isColliding.character['d']) {
      play(EXPLOSION);
      gameOver();
    }
    e->pos.y -= scr;
    e->dotDist -= esp;
    if (e->dotDist < 0) {
      e->dotDist += 6;
      ASSIGN_ARRAY_ITEM(notturnDots, notturnDotIndex, NotturnDot, nd);
      nd->pos = e->pos;
      nd->isAlive = true;
      notturnDotIndex = cgl_wrap(notturnDotIndex + 1, 0, NOTTURN_MAX_DOT_COUNT);
    }
    if (e->pos.y < -3) {
      if (notturnMultiplier > 1) {
        notturnMultiplier--;
      }
      e->isAlive = false;
      continue;
    }
  }
  color = YELLOW;
  FOR_EACH(notturnDots, i) {
    ASSIGN_ARRAY_ITEM(notturnDots, i, NotturnDot, d);
    SKIP_IS_NOT_ALIVE(d);
    Collision dc;
    character("i", d->pos.x, d->pos.y, &dc);
    if (dc.isColliding.character['a'] || dc.isColliding.character['b'] ||
        dc.isColliding.character['c'] || dc.isColliding.character['d']) {
      play(COIN);
      addScore(notturnMultiplier, d->pos.x, d->pos.y);
      notturnMultiplier++;
      d->isAlive = false;
      continue;
    }
    d->pos.y -= scr;
    if (d->pos.y < -3) {
      d->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(notturnMultiplier));
  text(multText, 3, 10, &scratch);
}

void addGameNotturn() {
  addGame(notturnTitle, notturnDescription, notturnCharacters, notturnCharactersCount,
          &notturnOptions, false, &notturnUpdate);
}
