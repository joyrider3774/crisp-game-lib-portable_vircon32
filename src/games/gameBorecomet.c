#include "../cglp.h"

int* borecometTitle = "BORE COMET";
int* borecometDescription = "[Hold] Drill thrust\n[Release] Fall";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] borecometCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int borecometCharactersCount = 0;

Options borecometOptions = {100, 100, 39, false};

struct BorecometPlayer {
  Vector pos;
  float vy;
};
BorecometPlayer borecometPlayer;

struct BorecometObstacle {
  Vector pos;
  float w;
  float h;
  float boreT;
  bool isAlive;
};
// Lifetime and spawn interval both scale with (0.5 + difficulty*0.1), so the
// concurrent count stays near a constant ~2.4-4.4 regardless of difficulty -
// sized with generous headroom.
#define BORECOMET_MAX_OBSTACLE_COUNT 24
BorecometObstacle[BORECOMET_MAX_OBSTACLE_COUNT] borecometObstacles;
int borecometObstacleIndex;

struct BorecometEnemy {
  Vector pos;
  float vx;
  bool isAlive;
};
// Concurrent enemy count stays near ~3 across the difficulty range (see
// port analysis) - sized generously.
#define BORECOMET_MAX_ENEMY_COUNT 32
BorecometEnemy[BORECOMET_MAX_ENEMY_COUNT] borecometEnemies;
int borecometEnemyIndex;

float borecometNextRockX;
float borecometNextEnemyT;
bool borecometWasInRock;
float borecometHitstop;
float borecometShakeMag;
float borecometShakeT;

struct BorecometFx {
  Vector pos;
  float t;
  int colorIndex;
  float radius;
};
BorecometFx borecometBurstFx;
BorecometFx borecometDestFx;

void borecometUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&borecometPlayer.pos, 35, 30);
    borecometPlayer.vy = 0;
    INIT_UNALIVED_ARRAY_FAST(borecometObstacles);
    borecometObstacleIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(borecometEnemies);
    borecometEnemyIndex = 0;
    borecometNextRockX = 20;
    borecometNextEnemyT = 60;
    borecometWasInRock = false;
    borecometHitstop = 0;
    borecometShakeMag = 0;
    borecometShakeT = 0;
    vectorSet(&borecometBurstFx.pos, 0, 0);
    borecometBurstFx.t = 0;
    borecometBurstFx.colorIndex = LIGHT_YELLOW;
    borecometBurstFx.radius = 22;
    vectorSet(&borecometDestFx.pos, 0, 0);
    borecometDestFx.t = 0;
    borecometDestFx.colorIndex = LIGHT_BLACK;
    borecometDestFx.radius = 0;
  }

  float spd = 0.5 + difficulty * 0.1;
  bool drilling = input.isPressed;
  bool frozen = borecometHitstop > 0;
  if (frozen) {
    borecometHitstop--;
  }

  bool inRock = false;
  BorecometObstacle* curRock = NULL;
  FOR_EACH(borecometObstacles, ri) {
    ASSIGN_ARRAY_ITEM(borecometObstacles, ri, BorecometObstacle, r);
    SKIP_IS_NOT_ALIVE(r);
    if (fabs(borecometPlayer.pos.x - r->pos.x) < r->w / 2 + 2 &&
        fabs(borecometPlayer.pos.y - r->pos.y) < r->h / 2 + 2) {
      inRock = true;
      curRock = r;
    }
  }

  if (!frozen) {
    if (drilling) {
      if (inRock) {
        borecometPlayer.vy -= 0.06;
      } else {
        borecometPlayer.vy -= 0.11;
      }
      if (inRock && ticks % 2 == 0) {
        particle(borecometPlayer.pos.x, borecometPlayer.pos.y, 3, 1.5, 0, CGLP_PI * 2);
      }
      if (inRock) {
        addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        curRock->boreT++;
        float boreThreshold = round(sqrt(curRock->w * curRock->h) / (spd / 0.5));
        if (curRock->boreT >= boreThreshold) {
          float destRadius = 15 + (curRock->w + curRock->h) / 4;
          int m = 0;
          FOR_EACH(borecometEnemies, ei) {
            ASSIGN_ARRAY_ITEM(borecometEnemies, ei, BorecometEnemy, e);
            SKIP_IS_NOT_ALIVE(e);
            if (distanceTo(&e->pos, borecometPlayer.pos.x, borecometPlayer.pos.y) < destRadius) {
              m++;
            }
          }
          play(EXPLOSION);
          color = LIGHT_BLACK;
          particle(curRock->pos.x, curRock->pos.y, 30, 3, 0, CGLP_PI * 2);
          addScore(boreThreshold, borecometPlayer.pos.x, borecometPlayer.pos.y - 6);
          if (m > 0) {
            FOR_EACH(borecometEnemies, ei2) {
              ASSIGN_ARRAY_ITEM(borecometEnemies, ei2, BorecometEnemy, e2);
              SKIP_IS_NOT_ALIVE(e2);
              if (distanceTo(&e2->pos, borecometPlayer.pos.x, borecometPlayer.pos.y) < destRadius) {
                color = PURPLE;
                particle(e2->pos.x, e2->pos.y, 10 + m * 2, 2, 0, CGLP_PI * 2);
                e2->isAlive = false;
                continue;
              }
            }
            addScore(m * m * 3, borecometPlayer.pos.x, borecometPlayer.pos.y);
            play(POWER_UP);
          }
          borecometHitstop = fmax(borecometHitstop, clamp(3 + m, 3, 8));
          borecometShakeMag = fmax(borecometShakeMag, clamp(2 + m * 0.8, 2, 6));
          borecometShakeT = 8;
          vectorSet(&borecometDestFx.pos, curRock->pos.x, curRock->pos.y);
          borecometDestFx.radius = destRadius;
          borecometDestFx.t = 14;
          curRock->isAlive = false;
          inRock = false;
        }
      }
    }
    borecometPlayer.vy += 0.05;
    borecometPlayer.vy = clamp(borecometPlayer.vy, -1.5, 1.8);
    if (inRock && drilling) {
      borecometPlayer.pos.y += borecometPlayer.vy * 0.45;
    } else {
      borecometPlayer.pos.y += borecometPlayer.vy;
    }
    if (borecometPlayer.pos.y < 10) {
      borecometPlayer.pos.y = 10;
      borecometPlayer.vy = 0;
    }
  }

  if (borecometWasInRock && !inRock && drilling) {
    int n = 0;
    FOR_EACH(borecometEnemies, ei3) {
      ASSIGN_ARRAY_ITEM(borecometEnemies, ei3, BorecometEnemy, e3);
      SKIP_IS_NOT_ALIVE(e3);
      if (distanceTo(&e3->pos, borecometPlayer.pos.x, borecometPlayer.pos.y) < 22) {
        n++;
      }
    }
    int burstColor;
    if (n >= 4) {
      burstColor = LIGHT_RED;
    } else if (n >= 2) {
      burstColor = YELLOW;
    } else {
      burstColor = LIGHT_YELLOW;
    }
    play(EXPLOSION);
    color = burstColor;
    particle(borecometPlayer.pos.x, borecometPlayer.pos.y, 20 + n * 6, 3, 0, CGLP_PI * 2);
    vectorSet(&borecometBurstFx.pos, borecometPlayer.pos.x, borecometPlayer.pos.y);
    borecometBurstFx.colorIndex = burstColor;
    borecometBurstFx.t = 12;
    if (n > 0) {
      FOR_EACH(borecometEnemies, ei4) {
        ASSIGN_ARRAY_ITEM(borecometEnemies, ei4, BorecometEnemy, e4);
        SKIP_IS_NOT_ALIVE(e4);
        if (distanceTo(&e4->pos, borecometPlayer.pos.x, borecometPlayer.pos.y) < 22) {
          color = burstColor;
          particle(e4->pos.x, e4->pos.y, 10 + n * 2, 2, 0, CGLP_PI * 2);
          e4->isAlive = false;
          continue;
        }
      }
      addScore(n * n * 3, borecometPlayer.pos.x, borecometPlayer.pos.y);
      play(POWER_UP);
      borecometHitstop = fmax(borecometHitstop, clamp(2 + n, 2, 8));
      borecometShakeMag = fmax(borecometShakeMag, clamp(1.2 + n * 0.8, 1.2, 6));
      borecometShakeT = 8;
    }
  }
  borecometWasInRock = inRock;

  if (!frozen) {
    borecometNextRockX -= spd;
    if (borecometNextRockX < 0) {
      ASSIGN_ARRAY_ITEM(borecometObstacles, borecometObstacleIndex, BorecometObstacle, nr);
      vectorSet(&nr->pos, 112, rnd(25, 75));
      nr->w = rnd(14, 30);
      nr->h = rnd(10, 22);
      nr->boreT = 0;
      nr->isAlive = true;
      borecometObstacleIndex = cgl_wrap(borecometObstacleIndex + 1, 0, BORECOMET_MAX_OBSTACLE_COUNT);
      borecometNextRockX = rnd(30, 55);
    }
    borecometNextEnemyT--;
    if (borecometNextEnemyT < 0) {
      ASSIGN_ARRAY_ITEM(borecometEnemies, borecometEnemyIndex, BorecometEnemy, ne);
      vectorSet(&ne->pos, 105, rnd(8, 84));
      ne->vx = -(spd + rnd(0.2, 0.5));
      ne->isAlive = true;
      borecometEnemyIndex = cgl_wrap(borecometEnemyIndex + 1, 0, BORECOMET_MAX_ENEMY_COUNT);
      borecometNextEnemyT = rnd(50, 90) / sqrt(difficulty);
    }
  }

  float shakeY = 0;
  if (borecometShakeT > 0) {
    borecometShakeT--;
    float m2 = borecometShakeMag * (borecometShakeT / 8);
    shakeY = rnd(-m2, m2);
  }
  color = LIGHT_CYAN;
  rect(0, 6 + shakeY, 100, 1, &scratch);
  color = LIGHT_RED;
  rect(0, 90 + shakeY, 100, 2, &scratch);

  if (borecometBurstFx.t > 0) {
    borecometBurstFx.t--;
    color = borecometBurstFx.colorIndex;
    thickness = 2;
    arc(borecometBurstFx.pos.x, borecometBurstFx.pos.y + shakeY,
        borecometBurstFx.radius * (1 - borecometBurstFx.t / 12), 0, CGLP_PI * 2, &scratch);
  }
  if (borecometDestFx.t > 0) {
    borecometDestFx.t--;
    color = borecometDestFx.colorIndex;
    thickness = 2;
    arc(borecometDestFx.pos.x, borecometDestFx.pos.y + shakeY,
        borecometDestFx.radius * (1 - borecometDestFx.t / 14), 0, CGLP_PI * 2, &scratch);
  }

  FOR_EACH(borecometObstacles, ri2) {
    ASSIGN_ARRAY_ITEM(borecometObstacles, ri2, BorecometObstacle, r2);
    SKIP_IS_NOT_ALIVE(r2);
    if (!frozen) {
      r2->pos.x -= spd;
    }
    color = LIGHT_BLACK;
    box(r2->pos.x, r2->pos.y, r2->w, r2->h, &scratch);
    if (r2->pos.x < -20) {
      r2->isAlive = false;
      continue;
    }
  }

  if (inRock && !drilling) {
    play(EXPLOSION);
    particle(borecometPlayer.pos.x, borecometPlayer.pos.y, 25, 3, 0, CGLP_PI * 2);
    gameOver();
  }

  FOR_EACH(borecometEnemies, ei5) {
    ASSIGN_ARRAY_ITEM(borecometEnemies, ei5, BorecometEnemy, e5);
    SKIP_IS_NOT_ALIVE(e5);
    if (!frozen) {
      e5->pos.x += e5->vx;
    }
    color = RED;
    box(e5->pos.x, e5->pos.y, 5, 4, &scratch);
    if (!inRock && distanceTo(&e5->pos, borecometPlayer.pos.x, borecometPlayer.pos.y) < 5) {
      play(EXPLOSION);
      particle(borecometPlayer.pos.x, borecometPlayer.pos.y, 25, 3, 0, CGLP_PI * 2);
      gameOver();
    }
    if (e5->pos.x < -8) {
      e5->isAlive = false;
      continue;
    }
  }

  color = RED;
  Collision seaCollision;
  rect(0, 93, 100, 7, &seaCollision);
  if (seaCollision.isColliding.rect[CYAN]) {
    play(EXPLOSION);
    gameOver();
  }

  if (fabs(borecometPlayer.vy) > 1.0) {
    if (inRock && drilling) {
      color = YELLOW;
    } else {
      color = LIGHT_CYAN;
    }
    box(borecometPlayer.pos.x, clamp(borecometPlayer.pos.y - borecometPlayer.vy * 2.5, 10, 88), 4, 4, &scratch);
    if (inRock && drilling) {
      color = LIGHT_YELLOW;
    } else {
      color = CYAN;
    }
    box(borecometPlayer.pos.x, clamp(borecometPlayer.pos.y - borecometPlayer.vy * 5, 10, 88), 3, 3, &scratch);
  }

  if (inRock && drilling) {
    color = YELLOW;
  } else {
    color = CYAN;
  }
  box(borecometPlayer.pos.x, borecometPlayer.pos.y, 5, 5, &scratch);
  if (drilling) {
    color = LIGHT_YELLOW;
    thickness = 2;
    bar(borecometPlayer.pos.x, borecometPlayer.pos.y - 4, 4, -CGLP_PI / 2, &scratch);
  }
  if (borecometPlayer.pos.y > 91) {
    play(EXPLOSION);
    particle(borecometPlayer.pos.x, borecometPlayer.pos.y, 25, 3, 0, CGLP_PI * 2);
    gameOver();
  }
}

void addGameBorecomet() {
  addGame(borecometTitle, borecometDescription, borecometCharacters,
          borecometCharactersCount, &borecometOptions, false, &borecometUpdate);
}
