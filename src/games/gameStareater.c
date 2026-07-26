#include "../cglp.h"

int* stareaterTitle = "STAR EATER";
int* stareaterDescription = "[Hold] Charge\n[Release] Blast";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] stareaterCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int stareaterCharactersCount = 0;

Options stareaterOptions = {100, 100, 0, false};

struct StareaterPlayer {
  Vector pos;
  float size;
  float baseSize;
  float breathPhase;
};
StareaterPlayer stareaterPlayer;

struct StareaterAsteroid {
  Vector pos;
  Vector vel;
  float size;
  float rotation;
  float rotSpeed;
  bool isAlive;
};
// Concurrent count stays ~self-similar across difficulty (spawn interval and
// speed both scale with difficulty in offsetting ways) - measured around
// 4-6 in practice, generous headroom applied.
#define STAREATER_MAX_ASTEROID_COUNT 32
StareaterAsteroid[STAREATER_MAX_ASTEROID_COUNT] stareaterAsteroids;
int stareaterAsteroidIndex;

struct StareaterTrail {
  float x;
  float y;
  float size;
  float alpha;
  bool isPlayer;
  bool isAlive;
};
#define STAREATER_MAX_TRAIL_COUNT 32
StareaterTrail[STAREATER_MAX_TRAIL_COUNT] stareaterTrails;
int stareaterTrailIndex;

struct StareaterDust {
  Vector pos;
  Vector vel;
  float life;
  int color;
  float size;
  bool isAlive;
};
// Sized for the worst case: a maxed-charge blast (radius ~63) can catch
// every concurrent asteroid (~6) at once (8 dust each), on top of the
// 16-particle blast burst and a few leftover charge particles.
#define STAREATER_MAX_DUST_COUNT 256
StareaterDust[STAREATER_MAX_DUST_COUNT] stareaterDust;
int stareaterDustIndex;

struct StareaterExplosion {
  bool active;
  Vector pos;
  float radius;
  int timer;
};
StareaterExplosion stareaterExplosion;

float stareaterNextAsteroidTicks;
float stareaterCharge;
int stareaterDestroyedCount;

void stareaterCheckExplosion(float x, float y, float radius, int timer) {
  Collision scratch;
  float phase = 1 - (float)timer / 12;
  float ringRadius = radius * (0.3 + phase * 0.7);

  color = LIGHT_YELLOW;
  thickness = 3 - (int)floor(phase * 2);
  arc(x, y, ringRadius, 0, CGLP_PI * 2, &scratch);

  if (timer > 6) {
    color = WHITE;
    thickness = 3;
    arc(x, y, ringRadius * 0.4, 0, CGLP_PI * 2, &scratch);
  }

  FOR_EACH(stareaterAsteroids, ai) {
    ASSIGN_ARRAY_ITEM(stareaterAsteroids, ai, StareaterAsteroid, ast);
    SKIP_IS_NOT_ALIVE(ast);
    float ddx = ast->pos.x - x;
    float ddy = ast->pos.y - y;
    float dist = sqrt(ddx * ddx + ddy * ddy);
    if (dist < radius + ast->size / 2) {
      int di;
      for (di = 0; di < 8; di++) {
        float angle = rnd(0, CGLP_PI * 2);
        float spd = rnd(1, 2.5);
        ASSIGN_ARRAY_ITEM(stareaterDust, stareaterDustIndex, StareaterDust, nd);
        vectorSet(&nd->pos, ast->pos.x, ast->pos.y);
        vectorSet(&nd->vel, cos(angle) * spd, sin(angle) * spd);
        nd->life = rnd(15, 25);
        if (rnd(0, 1) > 0.5) {
          nd->color = RED;
        } else {
          nd->color = LIGHT_RED;
        }
        nd->size = rnd(1, 3);
        nd->isAlive = true;
        stareaterDustIndex = cgl_wrap(stareaterDustIndex + 1, 0, STAREATER_MAX_DUST_COUNT);
      }
      stareaterDestroyedCount++;
      addScore(stareaterDestroyedCount * stareaterDestroyedCount, ast->pos.x, ast->pos.y);
      play(POWER_UP);
      ast->isAlive = false;
    }
  }
}

