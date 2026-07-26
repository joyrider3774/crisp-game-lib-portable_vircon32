#include "../cglp.h"

int* clockturretTitle = "CLOCK TURRET";
int* clockturretDescription = "[Hold]\n Stop and Shoot";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] clockturretCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int clockturretCharactersCount = 0;

Options clockturretOptions = {100, 100, 1, false};

struct ClockturretHand {
  float angle;
  float angleVel;
  float shootTicks;
};
ClockturretHand clockturretHand;

struct ClockturretShot {
  Vector pos;
  float angle;
  bool isAlive;
};
#define CLOCKTURRET_MAX_SHOT_COUNT 32
ClockturretShot[CLOCKTURRET_MAX_SHOT_COUNT] clockturretShots;
int clockturretShotIndex;

struct ClockturretEnemy {
  Vector pos;
  float angle;
  float angleToCenter;
  float speed;
  float bulletTicks;
  bool wasInScreen;
  bool isAlive;
};
#define CLOCKTURRET_MAX_ENEMY_COUNT 16
ClockturretEnemy[CLOCKTURRET_MAX_ENEMY_COUNT] clockturretEnemies;
int clockturretEnemyIndex;
float clockturretNextEnemyTicks;

struct ClockturretBullet {
  Vector pos;
  float angle;
  float speed;
  bool isAlive;
};
#define CLOCKTURRET_MAX_BULLET_COUNT 16
ClockturretBullet[CLOCKTURRET_MAX_BULLET_COUNT] clockturretBullets;
int clockturretBulletIndex;
float clockturretNextBulletTicks;

struct ClockturretCoin {
  Vector pos;
  float angle;
  bool isAlive;
};
#define CLOCKTURRET_MAX_COIN_COUNT 16
ClockturretCoin[CLOCKTURRET_MAX_COIN_COUNT] clockturretCoins;
int clockturretCoinIndex;

int clockturretMultiplier;

