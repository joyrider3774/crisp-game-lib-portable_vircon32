#include "../cglp.h"

int* rockettrailTitle = "ROCKET TRAIL";
int* rockettrailDescription = "[Hold]\n Thrust & destroy";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rockettrailCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int rockettrailCharactersCount = 1;

Options rockettrailOptions = {100, 100, 1, true};

struct RockettrailRocket {
  Vector pos;
  Vector vel;
  float thrustPower;
  float horizontalDir;
  float angle;
};
RockettrailRocket rockettrailRocket;

struct RockettrailStar {
  Vector pos;
  Vector vel;
  float size;
};
#define ROCKETTRAIL_STAR_COUNT 50
RockettrailStar[ROCKETTRAIL_STAR_COUNT] rockettrailStars;

struct RockettrailFlame {
  Vector pos;
  Vector vel;
  int age;
  bool isAlive;
};
#define ROCKETTRAIL_MAX_FLAME_COUNT 64
RockettrailFlame[ROCKETTRAIL_MAX_FLAME_COUNT] rockettrailFlames;
int rockettrailFlameIndex;

struct RockettrailFuel {
  Vector pos;
  Vector vel;
  int life;
  bool isAlive;
};
#define ROCKETTRAIL_MAX_FUEL_COUNT 64
RockettrailFuel[ROCKETTRAIL_MAX_FUEL_COUNT] rockettrailFuels;
int rockettrailFuelIndex;

struct RockettrailAsteroid {
  Vector pos;
  Vector vel;
  bool isAlive;
};
// Sized well beyond the ~15-20 typical count since spawn interval can go negative at high difficulty.
#define ROCKETTRAIL_MAX_ASTEROID_COUNT 256
RockettrailAsteroid[ROCKETTRAIL_MAX_ASTEROID_COUNT] rockettrailAsteroids;
int rockettrailAsteroidIndex;

float rockettrailAsteroidSpawnTimer;
float rockettrailAsteroidSpawnInterval;
int rockettrailMultiplier;

void rockettrailUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&rockettrailRocket.pos, 50, 65);
    vectorSet(&rockettrailRocket.vel, 0, 0);
    rockettrailRocket.thrustPower = 0.3;
    rockettrailRocket.horizontalDir = 1;
    rockettrailRocket.angle = CGLP_PI_2;
    INIT_UNALIVED_ARRAY_FAST(rockettrailAsteroids);
    rockettrailAsteroidIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(rockettrailFuels);
    rockettrailFuelIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(rockettrailFlames);
    rockettrailFlameIndex = 0;
    rockettrailAsteroidSpawnTimer = 0;
    rockettrailAsteroidSpawnInterval = 60;
    rockettrailMultiplier = 1;
    TIMES(ROCKETTRAIL_STAR_COUNT, sti) {
      RockettrailStar* s = &rockettrailStars[sti];
      vectorSet(&s->pos, rnd(0, 100), rnd(0, 100));
      vectorSet(&s->vel, 0, rnd(0.5, 2));
      s->size = rnd(0.5, 2);
    }
  }

  rockettrailRocket.vel.x *= 0.95;
  rockettrailRocket.vel.y += 0.1;
  rockettrailRocket.vel.y *= 0.98;

  if (rockettrailRocket.pos.x <= 45) {
    rockettrailRocket.horizontalDir = 1;
  } else if (rockettrailRocket.pos.x >= 55) {
    rockettrailRocket.horizontalDir = -1;
  }
  rockettrailRocket.angle +=
      (CGLP_PI_2 + rockettrailRocket.horizontalDir - rockettrailRocket.angle) * 0.05;

  if (input.isPressed) {
    addWithAngle(&rockettrailRocket.vel, rockettrailRocket.angle, -rockettrailRocket.thrustPower);
    if (rnd(0, 1) < 0.3) {
      ASSIGN_ARRAY_ITEM(rockettrailFuels, rockettrailFuelIndex, RockettrailFuel, nf);
      vectorSet(&nf->pos, rockettrailRocket.pos.x, rockettrailRocket.pos.y);
      vectorSet(&nf->vel, 0, 0);
      addWithAngle(&nf->vel, rockettrailRocket.angle + rnd(0, 0.5) * RNDPM(),
                   rockettrailRocket.thrustPower * 6);
      nf->life = 20;
      nf->isAlive = true;
      rockettrailFuelIndex = cgl_wrap(rockettrailFuelIndex + 1, 0, ROCKETTRAIL_MAX_FUEL_COUNT);
    }
  }
  if (input.isJustPressed) {
    play(EXPLOSION);
  }

  vectorAdd(&rockettrailRocket.pos, rockettrailRocket.vel.x, rockettrailRocket.vel.y);

  color = BLACK;
  TIMES(ROCKETTRAIL_STAR_COUNT, sti2) {
    RockettrailStar* s = &rockettrailStars[sti2];
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    box(s->pos.x, s->pos.y, s->size, s->size, &scratch);
    if (s->pos.y > 100) {
      s->pos.y = -5;
      s->pos.x = rnd(0, 100);
      s->vel.y = rnd(0.5, 2);
      s->size = rnd(0.5, 2);
    }
  }

  ASSIGN_ARRAY_ITEM(rockettrailFlames, rockettrailFlameIndex, RockettrailFlame, newFlame);
  vectorSet(&newFlame->pos, rockettrailRocket.pos.x, rockettrailRocket.pos.y);
  vectorSet(&newFlame->vel, 0, 0);
  addWithAngle(&newFlame->vel, rockettrailRocket.angle, rockettrailRocket.thrustPower * 7);
  newFlame->age = 0;
  newFlame->isAlive = true;
  rockettrailFlameIndex = cgl_wrap(rockettrailFlameIndex + 1, 0, ROCKETTRAIL_MAX_FLAME_COUNT);

  rockettrailRocket.pos.x = clamp(rockettrailRocket.pos.x, 1, 99);
  if (rockettrailRocket.pos.y < 0 || rockettrailRocket.pos.y > 105) {
    play(EXPLOSION);
    gameOver();
  }

  color = RED;
  thickness = 3;
  barCenterPosRatio = 1;
  bar(rockettrailRocket.pos.x, rockettrailRocket.pos.y, 5, rockettrailRocket.angle, &scratch);
  barCenterPosRatio = 0.5;

  color = YELLOW;
  FOR_EACH(rockettrailFlames, fi) {
    ASSIGN_ARRAY_ITEM(rockettrailFlames, fi, RockettrailFlame, f);
    SKIP_IS_NOT_ALIVE(f);
    f->age++;
    vectorAdd(&f->pos, f->vel.x, f->vel.y);
    if (f->age < 60) {
      float sz = 4 - ((float)f->age / 60) * 3;
      box(f->pos.x, f->pos.y, sz, sz, &scratch);
    }
    if (f->age >= 25) {
      f->isAlive = false;
      continue;
    }
  }

  FOR_EACH(rockettrailFuels, fuelI) {
    ASSIGN_ARRAY_ITEM(rockettrailFuels, fuelI, RockettrailFuel, p);
    SKIP_IS_NOT_ALIVE(p);
    vectorAdd(&p->pos, p->vel.x, p->vel.y);
    p->life--;
    if (p->life > 0) {
      box(p->pos.x, p->pos.y, 2, 2, &scratch);
    }
    if (p->life <= 0) {
      p->isAlive = false;
      continue;
    }
  }

  rockettrailAsteroidSpawnTimer++;
  COUNT_IS_ALIVE(rockettrailAsteroids, aliveAsteroidCount);
  if (aliveAsteroidCount == 0 || rockettrailAsteroidSpawnTimer >= rockettrailAsteroidSpawnInterval) {
    Vector apos;
    vectorSet(&apos, rnd(5, 95), -5);
    Vector avel;
    vectorSet(&avel, 0, 0);
    addWithAngle(&avel, angleTo(&apos, rnd(20, 80), 99), rnd(0.3, 0.5));
    ASSIGN_ARRAY_ITEM(rockettrailAsteroids, rockettrailAsteroidIndex, RockettrailAsteroid, na);
    na->pos = apos;
    na->vel = avel;
    na->isAlive = true;
    rockettrailAsteroidIndex = cgl_wrap(rockettrailAsteroidIndex + 1, 0, ROCKETTRAIL_MAX_ASTEROID_COUNT);
    rockettrailAsteroidSpawnTimer = 0;
    rockettrailAsteroidSpawnInterval = 80 - rnd(0, 30) * RNDPM() * difficulty;
  }

  color = BLACK;
  FOR_EACH(rockettrailAsteroids, ai) {
    ASSIGN_ARRAY_ITEM(rockettrailAsteroids, ai, RockettrailAsteroid, a);
    SKIP_IS_NOT_ALIVE(a);
    vectorAdd(&a->pos, a->vel.x, a->vel.y);
    box(a->pos.x, a->pos.y, 3, 3, &scratch);
    particle(a->pos.x, a->pos.y, 1, vectorLength(&a->vel), vectorAngle(&a->vel) + CGLP_PI, 1);

    if (scratch.isColliding.rect[YELLOW]) {
      addScore(rockettrailMultiplier, a->pos.x, a->pos.y);
      rockettrailMultiplier = (int)clamp(rockettrailMultiplier + 1, 1, 16);
      particle(a->pos.x, a->pos.y, 7, 2, 0, CGLP_PI * 2);
      play(EXPLOSION);
      a->isAlive = false;
      continue;
    }

    if (scratch.isColliding.rect[RED]) {
      rockettrailRocket.vel.y += 2;
      rockettrailMultiplier = 1;
      color = RED;
      particle(a->pos.x, a->pos.y, 25, 3, -CGLP_PI_2, 1);
      color = BLACK;
      play(EXPLOSION);
      a->isAlive = false;
      continue;
    }

    if (a->pos.y > 105) {
      rockettrailMultiplier = (int)clamp(rockettrailMultiplier - 1, 1, 16);
      a->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(rockettrailMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameRockettrail() {
  addGame(rockettrailTitle, rockettrailDescription, rockettrailCharacters,
          rockettrailCharactersCount, &rockettrailOptions, false, &rockettrailUpdate);
}
