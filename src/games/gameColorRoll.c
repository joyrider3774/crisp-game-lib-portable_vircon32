#include "../cglp.h"

char* colorrollTitle = "COLOR ROLL";
char* colorrollDescription = "[Tap] Shoot";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] colorrollCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int colorrollCharactersCount = 1;

Options colorrollOptions = {100, 100, 5, false};

struct ColorrollBar {
  float width;
  int color;
};

struct ColorrollLane {
  float x;
  float y;
  float vx;
  ColorrollBar[8] bars;
  int barCount;
};
#define COLOR_ROLL_MAX_LANE_COUNT 32
ColorrollLane[COLOR_ROLL_MAX_LANE_COUNT] colorrollLanes;
int colorrollLaneIndex;
float colorrollLaneY;
float colorrollShotY;
int colorrollHitColor;
int colorrollLaneCount;
int colorrollBaseMultiplier;
int colorrollMultiplier;
float colorrollPenalty;
#define COLOR_ROLL_BAR_COLOR_COUNT 4
int[COLOR_ROLL_BAR_COLOR_COUNT] colorrollBarColors = {RED, GREEN, BLUE, YELLOW};
float colorrollLaneHeight = 7;

int colorrollAddBars(ColorrollBar* bars, ColorrollBar* prevBars, int prevBarsCount) {
  int[9] cs;
  int csCount;
  if (prevBarsCount == 0) {
    cs[0] = colorrollBarColors[rndi(0, COLOR_ROLL_BAR_COLOR_COUNT)];
    csCount = 1;
  } else {
    TIMES(prevBarsCount, i) { cs[i] = prevBars[i].color; }
    csCount = prevBarsCount;
  }
  if (csCount == 1 || (csCount < 4 && rnd(0, 1) < 0.5)) {
    cs[csCount] = colorrollBarColors[rndi(0, COLOR_ROLL_BAR_COLOR_COUNT)];
    csCount++;
  } else {
    int ri = rndi(0, csCount);
    csCount--;
    memcpy(&cs[ri], &cs[ri + 1], (csCount - ri) * sizeof(cs[0]));
  }
  float lx = 99;
  float x = rnd(0, 99);
  TIMES(csCount, i) {
    ASSIGN_ARRAY_ITEM(bars, i, ColorrollBar, b);
    float width;
    if (i == csCount - 1) {
      width = lx;
    } else {
      width = (99 / csCount) * rnd(0.8, 1.2);
    }
    lx -= width;
    x = cgl_wrap(x + width, 0, 99);
    b->width = width;
    b->color = cs[i];
  }
  return csCount;
}

ColorrollBar* colorrollPrevBars;
int colorrollPrevBarsCount;

void colorrollAddLane() {
  play(SELECT);
  float x = rnd(0, 99);
  float vx = rnd(0.5, 1) * RNDPM() * sqrt(difficulty);
  ASSIGN_ARRAY_ITEM(colorrollLanes, colorrollLaneIndex, ColorrollLane, l);
  l->x = x;
  l->vx = vx;
  if (colorrollLaneIndex == 0) {
    colorrollLaneY = 0;
    l->y = 0;
    colorrollPrevBarsCount = l->barCount = colorrollAddBars(l->bars, NULL, 0);
  } else {
    l->y = -colorrollLaneIndex * colorrollLaneHeight;
    colorrollPrevBarsCount = l->barCount = colorrollAddBars(l->bars, colorrollPrevBars, colorrollPrevBarsCount);
  }
  colorrollPrevBars = l->bars;
  colorrollLaneIndex++;
}

