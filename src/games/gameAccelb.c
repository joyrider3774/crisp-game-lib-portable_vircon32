#include "../cglp.h"

int* accelbTitle = "ACCEL B";
int* accelbDescription = "[Tap]  Fire\n[Hold] Accel";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] accelbCharacters = {
    {
        "ll    ",
        " lllll",
    },
    {
        "   lll",
        " lllll",
        "llllll",
        "   ll ",
    },
};
int accelbCharactersCount = 2;

Options accelbOptions = {200, 100, 20, false};

struct AccelbPlayer {
  Vector pos;
  float vx;
};
AccelbPlayer accelbPlayer;

struct AccelbForest {
  Vector pos;
  Vector size;
};
#define ACCELB_FOREST_COUNT 7
AccelbForest[ACCELB_FOREST_COUNT] accelbForests;

struct AccelbSmoke {
  Vector pos;
  Vector vel;
  float ticks;
  bool isEnemy;
  bool isAlive;
};
#define ACCELB_MAX_SMOKE_COUNT 128
AccelbSmoke[ACCELB_MAX_SMOKE_COUNT] accelbSmokes;
int accelbSmokeIndex;

struct AccelbEnemy {
  Vector pos;
  float vx;
  float ma;
  int fireTicks;
  bool isAlive;
};
#define ACCELB_MAX_ENEMY_COUNT 32
AccelbEnemy[ACCELB_MAX_ENEMY_COUNT] accelbEnemies;
int accelbEnemyIndex;
float accelbNextEnemyDist;

struct AccelbMissile {
  Vector pos;
  float angle;
  float va;
  float speed;
  float smokeTicks;
  bool isAlive;
};
#define ACCELB_MAX_MISSILE_COUNT 32
AccelbMissile[ACCELB_MAX_MISSILE_COUNT] accelbMissiles;
int accelbMissileIndex;

// Vircon32 port note: upstream's playerMissile.target is a live reference to
// an enemy/enemy-missile's Vector (so it keeps homing on that object's
// current position even after the object itself is later removed from its
// own array, since the referenced Vector still exists as long as the
// missile holds it). There are no shared references in this dialect, so a
// missile instead stores which array/slot to keep reading from - this
// matches upstream as long as that array/slot isn't recycled by a new
// spawn before the missile finishes (enemies/enemy-missiles are capped
// well above what can realistically spawn within a missile's ~2 second
// homing lifetime, so that never happens in practice).
struct AccelbPlayerMissile {
  Vector pos;
  Vector vel;
  bool targetIsMissile;
  int targetIndex;
  float ticks;
  float exTicks;
  float smokeTicks;
  bool isAlive;
};
#define ACCELB_MAX_PLAYER_MISSILE_COUNT 64
AccelbPlayerMissile[ACCELB_MAX_PLAYER_MISSILE_COUNT] accelbPlayerMissiles;
int accelbPlayerMissileIndex;

int accelbMultiplier;

void accelbUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&accelbPlayer.pos, 20, 45);
    accelbPlayer.vx = 0;
    INIT_UNALIVED_ARRAY_FAST(accelbSmokes);
    accelbSmokeIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(accelbEnemies);
    accelbEnemyIndex = 0;
    accelbNextEnemyDist = 0;
    INIT_UNALIVED_ARRAY_FAST(accelbMissiles);
    accelbMissileIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(accelbPlayerMissiles);
    accelbPlayerMissileIndex = 0;
    TIMES(ACCELB_FOREST_COUNT, fi) {
      vectorSet(&accelbForests[fi].pos, -99 - fi * 40, 0);
      vectorSet(&accelbForests[fi].size, 0, 0);
    }
    accelbMultiplier = 1;
  }

  float scr = (accelbPlayer.pos.x - 20) * 0.1;
  color = YELLOW;
  rect(0, 90, 200, 10, &scratch);

  color = GREEN;
  TIMES(ACCELB_FOREST_COUNT, fi2) {
    AccelbForest* f = &accelbForests[fi2];
    box(f->pos.x, f->pos.y, f->size.x, f->size.y, &scratch);
    f->pos.x -= scr;
    if (f->pos.x < -99) {
      f->pos.x += 300 + rnd(0, 99);
      f->pos.y = rnd(90, 99);
      f->size.x = rnd(30, 50);
      f->size.y = rnd(5, 9);
    }
  }

  FOR_EACH(accelbSmokes, si) {
    ASSIGN_ARRAY_ITEM(accelbSmokes, si, AccelbSmoke, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    s->pos.x -= scr;
    vectorMul(&s->vel, 0.95);
    if (s->isEnemy && s->ticks < 20) {
      color = LIGHT_RED;
    } else {
      color = LIGHT_BLACK;
    }
    float ssize = 3 + cos(s->ticks * 0.03) * 5;
    box(s->pos.x, s->pos.y, ssize, ssize, &scratch);
    s->ticks += sqrt(difficulty);
    if (s->ticks > 60) {
      s->isAlive = false;
    }
  }

  float pressAccel;
  if (input.isPressed) {
    pressAccel = 2;
  } else {
    pressAccel = 0.2;
  }
  accelbPlayer.vx += (pressAccel * sqrt(difficulty) - accelbPlayer.vx) * 0.2;
  accelbPlayer.pos.x += accelbPlayer.vx - scr;
  color = BLUE;
  character("a", accelbPlayer.pos.x, accelbPlayer.pos.y, &scratch);
  color = PURPLE;
  rect(accelbPlayer.pos.x - 3, accelbPlayer.pos.y, -accelbPlayer.vx * 3, 1, &scratch);

  COUNT_IS_ALIVE(accelbPlayerMissiles, playerMissileCountBefore);
  if (playerMissileCountBefore == 0) {
    color = CYAN;
    box(accelbPlayer.pos.x, accelbPlayer.pos.y + 3, 5, 2, &scratch);
    if (input.isJustPressed) {
      play(SELECT);
      accelbMultiplier = 1;
      FOR_EACH(accelbEnemies, ei) {
        ASSIGN_ARRAY_ITEM(accelbEnemies, ei, AccelbEnemy, e);
        SKIP_IS_NOT_ALIVE(e);
        ASSIGN_ARRAY_ITEM(accelbPlayerMissiles, accelbPlayerMissileIndex, AccelbPlayerMissile, pm);
        vectorSet(&pm->pos, accelbPlayer.pos.x, accelbPlayer.pos.y + 3);
        vectorSet(&pm->vel, sqrt(difficulty) * 2, 0);
        pm->targetIsMissile = false;
        pm->targetIndex = ei;
        pm->ticks = 0;
        pm->exTicks = 0;
        pm->smokeTicks = 0;
        pm->isAlive = true;
        accelbPlayerMissileIndex = cgl_wrap(accelbPlayerMissileIndex + 1, 0, ACCELB_MAX_PLAYER_MISSILE_COUNT);
      }
      FOR_EACH(accelbMissiles, mi) {
        ASSIGN_ARRAY_ITEM(accelbMissiles, mi, AccelbMissile, m);
        SKIP_IS_NOT_ALIVE(m);
        ASSIGN_ARRAY_ITEM(accelbPlayerMissiles, accelbPlayerMissileIndex, AccelbPlayerMissile, pm);
        vectorSet(&pm->pos, accelbPlayer.pos.x, accelbPlayer.pos.y + 3);
        vectorSet(&pm->vel, sqrt(difficulty) * 2, 0);
        pm->targetIsMissile = true;
        pm->targetIndex = mi;
        pm->ticks = 0;
        pm->exTicks = 0;
        pm->smokeTicks = 0;
        pm->isAlive = true;
        accelbPlayerMissileIndex = cgl_wrap(accelbPlayerMissileIndex + 1, 0, ACCELB_MAX_PLAYER_MISSILE_COUNT);
      }
    }
  }

  FOR_EACH(accelbPlayerMissiles, pmi) {
    ASSIGN_ARRAY_ITEM(accelbPlayerMissiles, pmi, AccelbPlayerMissile, pm);
    SKIP_IS_NOT_ALIVE(pm);
    vectorAdd(&pm->pos, pm->vel.x, pm->vel.y);
    pm->pos.x += sqrt(difficulty) - scr;

    if (pm->exTicks > 0) {
      if (pm->pos.y > 90) {
        vectorSet(&pm->vel, 0, 0);
      }
      pm->exTicks += sqrt(difficulty);
      vectorMul(&pm->vel, 0.9);
      color = RED;
      float esize = 3 + cos(pm->exTicks * 0.05) * 9;
      box(pm->pos.x, pm->pos.y, esize, esize, &scratch);
      if (pm->exTicks > 30) {
        pm->isAlive = false;
      }
      continue;
    }

    float targetX;
    float targetY;
    if (pm->targetIsMissile) {
      targetX = accelbMissiles[pm->targetIndex].pos.x;
      targetY = accelbMissiles[pm->targetIndex].pos.y;
    } else {
      targetX = accelbEnemies[pm->targetIndex].pos.x;
      targetY = accelbEnemies[pm->targetIndex].pos.y;
    }
    float d = distanceTo(&pm->pos, targetX, targetY);
    if (d < 9 || pm->pos.y > 95 || pm->ticks > 120) {
      play(POWER_UP);
      pm->exTicks = 1;
      float s = vectorLength(&pm->vel);
      vectorSet(&pm->vel, 0, 0);
      addWithAngle(&pm->vel, angleTo(&pm->pos, targetX, targetY), s);
    }
    float speedFactor;
    if (pm->ticks < 9) {
      speedFactor = 0.1;
    } else if (pm->ticks < 20) {
      speedFactor = 3;
    } else {
      speedFactor = 1;
    }
    float mv = (sqrt(difficulty) / sqrt(d + 9)) * speedFactor;
    addWithAngle(&pm->vel, angleTo(&pm->pos, targetX, targetY), mv);
    if (pm->ticks < 20) {
      vectorMul(&pm->vel, 0.7);
    } else {
      vectorMul(&pm->vel, 0.95);
    }
    pm->ticks += sqrt(difficulty);
    color = CYAN;
    thickness = 2;
    barCenterPosRatio = 0.5;
    bar(pm->pos.x, pm->pos.y, 3, vectorAngle(&pm->vel), &scratch);
    pm->smokeTicks += sqrt(difficulty);
    if (pm->smokeTicks > 5) {
      ASSIGN_ARRAY_ITEM(accelbSmokes, accelbSmokeIndex, AccelbSmoke, ns);
      vectorSet(&ns->pos, pm->pos.x, pm->pos.y);
      vectorSet(&ns->vel, pm->vel.x * 0.5, pm->vel.y * 0.5);
      ns->ticks = 0;
      ns->isEnemy = false;
      ns->isAlive = true;
      accelbSmokeIndex = cgl_wrap(accelbSmokeIndex + 1, 0, ACCELB_MAX_SMOKE_COUNT);
      pm->smokeTicks -= 5;
    }
  }
  COUNT_IS_ALIVE(accelbPlayerMissiles, playerMissileCountAfter);
  if (playerMissileCountBefore > 0 && playerMissileCountAfter == 0) {
    play(COIN);
  }

  accelbNextEnemyDist -= scr;
  if (accelbNextEnemyDist < 0) {
    ASSIGN_ARRAY_ITEM(accelbEnemies, accelbEnemyIndex, AccelbEnemy, ne);
    float ey;
    if (rnd(0, 1) < 0.5) {
      ey = rnd(5, 35);
    } else {
      ey = rnd(55, 85);
    }
    vectorSet(&ne->pos, 203, ey);
    ne->vx = rnd(0, sqrt(difficulty)) * 0.5;
    ne->ma = CGLP_PI + rnd(0, CGLP_PI / 5) * RNDPM();
    ne->fireTicks = ceil(rnd(10, 30) / sqrt(difficulty));
    ne->isAlive = true;
    accelbEnemyIndex = cgl_wrap(accelbEnemyIndex + 1, 0, ACCELB_MAX_ENEMY_COUNT);
    accelbNextEnemyDist += rnd(40, 60) / difficulty;
  }

  FOR_EACH(accelbEnemies, ei2) {
    ASSIGN_ARRAY_ITEM(accelbEnemies, ei2, AccelbEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.x -= e->vx + scr;
    e->fireTicks--;
    if (e->fireTicks == 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(accelbMissiles, accelbMissileIndex, AccelbMissile, nm);
      vectorSet(&nm->pos, e->pos.x, e->pos.y + 3);
      nm->angle = e->ma;
      nm->va = 0;
      nm->speed = sqrt(difficulty);
      nm->smokeTicks = 0;
      nm->isAlive = true;
      accelbMissileIndex = cgl_wrap(accelbMissileIndex + 1, 0, ACCELB_MAX_MISSILE_COUNT);
    }
    color = PURPLE;
    Collision ec;
    character("b", e->pos.x, e->pos.y, &ec);
    if (ec.isColliding.rect[RED]) {
      play(EXPLOSION);
      particle(e->pos.x, e->pos.y, 15, 2, 0, CGLP_PI * 2);
      addScore(accelbMultiplier, e->pos.x, e->pos.y);
      if (accelbMultiplier < 64) {
        accelbMultiplier *= 2;
      }
      e->isAlive = false;
      continue;
    }
    if (e->fireTicks > 0) {
      color = BLACK;
      thickness = 3;
      barCenterPosRatio = 0.5;
      bar(e->pos.x, e->pos.y + 3, 3, e->ma, &scratch);
    }
    if (e->pos.x < -3) {
      e->isAlive = false;
      continue;
    }
  }

  float vva = sqrt(difficulty) * 0.0005;
  FOR_EACH(accelbMissiles, mi2) {
    ASSIGN_ARRAY_ITEM(accelbMissiles, mi2, AccelbMissile, m);
    SKIP_IS_NOT_ALIVE(m);
    addWithAngle(&m->pos, m->angle, m->speed);
    m->pos.x -= scr;
    float ta = angleTo(&m->pos, accelbPlayer.pos.x, accelbPlayer.pos.y);
    float oy = cgl_wrap(ta - m->angle, -CGLP_PI, CGLP_PI);
    if (oy > 0) {
      m->va += vva;
    } else {
      m->va -= vva;
    }
    m->angle = clamp(cgl_wrap(m->angle + m->va, 0, CGLP_PI * 2), (CGLP_PI / 4) * 3, (CGLP_PI / 4) * 5);
    color = BLACK;
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision c;
    bar(m->pos.x, m->pos.y, 3, m->angle, &c);
    m->smokeTicks += sqrt(difficulty);
    if (m->smokeTicks > 9) {
      ASSIGN_ARRAY_ITEM(accelbSmokes, accelbSmokeIndex, AccelbSmoke, ns2);
      vectorSet(&ns2->pos, m->pos.x, m->pos.y);
      Vector smokeVel;
      vectorSet(&smokeVel, 0, 0);
      addWithAngle(&smokeVel, m->angle, m->speed * 0.3);
      ns2->vel = smokeVel;
      ns2->ticks = 0;
      ns2->isEnemy = true;
      ns2->isAlive = true;
      accelbSmokeIndex = cgl_wrap(accelbSmokeIndex + 1, 0, ACCELB_MAX_SMOKE_COUNT);
      m->smokeTicks -= 9;
    }
    color = RED;
    if (c.isColliding.rect[RED]) {
      play(HIT);
      particle(m->pos.x, m->pos.y, 9, 2, 0, CGLP_PI * 2);
      addScore(accelbMultiplier, m->pos.x, m->pos.y);
      if (accelbMultiplier < 64) {
        accelbMultiplier *= 2;
      }
      m->isAlive = false;
      continue;
    } else if (c.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
    if (m->pos.y > 90) {
      particle(m->pos.x, m->pos.y, 16, 1, 0, CGLP_PI * 2);
      m->isAlive = false;
      continue;
    }
    if (m->pos.x < -3) {
      m->isAlive = false;
      continue;
    }
  }
}

void addGameAccelb() {
  addGame(accelbTitle, accelbDescription, accelbCharacters, accelbCharactersCount,
          &accelbOptions, false, &accelbUpdate);
}
