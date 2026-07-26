#include "../cglp.h"

int* slanesTitle = "S LANES";
int* slanesDescription = "[Hold]\n Shot & Forward";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] slanesCharacters = {
    {
        "rllbb ",
        "lllccb",
        "LlyL b",
    },
    {
        "  r rr",
        "rrRrRR",
        "  grr ",
        "  grr ",
        "rrRrRR",
        "  r rr",
    },
    {
        " LLLL ",
        "LyyyyL",
        "LyyyyL",
        "LyyyyL",
        "LyyyyL",
        " LLLL ",
    },
    {
        "l Llll",
        "l Llll",
    },
};
int slanesCharactersCount = 4;

Options slanesOptions = {100, 100, 1, true};

#define SLANES_LANE_WIDTH 20
#define SLANES_LANE_COUNT 4
#define SLANES_SHIP_X 3

struct SlanesEnemy {
  Vector pos;
  float vx;
  int laneIndex;
  bool isAlive;
};
// Spawn interval is rnd(60,90)/difficulty (scales with difficulty directly) while enemy speed only
// scales as sqrt(difficulty) - concurrent count grows unboundedly (~12*sqrt(difficulty)), reachable
// within roughly the first 10 minutes of play.
#define SLANES_MAX_ENEMY_COUNT 256
SlanesEnemy[SLANES_MAX_ENEMY_COUNT] slanesEnemies;
int slanesEnemyIndex;
float slanesNextEnemyTicks;

struct SlanesCoin {
  Vector pos;
  int laneIndex;
  bool isAlive;
};
#define SLANES_MAX_COIN_COUNT 32
SlanesCoin[SLANES_MAX_COIN_COUNT] slanesCoins;
int slanesCoinIndex;

struct SlanesShot {
  Vector pos;
  float vx;
  bool isAlive;
};
#define SLANES_MAX_SHOT_COUNT 32
SlanesShot[SLANES_MAX_SHOT_COUNT] slanesShots;
int slanesShotIndex;

struct SlanesShip {
  Vector pos;
  int laneIndex;
  float targetY;
  float laneTicks;
  float shotTicks;
};
SlanesShip slanesShip;

float[SLANES_LANE_COUNT] slanesLaneSpeeds;
int slanesMultiplier;

float slanesCalcY(int i) {
  return i * SLANES_LANE_WIDTH + SLANES_LANE_WIDTH / 2.0 + 12;
}

void slanesUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(slanesEnemies);
    slanesEnemyIndex = 0;
    slanesNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(slanesCoins);
    slanesCoinIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(slanesShots);
    slanesShotIndex = 0;
    slanesShip.pos.x = SLANES_SHIP_X;
    slanesShip.pos.y = slanesCalcY(0);
    slanesShip.laneIndex = 0;
    slanesShip.targetY = slanesCalcY(0);
    slanesShip.laneTicks = 0;
    slanesShip.shotTicks = 0;
    TIMES(SLANES_LANE_COUNT, i) { slanesLaneSpeeds[i] = 1; }
    slanesMultiplier = 1;
  }
  slanesNextEnemyTicks--;
  if (slanesNextEnemyTicks < 0) {
    play(SELECT);
    int laneIndex = rndi(0, SLANES_LANE_COUNT);
    ASSIGN_ARRAY_ITEM(slanesEnemies, slanesEnemyIndex, SlanesEnemy, ne);
    vectorSet(&ne->pos, 103, slanesCalcY(laneIndex));
    ne->vx = -rnd(1, sqrt(difficulty)) * 0.2;
    ne->laneIndex = laneIndex;
    ne->isAlive = true;
    slanesEnemyIndex = cgl_wrap(slanesEnemyIndex + 1, 0, SLANES_MAX_ENEMY_COUNT);
    slanesNextEnemyTicks = rnd(60, 90) / difficulty;
  }
  color = BLACK;
  if (input.isPressed) {
    if (input.isJustPressed) {
      slanesShip.pos.x = SLANES_SHIP_X;
    } else {
      slanesShip.pos.x = clamp(slanesShip.pos.x + sqrt(difficulty) * 0.3, 0, 50);
    }
    slanesShip.pos.y = slanesCalcY(slanesShip.laneIndex);
    slanesShip.targetY = slanesShip.pos.y;
    slanesShip.shotTicks--;
    if (slanesShip.shotTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(slanesShots, slanesShotIndex, SlanesShot, ns);
      vectorSet(&ns->pos, slanesShip.pos.x, cgl_wrap(slanesShip.pos.y, 0, 100));
      ns->vx = difficulty * 2;
      ns->isAlive = true;
      slanesShotIndex = cgl_wrap(slanesShotIndex + 1, 0, SLANES_MAX_SHOT_COUNT);
      slanesShip.shotTicks = 20 / sqrt(difficulty);
      slanesShip.laneTicks = slanesShip.shotTicks;
    }
  } else {
    play(HIT);
    slanesShip.laneTicks--;
    if (slanesShip.laneTicks < 0) {
      slanesShip.laneIndex = (int)cgl_wrap(slanesShip.laneIndex + 1, 0, SLANES_LANE_COUNT);
      float bonus;
      if (slanesShip.laneIndex == 0) {
        bonus = 100;
      } else {
        bonus = 0;
      }
      slanesShip.targetY = slanesCalcY(slanesShip.laneIndex) + bonus;
      slanesShip.pos.y = cgl_wrap(slanesShip.pos.y, 0, 100);
      slanesShip.laneTicks = 20 / sqrt(difficulty);
    }
    slanesShip.pos.x += (SLANES_SHIP_X - slanesShip.pos.x) * 0.3;
    slanesShip.pos.y += (slanesShip.targetY - slanesShip.pos.y) * 0.3;
  }
  character("a", slanesShip.pos.x, cgl_wrap(slanesShip.pos.y, 0, 100), &scratch);
  color = LIGHT_PURPLE;
  TIMES(SLANES_LANE_COUNT, i) {
    slanesLaneSpeeds[i] += (0.9 - slanesLaneSpeeds[i]) * 0.02;
    int n = (int)ceil(slanesLaneSpeeds[i]);
    if (n > 23) {
      n = 23;
    }
    int[24] laneText;
    TIMES(n, k) { laneText[k] = '<'; }
    laneText[n] = 0;
    text(laneText, 9, slanesCalcY(i), &scratch);
  }
  color = BLACK;
  float cvx = -difficulty * 0.2;
  FOR_EACH(slanesCoins, i) {
    ASSIGN_ARRAY_ITEM(slanesCoins, i, SlanesCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x += cvx;
    Collision cc;
    character("c", c->pos.x, c->pos.y, &cc);
    if (cc.isColliding.character['a']) {
      play(COIN);
      addScore(slanesMultiplier, c->pos.x, c->pos.y);
      slanesMultiplier++;
      c->isAlive = false;
      continue;
    }
    if (c->pos.x < -3) {
      play(EXPLOSION);
      slanesLaneSpeeds[c->laneIndex]++;
      if (slanesMultiplier > 1) {
        slanesMultiplier--;
      }
      c->isAlive = false;
      continue;
    }
  }
  FOR_EACH(slanesShots, i) {
    ASSIGN_ARRAY_ITEM(slanesShots, i, SlanesShot, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x += s->vx;
    character("d", s->pos.x, s->pos.y, &scratch);
  }
  color = BLACK;
  FOR_EACH(slanesEnemies, i) {
    ASSIGN_ARRAY_ITEM(slanesEnemies, i, SlanesEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.x += e->vx * slanesLaneSpeeds[e->laneIndex];
    Collision ec;
    character("b", e->pos.x, e->pos.y, &ec);
    if (ec.isColliding.character['d']) {
      play(POWER_UP);
      particle(e->pos.x, e->pos.y, 16, 1, 0, CGLP_PI * 2);
      ASSIGN_ARRAY_ITEM(slanesCoins, slanesCoinIndex, SlanesCoin, nc);
      nc->pos = e->pos;
      nc->laneIndex = e->laneIndex;
      nc->isAlive = true;
      slanesCoinIndex = cgl_wrap(slanesCoinIndex + 1, 0, SLANES_MAX_COIN_COUNT);
      e->isAlive = false;
      continue;
    }
    if (e->pos.x < 9) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      color = RED;
      text("X", 3, e->pos.y, &scratch);
      gameOver();
      color = BLACK;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(slanesShots, i) {
    ASSIGN_ARRAY_ITEM(slanesShots, i, SlanesShot, s);
    SKIP_IS_NOT_ALIVE(s);
    if (s->pos.x > 103) {
      s->isAlive = false;
      continue;
    }
    Collision sc;
    character("d", s->pos.x, s->pos.y, &sc);
    if (sc.isColliding.character['b']) {
      s->isAlive = false;
      continue;
    }
  }
}

void addGameSlanes() {
  addGame(slanesTitle, slanesDescription, slanesCharacters,
          slanesCharactersCount, &slanesOptions, false, &slanesUpdate);
}
