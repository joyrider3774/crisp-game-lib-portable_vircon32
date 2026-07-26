#include "../cglp.h"

int* ttfenceTitle = "TT FENCE";
int* ttfenceDescription = "[Tap] Place";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] ttfenceCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int ttfenceCharactersCount = 0;

Options ttfenceOptions = {100, 100, 1, false};

#define TTFENCE_GRID_SIZE 15
bool[TTFENCE_GRID_SIZE][TTFENCE_GRID_SIZE] ttfenceGrid;
bool[TTFENCE_GRID_SIZE][TTFENCE_GRID_SIZE] ttfenceTmpGrid;
bool[TTFENCE_GRID_SIZE][TTFENCE_GRID_SIZE] ttfenceBombEdgeGrid;
bool[TTFENCE_GRID_SIZE][TTFENCE_GRID_SIZE] ttfenceBombAnimGrid;

#define TTFENCE_BLOCK_PATTERN_COUNT 7
int[TTFENCE_BLOCK_PATTERN_COUNT][3] ttfenceBlockPatterns = {
    {0, 0, 0}, {0, 1, 0}, {0, 3, 0}, {0, 0, 1}, {0, 0, 3}, {1, 0, 0}, {3, 0, 0},
};
int[4][2] ttfenceAngleOfs = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

Vector ttfencePos;
int ttfenceAngle;
int ttfencePrevAngle;
int ttfenceType;
float ttfenceBombAnimTicks;
float ttfenceDamage;
float ttfenceDamageTarget;
bool ttfenceIsDamageShown;
float ttfenceNextRotationTicks;

void ttfenceAddAngle(Vector* p, int angle) {
  p->x += ttfenceAngleOfs[angle][0];
  p->y += ttfenceAngleOfs[angle][1];
}

bool ttfenceCanPlaceGrid(Vector* p) {
  bool inRect = p->x >= 0 && p->x < TTFENCE_GRID_SIZE && p->y >= 0 && p->y < TTFENCE_GRID_SIZE;
  if (!inRect) {
    return false;
  }
  return !ttfenceGrid[(int)p->x][(int)p->y];
}

bool ttfenceExistsGrid(Vector* p) {
  bool inRect = p->x >= 0 && p->x < TTFENCE_GRID_SIZE && p->y >= 0 && p->y < TTFENCE_GRID_SIZE;
  if (!inRect) {
    return false;
  }
  return ttfenceGrid[(int)p->x][(int)p->y];
}

void ttfenceDrawGrid(int x, int y, float size) {
  Collision scratch;
  box((x - TTFENCE_GRID_SIZE / 2.0) * 6 + 53, (y - TTFENCE_GRID_SIZE / 2.0) * 6 + 56, size, size,
      &scratch);
}

void ttfenceSetBlock(float rpx, float rpy, int type, int angle) {
  int hitCount = 0;
  Vector p;
  vectorSet(&p, rpx, rpy);
  ttfenceAddAngle(&p, angle);
  if (!ttfenceGrid[(int)p.x][(int)p.y]) {
    ttfenceGrid[(int)p.x][(int)p.y] = true;
  } else {
    hitCount++;
  }
  TIMES(3, bi) {
    int ba = ttfenceBlockPatterns[type][bi];
    ttfenceAddAngle(&p, (int)cgl_wrap(angle + ba, 0, 4));
    if (!ttfenceGrid[(int)p.x][(int)p.y]) {
      ttfenceGrid[(int)p.x][(int)p.y] = true;
    } else {
      hitCount++;
    }
  }
  if (hitCount > 0) {
    play(HIT);
    ttfenceDamageTarget += hitCount * 9 * sqrt(difficulty);
  }
  ttfencePos = p;
}

void ttfenceSetBlockTmp(float rpx, float rpy, int type, int angle) {
  Vector p;
  vectorSet(&p, rpx, rpy);
  ttfenceAddAngle(&p, angle);
  ttfenceTmpGrid[(int)p.x][(int)p.y] = true;
  TIMES(3, bi) {
    int ba = ttfenceBlockPatterns[type][bi];
    ttfenceAddAngle(&p, (int)cgl_wrap(angle + ba, 0, 4));
    ttfenceTmpGrid[(int)p.x][(int)p.y] = true;
  }
}

