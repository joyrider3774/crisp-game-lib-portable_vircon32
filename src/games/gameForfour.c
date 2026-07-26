#include "../cglp.h"

int* forfourTitle = "FORFOUR";
int* forfourDescription = "[Tap]\n Roll";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] forfourCharacters = {{
    " llll ",
    "llllll",
    "ll  ll",
    "ll  ll",
    "llllll",
    " llll ",
}};
int forfourCharactersCount = 1;

Options forfourOptions = {100, 100, 2, false};

#define FORFOUR_CIRCLE_COUNT 13
#define FORFOUR_CC 6
#define FORFOUR_CIRCLES_OFS_X 12
#define FORFOUR_CIRCLES_OFS_Y 18

int[5] forfourColors = {BLACK, RED, GREEN, BLUE, YELLOW};
int[5] forfourLightColors = {LIGHT_BLACK, LIGHT_RED, LIGHT_GREEN, LIGHT_BLUE, LIGHT_YELLOW};

int[FORFOUR_CIRCLE_COUNT][FORFOUR_CIRCLE_COUNT] forfourCircles;
int[FORFOUR_CIRCLE_COUNT][FORFOUR_CIRCLE_COUNT] forfourTmpCircles;
bool[FORFOUR_CIRCLE_COUNT][FORFOUR_CIRCLE_COUNT] forfourIc;

Vector forfourFlying;
int forfourFlyingCount;
int forfourFlyingAngle;
int forfourFlyingColor;

int forfourCheckSize() {
  int minX = FORFOUR_CIRCLE_COUNT - 1;
  int maxX = 0;
  int minY = FORFOUR_CIRCLE_COUNT - 1;
  int maxY = 0;
  TIMES(FORFOUR_CIRCLE_COUNT, y) {
    TIMES(FORFOUR_CIRCLE_COUNT, x) {
      if (forfourCircles[x][y] >= 0) {
        if (minX > x) {
          minX = x;
        }
        if (maxX < x) {
          maxX = x;
        }
        if (minY > y) {
          minY = y;
        }
        if (maxY < y) {
          maxY = y;
        }
      }
    }
  }
  minX = FORFOUR_CC - minX + 1;
  maxX = maxX - FORFOUR_CC + 1;
  minY = FORFOUR_CC - minY + 1;
  maxY = maxY - FORFOUR_CC + 1;
  int m = minX;
  if (maxX > m) {
    m = maxX;
  }
  if (minY > m) {
    m = minY;
  }
  if (maxY > m) {
    m = maxY;
  }
  return m;
}

// Fills the global forfourIc[][] grid with the connected component reachable
// from (px, py); cl > 0 restricts the match to that color index, cl < 0
// matches any non-empty cell. Returns the component's cell count.
int forfourCheckConnection(int px, int py, int cl) {
  int c = 1;
  TIMES(FORFOUR_CIRCLE_COUNT, x) { TIMES(FORFOUR_CIRCLE_COUNT, y) { forfourIc[x][y] = false; } }
  forfourIc[px][py] = true;
  TIMES(9, iter) {
    int pc = c;
    for (int y = 0; y < FORFOUR_CIRCLE_COUNT - 1; y++) {
      for (int x = 0; x < FORFOUR_CIRCLE_COUNT - 1; x++) {
        if (!forfourIc[x][y]) {
          continue;
        }
        bool matchRight;
        if (cl > 0) {
          matchRight = forfourCircles[x + 1][y] == cl;
        } else {
          matchRight = forfourCircles[x + 1][y] >= 0;
        }
        if (matchRight && !forfourIc[x + 1][y]) {
          forfourIc[x + 1][y] = true;
          c++;
        }
        bool matchDown;
        if (cl > 0) {
          matchDown = forfourCircles[x][y + 1] == cl;
        } else {
          matchDown = forfourCircles[x][y + 1] >= 0;
        }
        if (matchDown && !forfourIc[x][y + 1]) {
          forfourIc[x][y + 1] = true;
          c++;
        }
      }
    }
    for (int y = FORFOUR_CIRCLE_COUNT - 1; y > 0; y--) {
      for (int x = FORFOUR_CIRCLE_COUNT - 1; x > 0; x--) {
        if (!forfourIc[x][y]) {
          continue;
        }
        bool matchLeft;
        if (cl > 0) {
          matchLeft = forfourCircles[x - 1][y] == cl;
        } else {
          matchLeft = forfourCircles[x - 1][y] >= 0;
        }
        if (matchLeft && !forfourIc[x - 1][y]) {
          forfourIc[x - 1][y] = true;
          c++;
        }
        bool matchUp;
        if (cl > 0) {
          matchUp = forfourCircles[x][y - 1] == cl;
        } else {
          matchUp = forfourCircles[x][y - 1] >= 0;
        }
        if (matchUp && !forfourIc[x][y - 1]) {
          forfourIc[x][y - 1] = true;
          c++;
        }
      }
    }
    if (pc == c) {
      break;
    }
  }
  return c;
}

void forfourUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - every hit/
  // placement check is direct forfourCircles[][] grid indexing, so the
  // engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure
  // waste here. Restored automatically when the next real game starts,
  // via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    TIMES(FORFOUR_CIRCLE_COUNT, x) {
      TIMES(FORFOUR_CIRCLE_COUNT, y) {
        forfourCircles[x][y] = -1;
        forfourTmpCircles[x][y] = -1;
      }
    }
    forfourCircles[FORFOUR_CC][FORFOUR_CC] = 0;
    for (int i = FORFOUR_CC + 1; i < FORFOUR_CIRCLE_COUNT - 1; i++) {
      forfourCircles[i][FORFOUR_CC] = rndi(1, 5);
    }
    vectorSet(&forfourFlying, 0, 0);
    forfourFlyingCount = -999;
  }
  if (forfourFlyingCount < -900) {
    forfourFlyingCount = (int)(60 / difficulty);
    forfourFlyingAngle = rndi(0, 4);
    forfourFlyingColor = rndi(1, 5);
    int sz = forfourCheckSize();
    if (rnd(0, 1) < sqrt(sz) * 0.1) {
      forfourFlyingColor = 0;
      sz = (int)ceil(sz * 0.7);
    }
    int minV = FORFOUR_CC - sz + 1;
    int maxV = FORFOUR_CC + sz - 1;
    if (forfourFlyingAngle == 0) {
      vectorSet(&forfourFlying, -1, rndi(minV, maxV + 1));
    } else if (forfourFlyingAngle == 1) {
      vectorSet(&forfourFlying, rndi(minV, maxV + 1), -1);
    } else if (forfourFlyingAngle == 2) {
      vectorSet(&forfourFlying, FORFOUR_CIRCLE_COUNT, rndi(minV, maxV + 1));
    } else if (forfourFlyingAngle == 3) {
      vectorSet(&forfourFlying, rndi(minV, maxV + 1), FORFOUR_CIRCLE_COUNT);
    }
    vectorSet(&forfourFlying, forfourFlying.x * 6 + FORFOUR_CIRCLES_OFS_X,
              forfourFlying.y * 6 + FORFOUR_CIRCLES_OFS_Y);
  }
  forfourFlyingCount--;
  color = forfourLightColors[forfourFlyingColor];
  if (forfourFlyingAngle % 2 == 0) {
    rect(0, forfourFlying.y - 1, 99, 2, &scratch);
  } else {
    rect(forfourFlying.x - 1, 0, 2, 99, &scratch);
  }
  if (forfourFlyingCount < 0) {
    addWithAngle(&forfourFlying, (forfourFlyingAngle * CGLP_PI) / 2, difficulty);
    Vector c;
    vectorSet(&c, round((forfourFlying.x - FORFOUR_CIRCLES_OFS_X) / 6),
              round((forfourFlying.y - FORFOUR_CIRCLES_OFS_Y) / 6));
    bool inRectC = c.x >= 0 && c.x < FORFOUR_CIRCLE_COUNT && c.y >= 0 && c.y < FORFOUR_CIRCLE_COUNT;
    if (inRectC && forfourCircles[(int)c.x][(int)c.y] >= 0) {
      if (forfourFlyingColor == 0) {
        forfourFlyingColor = forfourCircles[(int)c.x][(int)c.y];
        if (forfourFlyingColor == 0) {
          forfourFlyingColor = rndi(1, 5);
        }
      }
      TIMES(99, i) {
        addWithAngle(&c, (forfourFlyingAngle * CGLP_PI) / 2 + CGLP_PI, 1);
        c.x = round(c.x);
        c.y = round(c.y);
        bool cInRect =
            c.x >= 0 && c.x < FORFOUR_CIRCLE_COUNT && c.y >= 0 && c.y < FORFOUR_CIRCLE_COUNT;
        if (!cInRect) {
          play(EXPLOSION);
          color = RED;
          text("X", FORFOUR_CIRCLES_OFS_X + c.x * 6, FORFOUR_CIRCLES_OFS_Y + c.y * 6, &scratch);
          gameOver();
          break;
        }
        if (forfourCircles[(int)c.x][(int)c.y] < 0) {
          play(LASER);
          forfourCircles[(int)c.x][(int)c.y] = forfourFlyingColor;
          forfourFlyingCount = -999;
          int cnt = forfourCheckConnection((int)c.x, (int)c.y, forfourFlyingColor);
          if (cnt >= 4) {
            play(COIN);
            int dcc = 0;
            TIMES(FORFOUR_CIRCLE_COUNT, y2) {
              TIMES(FORFOUR_CIRCLE_COUNT, x2) {
                if (forfourIc[x2][y2]) {
                  color = forfourColors[forfourCircles[x2][y2]];
                  particle(FORFOUR_CIRCLES_OFS_X + x2 * 6, FORFOUR_CIRCLES_OFS_Y + y2 * 6, 4, 1, 0,
                           CGLP_PI * 2);
                  forfourCircles[x2][y2] = -1;
                  dcc++;
                }
              }
            }
            forfourCheckConnection(FORFOUR_CC, FORFOUR_CC, -1);
            TIMES(FORFOUR_CIRCLE_COUNT, y3) {
              TIMES(FORFOUR_CIRCLE_COUNT, x3) {
                if (!forfourIc[x3][y3] && forfourCircles[x3][y3] > 0) {
                  color = forfourLightColors[forfourCircles[x3][y3]];
                  particle(FORFOUR_CIRCLES_OFS_X + x3 * 6, FORFOUR_CIRCLES_OFS_Y + y3 * 6, 4, 1, 0,
                           CGLP_PI * 2);
                  forfourCircles[x3][y3] = -1;
                  dcc++;
                }
              }
            }
            dcc -= 3;
            addScore(dcc * dcc, forfourFlying.x, forfourFlying.y);
          }
          break;
        }
      }
    }
  }
  bool flyingInRect = forfourFlying.x >= -3 && forfourFlying.x < 103 && forfourFlying.y >= -3 &&
                       forfourFlying.y < 103;
  if (!flyingInRect) {
    forfourFlyingCount = -999;
  }
  color = forfourColors[forfourFlyingColor];
  character("a", forfourFlying.x, forfourFlying.y, &scratch);
  if (input.isJustPressed) {
    play(SELECT);
    TIMES(FORFOUR_CIRCLE_COUNT, y) {
      TIMES(FORFOUR_CIRCLE_COUNT, x) { forfourTmpCircles[x][y] = forfourCircles[x][y]; }
    }
    TIMES(FORFOUR_CIRCLE_COUNT, y) {
      TIMES(FORFOUR_CIRCLE_COUNT, x) {
        forfourCircles[FORFOUR_CIRCLE_COUNT - 1 - y][x] = forfourTmpCircles[x][y];
      }
    }
  }
  TIMES(FORFOUR_CIRCLE_COUNT, y) {
    TIMES(FORFOUR_CIRCLE_COUNT, x) {
      int c = forfourCircles[x][y];
      if (c >= 0) {
        color = forfourColors[c];
        character("a", FORFOUR_CIRCLES_OFS_X + x * 6, FORFOUR_CIRCLES_OFS_Y + y * 6, &scratch);
      }
    }
  }
}

void addGameForfour() {
  addGame(forfourTitle, forfourDescription, forfourCharacters,
          forfourCharactersCount, &forfourOptions, false, &forfourUpdate);
}
