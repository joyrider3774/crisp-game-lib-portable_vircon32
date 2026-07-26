#include "../cglp.h"

int* stormveilTitle = "STORMVEIL";
int* stormveilDescription = "[Tap] Switch Lane";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] stormveilCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int stormveilCharactersCount = 1;

Options stormveilOptions = {100, 100, 0, false};

struct StormveilPlayer {
  float x;
  float y;
  float vx;
  float squash;
  float switchTimer;
};
StormveilPlayer stormveilPlayer;

struct StormveilObstacle {
  float x;
  float y;
  float phase;
  bool isAlive;
};
#define STORMVEIL_MAX_OBSTACLE_COUNT 64
StormveilObstacle[STORMVEIL_MAX_OBSTACLE_COUNT] stormveilObstacles;
int stormveilObstacleIndex;

struct StormveilCoin {
  float x;
  float y;
  float pulse;
  bool isAlive;
};
#define STORMVEIL_MAX_COIN_COUNT 32
StormveilCoin[STORMVEIL_MAX_COIN_COUNT] stormveilCoins;
int stormveilCoinIndex;

struct StormveilTrail {
  float x;
  float y;
  float alpha;
  bool isAlive;
};
#define STORMVEIL_MAX_TRAIL_COUNT 32
StormveilTrail[STORMVEIL_MAX_TRAIL_COUNT] stormveilTrails;
int stormveilTrailIndex;

int stormveilLane;
float stormveilNextObstacleTicks;
int stormveilMultiplier;

