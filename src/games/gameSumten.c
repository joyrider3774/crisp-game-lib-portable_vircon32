#include "../cglp.h"

int* sumtenTitle = "SUM TEN";
int* sumtenDescription = "[Tap] Forward";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] sumtenCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int sumtenCharactersCount = 0;

Options sumtenOptions = {150, 100, 8, false};

#define SUMTEN_GRID_COUNT 10
#define SUMTEN_GRID_SIZE 11

struct SumtenCell {
  int v;
  bool hasValue;
  bool isFilled;
};
SumtenCell[SUMTEN_GRID_COUNT][SUMTEN_GRID_COUNT] sumtenGrid;
Vector sumtenGridPos;

struct SumtenCursor {
  Vector pos;
  int angle;
  int ticks;
};
SumtenCursor sumtenCursor;

Vector[2] sumtenAngleOfs;

// Upstream can queue several completed "runs" faster than the 60-tick
// display/dismiss cycle drains them - sized generously; each run is capped
// at 10 numbers by the game's own rules (a longer run always ends the game).
#define SUMTEN_MAX_RUNS 16
#define SUMTEN_MAX_RUN_LEN 10
int[SUMTEN_MAX_RUNS][SUMTEN_MAX_RUN_LEN] sumtenNumbers;
int[SUMTEN_MAX_RUNS] sumtenNumbersLen;
int sumtenNumbersCount;
float sumtenShowingSumTicks;

void sumtenShiftNumbers() {
  TIMES(sumtenNumbersCount - 1, i) {
    memcpy(sumtenNumbers[i], sumtenNumbers[i + 1], sizeof(sumtenNumbers[0]));
    sumtenNumbersLen[i] = sumtenNumbersLen[i + 1];
  }
  sumtenNumbersCount--;
}

void sumtenUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - every position
  // check is direct sumtenGrid[x][y] indexing, so the engine's own O(n^2)
  // hitbox scan (see checkHitBox() in cglp.c) is pure waste here.
  // Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    TIMES(SUMTEN_GRID_COUNT, gx) {
      TIMES(SUMTEN_GRID_COUNT, gy) {
        bool iw = gx > 6 || (gx > 0 && gy != 6);
        sumtenGrid[gx][gy].isFilled = iw;
        if (iw) {
          sumtenGrid[gx][gy].hasValue = false;
          sumtenGrid[gx][gy].v = 0;
        } else {
          sumtenGrid[gx][gy].hasValue = true;
          sumtenGrid[gx][gy].v = rndi(1, 10);
        }
      }
    }
    int[6] initialLineValues = {6, 5, 8, 1, 7, 3};
    TIMES(6, li) {
      sumtenGrid[1 + li][6].v = initialLineValues[li];
      sumtenGrid[1 + li][6].hasValue = true;
    }
    vectorSet(&sumtenGridPos, 100, 0);
    vectorSet(&sumtenCursor.pos, 7, 6);
    sumtenCursor.angle = 0;
    sumtenCursor.ticks = 0;
    sumtenNumbersCount = 1;
    sumtenNumbersLen[0] = 0;
    sumtenShowingSumTicks = 0;
    vectorSet(&sumtenAngleOfs[0], -1, 0);
    vectorSet(&sumtenAngleOfs[1], 0, -1);
  }
  Vector np;
  sumtenCursor.ticks++;
  if (sumtenCursor.ticks >= 30) {
    int pa = sumtenCursor.angle;
    sumtenCursor.angle = (int)cgl_wrap(sumtenCursor.angle + 1, 0, 2);
    vectorSet(&np, sumtenCursor.pos.x, sumtenCursor.pos.y);
    vectorAdd(&np, sumtenAngleOfs[sumtenCursor.angle].x, sumtenAngleOfs[sumtenCursor.angle].y);
    if (!sumtenGrid[(int)np.x][(int)np.y].hasValue) {
      sumtenCursor.angle = pa;
    }
    sumtenCursor.ticks = 0;
  }
  vectorSet(&np, sumtenCursor.pos.x, sumtenCursor.pos.y);
  vectorAdd(&np, sumtenAngleOfs[sumtenCursor.angle].x, sumtenAngleOfs[sumtenCursor.angle].y);
  TIMES(SUMTEN_GRID_COUNT, gx2) {
    TIMES(SUMTEN_GRID_COUNT, gy2) {
      SumtenCell* g = &sumtenGrid[gx2][gy2];
      int c;
      if (gx2 == (int)sumtenCursor.pos.x && gy2 == (int)sumtenCursor.pos.y) {
        c = CYAN;
      } else if (gx2 == (int)np.x && gy2 == (int)np.y) {
        c = RED;
      } else {
        c = BLUE;
      }
      color = c;
      float rx = sumtenGridPos.x - gx2 * SUMTEN_GRID_SIZE;
      float ry = sumtenGridPos.y + gy2 * SUMTEN_GRID_SIZE + 1;
      rect(rx, ry, SUMTEN_GRID_SIZE - 1, SUMTEN_GRID_SIZE - 1, &scratch);
      color = WHITE;
      if (!g->isFilled) {
        rect(rx + 1, ry + 1, SUMTEN_GRID_SIZE - 3, SUMTEN_GRID_SIZE - 3, &scratch);
        color = c;
      }
      if (g->hasValue) {
        text(intToChar(g->v), round(sumtenGridPos.x - (gx2 - 0.5) * SUMTEN_GRID_SIZE) - 1,
             round(sumtenGridPos.y + (gy2 + 0.5) * SUMTEN_GRID_SIZE), &scratch);
      }
      if (c == CYAN && (rx < 0 || ry > 91)) {
        play(EXPLOSION);
        gameOver();
      }
    }
  }
  color = WHITE;
  rect(0, 0, 100, 10, &scratch);
  rect(100, 0, 50, 100, &scratch);
  if (input.isJustPressed) {
    play(SELECT);
    sumtenCursor.pos = np;
    sumtenGrid[(int)np.x][(int)np.y].isFilled = true;
    sumtenCursor.ticks = 0;
    int curRun = sumtenNumbersCount - 1;
    int v = sumtenGrid[(int)np.x][(int)np.y].v;
    sumtenNumbers[curRun][sumtenNumbersLen[curRun]] = v;
    sumtenNumbersLen[curRun]++;
    int s = 0;
    TIMES(sumtenNumbersLen[curRun], vi) { s += sumtenNumbers[curRun][vi]; }
    if (s % 10 == 0) {
      play(POWER_UP);
      if (sumtenNumbersCount < SUMTEN_MAX_RUNS) {
        sumtenNumbersCount++;
        sumtenNumbersLen[sumtenNumbersCount - 1] = 0;
      }
      int sc = s / 10;
      addScore(sc * sc * 10, 133, 20 + sumtenNumbersLen[curRun] * 8);
      if (sumtenShowingSumTicks <= 0) {
        sumtenShowingSumTicks = 60;
      }
    } else if (sumtenNumbersLen[curRun] >= 10) {
      sumtenShowingSumTicks = 60;
      memcpy(sumtenNumbers[0], sumtenNumbers[curRun], sizeof(sumtenNumbers[0]));
      sumtenNumbersLen[0] = sumtenNumbersLen[curRun];
      sumtenNumbersCount = 1;
      play(EXPLOSION);
      gameOver();
    }
    vectorSet(&np, sumtenCursor.pos.x, sumtenCursor.pos.y);
    vectorAdd(&np, sumtenAngleOfs[sumtenCursor.angle].x, sumtenAngleOfs[sumtenCursor.angle].y);
    if (!sumtenGrid[(int)np.x][(int)np.y].hasValue) {
      sumtenCursor.angle = (int)cgl_wrap(sumtenCursor.angle + 1, 0, 2);
    }
  }
  float sum = 0;
  float y = 12;
  color = BLACK;
  TIMES(sumtenNumbersLen[0], ni) {
    text(intToChar(sumtenNumbers[0][ni]), 135, y, &scratch);
    sum += sumtenNumbers[0][ni];
    y += 8;
  }
  if (sum > 0) {
    text("+)", 110, y - 8, &scratch);
    rect(115, y - 5, 30, 1, &scratch);
  }
  if (sumtenShowingSumTicks > 0) {
    text(intToChar((int)sum), 135 - 6, y, &scratch);
    sumtenShowingSumTicks--;
    if (sumtenShowingSumTicks == 0) {
      sumtenShiftNumbers();
      if (sumtenNumbersCount > 1) {
        sumtenShowingSumTicks = 60;
      }
    }
  }
  if (sumtenCursor.pos.x < 7) {
    sumtenGridPos.x -= (7 - sumtenCursor.pos.x) * 0.5;
  }
  if (sumtenCursor.pos.y < 6) {
    sumtenGridPos.y += (6 - sumtenCursor.pos.y) * 0.5;
  }
  float sspeed = sqrt(difficulty) * 0.01;
  vectorAdd(&sumtenGridPos, -sspeed, sspeed);
  if (sumtenGridPos.x <= 90) {
    for (int gx3 = 9; gx3 >= 0; gx3--) {
      int wi = rndi(1, 15);
      TIMES(10, gy3) {
        if (gx3 > 0) {
          sumtenGrid[gx3][gy3] = sumtenGrid[gx3 - 1][gy3];
        } else {
          bool isFilled = (gy3 == wi) && !sumtenGrid[gx3 + 1][gy3 - 1].isFilled;
          sumtenGrid[gx3][gy3].isFilled = isFilled;
          if (isFilled) {
            sumtenGrid[gx3][gy3].hasValue = false;
            sumtenGrid[gx3][gy3].v = 0;
          } else {
            sumtenGrid[gx3][gy3].hasValue = true;
            sumtenGrid[gx3][gy3].v = rndi(1, 10);
          }
        }
      }
    }
    sumtenGridPos.x += 10;
    sumtenCursor.pos.x++;
  }
  if (sumtenGridPos.y >= 10) {
    for (int gy4 = 9; gy4 >= 0; gy4--) {
      int wi2 = rndi(1, 15);
      TIMES(10, gx4) {
        if (gy4 > 0) {
          sumtenGrid[gx4][gy4] = sumtenGrid[gx4][gy4 - 1];
        } else {
          bool isFilled2 = (gx4 == wi2) && !sumtenGrid[gx4 - 1][gy4 + 1].isFilled;
          sumtenGrid[gx4][gy4].isFilled = isFilled2;
          if (isFilled2) {
            sumtenGrid[gx4][gy4].hasValue = false;
            sumtenGrid[gx4][gy4].v = 0;
          } else {
            sumtenGrid[gx4][gy4].hasValue = true;
            sumtenGrid[gx4][gy4].v = rndi(1, 10);
          }
        }
      }
    }
    sumtenGridPos.y -= 10;
    sumtenCursor.pos.y++;
  }
}

void addGameSumten() {
  addGame(sumtenTitle, sumtenDescription, sumtenCharacters, sumtenCharactersCount, &sumtenOptions,
          false, &sumtenUpdate);
}
