#include "../cglp.h"

int* spincellTitle = "SPIN CELL";
int* spincellDescription = "[Tap] Rotate the cell";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] spincellCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int spincellCharactersCount = 0;

Options spincellOptions = {100, 100, 25, false};

#define SPINCELL_WALL_COUNT 12
bool[SPINCELL_WALL_COUNT] spincellWallSpikes;

struct SpincellPlayer {
  Vector pos;
  float vx;
  float vy;
};
SpincellPlayer spincellPlayer;

int spincellGemCorner;
float spincellCreepT;
int spincellWarnSeg;
float spincellRotFx;
bool spincellDying;
int spincellDieT;
int spincellNudgeT;

#define SPINCELL_TRAIL_CAPACITY 4
Vector[SPINCELL_TRAIL_CAPACITY] spincellTrail;
int spincellTrailCount;

// Returns (via out-pointer) the on-screen center point of perimeter segment
// `i` (0..11) - used both to place spike-warning particles and to draw the
// rotation pulse ring's reference geometry.
void spincellSegCenter(int i, Vector* result) {
  int side = i / 3;
  int idx = i % 3;
  float L = 86.0 / 3;
  if (side == 0) {
    vectorSet(result, 7 + idx * L + L / 2, 5);
  } else if (side == 1) {
    vectorSet(result, 95, 7 + idx * L + L / 2);
  } else if (side == 2) {
    vectorSet(result, 7 + (2 - idx) * L + L / 2, 95);
  } else {
    vectorSet(result, 5, 7 + (2 - idx) * L + L / 2);
  }
}

