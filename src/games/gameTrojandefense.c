#include "../cglp.h"

int* trojandefenseTitle = "TROJAN DEFENSE";
int* trojandefenseDescription = "[Tap]\n Change Direction\n[Hold]\n Extend Shield";

// Character 5 ('e') is a placeholder: upstream references it via addWithCharCode but never defines it.
int[5][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] trojandefenseCharacters = {
    {
        "   ll ",
        "   l  ",
        " lLll ",
        "llllll",
        "l    l",
        "l    l",
    },
    {
        "   ll ",
        "   l  ",
        " lLll ",
        "llllll",
        "l    l",
        "l  l  ",
    },
    {
        "  rrr ",
        " rrrrr",
        "rrrrrr",
        " rrrrr",
        "  rrr ",
        "   r  ",
    },
    {
        "  ll  ",
        " llll ",
        "llllll",
        "llllll",
        " llll ",
        "  ll  ",
    },
    {
        "      ",
        " rrrr ",
        "rrrrrr",
        "rrrrrr",
        " rrrr ",
        "      ",
    },
};
int trojandefenseCharactersCount = 5;

Options trojandefenseOptions = {100, 100, 7, true};

struct TrojandefenseHorse {
  Vector pos;
  float radius;
};
TrojandefenseHorse trojandefenseHorse;

struct TrojandefenseShield {
  float angle;
  float length;
  float arcAngle;
  float rotationSpeed;
  int rotationDirection;
  bool extending;
};
TrojandefenseShield trojandefenseShield;

struct TrojandefenseEnemy {
  Vector pos;
  int type; // 0, 1, or 2 - selects sprite 'c'/'d'/'e' and fire stats
  float fireRate;
  float nextFire;
  float fireSpeed;
  bool isAlive;
};
// Enemies only die by deflecting one of their OWN shots back with the
// shield (no offscreen/timeout expiry), and new waves spawn on their own
// timer (rnd(180,300)/sqrt(difficulty), shrinking) regardless of whether
// the previous wave was cleared - a player who can't out-deflect the
// spawn rate keeps accumulating enemies across waves, so sized with real
// headroom above the original 64.
#define TROJANDEFENSE_MAX_ENEMY_COUNT 256
TrojandefenseEnemy[TROJANDEFENSE_MAX_ENEMY_COUNT] trojandefenseEnemies;
int trojandefenseEnemyIndex;

struct TrojandefenseProjectile {
  Vector pos;
  Vector vel;
  bool isDeflected;
  int deflectedTicks;
  bool isAlive;
};
// Scales with the enemy count above - more surviving enemies each firing
// periodically means more concurrent projectiles too.
#define TROJANDEFENSE_MAX_PROJECTILE_COUNT 384
TrojandefenseProjectile[TROJANDEFENSE_MAX_PROJECTILE_COUNT] trojandefenseProjectiles;
int trojandefenseProjectileIndex;

struct TrojandefenseExplosion {
  Vector pos;
  float radius;
  int duration;
  int ticks;
  bool isAlive;
};
#define TROJANDEFENSE_MAX_EXPLOSION_COUNT 32
TrojandefenseExplosion[TROJANDEFENSE_MAX_EXPLOSION_COUNT] trojandefenseExplosions;
int trojandefenseExplosionIndex;

int trojandefenseMultiplier;
int trojandefenseWaveCount;
float trojandefenseNextEnemyTicks;

void trojandefenseUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&trojandefenseHorse.pos, 50, 50);
    trojandefenseHorse.radius = 5;
    trojandefenseShield.angle = 0;
    trojandefenseShield.length = 10;
    trojandefenseShield.arcAngle = CGLP_PI / 8;
    trojandefenseShield.rotationSpeed = 0.06;
    trojandefenseShield.rotationDirection = 1;
    trojandefenseShield.extending = false;
    INIT_UNALIVED_ARRAY_FAST(trojandefenseEnemies);
    trojandefenseEnemyIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(trojandefenseProjectiles);
    trojandefenseProjectileIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(trojandefenseExplosions);
    trojandefenseExplosionIndex = 0;
    trojandefenseMultiplier = 1;
    trojandefenseWaveCount = 0;
    trojandefenseNextEnemyTicks = 60;
  }

  color = LIGHT_BLACK;
  thickness = 2;
  arc(trojandefenseHorse.pos.x, trojandefenseHorse.pos.y, 60, 0, CGLP_PI * 2, &scratch);

  if (input.isJustPressed) {
    play(SELECT);
    trojandefenseShield.rotationDirection *= -1;
    trojandefenseMultiplier = 1;
  }
  if (input.isPressed) {
    trojandefenseShield.extending = true;
    if (trojandefenseShield.arcAngle < CGLP_PI / 3) {
      trojandefenseShield.arcAngle += 0.02;
    }
    if (trojandefenseShield.length < 20) {
      trojandefenseShield.length += 0.5;
    }
  } else {
    trojandefenseShield.extending = false;
    if (trojandefenseShield.arcAngle > CGLP_PI / 8) {
      trojandefenseShield.arcAngle -= 0.015;
    }
    if (trojandefenseShield.length > 10) {
      trojandefenseShield.length -= 0.3;
    }
  }
  float speedFactor;
  if (trojandefenseShield.extending) {
    speedFactor = 1 - (trojandefenseShield.arcAngle - CGLP_PI / 8) / (CGLP_PI / 3 - CGLP_PI / 8) * 0.7;
  } else {
    speedFactor = 1.2;
  }
  trojandefenseShield.angle += trojandefenseShield.rotationDirection * trojandefenseShield.rotationSpeed * speedFactor * difficulty;

  color = BLACK;
  if (trojandefenseShield.extending) {
    character("b", trojandefenseHorse.pos.x, trojandefenseHorse.pos.y, &scratch);
  } else {
    character("a", trojandefenseHorse.pos.x, trojandefenseHorse.pos.y, &scratch);
  }

  color = CYAN;
  float shieldStartAngle = trojandefenseShield.angle - trojandefenseShield.arcAngle / 2;
  float shieldEndAngle = trojandefenseShield.angle + trojandefenseShield.arcAngle / 2;
  thickness = 1;
  for (float r = trojandefenseHorse.radius; r <= trojandefenseHorse.radius + trojandefenseShield.length; r += 1.5) {
    arc(trojandefenseHorse.pos.x, trojandefenseHorse.pos.y, r, shieldStartAngle, shieldEndAngle, &scratch);
  }

  trojandefenseNextEnemyTicks--;
  if (trojandefenseNextEnemyTicks < 0) {
    trojandefenseWaveCount++;
    int enemyCount = (int)floor(1 + trojandefenseWaveCount / 3.0);
    if (enemyCount > 5) {
      enemyCount = 5;
    }
    float baseAngle = rnd(0, CGLP_PI * 2);
    TIMES(enemyCount, wi) {
      float angleVariation = rnd(-CGLP_PI / 4, CGLP_PI / 4);
      float angle = baseAngle + angleVariation;
      float edistance = 50;
      Vector enemyPos;
      vectorSet(&enemyPos, trojandefenseHorse.pos.x + cos(angle) * edistance,
                trojandefenseHorse.pos.y + sin(angle) * edistance);
      int enemyType;
      if (rnd(0, 1) < 0.7) {
        enemyType = 0;
      } else if (rnd(0, 1) < 0.5) {
        enemyType = 1;
      } else {
        enemyType = 2;
      }
      float fireRate;
      float fireSpeed;
      if (enemyType == 0) {
        fireRate = rnd(120, 180) / sqrt(difficulty);
        fireSpeed = 0.5 * sqrt(difficulty);
      } else if (enemyType == 1) {
        fireRate = rnd(80, 120) / sqrt(difficulty);
        fireSpeed = 0.8 * sqrt(difficulty);
      } else {
        fireRate = rnd(150, 200) / sqrt(difficulty);
        fireSpeed = 0.4 * sqrt(difficulty);
      }
      ASSIGN_ARRAY_ITEM(trojandefenseEnemies, trojandefenseEnemyIndex, TrojandefenseEnemy, ne);
      ne->pos = enemyPos;
      ne->type = enemyType;
      ne->fireRate = fireRate;
      ne->nextFire = rnd(30, 60);
      ne->fireSpeed = fireSpeed;
      ne->isAlive = true;
      trojandefenseEnemyIndex = cgl_wrap(trojandefenseEnemyIndex + 1, 0, TROJANDEFENSE_MAX_ENEMY_COUNT);
    }
    trojandefenseNextEnemyTicks = rnd(180, 300) / sqrt(difficulty);
  }

  color = LIGHT_RED;
  FOR_EACH(trojandefenseExplosions, exi) {
    ASSIGN_ARRAY_ITEM(trojandefenseExplosions, exi, TrojandefenseExplosion, ex);
    SKIP_IS_NOT_ALIVE(ex);
    ex->ticks--;
    float radiusRatio = (float)ex->ticks / (float)ex->duration;
    float er = ex->radius * sin(radiusRatio * CGLP_PI);
    thickness = 3;
    arc(ex->pos.x, ex->pos.y, er, 0, CGLP_PI * 2, &scratch);
    if (ex->ticks < 0) {
      ex->isAlive = false;
      continue;
    }
  }

  FOR_EACH(trojandefenseProjectiles, projIdx) {
    ASSIGN_ARRAY_ITEM(trojandefenseProjectiles, projIdx, TrojandefenseProjectile, p);
    SKIP_IS_NOT_ALIVE(p);
    vectorAdd(&p->pos, p->vel.x, p->vel.y);
    color = RED;
    Collision pc;
    box(p->pos.x, p->pos.y, 3, 3, &pc);
    if (p->isDeflected) {
      color = YELLOW;
      box(p->pos.x, p->pos.y, 2.5, 2.5, &scratch);
    }
    if (!p->isDeflected) {
      if (pc.isColliding.rect[CYAN]) {
        p->isDeflected = true;
        p->deflectedTicks = 0;
        play(COIN);
        float angleToProjectile = angleTo(&trojandefenseHorse.pos, p->pos.x, p->pos.y);
        float reflectionAngle = angleToProjectile;
        float speed = vectorLength(&p->vel) * 1.5;
        vectorSet(&p->vel, speed, 0);
        rotate(&p->vel, reflectionAngle);
        particle(p->pos.x, p->pos.y, 5, 1, reflectionAngle, CGLP_PI / 4);
        continue;
      }
      if (pc.isColliding.character['a'] || pc.isColliding.character['b']) {
        play(EXPLOSION);
        gameOver();
        p->isAlive = false;
        continue;
      }
    } else {
      p->deflectedTicks++;
      if (pc.isColliding.character['c'] || pc.isColliding.character['d'] || pc.isColliding.character['e']) {
        p->isAlive = false;
        continue;
      }
    }
    float distanceFromCenter = distanceTo(&p->pos, trojandefenseHorse.pos.x, trojandefenseHorse.pos.y);
    if (distanceFromCenter > 55) {
      p->isAlive = false;
      continue;
    }
  }

  FOR_EACH(trojandefenseEnemies, ei) {
    ASSIGN_ARRAY_ITEM(trojandefenseEnemies, ei, TrojandefenseEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    color = RED;
    int[2] enemyChar;
    enemyChar[0] = 'c' + e->type;
    enemyChar[1] = 0;
    Collision ec;
    character(enemyChar, e->pos.x, e->pos.y, &ec);
    bool isHit = ec.isColliding.rect[YELLOW];
    if (isHit) {
      play(POWER_UP);
      addScore(trojandefenseMultiplier * (e->type + 1) * 10, e->pos.x, e->pos.y);
      trojandefenseMultiplier++;
      ASSIGN_ARRAY_ITEM(trojandefenseExplosions, trojandefenseExplosionIndex, TrojandefenseExplosion, nex);
      nex->pos = e->pos;
      nex->radius = 8;
      nex->duration = 20;
      nex->ticks = 20;
      nex->isAlive = true;
      trojandefenseExplosionIndex = cgl_wrap(trojandefenseExplosionIndex + 1, 0, TROJANDEFENSE_MAX_EXPLOSION_COUNT);
      particle(e->pos.x, e->pos.y, 15, 2, 0, CGLP_PI * 2);
      e->isAlive = false;
      continue;
    }
    e->nextFire--;
    if (e->nextFire <= 0) {
      float fireAngle = angleTo(&e->pos, trojandefenseHorse.pos.x, trojandefenseHorse.pos.y);
      ASSIGN_ARRAY_ITEM(trojandefenseProjectiles, trojandefenseProjectileIndex, TrojandefenseProjectile, np);
      np->pos = e->pos;
      vectorSet(&np->vel, e->fireSpeed, 0);
      rotate(&np->vel, fireAngle);
      np->isDeflected = false;
      np->deflectedTicks = 0;
      np->isAlive = true;
      trojandefenseProjectileIndex = cgl_wrap(trojandefenseProjectileIndex + 1, 0, TROJANDEFENSE_MAX_PROJECTILE_COUNT);
      play(LASER);
      e->nextFire = e->fireRate * rnd(0.8, 1.2);
    }
  }

  if (trojandefenseMultiplier > 1) {
    color = YELLOW;
    int[16] multText;
    strcpy(multText, "x");
    strcat(multText, intToChar(trojandefenseMultiplier));
    text(multText, 3, 9, &scratch);
  }
  COUNT_IS_ALIVE(trojandefenseEnemies, aliveEnemyCount);
  if (aliveEnemyCount == 0 && ticks > 100) {
    difficulty += 0.0005;
  }
}

void addGameTrojandefense() {
  addGame(trojandefenseTitle, trojandefenseDescription, trojandefenseCharacters,
          trojandefenseCharactersCount, &trojandefenseOptions, false, &trojandefenseUpdate);
}