void stareaterUpdate() {
  Collision scratch;
  // Never reads a Collision result - blasts are decided by distance math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&stareaterPlayer.pos, 50, 50);
    stareaterPlayer.size = 5;
    stareaterPlayer.baseSize = 5;
    stareaterPlayer.breathPhase = 0;
    INIT_UNALIVED_ARRAY_FAST(stareaterAsteroids);
    stareaterAsteroidIndex = 0;
    stareaterNextAsteroidTicks = 0;
    stareaterCharge = 0;
    stareaterExplosion.active = false;
    vectorSet(&stareaterExplosion.pos, 0, 0);
    stareaterExplosion.radius = 0;
    stareaterExplosion.timer = 0;
    stareaterDestroyedCount = 0;
    INIT_UNALIVED_ARRAY_FAST(stareaterTrails);
    stareaterTrailIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(stareaterDust);
    stareaterDustIndex = 0;
  }

  FOR_EACH(stareaterTrails, tri) {
    ASSIGN_ARRAY_ITEM(stareaterTrails, tri, StareaterTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->alpha -= 0.08;
    if (t->alpha <= 0) {
      t->isAlive = false;
    }
  }

  FOR_EACH(stareaterDust, dsi) {
    ASSIGN_ARRAY_ITEM(stareaterDust, dsi, StareaterDust, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.x += p->vel.x;
    p->pos.y += p->vel.y;
    vectorMul(&p->vel, 0.95);
    p->life -= 1;
    if (p->life <= 0) {
      p->isAlive = false;
    }
  }

  stareaterNextAsteroidTicks--;
  if (stareaterNextAsteroidTicks <= 0) {
    int side = rndi(0, 4);
    Vector pos;
    Vector vel;
    if (side == 0) {
      vectorSet(&pos, rnd(0, 100), 0);
      vectorSet(&vel, rnd(-0.3, 0.3), 0.5 + rnd(0, difficulty * 0.2));
    } else if (side == 1) {
      vectorSet(&pos, rnd(0, 100), 100);
      vectorSet(&vel, rnd(-0.3, 0.3), -0.5 - rnd(0, difficulty * 0.2));
    } else if (side == 2) {
      vectorSet(&pos, 0, rnd(0, 100));
      vectorSet(&vel, 0.5 + rnd(0, difficulty * 0.2), rnd(-0.3, 0.3));
    } else {
      vectorSet(&pos, 100, rnd(0, 100));
      vectorSet(&vel, -0.5 - rnd(0, difficulty * 0.2), rnd(-0.3, 0.3));
    }
    float d1 = distanceTo(&pos, stareaterPlayer.pos.x, stareaterPlayer.pos.y);
    float d2 = distanceTo(&pos, stareaterPlayer.pos.x - 100, stareaterPlayer.pos.y);
    if (d1 > 50 && d2 > 50) {
      vectorMul(&vel, sqrt(difficulty));
      ASSIGN_ARRAY_ITEM(stareaterAsteroids, stareaterAsteroidIndex, StareaterAsteroid, na);
      na->pos = pos;
      na->vel = vel;
      na->size = rnd(5, 9);
      na->rotation = rnd(0, CGLP_PI * 2);
      na->rotSpeed = rnd(-0.1, 0.1);
      na->isAlive = true;
      stareaterAsteroidIndex = cgl_wrap(stareaterAsteroidIndex + 1, 0, STAREATER_MAX_ASTEROID_COUNT);
      stareaterNextAsteroidTicks = rnd(22, 55) / difficulty;
    }
  }

  float speed;
  if (input.isPressed) {
    speed = 1.8 * sqrt(difficulty);
  } else {
    speed = 1 * sqrt(difficulty);
  }
  stareaterPlayer.pos.x += 0.2 * speed;
  if (stareaterPlayer.pos.x > 100) {
    stareaterPlayer.pos.x = 0;
  }
  stareaterPlayer.pos.y += sin(ticks * 0.02 * speed) * 0.1;
  stareaterPlayer.pos.y = clamp(stareaterPlayer.pos.y, 5, 95);

  stareaterPlayer.breathPhase += 0.08;
  float breathScale = 1 + sin(stareaterPlayer.breathPhase) * 0.05;

  if (ticks % 3 == 0) {
    ASSIGN_ARRAY_ITEM(stareaterTrails, stareaterTrailIndex, StareaterTrail, nt);
    nt->x = stareaterPlayer.pos.x;
    nt->y = stareaterPlayer.pos.y;
    nt->size = stareaterPlayer.size * 0.7;
    nt->alpha = 0.5;
    nt->isPlayer = true;
    nt->isAlive = true;
    stareaterTrailIndex = cgl_wrap(stareaterTrailIndex + 1, 0, STAREATER_MAX_TRAIL_COUNT);
  }

  if (input.isPressed) {
    stareaterCharge = fmin(stareaterCharge + difficulty, 60);
    stareaterPlayer.size = stareaterPlayer.baseSize + stareaterCharge * 0.15;
    breathScale = 1 + sin(ticks * 0.3) * 0.1;

    if (ticks % 4 == 0) {
      float angle = rnd(0, CGLP_PI * 2);
      float dist = stareaterPlayer.size + 10 + rnd(0, 5);
      ASSIGN_ARRAY_ITEM(stareaterDust, stareaterDustIndex, StareaterDust, cd);
      vectorSet(&cd->pos, stareaterPlayer.pos.x + cos(angle) * dist, stareaterPlayer.pos.y + sin(angle) * dist);
      vectorSet(&cd->vel, cos(angle + CGLP_PI) * 0.8, sin(angle + CGLP_PI) * 0.8);
      cd->life = 15;
      cd->color = YELLOW;
      cd->size = rnd(1, 2);
      cd->isAlive = true;
      stareaterDustIndex = cgl_wrap(stareaterDustIndex + 1, 0, STAREATER_MAX_DUST_COUNT);
    }
  }

  if (input.isJustReleased && stareaterCharge > 10) {
    play(EXPLOSION);
    stareaterExplosion.active = true;
    vectorSet(&stareaterExplosion.pos, stareaterPlayer.pos.x, stareaterPlayer.pos.y);
    stareaterExplosion.radius = 15 + stareaterCharge * 0.8;
    stareaterExplosion.timer = 12;

    int bi;
    for (bi = 0; bi < 16; bi++) {
      float angle = (CGLP_PI * 2 * bi) / 16 + rnd(-0.2, 0.2);
      float spd = rnd(1.5, 3);
      ASSIGN_ARRAY_ITEM(stareaterDust, stareaterDustIndex, StareaterDust, bd);
      vectorSet(&bd->pos, stareaterPlayer.pos.x, stareaterPlayer.pos.y);
      vectorSet(&bd->vel, cos(angle) * spd, sin(angle) * spd);
      bd->life = rnd(20, 35);
      bd->color = LIGHT_YELLOW;
      bd->size = rnd(1, 3);
      bd->isAlive = true;
      stareaterDustIndex = cgl_wrap(stareaterDustIndex + 1, 0, STAREATER_MAX_DUST_COUNT);
    }

    stareaterCharge = 0;
    stareaterPlayer.size = stareaterPlayer.baseSize;
    stareaterDestroyedCount = 0;
  }

  if (!input.isPressed && stareaterCharge > 0) {
    stareaterCharge = fmax(0, stareaterCharge - 2);
    stareaterPlayer.size = stareaterPlayer.baseSize + stareaterCharge * 0.15;
  }

  if (stareaterExplosion.active) {
    stareaterExplosion.timer--;
    if (stareaterExplosion.timer <= 0) {
      stareaterExplosion.active = false;
    }
  }

  FOR_EACH(stareaterAsteroids, ai2) {
    ASSIGN_ARRAY_ITEM(stareaterAsteroids, ai2, StareaterAsteroid, ast2);
    SKIP_IS_NOT_ALIVE(ast2);
    ast2->pos.x += ast2->vel.x;
    ast2->pos.y += ast2->vel.y;
    ast2->rotation += ast2->rotSpeed;

    if (ticks % 5 == 0) {
      ASSIGN_ARRAY_ITEM(stareaterTrails, stareaterTrailIndex, StareaterTrail, at);
      at->x = ast2->pos.x;
      at->y = ast2->pos.y;
      at->size = ast2->size * 0.4;
      at->alpha = 0.3;
      at->isPlayer = false;
      at->isAlive = true;
      stareaterTrailIndex = cgl_wrap(stareaterTrailIndex + 1, 0, STAREATER_MAX_TRAIL_COUNT);
    }

    if (!(ast2->pos.x > -10 && ast2->pos.x < 110 && ast2->pos.y > -10 && ast2->pos.y < 110)) {
      ast2->isAlive = false;
    }
  }

  FOR_EACH(stareaterTrails, tri2) {
    ASSIGN_ARRAY_ITEM(stareaterTrails, tri2, StareaterTrail, t2);
    SKIP_IS_NOT_ALIVE(t2);
    if (t2->isPlayer) {
      color = LIGHT_CYAN;
    } else {
      color = LIGHT_RED;
    }
    int a = (int)floor(t2->alpha * 3);
    if (a > 0) {
      thickness = a;
      arc(t2->x, t2->y, t2->size, 0, CGLP_PI * 2, &scratch);
    }
  }

  FOR_EACH(stareaterDust, dsi2) {
    ASSIGN_ARRAY_ITEM(stareaterDust, dsi2, StareaterDust, p2);
    SKIP_IS_NOT_ALIVE(p2);
    color = p2->color;
    box(p2->pos.x, p2->pos.y, p2->size, p2->size, &scratch);
  }

  if (stareaterExplosion.active) {
    stareaterCheckExplosion(stareaterExplosion.pos.x, stareaterExplosion.pos.y,
                             stareaterExplosion.radius, stareaterExplosion.timer);
    stareaterCheckExplosion(stareaterExplosion.pos.x - 100, stareaterExplosion.pos.y,
                             stareaterExplosion.radius, stareaterExplosion.timer);
  }

  if (stareaterCharge > 0) {
    color = YELLOW;
    thickness = 1;
    float pulseSize = stareaterPlayer.size + 2 + sin(ticks * 0.4) * 1.5;
    arc(stareaterPlayer.pos.x, stareaterPlayer.pos.y, pulseSize, 0, CGLP_PI * 2, &scratch);
  }

  color = CYAN;
  thickness = 3;
  float drawSize = stareaterPlayer.size * breathScale;
  arc(stareaterPlayer.pos.x, stareaterPlayer.pos.y, drawSize, 0, CGLP_PI * 2, &scratch);

  if (stareaterCharge > 20) {
    color = LIGHT_CYAN;
    thickness = 3;
    arc(stareaterPlayer.pos.x, stareaterPlayer.pos.y, drawSize * 0.5, 0, CGLP_PI * 2, &scratch);
  }

  color = RED;
  FOR_EACH(stareaterAsteroids, ai3) {
    ASSIGN_ARRAY_ITEM(stareaterAsteroids, ai3, StareaterAsteroid, ast3);
    SKIP_IS_NOT_ALIVE(ast3);
    float len = ast3->size * 0.8;
    thickness = ast3->size * 0.6;
    bar(ast3->pos.x, ast3->pos.y, len, ast3->rotation, &scratch);
    thickness = ast3->size * 0.6;
    bar(ast3->pos.x, ast3->pos.y, len, ast3->rotation + CGLP_PI / 2, &scratch);

    float dist = distanceTo(&stareaterPlayer.pos, ast3->pos.x, ast3->pos.y);
    if (dist < stareaterPlayer.size + ast3->size / 2) {
      int di2;
      for (di2 = 0; di2 < 12; di2++) {
        float angle = (CGLP_PI * 2 * di2) / 12;
        ASSIGN_ARRAY_ITEM(stareaterDust, stareaterDustIndex, StareaterDust, hd);
        vectorSet(&hd->pos, stareaterPlayer.pos.x, stareaterPlayer.pos.y);
        vectorSet(&hd->vel, cos(angle) * 2, sin(angle) * 2);
        hd->life = 20;
        hd->color = CYAN;
        hd->size = 2;
        hd->isAlive = true;
        stareaterDustIndex = cgl_wrap(stareaterDustIndex + 1, 0, STAREATER_MAX_DUST_COUNT);
      }
      play(HIT);
      gameOver();
    }
  }
}

void addGameStareater() {
  addGame(stareaterTitle, stareaterDescription, stareaterCharacters,
          stareaterCharactersCount, &stareaterOptions, false, &stareaterUpdate);
}