void colorrollUpdate() {
  Collision scratch;
  if (!ticks) {
    colorrollLaneIndex = 0;
    colorrollShotY = -999;
    colorrollHitColor = -999;
    colorrollLaneCount = 2;
    TIMES(colorrollLaneCount, i) { colorrollAddLane(); }
    colorrollBaseMultiplier = 0;
    colorrollMultiplier = 1;
    colorrollPenalty = 1;
  }
  float sy = 97;
  if (colorrollShotY != -999) {
    colorrollShotY -= sqrt(difficulty) * 3;
    sy = colorrollShotY;
  } else {
    colorrollHitColor = -999;
    if (input.isJustPressed) {
      play(LASER);
      colorrollMultiplier = 1;
      colorrollShotY = sy;
      colorrollLaneY += 2 * colorrollPenalty * sqrt(sqrt(difficulty));
    }
  }
  if (colorrollHitColor == -999) {
    color = BLACK;
  } else {
    color = colorrollHitColor;
  }
  rect(49, sy, 3, 99 - sy, &scratch);
  float my = colorrollLaneHeight * colorrollLaneCount;
  colorrollLaneY += sqrt(difficulty) * 0.005;
  if (colorrollLaneY < my) {
    colorrollLaneY += (my - colorrollLaneY) * 0.2;
  }
  float ly = colorrollLaneY;
  float maxY = 0;
  int li = 0;
  TIMES(colorrollLaneIndex, i) {
    if (li != i) {
      colorrollLanes[li] = colorrollLanes[i];
    }
    ASSIGN_ARRAY_ITEM(colorrollLanes, li, ColorrollLane, l);
    l->x = cgl_wrap(l->x + l->vx, 0, 99);
    l->y += (ly - l->y) * 0.2;
    float x = l->x;
    bool isRemoved = false;
    bool isShotRemoved = false;
    TIMES(l->barCount, j) {
      ASSIGN_ARRAY_ITEM(l->bars, j, ColorrollBar, b);
      color = b->color;
      Collision cl1;
      bool* c;
      if (x + b->width < 99) {
        rect(x, l->y, b->width - 1, -colorrollLaneHeight + 1, &cl1);
        c = cl1.isColliding.rect;
      } else {
        rect(x, l->y, 99 - x, -colorrollLaneHeight + 1, &cl1);
        c = cl1.isColliding.rect;
        Collision cl2;
        rect(0, l->y, b->width - (99 - x) - 1, -colorrollLaneHeight + 1, &cl2);
        bool* c2 = cl2.isColliding.rect;
        c[BLACK] |= c2[BLACK];
        TIMES(COLOR_ROLL_BAR_COLOR_COUNT, k) {
          int ci = colorrollBarColors[k];
          c[ci] |= c2[ci];
        }
      }
      if (c[BLACK]) {
        colorrollHitColor = b->color;
        isRemoved = true;
      } else if (colorrollHitColor != -999) {
        if (c[b->color]) {
          isRemoved = true;
        } else if (c[RED] || c[GREEN] || c[BLUE] || c[YELLOW]) {
          isShotRemoved = true;
        }
      }
      x = cgl_wrap(x + b->width, 0, 99);
    }
    ly -= colorrollLaneHeight;
    if (isShotRemoved) {
      play(HIT);
      colorrollShotY = -999;
      colorrollPenalty = clamp(colorrollPenalty * (3 / colorrollMultiplier), 1, 4);
    } else if (isRemoved) {
      play(COIN);
      addScore(colorrollMultiplier * pow(2, colorrollBaseMultiplier), 50, l->y);
      colorrollLaneY -= colorrollMultiplier;
      colorrollMultiplier *= 2;
      li--;
    }
    li++;
    if (l->y > maxY) {
      maxY = l->y;
    }
  }
  colorrollLaneIndex = li;
  if (colorrollLaneIndex == 0) {
    play(POWER_UP);
    colorrollShotY = -999;
    colorrollLaneCount++;
    if (colorrollLaneCount > clamp(5 + colorrollBaseMultiplier, 1, 10)) {
      colorrollBaseMultiplier = clamp(colorrollBaseMultiplier + 1, 1, 9);
      colorrollLaneCount = 2;
    }
  }
  if (colorrollShotY == -999) {
    TIMES(colorrollLaneCount - colorrollLaneIndex, i) { colorrollAddLane(); }
    if (maxY > 97) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void addGameColorRoll() {
  addGame(colorrollTitle, colorrollDescription, colorrollCharacters,
          colorrollCharactersCount, &colorrollOptions, false, &colorrollUpdate);
}