void spincellUpdate() {
  Collision scratch;
  // Never reads a Collision result - bounces/spikes are decided by geometry math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&spincellPlayer.pos, 50, 40);
    spincellPlayer.vx = rnd(0.4, 0.8);
    spincellPlayer.vy = 0;
    TIMES(SPINCELL_WALL_COUNT, wi0) {
      spincellWallSpikes[wi0] = false;
    }
    spincellWallSpikes[1] = true;
    spincellWallSpikes[7] = true;
    spincellGemCorner = rndi(0, 4);
    spincellCreepT = 400;
    spincellWarnSeg = -1;
    spincellRotFx = 0;
    spincellTrailCount = 0;
    spincellDying = false;
    spincellDieT = 0;
    spincellNudgeT = 0;
  }

  if (!spincellDying) {
    if (input.isJustPressed) {
      bool[SPINCELL_WALL_COUNT] nw;
      TIMES(SPINCELL_WALL_COUNT, wi) {
        nw[(wi + 1) % SPINCELL_WALL_COUNT] = spincellWallSpikes[wi];
      }
      TIMES(SPINCELL_WALL_COUNT, wi2) {
        spincellWallSpikes[wi2] = nw[wi2];
      }
      if (spincellWarnSeg >= 0) {
        spincellWarnSeg = (spincellWarnSeg + 1) % SPINCELL_WALL_COUNT;
      }
      if (spincellPlayer.pos.y > 87) {
        spincellPlayer.vx -= 0.6;
        spincellPlayer.vx = clamp(spincellPlayer.vx, -1.4, 1.4);
        spincellNudgeT = 8;
        color = LIGHT_CYAN;
        particle(spincellPlayer.pos.x, spincellPlayer.pos.y, 8, 2, 0, CGLP_PI / 4);
        play(CLICK);
      }
      spincellRotFx = 12;
      play(SELECT);
    }

    spincellCreepT--;
    if (spincellCreepT < 0) {
      if (spincellWarnSeg < 0) {
        int[SPINCELL_WALL_COUNT] cand;
        int candCount = 0;
        TIMES(SPINCELL_WALL_COUNT, ci) {
          if (!spincellWallSpikes[ci]) {
            cand[candCount] = ci;
            candCount++;
          }
        }
        if (candCount > 4) {
          spincellWarnSeg = cand[rndi(0, candCount)];
          spincellCreepT = 90;
        } else {
          spincellCreepT = 500;
        }
      } else {
        spincellWallSpikes[spincellWarnSeg] = true;
        color = RED;
        Vector segC;
        spincellSegCenter(spincellWarnSeg, &segC);
        particle(segC.x, segC.y, 10, 1.5, 0, CGLP_PI * 2);
        spincellWarnSeg = -1;
        int spikeCount = 0;
        TIMES(SPINCELL_WALL_COUNT, si) {
          if (spincellWallSpikes[si]) {
            spikeCount++;
          }
        }
        spincellCreepT = (200 * sqrt(spikeCount + 1)) / sqrt(difficulty);
        play(HIT);
      }
    }

    spincellPlayer.vy += 0.06;
    spincellPlayer.pos.x += spincellPlayer.vx;
    spincellPlayer.pos.y += spincellPlayer.vy;

    if (spincellTrailCount < SPINCELL_TRAIL_CAPACITY) {
      spincellTrail[spincellTrailCount] = spincellPlayer.pos;
      spincellTrailCount++;
    } else {
      TIMES(SPINCELL_TRAIL_CAPACITY - 1, si2) {
        spincellTrail[si2] = spincellTrail[si2 + 1];
      }
      spincellTrail[SPINCELL_TRAIL_CAPACITY - 1] = spincellPlayer.pos;
    }

    int hitSide = -1;
    float t = 0;
    if (spincellPlayer.pos.y < 9) {
      hitSide = 0;
      t = (spincellPlayer.pos.x - 7) / 86;
      spincellPlayer.pos.y = 9;
      spincellPlayer.vy = fabs(spincellPlayer.vy) * 0.9;
    } else if (spincellPlayer.pos.x > 91) {
      hitSide = 1;
      t = (spincellPlayer.pos.y - 7) / 86;
      spincellPlayer.pos.x = 91;
      spincellPlayer.vx = -fabs(spincellPlayer.vx) * 0.95;
    } else if (spincellPlayer.pos.y > 91) {
      hitSide = 2;
      t = (spincellPlayer.pos.x - 7) / 86;
      spincellPlayer.pos.y = 91;
      spincellPlayer.vy = -(3.0 + difficulty * 0.1);
      spincellPlayer.vx += rnd(-0.25, 0.25);
      spincellPlayer.vx = clamp(spincellPlayer.vx, -1.2, 1.2);
      play(JUMP);
    } else if (spincellPlayer.pos.x < 9) {
      hitSide = 3;
      t = (spincellPlayer.pos.y - 7) / 86;
      spincellPlayer.pos.x = 9;
      spincellPlayer.vx = fabs(spincellPlayer.vx) * 0.95;
    }
    if (hitSide >= 0) {
      int idx = clamp(floor(t * 3), 0, 2);
      if (hitSide == 2 || hitSide == 3) {
        idx = 2 - idx;
      }
      if (spincellWallSpikes[hitSide * 3 + idx]) {
        spincellDying = true;
        spincellDieT = 8;
        play(EXPLOSION);
        particle(spincellPlayer.pos.x, spincellPlayer.pos.y, 25, 3, 0, CGLP_PI * 2);
      }
    }
  }

  if (spincellRotFx > 0) {
    spincellRotFx--;
    color = LIGHT_BLACK;
    thickness = 1;
    arc(50, 50, 44 * (1 - spincellRotFx / 12), 0, CGLP_PI * 2, &scratch);
  }

  TIMES(SPINCELL_WALL_COUNT, di) {
    int side = di / 3;
    int idx = di % 3;
    float L = 86.0 / 3;
    if (spincellWallSpikes[di]) {
      color = RED;
    } else if (di == spincellWarnSeg && fmod(spincellCreepT, 10) < 5) {
      color = LIGHT_RED;
    } else {
      color = LIGHT_BLACK;
    }
    if (side == 0) {
      rect(7 + idx * L, 3, L, 4, &scratch);
    }
    if (side == 1) {
      rect(93, 7 + idx * L, 4, L, &scratch);
    }
    if (side == 2) {
      rect(7 + (2 - idx) * L, 93, L, 4, &scratch);
    }
    if (side == 3) {
      rect(3, 7 + (2 - idx) * L, 4, L, &scratch);
    }
  }

  float gpx;
  float gpy;
  if (spincellGemCorner == 0) {
    gpx = 14;
    gpy = 14;
  } else if (spincellGemCorner == 1) {
    gpx = 86;
    gpy = 14;
  } else if (spincellGemCorner == 2) {
    gpx = 86;
    gpy = 86;
  } else {
    gpx = 14;
    gpy = 86;
  }
  color = YELLOW;
  box(gpx, gpy, 5, 5, &scratch);
  if (!spincellDying && distanceTo(&spincellPlayer.pos, gpx, gpy) < 8) {
    int[SPINCELL_WALL_COUNT] spikeIdx;
    int spikeIdxCount = 0;
    TIMES(SPINCELL_WALL_COUNT, si3) {
      if (spincellWallSpikes[si3]) {
        spikeIdx[spikeIdxCount] = si3;
        spikeIdxCount++;
      }
    }
    addScore(pow((float)max(spikeIdxCount, 1), 2), gpx, gpy);
    play(COIN);
    particle(gpx, gpy, 12, 2, 0, CGLP_PI * 2);
    if (spikeIdxCount > 0) {
      int revert = spikeIdx[rndi(0, spikeIdxCount)];
      spincellWallSpikes[revert] = false;
      color = LIGHT_BLACK;
      Vector revertC;
      spincellSegCenter(revert, &revertC);
      particle(revertC.x, revertC.y, 10, 1.5, 0, CGLP_PI * 2);
      play(POWER_UP);
    }
    spincellGemCorner = (spincellGemCorner + 1 + rndi(0, 3)) % 4;
  }

  float spd = sqrt(spincellPlayer.vx * spincellPlayer.vx + spincellPlayer.vy * spincellPlayer.vy);
  if (spd > 1.5) {
    TIMES(spincellTrailCount, tri) {
      if (tri % 2 == 0) {
        color = LIGHT_CYAN;
      } else {
        color = CYAN;
      }
      box(spincellTrail[tri].x, spincellTrail[tri].y, 3, 3, &scratch);
    }
  }

  if (spincellNudgeT > 0) {
    spincellNudgeT--;
    if (spincellNudgeT % 2 == 0) {
      color = LIGHT_CYAN;
    } else {
      color = CYAN;
    }
  } else {
    color = CYAN;
  }
  box(spincellPlayer.pos.x, spincellPlayer.pos.y, 5, 5, &scratch);

  if (spincellDying) {
    spincellDieT--;
    if (spincellDieT <= 0) {
      gameOver();
    }
  }
}

void addGameSpincell() {
  addGame(spincellTitle, spincellDescription, spincellCharacters,
          spincellCharactersCount, &spincellOptions, false, &spincellUpdate);
}