void clockturretUpdate() {
  Collision scratch;
  if (!ticks) {
    clockturretHand.angle = -CGLP_PI_2;
    clockturretHand.angleVel = 1;
    clockturretHand.shootTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(clockturretShots);
    clockturretShotIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(clockturretEnemies);
    clockturretEnemyIndex = 0;
    clockturretNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(clockturretBullets);
    clockturretBulletIndex = 0;
    clockturretNextBulletTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(clockturretCoins);
    clockturretCoinIndex = 0;
    clockturretMultiplier = 1;
  }
  color = LIGHT_BLACK;
  TIMES(12, i) {
    Vector lp;
    vectorSet(&lp, 50, 50);
    addWithAngle(&lp, -CGLP_PI_2 + (i + 1) * CGLP_PI * 2 / 12, 25);
    if (i >= 9) {
      lp.x -= 3;
    }
    text(intToChar(i + 1), lp.x, lp.y, &scratch);
  }
  color = BLUE;
  barCenterPosRatio = 0.5;
  FOR_EACH(clockturretShots, i) {
    ASSIGN_ARRAY_ITEM(clockturretShots, i, ClockturretShot, s);
    SKIP_IS_NOT_ALIVE(s);
    addWithAngle(&s->pos, s->angle, sqrt(difficulty) * 3);
    thickness = 2;
    bar(s->pos.x, s->pos.y, 4, s->angle, &scratch);
    if (!(s->pos.x >= -5 && s->pos.x <= 105 && s->pos.y >= -5 && s->pos.y <= 105)) {
      s->isAlive = false;
    }
  }
  color = BLACK;
  if (input.isJustPressed) {
    play(SELECT);
    clockturretHand.angleVel *= -1;
  }
  if (input.isPressed) {
    clockturretHand.shootTicks += difficulty;
    if (clockturretHand.shootTicks > 7) {
      play(HIT);
      clockturretHand.shootTicks -= 7;
      ASSIGN_ARRAY_ITEM(clockturretShots, clockturretShotIndex, ClockturretShot, ns1);
      vectorSet(&ns1->pos, 50, 50);
      addWithAngle(&ns1->pos, clockturretHand.angle, 12);
      ns1->angle = clockturretHand.angle;
      ns1->isAlive = true;
      clockturretShotIndex = cgl_wrap(clockturretShotIndex + 1, 0, CLOCKTURRET_MAX_SHOT_COUNT);
      ASSIGN_ARRAY_ITEM(clockturretShots, clockturretShotIndex, ClockturretShot, ns2);
      vectorSet(&ns2->pos, 50, 50);
      addWithAngle(&ns2->pos, clockturretHand.angle, -5);
      ns2->angle = clockturretHand.angle + CGLP_PI;
      ns2->isAlive = true;
      clockturretShotIndex = cgl_wrap(clockturretShotIndex + 1, 0, CLOCKTURRET_MAX_SHOT_COUNT);
    }
  } else {
    clockturretHand.angle += difficulty * clockturretHand.angleVel * 0.1;
  }
  thickness = 3;
  barCenterPosRatio = 0.2;
  bar(50, 50, 20, clockturretHand.angle, &scratch);
  color = BLUE;
  box(50, 50, 5, 5, &scratch);
  clockturretNextBulletTicks -= difficulty;
  if (clockturretNextBulletTicks < 0) {
    COUNT_IS_ALIVE(clockturretEnemies, aliveEnemyCount);
    if (aliveEnemyCount > 0) {
      int target = rndi(0, aliveEnemyCount);
      int seen = 0;
      ClockturretEnemy* e = &clockturretEnemies[0];
      FOR_EACH(clockturretEnemies, i) {
        ASSIGN_ARRAY_ITEM(clockturretEnemies, i, ClockturretEnemy, ce);
        SKIP_IS_NOT_ALIVE(ce);
        if (seen == target) {
          e = ce;
          break;
        }
        seen++;
      }
      Vector fp;
      fp = e->pos;
      addWithAngle(&fp, e->angle, e->speed * 45 / difficulty);
      float oa = cgl_wrap(angleTo(&fp, 50, 50) - e->angleToCenter, -CGLP_PI, CGLP_PI);
      if (fp.x >= 5 && fp.x <= 95 && fp.y >= 5 && fp.y <= 95 && fabs(oa) > CGLP_PI * 0.07) {
        e->bulletTicks = 45;
        clockturretNextBulletTicks = rnd(45, 60);
      }
    }
  }
  color = YELLOW;
  FOR_EACH(clockturretCoins, i) {
    ASSIGN_ARRAY_ITEM(clockturretCoins, i, ClockturretCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    addWithAngle(&c->pos, c->angle, difficulty);
    box(c->pos.x, c->pos.y, 6, 6, &scratch);
    if (scratch.isColliding.rect[BLACK]) {
      play(COIN);
      clockturretMultiplier++;
      c->isAlive = false;
      continue;
    }
    if (!(c->pos.x >= -5 && c->pos.x <= 105 && c->pos.y >= -5 && c->pos.y <= 105)) {
      c->isAlive = false;
      continue;
    }
  }
  clockturretNextEnemyTicks -= difficulty;
  if (clockturretNextEnemyTicks < 0) {
    float angle = rnd(0, CGLP_PI * 2);
    float sign;
    if (rndi(0, 2) == 1) {
      sign = 1;
    } else {
      sign = -1;
    }
    float angleToCenter = angle + CGLP_PI_2 * sign;
    float d = rnd(30, 40);
    Vector pos;
    vectorSet(&pos, 50, 50);
    addWithAngle(&pos, angleToCenter, -d);
    addWithAngle(&pos, angle + CGLP_PI, 60);
    ASSIGN_ARRAY_ITEM(clockturretEnemies, clockturretEnemyIndex, ClockturretEnemy, ne);
    ne->pos = pos;
    ne->angle = angle;
    ne->angleToCenter = angleToCenter;
    ne->speed = rnd(1, difficulty) * 0.5;
    ne->bulletTicks = 0;
    ne->wasInScreen = false;
    ne->isAlive = true;
    clockturretEnemyIndex = cgl_wrap(clockturretEnemyIndex + 1, 0, CLOCKTURRET_MAX_ENEMY_COUNT);
    clockturretNextEnemyTicks = rnd(50, 90);
  }
  FOR_EACH(clockturretEnemies, i) {
    ASSIGN_ARRAY_ITEM(clockturretEnemies, i, ClockturretEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    addWithAngle(&e->pos, e->angle, e->speed);
    if (e->bulletTicks > 0) {
      e->bulletTicks -= difficulty;
      color = LIGHT_RED;
      float w = e->bulletTicks / 6 + 1;
      thickness = w;
      barCenterPosRatio = 0;
      bar(e->pos.x, e->pos.y, 100, e->angleToCenter, &scratch);
      if (e->bulletTicks <= 0) {
        play(LASER);
        ASSIGN_ARRAY_ITEM(clockturretBullets, clockturretBulletIndex, ClockturretBullet, nb);
        nb->pos = e->pos;
        nb->angle = e->angleToCenter;
        nb->speed = difficulty * 9;
        nb->isAlive = true;
        clockturretBulletIndex = cgl_wrap(clockturretBulletIndex + 1, 0, CLOCKTURRET_MAX_BULLET_COUNT);
      }
    }
    color = PURPLE;
    thickness = 5;
    barCenterPosRatio = 0.5;
    bar(e->pos.x, e->pos.y, 7, e->angleToCenter, &scratch);
    if (scratch.isColliding.rect[BLUE]) {
      play(POWER_UP);
      particle(e->pos.x, e->pos.y, 9, 1, 0, CGLP_PI * 2);
      addScore(clockturretMultiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      ASSIGN_ARRAY_ITEM(clockturretCoins, clockturretCoinIndex, ClockturretCoin, nc);
      nc->pos = e->pos;
      nc->angle = e->angleToCenter;
      nc->isAlive = true;
      clockturretCoinIndex = cgl_wrap(clockturretCoinIndex + 1, 0, CLOCKTURRET_MAX_COIN_COUNT);
      e->isAlive = false;
      continue;
    }
    if (!e->wasInScreen && e->pos.x >= 0 && e->pos.x <= 100 && e->pos.y >= 0 && e->pos.y <= 100) {
      e->wasInScreen = true;
    }
    if (e->wasInScreen &&
        !(e->pos.x >= -5 && e->pos.x <= 105 && e->pos.y >= -5 && e->pos.y <= 105)) {
      e->isAlive = false;
      continue;
    }
  }
  color = RED;
  barCenterPosRatio = 0.5;
  FOR_EACH(clockturretBullets, i) {
    ASSIGN_ARRAY_ITEM(clockturretBullets, i, ClockturretBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    addWithAngle(&b->pos, b->angle, b->speed);
    thickness = 4;
    bar(b->pos.x, b->pos.y, b->speed * 1.2, b->angle, &scratch);
    if (scratch.isColliding.rect[BLACK]) {
      play(EXPLOSION);
      gameOver();
    }
    if (!(b->pos.x >= -5 && b->pos.x <= 105 && b->pos.y >= -5 && b->pos.y <= 105)) {
      b->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(clockturretMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameClockturret() {
  addGame(clockturretTitle, clockturretDescription, clockturretCharacters,
          clockturretCharactersCount, &clockturretOptions, false,
          &clockturretUpdate);
}
