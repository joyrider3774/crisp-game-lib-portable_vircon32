#include "../cglp.h"

int* regeneTitle = "REGENE";
int* regeneDescription = "[Slide]\n Erase wall";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] regeneCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int regeneCharactersCount = 1;

Options regeneOptions = {100, 100, 16, false};

struct RegeneWall {
  Vector pos;
  bool isBall;
  bool isAlive;
};
#define REGENE_MAX_WALL_COUNT 128
RegeneWall[REGENE_MAX_WALL_COUNT] regeneWalls;
int regeneWallIndex;

#define REGENE_MAX_REMOVED_COUNT 32
Vector[REGENE_MAX_REMOVED_COUNT] regeneRemovedWalls;
int regeneRemovedWallCount;
int regeneRemoveWallCount;
float regeneNextWallsDist;

struct RegeneBall {
  Vector pos;
  Vector vel;
  int hitCount;
  bool isAlive;
};
#define REGENE_MAX_BALL_COUNT 16
RegeneBall[REGENE_MAX_BALL_COUNT] regeneBalls;
int regeneBallIndex;

float regeneRandomBallAngle() {
  float a = rnd(0, CGLP_PI * 0.1) * RNDPM();
  if (rnd(0, 1) < 0.5) {
    a += CGLP_PI * 0.7;
  } else {
    a -= CGLP_PI * 0.7;
  }
  return a;
}

void regeneUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(regeneWalls);
    regeneWallIndex = 0;
    regeneRemovedWallCount = 0;
    regeneRemoveWallCount = 9;
    regeneNextWallsDist = 0;
    INIT_UNALIVED_ARRAY_FAST(regeneBalls);
    regeneBallIndex = 0;
    ASSIGN_ARRAY_ITEM(regeneBalls, regeneBallIndex, RegeneBall, b0);
    vectorSet(&b0->pos, 20, 50);
    vectorSet(&b0->vel, 1, 0);
    rotate(&b0->vel, regeneRandomBallAngle());
    b0->hitCount = 0;
    b0->isAlive = true;
    regeneBallIndex = cgl_wrap(regeneBallIndex + 1, 0, REGENE_MAX_BALL_COUNT);
  }
  color = LIGHT_BLUE;
  rect(0, 0, 100, 10, &scratch);
  rect(0, 90, 100, 10, &scratch);
  rect(0, 10, 10, 80, &scratch);
  COUNT_IS_ALIVE(regeneBalls, aliveBallCount);
  float scr = difficulty * 0.05 * sqrt(aliveBallCount);
  float ipx = clamp(input.pos.x, 0, 99);
  float ipy = clamp(input.pos.y, 0, 99);
  if (ipx > 90) {
    scr += (ipx - 90) * 0.05;
  }
  regeneNextWallsDist -= scr;
  if (regeneNextWallsDist < 0) {
    int w = rndi(3, 7);
    int h;
    if (rnd(0, 1) < 0.4) {
      h = 0;
    } else {
      h = rndi(2, 6);
    }
    int hy = rndi(h, 8 - h);
    TIMES(w, x) {
      TIMES(8, y) {
        if (y < hy || y >= hy + h) {
          ASSIGN_ARRAY_ITEM(regeneWalls, regeneWallIndex, RegeneWall, nw);
          vectorSet(&nw->pos, x * 10 + 105 - regeneNextWallsDist, y * 10 + 15);
          nw->isBall = rnd(0, 1) < 0.1 / sqrt(aliveBallCount);
          nw->isAlive = true;
          regeneWallIndex = cgl_wrap(regeneWallIndex + 1, 0, REGENE_MAX_WALL_COUNT);
        }
      }
    }
    regeneNextWallsDist += (w + rndi(0, 3)) * 10;
  }
  color = LIGHT_BLACK;
  box(ipx, ipy, 5, 5, &scratch);
  int ri = 0;
  while (ri < regeneRemovedWallCount) {
    regeneRemovedWalls[ri].x -= scr;
    if (regeneRemovedWalls[ri].x < 14) {
      memcpy(&regeneRemovedWalls[ri], &regeneRemovedWalls[ri + 1],
             (regeneRemovedWallCount - 1 - ri) * sizeof(regeneRemovedWalls[0]));
      regeneRemovedWallCount--;
    } else {
      ri++;
    }
  }
  FOR_EACH(regeneWalls, i) {
    ASSIGN_ARRAY_ITEM(regeneWalls, i, RegeneWall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.x -= scr;
    if (w->isBall) {
      color = LIGHT_RED;
    } else {
      color = PURPLE;
    }
    float wsize;
    if (w->isBall) {
      wsize = 6;
    } else {
      wsize = 8;
    }
    Collision wc;
    box(w->pos.x, w->pos.y, wsize, wsize, &wc);
    if (wc.isColliding.rect[LIGHT_BLACK]) {
      if (w->isBall) {
        if (w->pos.x < 97) {
          play(POWER_UP);
          ASSIGN_ARRAY_ITEM(regeneBalls, regeneBallIndex, RegeneBall, nb);
          nb->pos = w->pos;
          vectorSet(&nb->vel, 1, 0);
          rotate(&nb->vel, regeneRandomBallAngle());
          nb->hitCount = 0;
          nb->isAlive = true;
          regeneBallIndex = cgl_wrap(regeneBallIndex + 1, 0, REGENE_MAX_BALL_COUNT);
          w->isAlive = false;
          continue;
        }
      } else {
        play(LASER);
        regeneRemovedWalls[regeneRemovedWallCount] = w->pos;
        regeneRemovedWallCount++;
        w->isAlive = false;
        continue;
      }
    }
    w->isAlive = !wc.isColliding.rect[LIGHT_BLUE];
  }
  while (regeneRemovedWallCount > regeneRemoveWallCount) {
    ASSIGN_ARRAY_ITEM(regeneWalls, regeneWallIndex, RegeneWall, rw);
    rw->pos = regeneRemovedWalls[0];
    rw->isBall = false;
    rw->isAlive = true;
    regeneWallIndex = cgl_wrap(regeneWallIndex + 1, 0, REGENE_MAX_WALL_COUNT);
    memcpy(&regeneRemovedWalls[0], &regeneRemovedWalls[1],
           (regeneRemovedWallCount - 1) * sizeof(regeneRemovedWalls[0]));
    regeneRemovedWallCount--;
  }
  if (regeneRemovedWallCount >= regeneRemoveWallCount) {
    color = LIGHT_PURPLE;
    box(regeneRemovedWalls[0].x, regeneRemovedWalls[0].y, 8, 8, &scratch);
  }
  color = LIGHT_BLUE;
  rect(100, 10, 10, 80, &scratch);
  FOR_EACH(regeneBalls, i) {
    ASSIGN_ARRAY_ITEM(regeneBalls, i, RegeneBall, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x -= scr;
    color = TRANSPARENT;
    float spd = sqrt(difficulty);
    bool isHitX = false;
    bool isHitY = false;
    Collision cx;
    box(b->pos.x + b->vel.x * spd, b->pos.y, 6, 6, &cx);
    if (cx.isColliding.rect[PURPLE] || cx.isColliding.rect[LIGHT_BLUE]) {
      isHitX = true;
    }
    Collision cy;
    box(b->pos.x, b->pos.y + b->vel.y * spd, 6, 6, &cy);
    if (cy.isColliding.rect[PURPLE] || cy.isColliding.rect[LIGHT_BLUE]) {
      isHitY = true;
    }
    if (isHitX) {
      b->vel.x *= -1;
      b->pos.x += b->vel.x * spd * 2;
    }
    if (isHitY) {
      b->vel.y *= -1;
      b->pos.y += b->vel.y * spd * 2;
    }
    if (b->pos.x < 14) {
      b->pos.x = 14;
      if (b->vel.x < 0) {
        b->vel.x *= -1;
      }
    }
    color = RED;
    if (isHitX || isHitY) {
      if (b->hitCount == 0) {
        COUNT_IS_ALIVE(regeneBalls, currentAliveBallCount);
        addScore(currentAliveBallCount, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      }
      b->hitCount++;
      if (b->hitCount > 9) {
        particle(b->pos.x, b->pos.y, 16, 1, 0, CGLP_PI * 2);
        play(HIT);
        b->isAlive = false;
        continue;
      }
    } else {
      b->hitCount = 0;
    }
    b->pos.x += b->vel.x * spd;
    b->pos.y += b->vel.y * spd;
    box(b->pos.x, b->pos.y, 6, 6, &scratch);
  }
  COUNT_IS_ALIVE(regeneBalls, finalAliveBallCount);
  if (finalAliveBallCount == 0) {
    play(RANDOM);
    gameOver();
  }
}

void addGameRegene() {
  addGame(regeneTitle, regeneDescription, regeneCharacters,
          regeneCharactersCount, &regeneOptions, true, &regeneUpdate);
}
