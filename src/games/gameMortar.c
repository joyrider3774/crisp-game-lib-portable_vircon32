#include "../cglp.h"

int* mortarTitle = "MORTAR";
int* mortarDescription = "[Hold]\n Adjust distance\n[Release]\n Fire";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] mortarCharacters = {
    {
        "  ll  ",
        "  ll  ",
        "  ll  ",
        "l ll l",
        "llllll",
        "ll  ll",
    },
    {
        "llllll",
        " l  l ",
        " l  l ",
        "llllll",
        "  ll  ",
        "  ll  ",
    },
};
int mortarCharactersCount = 2;

Options mortarOptions = {100, 100, 3, false};

struct MortarEnemy {
  Vector pos;
  float vy;
  bool isAlive;
};
#define MORTAR_MAX_ENEMY_COUNT 64
MortarEnemy[MORTAR_MAX_ENEMY_COUNT] mortarEnemies;
int mortarEnemyIndex;
float mortarNextEnemyTicks;
float mortarMaxEnemyY;

struct MortarCannon {
  Vector pos;
  float vx;
  bool hasSight;
  float sightY;
};
MortarCannon mortarCannon;

struct MortarShot {
  Vector pos;
  bool active;
  float width;
};
MortarShot mortarShot;

struct MortarExplosion {
  Vector pos;
  bool active;
  float targetRadius;
  float radius;
};
MortarExplosion mortarExplosion;

float mortarOy;
int mortarEndCount;
int mortarMultiplier;
float mortarSightSpeedRatio;

#define MORTAR_GROUND_COUNT 99
Vector[MORTAR_GROUND_COUNT] mortarGrounds;

void mortarUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(mortarEnemies);
    mortarEnemyIndex = 0;
    TIMES(5, i) {
      ASSIGN_ARRAY_ITEM(mortarEnemies, mortarEnemyIndex, MortarEnemy, ne);
      vectorSet(&ne->pos, rnd(49, 60), rnd(-149, -140));
      ne->vy = 0.1;
      ne->isAlive = true;
      mortarEnemyIndex = cgl_wrap(mortarEnemyIndex + 1, 0, MORTAR_MAX_ENEMY_COUNT);
    }
    mortarNextEnemyTicks = 0;
    mortarMaxEnemyY = 0;
    vectorSet(&mortarCannon.pos, 30, 95);
    mortarCannon.vx = 1;
    mortarCannon.hasSight = false;
    mortarCannon.sightY = 0;
    mortarShot.active = false;
    vectorSet(&mortarShot.pos, 0, 0);
    mortarShot.width = 0;
    mortarExplosion.active = false;
    vectorSet(&mortarExplosion.pos, 0, 0);
    mortarOy = 0;
    mortarEndCount = 0;
    mortarMultiplier = 1;
    mortarSightSpeedRatio = 3;
    TIMES(MORTAR_GROUND_COUNT, i) { vectorSet(&mortarGrounds[i], rnd(0, 99), rnd(-300, 100)); }
  }
  color = LIGHT_BLACK;
  TIMES(MORTAR_GROUND_COUNT, i) { box(mortarGrounds[i].x, mortarGrounds[i].y + mortarOy, 3, 1, &scratch); }
  if (mortarExplosion.active) {
    color = RED;
    mortarExplosion.radius += (mortarExplosion.targetRadius - mortarExplosion.radius) * 0.1;
    thickness = 5;
    arc(mortarExplosion.pos.x, mortarExplosion.pos.y + mortarOy, mortarExplosion.radius, 0,
        CGLP_PI * 2, &scratch);
    if (mortarExplosion.targetRadius - mortarExplosion.radius < 1) {
      mortarExplosion.active = false;
    }
  } else if (mortarCannon.hasSight) {
    mortarCannon.sightY -= sqrt(difficulty) * 2 * mortarSightSpeedRatio;
    float radius = 0;
    if (mortarCannon.sightY < 0) {
      mortarOy +=
          (90 - mortarCannon.sightY - mortarOy) * (0.05 * sqrt(difficulty) * mortarSightSpeedRatio);
      radius = clamp(-mortarCannon.sightY * 0.3, 0, 30);
    }
    radius += 2;
    color = BLACK;
    thickness = 2;
    arc(mortarCannon.pos.x, mortarCannon.sightY + mortarOy, radius, 0, CGLP_PI * 2, &scratch);
    if (input.isJustReleased || mortarCannon.sightY < -200) {
      if (radius == 2) {
        if (!mortarShot.active) {
          play(LASER);
          mortarShot.pos = mortarCannon.pos;
          mortarShot.width = ceil((91 - mortarCannon.sightY) * 0.2);
          mortarShot.active = true;
        }
      } else {
        play(EXPLOSION);
        mortarExplosion.targetRadius = radius;
        mortarExplosion.radius = 0;
        vectorSet(&mortarExplosion.pos, mortarCannon.pos.x, mortarCannon.sightY);
        mortarExplosion.active = true;
      }
      mortarCannon.hasSight = false;
      if (mortarSightSpeedRatio > 1) {
        mortarSightSpeedRatio--;
      }
    }
  } else {
    mortarOy *= 0.8;
    mortarCannon.pos.x += mortarCannon.vx * difficulty;
    if ((mortarCannon.pos.x < 3 && mortarCannon.vx < 0) ||
        (mortarCannon.pos.x > 96 && mortarCannon.vx > 0)) {
      mortarCannon.vx *= -1;
    }
    if (input.isJustPressed) {
      play(SELECT);
      mortarCannon.hasSight = true;
      mortarCannon.sightY = 90;
      mortarMultiplier = 1;
    }
  }
  if (mortarShot.active) {
    mortarShot.pos.y -= sqrt(difficulty) * 3;
    color = RED;
    box(mortarShot.pos.x, mortarShot.pos.y + mortarOy, mortarShot.width, 6, &scratch);
    if (mortarShot.pos.y < -3) {
      mortarShot.active = false;
    }
  }
  if (mortarEndCount > 0) {
    mortarOy *= 0.5;
    if (mortarEndCount > 9) {
      gameOver();
    }
  }
  color = BLACK;
  character("a", mortarCannon.pos.x, mortarCannon.pos.y + mortarOy, &scratch);
  COUNT_IS_ALIVE(mortarEnemies, aliveEnemyCount0);
  if (aliveEnemyCount0 == 0) {
    mortarNextEnemyTicks = 0;
  }
  mortarNextEnemyTicks--;
  if (mortarNextEnemyTicks < 0) {
    float vy = rnd(0.5, sqrt(difficulty) * 2) * 0.1;
    float x = rnd(20, 80);
    int spawnCount = rndi(5, 9);
    TIMES(spawnCount, k) {
      ASSIGN_ARRAY_ITEM(mortarEnemies, mortarEnemyIndex, MortarEnemy, ne);
      vectorSet(&ne->pos, x + rnd(0, 9) * RNDPM(), -280 + rnd(0, 9) * RNDPM());
      ne->vy = vy * rnd(0.9, 1.1);
      ne->isAlive = true;
      mortarEnemyIndex = cgl_wrap(mortarEnemyIndex + 1, 0, MORTAR_MAX_ENEMY_COUNT);
    }
    COUNT_IS_ALIVE(mortarEnemies, aliveEnemyCountAfterSpawn);
    mortarNextEnemyTicks = (99 * sqrt(aliveEnemyCountAfterSpawn)) / difficulty;
  }
  color = RED;
  float my = -200;
  FOR_EACH(mortarEnemies, i) {
    ASSIGN_ARRAY_ITEM(mortarEnemies, i, MortarEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    Collision ec;
    character("b", e->pos.x, e->pos.y + mortarOy, &ec);
    if (e->pos.y > 99) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      text("X", e->pos.x, 97, &scratch);
      mortarEndCount++;
    } else {
      float vy = (e->vy * sqrt(120 - mortarMaxEnemyY)) / 4;
      if (mortarCannon.hasSight) {
        vy *= 0.3;
      }
      e->pos.y += vy;
      if (ec.isColliding.rect[RED]) {
        play(POWER_UP);
        addScore(mortarMultiplier, e->pos.x, e->pos.y + mortarOy);
        mortarMultiplier++;
        e->isAlive = false;
        continue;
      }
      if (e->pos.y > my) {
        my = e->pos.y;
      }
    }
  }
  mortarMaxEnemyY = my;
}

void addGameMortar() {
  addGame(mortarTitle, mortarDescription, mortarCharacters, mortarCharactersCount,
          &mortarOptions, false, &mortarUpdate);
}
