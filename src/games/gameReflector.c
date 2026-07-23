#include "../cglp.h"

char* reflectorTitle = "REFLECTOR";
char* reflectorDescription = "[Tap]\n Turn\n[Hold]\n Enforce\n reflector";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] reflectorCharacters = {
    {
        "      ",
        "y cc y",
        " cccc ",
        "llllll",
        "lllll ",
        " l l  ",
    },
    {
        "      ",
        "y cc y",
        " cccc ",
        "llllll",
        " lllll",
        "  l l ",
    },
    {
        "      ",
        " rrr  ",
        "rrrrr ",
        "rrrrrr",
        "lwlwll",
        " lwll ",
    },
    {
        "      ",
        " rrr  ",
        "rrrrr ",
        "rrrrrr",
        "llwlwl",
        " llwl ",
    }};
int reflectorCharactersCount = 4;

Options reflectorOptions = {100, 100, 1, false};

struct ReflectorUfo {
  Vector pos;
  float vx;
  float angle;
  float power;
};
ReflectorUfo reflectorUfo;
struct ReflectorTank {
  Vector pos;
  float vx;
  float angle;
  float angleVel;
  float speed;
  float fireTicks;
  float fireInterval;
  float fireSpeed;
  bool isAlive;
};
#define REFLECTOR_MAX_TANK_COUNT 32
ReflectorTank[REFLECTOR_MAX_TANK_COUNT] reflectorTanks;
int reflectorTankIndex;
float reflectorNextTankTicks;
struct ReflectorBullet {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define REFLECTOR_MAX_BULLET_COUNT 32
ReflectorBullet[REFLECTOR_MAX_BULLET_COUNT] reflectorBullets;
int reflectorBulletIndex;
struct ReflectorExplosion {
  Vector pos;
  float radius;
  float ticks;
  float duration;
  bool isAlive;
};
#define REFLECTOR_MAX_EXPLOSION_COUNT 8
ReflectorExplosion[REFLECTOR_MAX_EXPLOSION_COUNT] reflectorExplosions;
int reflectorExplosionIndex;
float reflectorMultiplier;

void reflectorUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&reflectorUfo.pos, 40, 9);
    reflectorUfo.vx = 1;
    reflectorUfo.angle = 0;
    reflectorUfo.power = 0;
    INIT_UNALIVED_ARRAY_FAST(reflectorTanks);
    reflectorTankIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(reflectorBullets);
    reflectorBulletIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(reflectorExplosions);
    reflectorExplosionIndex = 0;
    reflectorMultiplier = 1;
  }
  color = LIGHT_BLACK;
  rect(0, 90, 100, 10, &scratch);
  if (input.isJustPressed || (reflectorUfo.pos.x < 3 && reflectorUfo.vx < 0) ||
      (reflectorUfo.pos.x > 97 && reflectorUfo.vx > 0)) {
    play(SELECT);
    reflectorUfo.vx *= -1;
  }
  if (input.isPressed) {
    reflectorUfo.power += (1 - reflectorUfo.power) * 0.05;
  } else {
    reflectorUfo.power *= 0.9;
  }
  float uSpeedFactor;
  if (input.isPressed) {
    uSpeedFactor = 0.5;
  } else {
    uSpeedFactor = 1;
  }
  reflectorUfo.pos.x += reflectorUfo.vx * sqrt(difficulty) * uSpeedFactor;
  if (!input.isPressed) {
    reflectorUfo.angle = clamp(reflectorUfo.angle - reflectorUfo.vx * sqrt(difficulty) * 0.07, -M_PI / 4,
                      M_PI / 4);
  }
  color = BLACK;
  int[2] uc;
  uc[0] = 'a' + (int)(ticks / 15) % 2;
  uc[1] = 0;
  character(uc, reflectorUfo.pos.x, reflectorUfo.pos.y, &scratch);
  color = BLUE;
  Vector up;
  addWithAngle(vectorSet(&up, reflectorUfo.pos.x, reflectorUfo.pos.y), reflectorUfo.angle + M_PI / 2, 6);
  thickness = 3 + reflectorUfo.power * 3;
  bar(up.x, up.y, 9 - reflectorUfo.power * 9, reflectorUfo.angle, &scratch);
  thickness = 3;
  COUNT_IS_ALIVE(reflectorTanks, tc);
  if (tc == 0) {
    reflectorNextTankTicks = 0;
  }
  reflectorNextTankTicks--;
  if (reflectorNextTankTicks < 0) {
    ASSIGN_ARRAY_ITEM(reflectorTanks, reflectorTankIndex, ReflectorTank, t);
    float vx = RNDPM();
    float fireInterval = rnd(200, 300) / difficulty;
    float av = rnd(1, 5);
    float startX;
    if (vx > 0) {
      startX = -3;
    } else {
      startX = 103;
    }
    vectorSet(&t->pos, startX, 87);
    t->vx = vx;
    t->angle = -M_PI / 2;
    t->angleVel = av * 0.002;
    t->speed = 0.1 / sqrt(av) * sqrt(difficulty);
    t->fireTicks = t->fireInterval = fireInterval;
    t->fireSpeed = rnd(1, 1.5) * sqrt(difficulty);
    t->isAlive = true;
    reflectorNextTankTicks = rnd(60, 80) / sqrt(difficulty);
    reflectorTankIndex = cgl_wrap(reflectorTankIndex + 1, 0, REFLECTOR_MAX_TANK_COUNT);
  }
  color = LIGHT_RED;
  COUNT_IS_ALIVE(reflectorExplosions, ec);
  if (ec == 0) {
    reflectorMultiplier = 1;
  }
  FOR_EACH(reflectorExplosions, i) {
    ASSIGN_ARRAY_ITEM(reflectorExplosions, i, ReflectorExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    e->ticks--;
    float r = e->radius * sin(e->ticks / e->duration * M_PI);
    if (r < 0) {
      // ticks has just gone negative (the isAlive check below hasn't
      // caught it yet this frame - that's next frame's job), pushing
      // sin() just past zero into slightly-negative territory. arc()
      // guards against a negative radius too now, but there's no
      // reason to ever compute one here in the first place.
      r = 0;
    }
    arc(e->pos.x, e->pos.y, r, 0, M_PI * 2, &scratch);
    e->isAlive = e->ticks >= 0;
  }
  color = BLACK;
  thickness = 2;
  barCenterPosRatio = -0.5;
  FOR_EACH(reflectorTanks, i) {
    ASSIGN_ARRAY_ITEM(reflectorTanks, i, ReflectorTank, t);
    SKIP_IS_NOT_ALIVE(t);
    t->pos.x += t->vx * t->speed;
    float ta = angleTo(&t->pos, reflectorUfo.pos.x, reflectorUfo.pos.y);
    if (fabs(ta) < t->angleVel) {
      t->angle = ta;
    } else if (ta < t->angle) {
      t->angle -= t->angleVel;
    } else {
      t->angle += t->angleVel;
    }
    bar(t->pos.x, t->pos.y, 3, t->angle, &scratch);
    int[2] tankChar;
    tankChar[0] = 'c' + (int)(ticks / 25) % 2;
    tankChar[1] = 0;
    characterOptions.isMirrorX = t->vx < 0;
    Collision tcl;
    character(tankChar, t->pos.x, t->pos.y, &tcl);
    if (tcl.isColliding.rect[LIGHT_RED]) {
      play(POWER_UP);
      addScore(reflectorMultiplier, t->pos.x, t->pos.y);
      reflectorMultiplier++;
      particle(t->pos.x, t->pos.y, 16, 1, 0, M_PI * 2);
      t->isAlive = false;
      continue;
    }
    t->fireTicks--;
    if (t->fireTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(reflectorBullets, reflectorBulletIndex, ReflectorBullet, b);
      vectorSet(&b->pos, t->pos.x, t->pos.y);
      rotate(vectorSet(&b->vel, t->fireSpeed, 0), t->angle);
      b->isAlive = true;
      reflectorBulletIndex = cgl_wrap(reflectorBulletIndex + 1, 0, REFLECTOR_MAX_BULLET_COUNT);
      t->fireTicks = t->fireInterval;
    }
    t->isAlive = !(t->pos.x < -3 || t->pos.x > 103);
  }
  thickness = 3;
  barCenterPosRatio = 0.5;
  FOR_EACH(reflectorBullets, i) {
    ASSIGN_ARRAY_ITEM(reflectorBullets, i, ReflectorBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    Collision bcl;
    bar(b->pos.x, b->pos.y, 4, vectorAngle(&b->vel), &bcl);
    Collisions c = bcl.isColliding;
    if (b->vel.y < 0) {
      if (c.rect[BLUE]) {
        play(COIN);
        particle(b->pos.x, b->pos.y, 9, 1 + reflectorUfo.power * 2, reflectorUfo.angle + M_PI / 2,
                 M_PI / 8);
        float ra = vectorAngle(&b->vel) - reflectorUfo.angle;
        float a = vectorAngle(&b->vel);
        float s = vectorLength(&b->vel) * (1 + reflectorUfo.power * 4);
        addWithAngle(vectorSet(&b->vel, 0, 0), a - ra * 2, s);
        addWithAngle(&b->pos, a - ra * 2, s * 2);
        if (b->pos.y < 20) {
          b->pos.y = 20;
        }
      } else if (c.character['a'] || c.character['b']) {
        play(RANDOM);
        gameOver();
      }
    }
    if (b->vel.y > 0 &&
        (c.character['c'] || c.character['d'] || c.rect[LIGHT_BLACK])) {
      play(EXPLOSION);
      float s = vectorLength(&b->vel) / sqrt(difficulty);
      float radius = s * s;
      float duration = sqrt(radius) * 9;
      ASSIGN_ARRAY_ITEM(reflectorExplosions, reflectorExplosionIndex, ReflectorExplosion, e);
      vectorSet(&e->pos, b->pos.x, b->pos.y);
      e->radius = radius;
      e->ticks = e->duration = duration;
      e->isAlive = true;
      reflectorExplosionIndex =
          cgl_wrap(reflectorExplosionIndex + 1, 0, REFLECTOR_MAX_EXPLOSION_COUNT);
      b->isAlive = false;
      continue;
    }
    b->isAlive =
        b->pos.x > -3 && b->pos.x < 106 && b->pos.y > -3 && b->pos.y < 106;
  }
}

void addGameReflector() {
  addGame(reflectorTitle, reflectorDescription, reflectorCharacters,
          reflectorCharactersCount, &reflectorOptions, false, &reflectorUpdate);
}
