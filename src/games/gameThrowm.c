#include "../cglp.h"

int* throwmTitle = "THROW M";
int* throwmDescription = "[Hold]\n Set angle\n[Release]\n Shoot";

int[5][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] throwmCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
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
        " lll  ",
        "ll ll ",
        "l lll ",
        "lllll ",
        " lll  ",
        "  ll  ",
    },
    {
        "r  r  ",
        "llllrr",
        "r  r  ",
    },
    {
        " yyyy ",
        " y yy ",
        "l yyyl",
        "lyyyyl",
        " yyyy ",
        " yyyy ",
    },
};
int throwmCharactersCount = 5;

Options throwmOptions = {100, 100, 6, false};

int[4] throwmEnemyColors = {RED, CYAN, YELLOW, GREEN};

struct ThrowmEnemy {
  Vector pos;
  float vy;
  float fireInterval;
  float fireTicks;
  int color;
  bool isFalling;
  bool isAlive;
};
// Unhit enemies fall until off-screen; spawn interval shrinks as
// difficulty^-1.5 while lifetime only shrinks as difficulty^-1, so
// concurrent count grows ~8*sqrt(difficulty) over a long session - sized
// with headroom well above that.
#define THROWM_MAX_ENEMY_COUNT 512
ThrowmEnemy[THROWM_MAX_ENEMY_COUNT] throwmEnemies;
int throwmEnemyIndex;
float throwmNextEnemyTicks;

struct ThrowmBullet {
  Vector pos;
  bool isAlive;
};
#define THROWM_MAX_BULLET_COUNT 64
ThrowmBullet[THROWM_MAX_BULLET_COUNT] throwmBullets;
int throwmBulletIndex;

struct ThrowmPlayer {
  Vector pos;
  float vy;
  bool hasFireAngle;
  float fireAngle;
};
ThrowmPlayer throwmPlayer;