void stormveilUpdate() {
  Collision scratch;
  if (!ticks) {
    stormveilLane = 1;
    stormveilPlayer.x = 50;
    stormveilPlayer.y = 85;
    stormveilPlayer.vx = 0;
    stormveilPlayer.squash = 1;
    stormveilPlayer.switchTimer = 0;
    INIT_UNALIVED_ARRAY_FAST(stormveilObstacles);
    stormveilObstacleIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(stormveilCoins);
    stormveilCoinIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(stormveilTrails);
    stormveilTrailIndex = 0;
    stormveilNextObstacleTicks = 0;
    stormveilMultiplier = 1;
  }

  int[3] laneX;
  laneX[0] = 25;
  laneX[1] = 50;
  laneX[2] = 75;
  float speed = 1.5 * sqrt(difficulty);
  float prevX = stormveilPlayer.x;

  if (input.isJustPressed) {
    stormveilLane = cgl_wrap(stormveilLane + 1, 0, 3);
    play(JUMP);
    stormveilPlayer.squash = 0.6;
    stormveilPlayer.switchTimer = 10;
    color = LIGHT_BLACK;
    particle(stormveilPlayer.x, stormveilPlayer.y + 3, 5, 0.5, -CGLP_PI_2, CGLP_PI_4);
  }

  stormveilPlayer.x += (laneX[stormveilLane] - stormveilPlayer.x) * 0.5;
  stormveilPlayer.vx = stormveilPlayer.x - prevX;

  stormveilPlayer.squash += (1 - stormveilPlayer.squash) * 0.15;
  if (stormveilPlayer.switchTimer > 0) {
    stormveilPlayer.switchTimer--;
  }

  if (fabs(stormveilPlayer.vx) > 0.5) {
    ASSIGN_ARRAY_ITEM(stormveilTrails, stormveilTrailIndex, StormveilTrail, nt);
    nt->x = stormveilPlayer.x;
    nt->y = stormveilPlayer.y;
    nt->alpha = 0.5;
    nt->isAlive = true;
    stormveilTrailIndex = cgl_wrap(stormveilTrailIndex + 1, 0, STORMVEIL_MAX_TRAIL_COUNT);
  }

  stormveilNextObstacleTicks -= difficulty;
  if (stormveilNextObstacleTicks < 0) {
    stormveilNextObstacleTicks += 40;
    int blockedCount = (int)clamp(floor(rnd(1, sqrt(difficulty))), 1, 2);
    int[3] lanesArr;
    lanesArr[0] = 0;
    lanesArr[1] = 1;
    lanesArr[2] = 2;
    int lanesCount = 3;
    int[2] blocked;
    TIMES(blockedCount, bi) {
      int idx = (int)floor(rnd(0, lanesCount));
      blocked[bi] = lanesArr[idx];
      int k;
      for (k = idx; k < lanesCount - 1; k++) {
        lanesArr[k] = lanesArr[k + 1];
      }
      lanesCount--;
    }
    int li;
    for (li = 0; li < 3; li++) {
      bool isBlocked = false;
      int bj;
      for (bj = 0; bj < blockedCount; bj++) {
        if (blocked[bj] == li) {
          isBlocked = true;
        }
      }
      if (isBlocked) {
        ASSIGN_ARRAY_ITEM(stormveilObstacles, stormveilObstacleIndex, StormveilObstacle, no);
        no->x = laneX[li];
        no->y = -5;
        no->phase = rnd(0, CGLP_PI * 2);
        no->isAlive = true;
        stormveilObstacleIndex = cgl_wrap(stormveilObstacleIndex + 1, 0, STORMVEIL_MAX_OBSTACLE_COUNT);
      }
    }
    if (lanesCount > 0) {
      ASSIGN_ARRAY_ITEM(stormveilCoins, stormveilCoinIndex, StormveilCoin, nc);
      nc->x = laneX[lanesArr[(int)floor(rnd(0, lanesCount))]];
      nc->y = -5;
      nc->pulse = 0;
      nc->isAlive = true;
      stormveilCoinIndex = cgl_wrap(stormveilCoinIndex + 1, 0, STORMVEIL_MAX_COIN_COUNT);
    }
  }

  color = LIGHT_BLACK;
  thickness = 1;
  int li2;
  for (li2 = 0; li2 < 3; li2++) {
    line(laneX[li2], 0, laneX[li2], 100, &scratch);
  }

  FOR_EACH(stormveilTrails, ti) {
    ASSIGN_ARRAY_ITEM(stormveilTrails, ti, StormveilTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->alpha -= 0.1;
    if (t->alpha <= 0) {
      t->isAlive = false;
      continue;
    }
    color = LIGHT_CYAN;
    box(t->x, t->y, 5 * t->alpha, 5 * t->alpha, &scratch);
  }

  color = CYAN;
  float breathe = 1 + sin(ticks * 0.1) * 0.05;
  float stretchX = stormveilPlayer.squash * breathe;
  float stretchY = (2 - stormveilPlayer.squash) * breathe;
  float tilt = -stormveilPlayer.vx * 0.08;
  thickness = 6 * stretchX;
  bar(stormveilPlayer.x, stormveilPlayer.y, 6 * stretchY, tilt, &scratch);

  FOR_EACH(stormveilObstacles, oi) {
    ASSIGN_ARRAY_ITEM(stormveilObstacles, oi, StormveilObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->y += speed;
    o->phase += 0.2;
    color = RED;
    float pulse = 1 + sin(o->phase) * 0.15;
    float sizeX = 6 * pulse;
    float sizeY = 6 * (2 - pulse);
    box(o->x, o->y, sizeX, sizeY, &scratch);
    if (scratch.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      color = RED;
      particle(stormveilPlayer.x, stormveilPlayer.y, 20, 2, 0, CGLP_PI * 2);
      gameOver();
    }
    if (o->y > 105) {
      color = LIGHT_BLACK;
      particle(o->x, 100, 3, 0.3, CGLP_PI_2, CGLP_PI_4);
      o->isAlive = false;
      continue;
    }
  }

  FOR_EACH(stormveilCoins, ci) {
    ASSIGN_ARRAY_ITEM(stormveilCoins, ci, StormveilCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->y += speed;
    c->pulse += 0.12;
    float spinW = 2 + fabs(cos(c->pulse)) * 4;
    color = YELLOW;
    box(c->x, c->y, spinW, 6, &scratch);
    if (scratch.isColliding.rect[CYAN]) {
      addScore(stormveilMultiplier, c->x, c->y);
      stormveilMultiplier = (int)clamp(stormveilMultiplier + 1, 1, 16);
      play(COIN);
      color = YELLOW;
      particle(c->x, c->y, 10, 1.5, 0, CGLP_PI * 2);
      c->isAlive = false;
      continue;
    }
    if (c->y > 105) {
      stormveilMultiplier = 1;
      c->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(stormveilMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameStormveil() {
  addGame(stormveilTitle, stormveilDescription, stormveilCharacters,
          stormveilCharactersCount, &stormveilOptions, false, &stormveilUpdate);
}
