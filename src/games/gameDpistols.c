#include "../cglp.h"

int* dpistolsTitle = "D PISTOLS";
int* dpistolsDescription = "[Tap]\n Turn & Fire\n[Hold]\n Cross Fire";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] dpistolsCharacters = {
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
int dpistolsCharactersCount = 4;

Options dpistolsOptions = {100, 100, 7, true};

struct DpistolsPlayer {
  Vector pos;
  int my;
};
DpistolsPlayer dpistolsPlayer;
float dpistolsHoldTicks;

struct DpistolsShot {
  Vector pos;
  float angle;
  bool isAlive;
};
#define DPISTOLS_MAX_SHOT_COUNT 32
DpistolsShot[DPISTOLS_MAX_SHOT_COUNT] dpistolsShots;
int dpistolsShotIndex;

struct DpistolsEnemy {
  Vector pos;
  Vector vel;
  int ticks;
  bool isAlive;
};
// Enemies only ever expire via being shot or hitting the player - once past
// x==50 they turn to vertical flight with no offscreen/bounds removal at
// all, so any enemy the player merely lets fly by (not directly hit) stays
// "alive" in this array forever; spawn rate also grows with difficulty
// (~2*difficulty/sec), so 32 fills up within seconds of normal play.
#define DPISTOLS_MAX_ENEMY_COUNT 1024
DpistolsEnemy[DPISTOLS_MAX_ENEMY_COUNT] dpistolsEnemies;
int dpistolsEnemyIndex;
float dpistolsNextEnemyTicks;
float dpistolsNextEnemyY;
float dpistolsPrevEnemyVx;
int dpistolsMultiplier;

void dpistolsUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&dpistolsPlayer.pos, 50, 20);
    dpistolsPlayer.my = 1;
    dpistolsHoldTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(dpistolsShots);
    dpistolsShotIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(dpistolsEnemies);
    dpistolsEnemyIndex = 0;
    dpistolsNextEnemyTicks = 0;
    dpistolsNextEnemyY = 50;
    dpistolsPrevEnemyVx = 1;
    dpistolsMultiplier = 0;
  }
  if (input.isJustPressed) {
    play(LASER);
    dpistolsPlayer.my *= -1;
    TIMES(2, i) {
      ASSIGN_ARRAY_ITEM(dpistolsShots, dpistolsShotIndex, DpistolsShot, s);
      vectorSet(&s->pos, dpistolsPlayer.pos.x, dpistolsPlayer.pos.y + dpistolsPlayer.my);
      s->angle = i * CGLP_PI;
      s->isAlive = true;
      dpistolsShotIndex = cgl_wrap(dpistolsShotIndex + 1, 0, DPISTOLS_MAX_SHOT_COUNT);
    }
    dpistolsMultiplier = (int)clamp(dpistolsMultiplier - 1, 0, 99);
  }
  if (input.isPressed) {
    dpistolsHoldTicks += difficulty;
    if (dpistolsHoldTicks > 30) {
      play(SELECT);
      TIMES(4, i) {
        ASSIGN_ARRAY_ITEM(dpistolsShots, dpistolsShotIndex, DpistolsShot, s2);
        vectorSet(&s2->pos, dpistolsPlayer.pos.x, dpistolsPlayer.pos.y);
        s2->angle = (i * CGLP_PI) / 2;
        s2->isAlive = true;
        dpistolsShotIndex = cgl_wrap(dpistolsShotIndex + 1, 0, DPISTOLS_MAX_SHOT_COUNT);
      }
      dpistolsHoldTicks = 0;
      dpistolsMultiplier = (int)clamp(dpistolsMultiplier - 5, 1, 99);
    }
  } else {
    dpistolsHoldTicks = 0;
  }
  dpistolsPlayer.pos.y += dpistolsPlayer.my * difficulty * (1 - dpistolsHoldTicks / 30);
  if ((dpistolsPlayer.pos.y < 0 && dpistolsPlayer.my < 0) ||
      (dpistolsPlayer.pos.y > 99 && dpistolsPlayer.my > 0)) {
    dpistolsPlayer.my *= -1;
  }
  color = BLACK;
  int[2] pc;
  pc[0] = 'a' + (int)floor(ticks / 15) % 2;
  pc[1] = 0;
  characterOptions.isMirrorX = dpistolsPlayer.my == -1;
  character(pc, dpistolsPlayer.pos.x, dpistolsPlayer.pos.y, &scratch);
  characterOptions.isMirrorX = false;
  FOR_EACH(dpistolsShots, i) {
    ASSIGN_ARRAY_ITEM(dpistolsShots, i, DpistolsShot, s);
    SKIP_IS_NOT_ALIVE(s);
    addWithAngle(&s->pos, s->angle, difficulty * 2);
    thickness = 6;
    barCenterPosRatio = 0.5;
    bar(s->pos.x, s->pos.y, 1, s->angle, &scratch);
    bool inRect = s->pos.x >= -3 && s->pos.x < 103 && s->pos.y >= -3 && s->pos.y < 103;
    s->isAlive = inRect;
  }
  COUNT_IS_ALIVE(dpistolsEnemies, aliveEnemyCount);
  if (aliveEnemyCount == 0) {
    dpistolsNextEnemyTicks = 0;
  }
  dpistolsNextEnemyTicks--;
  if (dpistolsNextEnemyTicks < 0) {
    if (rnd(0, 1) < 0.3) {
      dpistolsNextEnemyY = rnd(9, 90);
      dpistolsPrevEnemyVx *= -1;
    }
    Vector vel;
    vectorSet(&vel, dpistolsPrevEnemyVx, 0);
    vel.x *= rnd(1, difficulty) * 0.3;
    ASSIGN_ARRAY_ITEM(dpistolsEnemies, dpistolsEnemyIndex, DpistolsEnemy, ne);
    float ex;
    if (vel.x > 0) {
      ex = -3;
    } else {
      ex = 103;
    }
    vectorSet(&ne->pos, ex, dpistolsNextEnemyY);
    ne->vel = vel;
    ne->ticks = 0;
    ne->isAlive = true;
    dpistolsEnemyIndex = cgl_wrap(dpistolsEnemyIndex + 1, 0, DPISTOLS_MAX_ENEMY_COUNT);
    dpistolsNextEnemyTicks = rnd(20, 40) / difficulty;
  }
  color = RED;
  FOR_EACH(dpistolsEnemies, i) {
    ASSIGN_ARRAY_ITEM(dpistolsEnemies, i, DpistolsEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    if ((e->pos.x > 50 && e->vel.x > 0.1) || (e->pos.x < 50 && e->vel.x < -0.1)) {
      e->pos.x = 50;
      if (dpistolsPlayer.pos.y < e->pos.y) {
        e->vel.y = -fabs(e->vel.x);
      } else {
        e->vel.y = fabs(e->vel.x);
      }
      e->vel.x *= 0.0001;
    }
    e->ticks++;
    int[2] ec;
    ec[0] = 'c' + (int)floor(e->ticks / 15) % 2;
    ec[1] = 0;
    characterOptions.isMirrorX = e->vel.x < 0;
    Collision c;
    character(ec, e->pos.x, e->pos.y, &c);
    characterOptions.isMirrorX = false;
    if (c.isColliding.rect[BLACK]) {
      play(HIT);
      dpistolsMultiplier = (int)clamp(dpistolsMultiplier + 1, 0, 99);
      addScore(dpistolsMultiplier, e->pos.x, e->pos.y);
      particle(e->pos.x, e->pos.y, 16, 1, 0, CGLP_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (c.isColliding.character['a'] || c.isColliding.character['b']) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void addGameDpistols() {
  addGame(dpistolsTitle, dpistolsDescription, dpistolsCharacters,
          dpistolsCharactersCount, &dpistolsOptions, false, &dpistolsUpdate);
}
