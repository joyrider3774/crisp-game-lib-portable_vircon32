#include "../cglp.h"

int* photonlineTitle = "PHOTON LINE";
int* photonlineDescription = "[Tap]\n Turn\n[Hold]\n Flick";

int[8][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] photonlineCharacters = {
    {
        "rllllr",
        "lrllrl",
        "ll lll",
        "llllll",
        "lrllrl",
        "rllllr",
    },
    {
        "r pprr",
        " ppprr",
        "pp ppp",
        "pppppp",
        "rrppp ",
        "rrpp r",
    },
    {
        "r yyrr",
        " yyyrr",
        "yy yyy",
        "yyyyyy",
        "rryyy ",
        "rryy r",
    },
    {
        "r ccrr",
        " cccrr",
        "cc ccc",
        "cccccc",
        "rrccc ",
        "rrcc r",
    },
    {
        "r ggrr",
        " gggrr",
        "gg ggg",
        "gggggg",
        "rrggg ",
        "rrgg r",
    },
    {
        "r    b",
        " r  b ",
        "  rb  ",
        "  br  ",
        " b  r ",
        "b    r",
    },
    {
        "   l  ",
        "llllll",
        "l    l",
        " llll ",
        " ll   ",
        "l llll",
    },
    {
        "   l l",
        "llllll",
        "l   l ",
        "lll ll",
        "l l ll",
        "l l ll",
    },
};
int photonlineCharactersCount = 8;

Options photonlineOptions = {200, 100, 2, false};

struct PhotonlinePhoton {
  float x;
  float vx;
  int index;
  int w;
  bool isReflected;
  bool isAlive;
};
#define PHOTONLINE_MAX_PHOTON_COUNT 64
PhotonlinePhoton[PHOTONLINE_MAX_PHOTON_COUNT] photonlinePhotons;
int photonlinePhotonIndex;
float photonlineNextPhotonTicks;

float photonlinePlayerAngle;
bool photonlinePlayerIsRotating;
float photonlinePlayerTa;
// Real JS lengths stay small (splice(5) on target, a soft appWidth-halving
// cap past 9 on hands) - sized with generous headroom above both.
#define PHOTONLINE_MAX_HAND_LEN 16
int[2][PHOTONLINE_MAX_HAND_LEN] photonlineHands;
int[2] photonlineHandsLen;

int[2][PHOTONLINE_MAX_HAND_LEN] photonlineTarget;
int[2] photonlineTargetLen;

int photonlineWorld;
int photonlineStep;
float photonlineNextStepTicks;
float photonlineAppWidth;
int photonlineColorCount;

#define PHOTONLINE_MAX_NEXT_PHOTONS 16
int[PHOTONLINE_MAX_NEXT_PHOTONS] photonlineNextPhotons;
int photonlineNextPhotonsLen;

void photonlineUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - a photon
  // "hitting" a hand is a direct x-threshold comparison (see "hit" below),
  // so the engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c)
  // is pure waste here. Restored automatically when the next real game
  // starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    photonlinePlayerAngle = 0;
    photonlinePlayerIsRotating = false;
    photonlinePlayerTa = 0;
    photonlineHands[0][0] = 0;
    photonlineHandsLen[0] = 1;
    photonlineHands[1][0] = 1;
    photonlineHandsLen[1] = 1;
    photonlineTarget[0][0] = 0;
    photonlineTargetLen[0] = 1;
    photonlineTarget[1][0] = 1;
    photonlineTargetLen[1] = 1;
    INIT_UNALIVED_ARRAY_FAST(photonlinePhotons);
    photonlinePhotonIndex = 0;
    photonlineNextPhotonTicks = 0;
    photonlineWorld = 1;
    photonlineStep = 1;
    photonlineNextStepTicks = 0;
    photonlineAppWidth = 30;
    photonlineColorCount = 2;
    photonlineNextPhotonsLen = 0;
  }
  if (photonlineNextStepTicks == 0) {
    if (photonlineStep > clamp(photonlineWorld + 1, 1, 5)) {
      photonlineStep = 1;
      photonlineWorld++;
      photonlineColorCount = clamp(rndi(2, photonlineWorld + 2), 2, 4);
      photonlineTarget[0][0] = rndi(0, photonlineColorCount);
      photonlineTargetLen[0] = 1;
      photonlineTarget[1][0] = rndi(0, photonlineColorCount);
      photonlineTargetLen[1] = 1;
      photonlineHands[0][0] = photonlineTarget[0][0];
      photonlineHandsLen[0] = 1;
      photonlineHands[1][0] = photonlineTarget[1][0];
      photonlineHandsLen[1] = 1;
    }
    int[2][PHOTONLINE_MAX_HAND_LEN] prevTarget;
    int[2] prevTargetLen;
    TIMES(2, pIdx) {
      prevTargetLen[pIdx] = photonlineTargetLen[pIdx];
      TIMES(photonlineTargetLen[pIdx], pj) { prevTarget[pIdx][pj] = photonlineTarget[pIdx][pj]; }
    }
    TIMES(2, ti) {
      if (photonlineTargetLen[ti] > 4) {
        photonlineTargetLen[ti] = 4;
      }
    }
    float cr = 3.0 / photonlineColorCount;
    int ac = (int)round(rnd(1, photonlineWorld) * cr);
    int dc = (int)round(clamp(rnd(0, photonlineWorld) - 1, 0, 2) * cr);
    if (ac == dc) {
      dc--;
    }
    TIMES(dc, dci) {
      int h = rndi(0, 2);
      if (photonlineTargetLen[h] > 0) {
        photonlineTargetLen[h]--;
      }
    }
    TIMES(ac + dc, aci) {
      int h = rndi(0, 2);
      if (photonlineTargetLen[h] < PHOTONLINE_MAX_HAND_LEN) {
        photonlineTarget[h][photonlineTargetLen[h]] = rndi(0, photonlineColorCount);
        photonlineTargetLen[h]++;
      }
    }
    TIMES(2, ti2) {
      if (photonlineTargetLen[ti2] > 5) {
        photonlineTargetLen[ti2] = 5;
      }
    }
    bool isSame = photonlineTargetLen[0] == prevTargetLen[0] &&
                  photonlineTargetLen[1] == prevTargetLen[1];
    if (isSame) {
      TIMES(2, si) {
        TIMES(photonlineTargetLen[si], sj) {
          if (prevTarget[si][sj] != photonlineTarget[si][sj]) {
            isSame = false;
          }
        }
      }
    }
    if (isSame && photonlineTargetLen[0] > 0) {
      int lastIdx = photonlineTargetLen[0] - 1;
      photonlineTarget[0][lastIdx] =
          (int)cgl_wrap(photonlineTarget[0][lastIdx] + 1, 0, photonlineColorCount);
    }
    photonlineStep++;
    photonlineNextPhotonTicks = 60;
    photonlineAppWidth = 30;
    photonlineNextPhotonsLen = 0;
  }
  if (photonlineNextStepTicks < 0 && photonlineNextStepTicks > -100) {
    int[32] worldStepText;
    strcpy(worldStepText, "WORLD ");
    strcat(worldStepText, intToChar(photonlineWorld));
    strcat(worldStepText, "   STEP ");
    strcat(worldStepText, intToChar(photonlineStep - 1));
    text(worldStepText, 50, 20, &scratch);
  }
  if (photonlineNextStepTicks > 0) {
    character("g", 80, 50, &scratch);
    character("h", 120, 50, &scratch);
    if (photonlineAppWidth > -3) {
      photonlineAppWidth--;
      addScore(photonlineWorld, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    }
  }
  photonlineNextStepTicks--;
  photonlineNextPhotonTicks--;
  if (photonlineNextPhotonTicks < 0) {
    if (photonlineNextPhotonsLen == 0) {
      TIMES(photonlineColorCount, npi) {
        photonlineNextPhotons[photonlineNextPhotonsLen] = npi;
        photonlineNextPhotonsLen++;
        photonlineNextPhotons[photonlineNextPhotonsLen] = npi;
        photonlineNextPhotonsLen++;
      }
      int extraCount = (int)ceil(photonlineColorCount * 0.4);
      TIMES(extraCount, eci) {
        photonlineNextPhotons[photonlineNextPhotonsLen] = 4;
        photonlineNextPhotonsLen++;
      }
      int nl = photonlineNextPhotonsLen;
      TIMES(99, shi) {
        int i1 = rndi(0, nl);
        int i2 = rndi(0, nl);
        int t = photonlineNextPhotons[i1];
        photonlineNextPhotons[i1] = photonlineNextPhotons[i2];
        photonlineNextPhotons[i2] = t;
      }
    }
    int w;
    if (rnd(0, 1) < 0.5) {
      w = 1;
    } else {
      w = -1;
    }
    float s = sqrt(difficulty) * 0.4;
    ASSIGN_ARRAY_ITEM(photonlinePhotons, photonlinePhotonIndex, PhotonlinePhoton, np);
    if (w > 0) {
      np->x = photonlineAppWidth;
    } else {
      np->x = 200 - photonlineAppWidth;
    }
    np->vx = w * s;
    np->w = w;
    photonlineNextPhotonsLen--;
    np->index = photonlineNextPhotons[photonlineNextPhotonsLen];
    np->isReflected = false;
    np->isAlive = true;
    photonlinePhotonIndex = cgl_wrap(photonlinePhotonIndex + 1, 0, PHOTONLINE_MAX_PHOTON_COUNT);
    photonlineNextPhotonTicks = rnd(60, 90) / sqrt(difficulty);
  }
  bool photonlineClearAll = false;
  FOR_EACH(photonlinePhotons, i) {
    ASSIGN_ARRAY_ITEM(photonlinePhotons, i, PhotonlinePhoton, p);
    SKIP_IS_NOT_ALIVE(p);
    p->x += p->vx;
    int[2] pcode;
    pcode[0] = 'b' + p->index;
    pcode[1] = 0;
    character(pcode, p->x, 60, &scratch);
    if (p->isReflected) {
      if (p->x < photonlineAppWidth || p->x > 200 - photonlineAppWidth) {
        p->isAlive = false;
      }
      continue;
    }
    int sideBit;
    if (p->w > 0) {
      sideBit = 0;
    } else {
      sideBit = 1;
    }
    int angleBit;
    if (photonlinePlayerAngle < CGLP_PI / 2 || photonlinePlayerAngle > (CGLP_PI / 2) * 3) {
      angleBit = 0;
    } else {
      angleBit = 1;
    }
    int hi = (int)cgl_wrap(sideBit + angleBit, 0, 2);
    float hl = photonlineHandsLen[hi] * 7 + 6;
    float x = 100 - hl * p->w;
    bool hit = false;
    if (p->w == 1 && p->x > x) {
      hit = true;
    } else if (p->w == -1 && p->x < x) {
      hit = true;
    }
    if (hit) {
      if (photonlinePlayerIsRotating) {
        play(LASER);
        p->isReflected = true;
        p->vx *= -5;
      } else {
        if (p->index == 4) {
          if (photonlineHandsLen[hi] > 0) {
            play(HIT);
            particle(p->x, 60, 16, 1, 0, CGLP_PI * 2);
            photonlineHandsLen[hi]--;
          }
        } else {
          play(SELECT);
          if (photonlineHandsLen[hi] < PHOTONLINE_MAX_HAND_LEN) {
            photonlineHands[hi][photonlineHandsLen[hi]] = p->index;
            photonlineHandsLen[hi]++;
          }
          if (photonlineHandsLen[hi] > 9) {
            photonlineAppWidth /= 2;
          }
        }
        bool isMatching = true;
        TIMES(2, mi) {
          if (photonlineTargetLen[mi] != photonlineHandsLen[mi]) {
            isMatching = false;
          } else {
            TIMES(photonlineTargetLen[mi], mj) {
              if (photonlineTarget[mi][mj] != photonlineHands[mi][mj]) {
                isMatching = false;
              }
            }
          }
        }
        if (isMatching && photonlinePlayerAngle > CGLP_PI / 2) {
          photonlinePlayerIsRotating = true;
          photonlinePlayerTa += CGLP_PI;
        }
        if (!isMatching) {
          isMatching = true;
          TIMES(2, mi2) {
            int otherIdx;
            if (mi2 == 0) {
              otherIdx = 1;
            } else {
              otherIdx = 0;
            }
            if (photonlineTargetLen[mi2] != photonlineHandsLen[otherIdx]) {
              isMatching = false;
            } else {
              TIMES(photonlineTargetLen[mi2], mj2) {
                if (photonlineTarget[mi2][mj2] != photonlineHands[otherIdx][mj2]) {
                  isMatching = false;
                }
              }
            }
          }
          if (isMatching && photonlinePlayerAngle < CGLP_PI / 2) {
            photonlinePlayerIsRotating = true;
            photonlinePlayerTa += CGLP_PI;
          }
        }
        if (isMatching) {
          play(POWER_UP);
          photonlineClearAll = true;
          photonlineNextStepTicks = 60;
          photonlineNextPhotonTicks = 9999;
        }
        p->isAlive = false;
        continue;
      }
    }
  }
  if (photonlineClearAll) {
    INIT_UNALIVED_ARRAY_FAST(photonlinePhotons);
  }
  character("a", 100, 30, &scratch);
  Vector p2;
  TIMES(2, wi) {
    vectorSet(&p2, 100, 30);
    TIMES(photonlineTargetLen[wi], ii) {
      p2.x += 7 * (wi * 2 - 1);
      int[2] tc;
      tc[0] = 'b' + photonlineTarget[wi][ii];
      tc[1] = 0;
      character(tc, p2.x, p2.y, &scratch);
    }
  }
  if (!photonlinePlayerIsRotating && photonlineNextStepTicks < 0 && input.isJustPressed) {
    play(COIN);
    photonlinePlayerIsRotating = true;
    photonlinePlayerTa += CGLP_PI;
  }
  if (photonlinePlayerIsRotating) {
    photonlinePlayerAngle += 0.2;
    if (photonlinePlayerAngle >= photonlinePlayerTa) {
      if (!input.isPressed) {
        photonlinePlayerAngle = photonlinePlayerTa;
        photonlinePlayerIsRotating = false;
      } else {
        photonlinePlayerTa += CGLP_PI;
      }
    }
    if (photonlinePlayerAngle >= CGLP_PI * 2) {
      photonlinePlayerAngle -= CGLP_PI * 2;
      photonlinePlayerTa -= CGLP_PI * 2;
    }
  }
  character("a", 100, 60, &scratch);
  TIMES(2, hwi) {
    vectorSet(&p2, 100, 60);
    TIMES(photonlineHandsLen[hwi], hii) {
      addWithAngle(&p2, photonlinePlayerAngle, 7 * (hwi * 2 - 1));
      int[2] hc;
      hc[0] = 'b' + photonlineHands[hwi][hii];
      hc[1] = 0;
      character(hc, p2.x, p2.y, &scratch);
    }
  }
  photonlineAppWidth -= 0.02;
  rect(0, 56, photonlineAppWidth + 3, 8, &scratch);
  rect(200, 56, -photonlineAppWidth - 3, 8, &scratch);
  if (photonlineNextStepTicks < 0 && photonlineAppWidth < -3) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
}

void addGamePhotonline() {
  addGame(photonlineTitle, photonlineDescription, photonlineCharacters,
          photonlineCharactersCount, &photonlineOptions, false, &photonlineUpdate);
}
