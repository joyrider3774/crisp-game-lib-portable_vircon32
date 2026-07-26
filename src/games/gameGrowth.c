#include "../cglp.h"

int* growthTitle = "GROWTH";
int* growthDescription = "[Hold] Growth";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] growthCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int growthCharactersCount = 1;

Options growthOptions = {200, 70, 30, false};

#define GROWTH_FLOOR_Y 60

struct GrowthPlayer {
  float x;
  float vx;
  float size;
};
GrowthPlayer growthPlayer;

struct GrowthEnemy {
  float x;
  float size;
  bool isAlive;
};
#define GROWTH_MAX_ENEMY_COUNT 32
GrowthEnemy[GROWTH_MAX_ENEMY_COUNT] growthEnemies;
int growthEnemyIndex;
float growthNextEnemyDist;

void growthUpdate() {
  Collision scratch;
  if (!ticks) {
    growthPlayer.x = 9;
    growthPlayer.vx = 1;
    growthPlayer.size = 5;
    INIT_UNALIVED_ARRAY_FAST(growthEnemies);
    growthEnemyIndex = 0;
    growthNextEnemyDist = 0;
  }
  float scr;
  if (growthPlayer.x > 9) {
    scr = (growthPlayer.x - 9) * 0.5;
  } else {
    scr = 0;
  }
  color = LIGHT_BLUE;
  rect(0, GROWTH_FLOOR_Y, 200, 10, &scratch);
  if (input.isJustPressed) {
    play(LASER);
  }
  float targetSize;
  if (input.isPressed) {
    targetSize = 50;
  } else {
    targetSize = 5;
  }
  growthPlayer.size +=
      (targetSize - growthPlayer.size) * clamp(growthPlayer.vx, 1, 999) * 0.01;
  growthPlayer.vx += (15 / growthPlayer.size - 1) * 0.02 * sqrt(difficulty);
  growthPlayer.x += growthPlayer.vx - scr;
  if (growthPlayer.x + growthPlayer.size / 2 < 1) {
    gameOver();
  }
  color = YELLOW;
  rect(0, GROWTH_FLOOR_Y, growthPlayer.x + growthPlayer.size / 2,
       -growthPlayer.size, &scratch);
  growthNextEnemyDist -= scr;
  if (growthNextEnemyDist < 0) {
    float size;
    if (rnd(0, 1) < 0.8) {
      size = 3;
    } else {
      size = rnd(0, 5) * rnd(0, 5) + 3;
    }
    if (size < 7) {
      size = 3;
    }
    ASSIGN_ARRAY_ITEM(growthEnemies, growthEnemyIndex, GrowthEnemy, e);
    e->x = 400;
    e->size = size;
    e->isAlive = true;
    growthEnemyIndex = cgl_wrap(growthEnemyIndex + 1, 0, GROWTH_MAX_ENEMY_COUNT);
    growthNextEnemyDist += rnd(30, 50);
  }
  FOR_EACH(growthEnemies, i) {
    ASSIGN_ARRAY_ITEM(growthEnemies, i, GrowthEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x -= scr;
    if (e->size > growthPlayer.size) {
      color = RED;
    } else {
      color = CYAN;
    }
    float sc;
    if (e->x > 100) {
      sc = (e->x - 100) / 300 + 1;
    } else {
      sc = 1;
    }
    float sz = e->size / sc;
    Collision ec;
    rect(e->x / sc, GROWTH_FLOOR_Y, sz, -sz, &ec);
    if (ec.isColliding.rect[YELLOW]) {
      if (e->size > growthPlayer.size) {
        play(EXPLOSION);
        gameOver();
      } else {
        if (e->size < 5) {
          play(HIT);
        } else {
          play(POWER_UP);
        }
        float ss = sqrt(e->size);
        particle(e->x, GROWTH_FLOOR_Y - e->size / 2, ss, ss, 0, CGLP_PI / 2);
        addScore(ceil(clamp(growthPlayer.vx, 1, 999) * e->size), e->x,
                 GROWTH_FLOOR_Y - growthPlayer.size);
      }
      e->isAlive = false;
      continue;
    }
  }
}

void addGameGrowth() {
  addGame(growthTitle, growthDescription, growthCharacters,
          growthCharactersCount, &growthOptions, false, &growthUpdate);
}
