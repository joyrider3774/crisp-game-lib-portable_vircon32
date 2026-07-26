#include "../cglp.h"

int* sightonTitle = "SIGHT ON";
int* sightonDescription = "[Tap]\n Fire";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] sightonCharacters = {
    {
        " llll ",
        "l ll l",
        "l ll l",
        " llll ",
        "ll  ll",
        "ll  ll",
    },
    {
        "ll  ll",
        "l    l",
        "  ll  ",
        "  ll  ",
        "l    l",
        "ll  ll",
    },
};
int sightonCharactersCount = 2;

Options sightonOptions = {100, 100, 7, true};

int[3] sightonStarColors = {CYAN, GREEN, BLACK};

struct SightonTarget {
  Vector pos;
  Vector vel;
  bool isAlive;
};
// Target spawn interval is rnd(120,150)/difficulty (scales with difficulty directly) while each
// target's lifetime only shrinks as 1/sqrt(difficulty) (vel scaled by sqrt(difficulty)*0.1) -
// concurrent count grows unboundedly (~11*sqrt(difficulty)), reachable within ~30min of play.
#define SIGHTON_MAX_TARGET_COUNT 512
SightonTarget[SIGHTON_MAX_TARGET_COUNT] sightonTargets;
int sightonTargetIndex;

struct SightonCurrentEnemy {
  Vector pos;
  Vector vel;
  Vector sink;
  int targetIndex;
};
SightonCurrentEnemy sightonCurrentEnemy;

struct SightonEnemy {
  Vector pos;
  Vector vel;
  Vector sink;
  int targetIndex;
  bool isAlive;
};
#define SIGHTON_MAX_ENEMY_COUNT 64
SightonEnemy[SIGHTON_MAX_ENEMY_COUNT] sightonEnemies;
int sightonEnemyIndex;

int sightonEnemyCount;
float sightonNextEnemyInterval;
float sightonNextEnemyTicks;
float sightonNextTargetTicks;

struct SightonExplosion {
  Vector pos;
  int ticks;
  bool isAlive;
};
#define SIGHTON_MAX_EXPLOSION_COUNT 32
SightonExplosion[SIGHTON_MAX_EXPLOSION_COUNT] sightonExplosions;
int sightonExplosionIndex;

struct SightonSight {
  Vector pos;
  Vector vel;
};
SightonSight sightonSight;

int sightonMultiplier;
float sightonSightDownY;

struct SightonStar {
  Vector pos;
  int color;
  int ticks;
};
SightonStar[30] sightonStars;

void sightonUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(sightonEnemies);
    sightonEnemyIndex = 0;
    sightonEnemyCount = 0;
    sightonNextEnemyInterval = 0;
    sightonNextEnemyTicks = 0;
    sightonNextTargetTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(sightonTargets);
    sightonTargetIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(sightonExplosions);
    sightonExplosionIndex = 0;
    vectorSet(&sightonSight.pos, 50, 50);
    vectorSet(&sightonSight.vel, 0, 0);
    sightonMultiplier = 1;
    sightonSightDownY = 0;
    TIMES(30, si) {
      vectorSet(&sightonStars[si].pos, rnd(0, 99), rnd(0, 99));
      sightonStars[si].color = sightonStarColors[rndi(0, 3)];
      sightonStars[si].ticks = rndi(0, 999);
    }
  }
  TIMES(30, si2) {
    SightonStar* s = &sightonStars[si2];
    s->ticks++;
    if (s->ticks % 150 < 100) {
      color = s->color;
      box(s->pos.x, s->pos.y, 1, 1, &scratch);
      s->pos.y += sqrt(difficulty) * 0.5;
      if (s->pos.y > 99) {
        s->pos.x = rnd(0, 99);
        s->pos.y = -rnd(0, 9);
      }
    }
  }
  color = LIGHT_RED;
  rect(0, 90, 100, 10, &scratch);
  sightonNextTargetTicks--;
  if (sightonNextTargetTicks < 0) {
    play(SELECT);
    float sd = sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(sightonTargets, sightonTargetIndex, SightonTarget, nt);
    vectorSet(&nt->pos, rnd(20, 80), 0);
    vectorSet(&nt->vel, rnd(0, sd) * RNDPM() * 0.1, sd * 0.1);
    nt->isAlive = true;
    int newTargetIndex = sightonTargetIndex;
    sightonTargetIndex = cgl_wrap(sightonTargetIndex + 1, 0, SIGHTON_MAX_TARGET_COUNT);
    vectorSet(&sightonCurrentEnemy.pos, rnd(10, 90), -5);
    vectorSet(&sightonCurrentEnemy.vel, rnd(1, 2) * RNDPM(), rnd(1, 2) * RNDPM());
    vectorSet(&sightonCurrentEnemy.sink, rnd(0.02, 0.07), rnd(0.02, 0.07));
    sightonCurrentEnemy.targetIndex = newTargetIndex;
    sightonEnemyCount = rndi(2, 5);
    sightonNextEnemyInterval = rnd(3, 9) / sqrt(difficulty);
    sightonNextEnemyTicks = 0;
    sightonNextTargetTicks = rnd(120, 150) / difficulty;
  }
  FOR_EACH(sightonTargets, ti) {
    ASSIGN_ARRAY_ITEM(sightonTargets, ti, SightonTarget, t);
    SKIP_IS_NOT_ALIVE(t);
    vectorAdd(&t->pos, t->vel.x, t->vel.y);
    t->pos.x = cgl_wrap(t->pos.x, 0, 99);
    if (t->pos.y > 150) {
      t->isAlive = false;
      continue;
    }
  }
  vectorAdd(&sightonCurrentEnemy.pos, sightonTargets[sightonCurrentEnemy.targetIndex].vel.x,
            sightonTargets[sightonCurrentEnemy.targetIndex].vel.y);
  sightonNextEnemyTicks--;
  if (sightonNextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(sightonEnemies, sightonEnemyIndex, SightonEnemy, ne);
    ne->pos = sightonCurrentEnemy.pos;
    ne->vel = sightonCurrentEnemy.vel;
    ne->sink = sightonCurrentEnemy.sink;
    ne->targetIndex = sightonCurrentEnemy.targetIndex;
    ne->isAlive = true;
    sightonEnemyIndex = cgl_wrap(sightonEnemyIndex + 1, 0, SIGHTON_MAX_ENEMY_COUNT);
    sightonNextEnemyTicks = sightonNextEnemyInterval;
    sightonEnemyCount--;
    if (sightonEnemyCount == 0) {
      sightonNextEnemyTicks = 9999;
    }
  }
  COUNT_IS_ALIVE(sightonEnemies, sightonAliveEnemyCount);
  if (sightonAliveEnemyCount == 0 && sightonEnemyCount == 0) {
    sightonNextTargetTicks = 0;
  }
  float maxY = 0;
  bool hasSightEnemy = false;
  int sightEnemyIndex2 = -1;
  color = YELLOW;
  FOR_EACH(sightonExplosions, ei) {
    ASSIGN_ARRAY_ITEM(sightonExplosions, ei, SightonExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    float s = cos(e->ticks * 0.05) * 15;
    e->ticks++;
    if (s < 0) {
      e->isAlive = false;
      continue;
    }
    box(e->pos.x, e->pos.y, s, s, &scratch);
  }
  color = RED;
  FOR_EACH(sightonEnemies, ei2) {
    ASSIGN_ARRAY_ITEM(sightonEnemies, ei2, SightonEnemy, e2);
    SKIP_IS_NOT_ALIVE(e2);
    SightonTarget* tgt = &sightonTargets[e2->targetIndex];
    e2->vel.x += cgl_wrap(tgt->pos.x - e2->pos.x, -50, 50) * e2->sink.x;
    e2->vel.y += (tgt->pos.y - e2->pos.y) * e2->sink.y;
    vectorMul(&e2->vel, 0.997);
    e2->pos.x += e2->vel.x * (sqrt(difficulty) - 0.8);
    e2->pos.y += e2->vel.y * (sqrt(difficulty) - 0.8);
    e2->pos.x = cgl_wrap(e2->pos.x, 0, 99);
    Collision ec;
    character("a", e2->pos.x, e2->pos.y, &ec);
    if (ec.isColliding.rect[YELLOW]) {
      play(HIT);
      particle(e2->pos.x, e2->pos.y, 16, 1, 0, CGLP_PI * 2);
      addScore(sightonMultiplier, e2->pos.x, clamp(e2->pos.y, 20, 99));
      sightonMultiplier++;
      e2->isAlive = false;
      continue;
    }
    if (e2->pos.y > maxY) {
      hasSightEnemy = true;
      sightEnemyIndex2 = ei2;
      maxY = e2->pos.y;
    }
    if (e2->pos.y > 90) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  float sex, sey;
  if (hasSightEnemy) {
    sex = sightonEnemies[sightEnemyIndex2].pos.x;
    sey = sightonEnemies[sightEnemyIndex2].pos.y;
  } else {
    sex = 50;
    sey = 50;
  }
  sightonSight.vel.x += cgl_wrap(sex - sightonSight.pos.x, -50, 50) * 0.01;
  sightonSight.vel.y += (sey - sightonSight.pos.y) * 0.01;
  vectorMul(&sightonSight.vel, 0.97);
  sightonSight.pos.x += sightonSight.vel.x * (sqrt(difficulty) - 0.8);
  sightonSight.pos.y += sightonSight.vel.y * (sqrt(difficulty) - 0.8);
  sightonSight.pos.x = cgl_wrap(sightonSight.pos.x, 0, 99);
  color = BLACK;
  character("b", sightonSight.pos.x, sightonSight.pos.y, &scratch);
  if (input.isJustPressed) {
    play(EXPLOSION);
    sightonMultiplier = 1;
    thickness = 1;
    line(0, 90, sightonSight.pos.x, sightonSight.pos.y, &scratch);
    line(99, 90, sightonSight.pos.x, sightonSight.pos.y, &scratch);
    ASSIGN_ARRAY_ITEM(sightonExplosions, sightonExplosionIndex, SightonExplosion, nex);
    nex->pos = sightonSight.pos;
    nex->ticks = 0;
    nex->isAlive = true;
    sightonExplosionIndex = cgl_wrap(sightonExplosionIndex + 1, 0, SIGHTON_MAX_EXPLOSION_COUNT);
    FOR_EACH(sightonTargets, ti2) {
      ASSIGN_ARRAY_ITEM(sightonTargets, ti2, SightonTarget, t2);
      SKIP_IS_NOT_ALIVE(t2);
      t2->pos.y += sightonSightDownY;
    }
    sightonSightDownY++;
    vectorMul(&sightonSight.vel, 0.1);
  }
  sightonSightDownY *= 0.99;
}

void addGameSighton() {
  addGame(sightonTitle, sightonDescription, sightonCharacters, sightonCharactersCount,
          &sightonOptions, false, &sightonUpdate);
}