bool ttfenceCheckBlock(float rpx, float rpy, int type, int angle) {
  Vector p;
  vectorSet(&p, rpx, rpy);
  ttfenceAddAngle(&p, angle);
  bool canPlacing = ttfenceCanPlaceGrid(&p);
  TIMES(3, bi) {
    int ba = ttfenceBlockPatterns[type][bi];
    ttfenceAddAngle(&p, (int)cgl_wrap(angle + ba, 0, 4));
    canPlacing = canPlacing && ttfenceCanPlaceGrid(&p);
  }
  return canPlacing;
}

bool ttfenceCheckBlockInGrid(float rpx, float rpy, int type, int angle) {
  Vector p;
  vectorSet(&p, rpx, rpy);
  ttfenceAddAngle(&p, angle);
  bool canPlacing = p.x >= 0 && p.x < TTFENCE_GRID_SIZE && p.y >= 0 && p.y < TTFENCE_GRID_SIZE;
  TIMES(3, bi) {
    int ba = ttfenceBlockPatterns[type][bi];
    ttfenceAddAngle(&p, (int)cgl_wrap(angle + ba, 0, 4));
    bool inR = p.x >= 0 && p.x < TTFENCE_GRID_SIZE && p.y >= 0 && p.y < TTFENCE_GRID_SIZE;
    canPlacing = canPlacing && inR;
  }
  return canPlacing;
}

int ttfenceFillBombDown() {
  int bc = 0;
  Vector p;
  for (int y = 0; y < TTFENCE_GRID_SIZE; y++) {
    for (int x = 0; x < TTFENCE_GRID_SIZE; x++) {
      vectorSet(&p, x, y);
      if (ttfenceCanPlaceGrid(&p) && ttfenceBombEdgeGrid[x][y]) {
        vectorSet(&p, x, y);
        ttfenceAddAngle(&p, 0);
        if (ttfenceCanPlaceGrid(&p) && !ttfenceBombEdgeGrid[(int)p.x][(int)p.y]) {
          ttfenceBombEdgeGrid[(int)p.x][(int)p.y] = true;
          bc++;
        }
        vectorSet(&p, x, y);
        ttfenceAddAngle(&p, 1);
        if (ttfenceCanPlaceGrid(&p) && !ttfenceBombEdgeGrid[(int)p.x][(int)p.y]) {
          ttfenceBombEdgeGrid[(int)p.x][(int)p.y] = true;
          bc++;
        }
      }
    }
  }
  return bc;
}

int ttfenceFillBombUp() {
  int bc = 0;
  Vector p;
  for (int y = TTFENCE_GRID_SIZE - 1; y >= 0; y--) {
    for (int x = TTFENCE_GRID_SIZE - 1; x >= 0; x--) {
      vectorSet(&p, x, y);
      if (ttfenceCanPlaceGrid(&p) && ttfenceBombEdgeGrid[x][y]) {
        vectorSet(&p, x, y);
        ttfenceAddAngle(&p, 2);
        if (ttfenceCanPlaceGrid(&p) && !ttfenceBombEdgeGrid[(int)p.x][(int)p.y]) {
          ttfenceBombEdgeGrid[(int)p.x][(int)p.y] = true;
          bc++;
        }
        vectorSet(&p, x, y);
        ttfenceAddAngle(&p, 3);
        if (ttfenceCanPlaceGrid(&p) && !ttfenceBombEdgeGrid[(int)p.x][(int)p.y]) {
          ttfenceBombEdgeGrid[(int)p.x][(int)p.y] = true;
          bc++;
        }
      }
    }
  }
  return bc;
}