struct ThrowmShot {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define THROWM_MAX_SHOT_COUNT 32
ThrowmShot[THROWM_MAX_SHOT_COUNT] throwmShots;
int throwmShotIndex;

int throwmMultiplier;

void throwmUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(throwmEnemies);
    throwmEnemyIndex = 0;
    throwmNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(throwmBullets);
    throwmBulletIndex = 0;
    vectorSet(&throwmPlayer.pos, 90, 50);
    throwmPlayer.vy = -1;
    throwmPlayer.hasFireAngle = false;
    INIT_UNALIVED_ARRAY_FAST(throwmShots);
    throwmShotIndex = 0;
    throwmMultiplier = 1;
  }
  color = LIGHT_BLACK;
  rect(85, 0, 15, 10, &scratch);
  rect(85, 92, 15, 8, &scratch);
  color = BLACK;
  FOR_EACH(throwmShots, si) {
    ASSIGN_ARRAY_ITEM(throwmShots, si, ThrowmShot, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    s->vel.y += difficulty * 0.07;
    character("e", s->pos.x, s->pos.y, &scratch);
    if (s->pos.y > 103) {
      s->isAlive = false;
      continue;
    }
  }
  throwmNextEnemyTicks--;
  if (throwmNextEnemyTicks < 0) {
    float fireInterval = rnd(200, 300) / sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(throwmEnemies, throwmEnemyIndex, ThrowmEnemy, ne);
    vectorSet(&ne->pos, rnd(3, 50), -5);
    ne->vy = rnd(0.1, 0.4) * difficulty;
    ne->fireInterval = fireInterval;
    ne->fireTicks = rnd(0, fireInterval);
    ne->color = throwmEnemyColors[rndi(0, 4)];
    ne->isFalling = false;
    ne->isAlive = true;
    throwmEnemyIndex = cgl_wrap(throwmEnemyIndex + 1, 0, THROWM_MAX_ENEMY_COUNT);
    throwmNextEnemyTicks = rnd(50, 60) / difficulty / sqrt(difficulty);
  }
  FOR_EACH(throwmEnemies, ei) {
    ASSIGN_ARRAY_ITEM(throwmEnemies, ei, ThrowmEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.y += e->vy;
    bool isHit = false;
    if (e->isFalling) {
      e->vy += 0.1;
    } else {
      e->fireTicks--;
      if (e->fireTicks < 0) {
        ASSIGN_ARRAY_ITEM(throwmBullets, throwmBulletIndex, ThrowmBullet, nb);
        nb->pos = e->pos;
        nb->isAlive = true;
        throwmBulletIndex = cgl_wrap(throwmBulletIndex + 1, 0, THROWM_MAX_BULLET_COUNT);
        e->fireTicks = e->fireInterval;
      }
      color = e->color;
      Collision ec;
      character("c", e->pos.x, e->pos.y - 6, &ec);
      if (ec.isColliding.character['e']) {
        isHit = true;
      }
    }
    color = BLACK;
    characterOptions.isMirrorY = e->isFalling;
    Collision ec2;
    character("b", e->pos.x, e->pos.y, &ec2);
    characterOptions.isMirrorY = false;
    if (ec2.isColliding.character['e'] && !e->isFalling) {
      isHit = true;
    }
    if (isHit) {
      play(POWER_UP);
      particle(e->pos.x, e->pos.y - 6, 16, 1, 0, CGLP_PI * 2);
      e->isFalling = true;
      addScore(throwmMultiplier, e->pos.x, e->pos.y);
      throwmMultiplier *= 2;
    }
    if (e->pos.y > 105) {
      e->isAlive = false;
      continue;
    }
  }
  float bs = difficulty * 0.5;
  color = BLACK;
  FOR_EACH(throwmBullets, bi) {
    ASSIGN_ARRAY_ITEM(throwmBullets, bi, ThrowmBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x += bs;
    Collision bc;
    character("d", b->pos.x, b->pos.y, &bc);
    if (bc.isColliding.character['e']) {
      play(HIT);
      particle(b->pos.x, b->pos.y, 16, 1, 0, CGLP_PI * 2);
      addScore(throwmMultiplier, b->pos.x, b->pos.y);
      throwmMultiplier++;
      b->isAlive = false;
      continue;
    }
    if (b->pos.x > 103 || bc.isColliding.rect[LIGHT_BLACK]) {
      b->isAlive = false;
      continue;
    }
  }
  throwmPlayer.pos.y += throwmPlayer.vy * difficulty * 0.5;
  if ((throwmPlayer.pos.y < 19 && throwmPlayer.vy < 0) ||
      (throwmPlayer.pos.y > 90 && throwmPlayer.vy > 0)) {
    throwmPlayer.vy *= -1;
  }
  color = BLUE;
  characterOptions.isMirrorX = true;
  Collision pc;
  character("c", throwmPlayer.pos.x, throwmPlayer.pos.y - 6, &pc);
  characterOptions.isMirrorX = false;
  if (pc.isColliding.character['d']) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLACK;
  characterOptions.isMirrorX = true;
  character("b", throwmPlayer.pos.x, throwmPlayer.pos.y, &scratch);
  characterOptions.isMirrorX = false;
  if (!throwmPlayer.hasFireAngle) {
    if (input.isJustPressed) {
      throwmPlayer.fireAngle = (CGLP_PI / 4) * 3;
      throwmPlayer.hasFireAngle = true;
    }
  }
  if (throwmPlayer.hasFireAngle) {
    throwmPlayer.fireAngle += 0.1 * difficulty;
    color = BLACK;
    Vector lineEnd;
    vectorSet(&lineEnd, throwmPlayer.pos.x, throwmPlayer.pos.y);
    addWithAngle(&lineEnd, throwmPlayer.fireAngle, 5);
    thickness = 2;
    line(throwmPlayer.pos.x, throwmPlayer.pos.y, lineEnd.x, lineEnd.y, &scratch);
    if (input.isJustReleased || throwmPlayer.fireAngle > (CGLP_PI / 8) * 11) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(throwmShots, throwmShotIndex, ThrowmShot, ns);
      ns->pos = throwmPlayer.pos;
      vectorSet(&ns->vel, 0, 0);
      addWithAngle(&ns->vel, throwmPlayer.fireAngle, sqrt(difficulty) * 3);
      ns->isAlive = true;
      throwmShotIndex = cgl_wrap(throwmShotIndex + 1, 0, THROWM_MAX_SHOT_COUNT);
      throwmPlayer.hasFireAngle = false;
      throwmMultiplier = 1;
    }
  }
}

void addGameThrowm() {
  addGame(throwmTitle, throwmDescription, throwmCharacters, throwmCharactersCount,
          &throwmOptions, false, &throwmUpdate);
}
