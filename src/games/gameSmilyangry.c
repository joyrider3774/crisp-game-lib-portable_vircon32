#include "../cglp.h"

int* smilyangryTitle = "SMILY ANGRY";
int* smilyangryDescription = "[Tap] Turn";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] smilyangryCharacters = {
    {
        " llll ",
        "l ll l",
        " llll ",
        "l ll l",
        "ll  ll",
        " llll ",
    },
    {
        " llll ",
        "l ll l",
        "llllll",
        "ll  ll",
        "l ll l",
        " llll ",
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
int smilyangryCharactersCount = 4;

Options smilyangryOptions = {100, 100, 1, true};

struct SmilyangrySmile {
  Vector pos;
  Vector targetPos;
  float fireInterval;
  float fireSpeed;
  float fireTicks;
  bool isSmile;
  bool isRed;
};
SmilyangrySmile[2] smilyangrySmiles;

struct SmilyangryBullet {
  Vector pos;
  Vector vel;
  bool isRed;
  bool isBonus;
  bool isAlive;
};
#define SMILYANGRY_MAX_BULLET_COUNT 64
SmilyangryBullet[SMILYANGRY_MAX_BULLET_COUNT] smilyangryBullets;
int smilyangryBulletIndex;

struct SmilyangryPlayer {
  Vector pos;
  int vx;
  float speed;
};
SmilyangryPlayer smilyangryPlayer;

int smilyangryMultiplier;

void smilyangryFire(float x, float y, float speed, bool isSmile, bool isRed) {
  if (isSmile) {
    play(LASER);
  } else {
    play(HIT);
  }
  Vector vel;
  if (isRed) {
    float t1 = distanceTo(&smilyangryPlayer.pos, x, y) / speed;
    Vector p2;
    vectorSet(&p2, smilyangryPlayer.pos.x + t1 * smilyangryPlayer.vx * smilyangryPlayer.speed,
              smilyangryPlayer.pos.y);
    float t2 = distanceTo(&p2, x, y) / speed;
    Vector p3;
    vectorSet(&p3, smilyangryPlayer.pos.x + t2 * smilyangryPlayer.vx * smilyangryPlayer.speed,
              smilyangryPlayer.pos.y);
    Vector src;
    vectorSet(&src, x, y);
    float a = angleTo(&src, p3.x, p3.y);
    vectorSet(&vel, 0, 0);
    addWithAngle(&vel, a, speed);
  } else {
    Vector src2;
    vectorSet(&src2, x, y);
    float a2 = angleTo(&src2, smilyangryPlayer.pos.x, smilyangryPlayer.pos.y);
    vectorSet(&vel, 0, 0);
    addWithAngle(&vel, a2, speed);
  }
  ASSIGN_ARRAY_ITEM(smilyangryBullets, smilyangryBulletIndex, SmilyangryBullet, nb);
  vectorSet(&nb->pos, x, y);
  nb->vel = vel;
  nb->isRed = isRed;
  nb->isBonus = isSmile;
  nb->isAlive = true;
  smilyangryBulletIndex = cgl_wrap(smilyangryBulletIndex + 1, 0, SMILYANGRY_MAX_BULLET_COUNT);
}

void smilyangryUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&smilyangrySmiles[0].pos, 30, 20);
    vectorSet(&smilyangrySmiles[0].targetPos, 30, 20);
    smilyangrySmiles[0].fireInterval = 30;
    smilyangrySmiles[0].fireSpeed = 1;
    smilyangrySmiles[0].fireTicks = 20;
    smilyangrySmiles[0].isSmile = true;
    smilyangrySmiles[0].isRed = true;
    vectorSet(&smilyangrySmiles[1].pos, 70, 20);
    vectorSet(&smilyangrySmiles[1].targetPos, 70, 20);
    smilyangrySmiles[1].fireInterval = 30;
    smilyangrySmiles[1].fireSpeed = 1;
    smilyangrySmiles[1].fireTicks = 50;
    smilyangrySmiles[1].isSmile = false;
    smilyangrySmiles[1].isRed = false;
    INIT_UNALIVED_ARRAY_FAST(smilyangryBullets);
    smilyangryBulletIndex = 0;
    vectorSet(&smilyangryPlayer.pos, 50, 90);
    smilyangryPlayer.vx = 1;
    smilyangryPlayer.speed = 1;
    smilyangryMultiplier = 1;
  }
  color = LIGHT_BLACK;
  rect(0, 93, 100, 7, &scratch);
  TIMES(2, si) {
    SmilyangrySmile* s = &smilyangrySmiles[si];
    s->pos.x += (s->targetPos.x - s->pos.x) * 0.1;
    s->pos.y += (s->targetPos.y - s->pos.y) * 0.1;
    s->fireTicks--;
    if (s->fireTicks < 0) {
      smilyangryFire(s->pos.x, s->pos.y, s->fireSpeed, s->isSmile, s->isRed);
      s->fireTicks = s->fireInterval;
    }
    float changeRatio = 0.002 * sqrt(difficulty);
    if (rnd(0, 1) < changeRatio) {
      vectorSet(&s->targetPos, rnd(10, 90), rnd(10, 40));
    }
    if (rnd(0, 1) < changeRatio) {
      s->isSmile = !s->isSmile;
    }
    if (rnd(0, 1) < changeRatio) {
      s->isRed = !s->isRed;
    }
    if (rnd(0, 1) < changeRatio) {
      s->fireInterval = rnd(30, 40) / sqrt(difficulty);
      s->fireSpeed = rnd(0.9, 1.2) * sqrt(difficulty);
    }
    if (s->isRed) {
      if (s->isSmile) {
        color = YELLOW;
      } else {
        color = RED;
      }
    } else {
      if (s->isSmile) {
        color = GREEN;
      } else {
        color = CYAN;
      }
    }
    if (s->isSmile) {
      character("a", s->pos.x, s->pos.y, &scratch);
    } else {
      character("b", s->pos.x, s->pos.y, &scratch);
    }
  }
  smilyangryPlayer.speed = sqrt(difficulty) * 0.5;
  if (input.isJustPressed) {
    play(SELECT);
    smilyangryPlayer.vx *= -1;
  }
  smilyangryPlayer.pos.x =
      cgl_wrap(smilyangryPlayer.pos.x + smilyangryPlayer.vx * smilyangryPlayer.speed, -3, 103);
  color = BLACK;
  int[2] pc;
  pc[0] = 'c' + (ticks / 30) % 2;
  pc[1] = 0;
  characterOptions.isMirrorX = smilyangryPlayer.vx < 0;
  character(pc, smilyangryPlayer.pos.x, smilyangryPlayer.pos.y, &scratch);
  characterOptions.isMirrorX = false;
  FOR_EACH(smilyangryBullets, bi) {
    ASSIGN_ARRAY_ITEM(smilyangryBullets, bi, SmilyangryBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    b->pos.x = cgl_wrap(b->pos.x, -3, 103);
    if (b->isRed) {
      color = RED;
    } else {
      color = CYAN;
    }
    Collision bc;
    if (b->isBonus) {
      text("$", b->pos.x, b->pos.y, &bc);
    } else {
      thickness = 2;
      bar(b->pos.x, b->pos.y, 3, vectorAngle(&b->vel), &bc);
    }
    if (bc.isColliding.character['c'] || bc.isColliding.character['d']) {
      if (b->isBonus) {
        play(POWER_UP);
        addScore(smilyangryMultiplier, smilyangryPlayer.pos.x, smilyangryPlayer.pos.y);
        smilyangryMultiplier++;
        b->isAlive = false;
        continue;
      } else {
        play(EXPLOSION);
        gameOver();
      }
    }
    if (bc.isColliding.rect[LIGHT_BLACK]) {
      if (b->isBonus && smilyangryMultiplier > 1) {
        smilyangryMultiplier--;
      }
      b->isAlive = false;
      continue;
    }
  }
}

void addGameSmilyangry() {
  addGame(smilyangryTitle, smilyangryDescription, smilyangryCharacters,
          smilyangryCharactersCount, &smilyangryOptions, false, &smilyangryUpdate);
}