void ttfenceBomb() {
  int bc = 0;
  Vector bp;
  vectorSet(&bp, 0, 0);
  memset(ttfenceBombAnimGrid, 0, sizeof(ttfenceBombAnimGrid));
  Vector p;
  for (int y = 0; y < TTFENCE_GRID_SIZE; y++) {
    for (int x = 0; x < TTFENCE_GRID_SIZE; x++) {
      vectorSet(&p, x, y);
      if (ttfenceCanPlaceGrid(&p) && !ttfenceBombEdgeGrid[x][y]) {
        TIMES(4, oi) {
          int ox = ttfenceAngleOfs[oi][0];
          int oy = ttfenceAngleOfs[oi][1];
          int nx = x + ox;
          int ny = y + oy;
          // Bounds check added: x/y at a grid edge (0 or GRID_SIZE-1) plus
          // ox/oy=+-1 would index ttfenceBombAnimGrid out of range, unlike
          // the second diffusion loop below which is guarded via
          // ttfenceExistsGrid().
          if (nx < 0 || nx >= TTFENCE_GRID_SIZE || ny < 0 || ny >= TTFENCE_GRID_SIZE) {
            continue;
          }
          if (!ttfenceBombAnimGrid[nx][ny]) {
            ttfenceBombAnimGrid[nx][ny] = true;
            bc++;
            bp.x += nx;
            bp.y += ny;
          }
        }
      }
    }
  }
  int loopMax = (int)floor(sqrt(bc) * 0.5);
  TIMES(loopMax, li) {
    int pbc = bc;
    for (int y = 0; y < TTFENCE_GRID_SIZE; y++) {
      for (int x = 0; x < TTFENCE_GRID_SIZE; x++) {
        if (ttfenceBombAnimGrid[x][y]) {
          TIMES(4, oi2) {
            int ox = ttfenceAngleOfs[oi2][0];
            int oy = ttfenceAngleOfs[oi2][1];
            vectorSet(&p, x + ox, y + oy);
            if (ttfenceExistsGrid(&p) && !ttfenceBombAnimGrid[(int)p.x][(int)p.y]) {
              ttfenceBombAnimGrid[(int)p.x][(int)p.y] = true;
              bc++;
              bp.x += p.x;
              bp.y += p.y;
            }
          }
        }
      }
    }
    if (pbc == bc) {
      break;
    }
  }
  for (int y = 0; y < TTFENCE_GRID_SIZE; y++) {
    for (int x = 0; x < TTFENCE_GRID_SIZE; x++) {
      if (ttfenceBombAnimGrid[x][y]) {
        ttfenceGrid[x][y] = false;
      }
    }
  }
  ttfenceBombAnimTicks = 30;
  if (bc > 0) {
    play(EXPLOSION);
    bp.x /= bc;
    bp.y /= bc;
    int sc = (int)ceil(bc * sqrt(bc));
    addScore(sc, (bp.x - TTFENCE_GRID_SIZE / 2.0) * 6 + 53,
             (bp.y - TTFENCE_GRID_SIZE / 2.0) * 6 + 56);
    ttfenceDamageTarget = clamp(ttfenceDamageTarget - sc * 0.1, 0, 99);
  }
}

void ttfenceCheckBomb() {
  memset(ttfenceBombEdgeGrid, 0, sizeof(ttfenceBombEdgeGrid));
  TIMES(TTFENCE_GRID_SIZE, i) {
    ttfenceBombEdgeGrid[i][0] = true;
    ttfenceBombEdgeGrid[i][TTFENCE_GRID_SIZE - 1] = true;
    ttfenceBombEdgeGrid[0][i] = true;
    ttfenceBombEdgeGrid[TTFENCE_GRID_SIZE - 1][i] = true;
  }
  TIMES(99, i2) {
    if (ttfenceFillBombDown() + ttfenceFillBombUp() == 0) {
      break;
    }
  }
  ttfenceBomb();
}

void ttfenceUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - every
  // placement/overlap check goes through ttfenceCanPlaceGrid()/
  // ttfenceExistsGrid() (direct bool grid indexing), so the engine's own
  // O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure waste here.
  // Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    memset(ttfenceGrid, 0, sizeof(ttfenceGrid));
    memset(ttfenceTmpGrid, 0, sizeof(ttfenceTmpGrid));
    memset(ttfenceBombEdgeGrid, 0, sizeof(ttfenceBombEdgeGrid));
    memset(ttfenceBombAnimGrid, 0, sizeof(ttfenceBombAnimGrid));
    vectorSet(&ttfencePos, 0, 0);
    ttfenceAngle = 0;
    ttfencePrevAngle = 0;
    ttfenceType = rndi(0, TTFENCE_BLOCK_PATTERN_COUNT);
    ttfenceBombAnimTicks = 0;
    ttfenceDamage = 0;
    ttfenceDamageTarget = 0;
    ttfenceIsDamageShown = true;
    ttfenceNextRotationTicks = 0;
    ttfenceSetBlock(7, 7, rndi(0, TTFENCE_BLOCK_PATTERN_COUNT), rndi(0, 4));
  }
  ttfenceBombAnimTicks--;
  TIMES(TTFENCE_GRID_SIZE, gx) {
    TIMES(TTFENCE_GRID_SIZE, gy) {
      if (ttfenceGrid[gx][gy] && ttfenceTmpGrid[gx][gy]) {
        color = PURPLE;
        ttfenceDrawGrid(gx, gy, 6);
      } else if (ttfenceGrid[gx][gy]) {
        color = BLUE;
        ttfenceDrawGrid(gx, gy, 6);
      } else if (ttfenceTmpGrid[gx][gy]) {
        color = LIGHT_BLUE;
        ttfenceDrawGrid(gx, gy, 6);
      } else if (ttfenceBombAnimTicks > 0 && ttfenceBombAnimGrid[gx][gy]) {
        color = RED;
        ttfenceDrawGrid(gx, gy, sin((ttfenceBombAnimTicks / 30) * CGLP_PI) * 6);
      }
    }
  }
  if (input.isJustPressed) {
    play(SELECT);
    ttfenceSetBlock(ttfencePos.x, ttfencePos.y, ttfenceType, ttfenceAngle);
    ttfenceCheckBomb();
    ttfenceType = rndi(0, TTFENCE_BLOCK_PATTERN_COUNT);
    ttfenceNextRotationTicks = 0;
    ttfenceDamageTarget += sqrt(difficulty) * 2;
  }
  ttfenceNextRotationTicks--;
  if (ttfenceNextRotationTicks < 0) {
    bool canPlacing = false;
    TIMES(4, i3) {
      ttfenceAngle = (int)cgl_wrap(ttfenceAngle + 1, 0, 4);
      if (ttfenceCheckBlock(ttfencePos.x, ttfencePos.y, ttfenceType, ttfenceAngle)) {
        canPlacing = true;
        break;
      }
    }
    memset(ttfenceTmpGrid, 0, sizeof(ttfenceTmpGrid));
    if (!canPlacing) {
      TIMES(4, i4) {
        ttfenceAngle = (int)cgl_wrap(ttfenceAngle + 1, 0, 4);
        if (ttfenceCheckBlockInGrid(ttfencePos.x, ttfencePos.y, ttfenceType, ttfenceAngle)) {
          canPlacing = true;
          break;
        }
      }
    }
    if (!canPlacing) {
      ttfenceDamageTarget = 100;
    } else {
      ttfenceSetBlockTmp(ttfencePos.x, ttfencePos.y, ttfenceType, ttfenceAngle);
    }
    if (ttfenceAngle != ttfencePrevAngle) {
      play(LASER);
    }
    ttfencePrevAngle = ttfenceAngle;
    ttfenceNextRotationTicks = 30 / sqrt(sqrt(difficulty));
  }
  ttfenceDamageTarget += sqrt(difficulty) * 0.01;
  ttfenceDamage += (ttfenceDamageTarget - ttfenceDamage) * 0.1;
  color = RED;
  if (ttfenceDamage > 99) {
    rect(0, 98, 100, 2, &scratch);
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  } else if (ttfenceDamage < 80 ||
             ticks % (int)ceil(109 - ttfenceDamage) < (109 - ttfenceDamage) * 0.8) {
    if (!ttfenceIsDamageShown) {
      play(COIN);
    }
    rect(0, 98, ttfenceDamage, 2, &scratch);
    rect(99, 98, 1, 2, &scratch);
    ttfenceIsDamageShown = true;
  } else {
    ttfenceIsDamageShown = false;
  }
}

void addGameTtfence() {
  addGame(ttfenceTitle, ttfenceDescription, ttfenceCharacters, ttfenceCharactersCount,
          &ttfenceOptions, false, &ttfenceUpdate);
}
