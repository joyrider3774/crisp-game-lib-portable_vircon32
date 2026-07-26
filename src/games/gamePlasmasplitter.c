#include "../cglp.h"

int* plasmasplitterTitle = "PLASMA SPLITTER";
int* plasmasplitterDescription = "[Hold] Stop";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] plasmasplitterCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int plasmasplitterCharactersCount = 0;

Options plasmasplitterOptions = {100, 100, 50, false};

#define PLASMASPLITTER_SCROLLING_BASE_SPEED 0.2

struct PlasmasplitterPlayer {
  Vector pos;
  float size;
  bool isSplit;
  float splitPos;
  float vx;
};
PlasmasplitterPlayer plasmasplitterPlayer;

struct PlasmasplitterGap {
  float start;
  float end;
};
struct PlasmasplitterWall {
  Vector pos;
  PlasmasplitterGap[3] gaps;
  int gapCount;
  bool isAlive;
};
#define PLASMASPLITTER_MAX_WALL_COUNT 16
PlasmasplitterWall[PLASMASPLITTER_MAX_WALL_COUNT] plasmasplitterWalls;
int plasmasplitterWallIndex;
float plasmasplitterNextWallDist;
float plasmasplitterScrollingY;

int[6] plasmasplitterGapCountTable = {1, 2, 2, 3, 3, 3};

void plasmasplitterDrawLightning(float x1, float y1, float x2, float y2, int divisionCount) {
  Collision scratch;
  if (divisionCount == 0) {
    thickness = 2;
    line(x1, y1, x2, y2, &scratch);
    return;
  }
  float midX = (x1 + x2) / 2 + rnd(0, 3) * RNDPM();
  float midY = (y1 + y2) / 2 + rnd(0, 3) * RNDPM();
  plasmasplitterDrawLightning(x1, y1, midX, midY, divisionCount - 1);
  plasmasplitterDrawLightning(midX, midY, x2, y2, divisionCount - 1);
}

void plasmasplitterUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&plasmasplitterPlayer.pos, 50, 96);
    plasmasplitterPlayer.size = 8;
    plasmasplitterPlayer.isSplit = false;
    plasmasplitterPlayer.splitPos = 0;
    plasmasplitterPlayer.vx = 1;
    INIT_UNALIVED_ARRAY_FAST(plasmasplitterWalls);
    plasmasplitterWallIndex = 0;
    plasmasplitterNextWallDist = 0;
    plasmasplitterScrollingY = PLASMASPLITTER_SCROLLING_BASE_SPEED;
  }
  float sd = sqrt(difficulty);
  if (input.isPressed) {
    if (input.isJustPressed) {
      play(LASER);
    }
    plasmasplitterScrollingY += sd * 0.01;
  } else {
    if (input.isJustReleased) {
      play(HIT);
      plasmasplitterPlayer.splitPos = plasmasplitterPlayer.pos.x;
    }
    plasmasplitterScrollingY +=
        (sd * PLASMASPLITTER_SCROLLING_BASE_SPEED - plasmasplitterScrollingY) * 0.1;
    plasmasplitterPlayer.pos.x += plasmasplitterPlayer.vx * sd;
    if (plasmasplitterPlayer.pos.x < plasmasplitterPlayer.size / 2 ||
        plasmasplitterPlayer.pos.x > 100 - plasmasplitterPlayer.size / 2) {
      plasmasplitterPlayer.vx *= -1;
    }
  }
  color = LIGHT_BLUE;
  rect(plasmasplitterPlayer.pos.x, plasmasplitterPlayer.pos.y - 2,
       plasmasplitterPlayer.splitPos - plasmasplitterPlayer.pos.x, 2, &scratch);
  color = YELLOW;
  TIMES(2, k) {
    plasmasplitterDrawLightning(plasmasplitterPlayer.pos.x, plasmasplitterPlayer.pos.y,
                                 plasmasplitterPlayer.splitPos, plasmasplitterPlayer.pos.y, 2);
  }
  color = CYAN;
  box(plasmasplitterPlayer.splitPos, plasmasplitterPlayer.pos.y, plasmasplitterPlayer.size,
      plasmasplitterPlayer.size, &scratch);
  box(plasmasplitterPlayer.pos.x, plasmasplitterPlayer.pos.y, plasmasplitterPlayer.size,
      plasmasplitterPlayer.size, &scratch);
  plasmasplitterNextWallDist -= plasmasplitterScrollingY;
  if (plasmasplitterNextWallDist < 0) {
    int gapCount = plasmasplitterGapCountTable[rndi(0, 6)];
    ASSIGN_ARRAY_ITEM(plasmasplitterWalls, plasmasplitterWallIndex, PlasmasplitterWall, nw);
    vectorSet(&nw->pos, 0, -3);
    nw->gapCount = gapCount;
    float gx = 0;
    TIMES(gapCount, i) {
      float gw = plasmasplitterPlayer.size * 3;
      float lo;
      if (i > 0) {
        lo = 10;
      } else {
        lo = 0;
      }
      gx += rnd(lo, 100 - gx - gw - (gapCount - i - 1) * (gw + 10));
      nw->gaps[i].start = gx;
      nw->gaps[i].end = gx + gw;
      gx += gw;
    }
    nw->isAlive = true;
    plasmasplitterWallIndex =
        cgl_wrap(plasmasplitterWallIndex + 1, 0, PLASMASPLITTER_MAX_WALL_COUNT);
    plasmasplitterNextWallDist = rnd(50, 80);
  }
  color = BLUE;
  FOR_EACH(plasmasplitterWalls, i) {
    ASSIGN_ARRAY_ITEM(plasmasplitterWalls, i, PlasmasplitterWall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.y += plasmasplitterScrollingY;
    float wx = 0;
    bool isDestroyed = false;
    TIMES(w->gapCount, k) {
      PlasmasplitterGap* g = &w->gaps[k];
      rect(wx, w->pos.y, g->start - wx, 3, &scratch);
      if (scratch.isColliding.rect[CYAN]) {
        play(EXPLOSION);
        gameOver();
      } else if (scratch.isColliding.rect[LIGHT_BLUE]) {
        isDestroyed = true;
        float dw = g->start - wx;
        float dx = wx + dw / 2;
        play(POWER_UP);
        particle(dx, w->pos.y, 9, 2, 0, CGLP_PI * 2);
        addScore(floor(plasmasplitterScrollingY / PLASMASPLITTER_SCROLLING_BASE_SPEED * dw), dx,
                 w->pos.y);
      }
      wx = g->end;
    }
    rect(wx, w->pos.y, 100 - wx, 3, &scratch);
    if (scratch.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    }
    if (isDestroyed) {
      w->isAlive = false;
      continue;
    }
    if (w->pos.y > 103) {
      w->isAlive = false;
      continue;
    }
  }
}

void addGamePlasmasplitter() {
  addGame(plasmasplitterTitle, plasmasplitterDescription, plasmasplitterCharacters,
          plasmasplitterCharactersCount, &plasmasplitterOptions, false,
          &plasmasplitterUpdate);
}
