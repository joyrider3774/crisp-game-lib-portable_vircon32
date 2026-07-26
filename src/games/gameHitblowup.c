#include "../cglp.h"

int* hitblowupTitle = "HIT BLOW UP";
int* hitblowupDescription = "[Tap]\n Select color";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] hitblowupCharacters = {
    {
        " ll   ",
        "llll  ",
        "llll  ",
        " ll   ",
    },
    {
        " ll   ",
        "l  l  ",
        "l  l  ",
        " ll   ",
    },
};
int hitblowupCharactersCount = 2;

Options hitblowupOptions = {100, 100, 4, false};

#define HITBLOWUP_MAX_TARGET_LEN 6
#define HITBLOWUP_MAX_HIST_COUNT 64

int[6] hitblowupColors = {RED, GREEN, BLUE, YELLOW, CYAN, PURPLE};

struct HitblowupHistEntry {
  int[HITBLOWUP_MAX_TARGET_LEN] colorsArr;
  int len;
  int hit;
  int blow;
};
HitblowupHistEntry[HITBLOWUP_MAX_HIST_COUNT] hitblowupHist;
int hitblowupHistCount;

bool hitblowupIsGoingNextStage;
int hitblowupStageCount;
int hitblowupLoopCount;
int[6] hitblowupSelectorBase;
int[6] hitblowupSelector;
int hitblowupSelectorLen;
float hitblowupSelectorY;
int[HITBLOWUP_MAX_TARGET_LEN] hitblowupTarget;
int hitblowupTargetLen;
int[HITBLOWUP_MAX_TARGET_LEN] hitblowupCurrent;
int hitblowupCurrentIndex;
float hitblowupHitBlowTicks;
float hitblowupNextStageTicks;
int hitblowupHitCount;
int hitblowupBlowCount;

void hitblowupDrawAnswer() {
  Collision scratch;
  float tx = 50 - ((hitblowupTargetLen - 1) / 2.0) * 7;
  TIMES(hitblowupTargetLen, i) {
    color = hitblowupColors[hitblowupTarget[i]];
    character("a", tx, hitblowupSelectorY + 8, &scratch);
    tx += 7;
  }
}

void hitblowupUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file (including the
  // hitblowupDrawAnswer() helper above) - the selected color is computed
  // directly from input.pos via grid math (see "si" below), so the
  // engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure
  // waste here. Restored automatically when the next real game starts,
  // via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    hitblowupIsGoingNextStage = true;
    hitblowupStageCount = 0;
  }
  if (hitblowupIsGoingNextStage) {
    hitblowupLoopCount = (int)floor(hitblowupStageCount / 6.0);
    int s = hitblowupStageCount % 6;
    if (s == 0) {
      TIMES(6, i) { hitblowupSelectorBase[i] = i; }
      TIMES(99, k) {
        int n1 = rndi(0, 6);
        int n2 = rndi(0, 6);
        int t = hitblowupSelectorBase[n1];
        hitblowupSelectorBase[n1] = hitblowupSelectorBase[n2];
        hitblowupSelectorBase[n2] = t;
      }
    }
    int sc = 3 + (int)floor((s + 1) / 2.0);
    hitblowupSelectorLen = sc;
    TIMES(sc, i) { hitblowupSelector[i] = hitblowupSelectorBase[i]; }
    int tc = 2 + (int)floor(s / 2.0);
    TIMES(sc, i) { hitblowupTarget[i] = hitblowupSelector[i]; }
    TIMES(99, k) {
      int n1 = rndi(0, sc);
      int n2 = rndi(0, sc);
      int t = hitblowupTarget[n1];
      hitblowupTarget[n1] = hitblowupTarget[n2];
      hitblowupTarget[n2] = t;
    }
    hitblowupTargetLen = tc;
    hitblowupHistCount = 0;
    hitblowupSelectorY = 90;
    hitblowupIsGoingNextStage = false;
    TIMES(tc, i) { hitblowupCurrent[i] = -1; }
    hitblowupCurrentIndex = 0;
    hitblowupHitBlowTicks = -1;
    hitblowupNextStageTicks = -1;
  }
  if (hitblowupNextStageTicks < 0) {
    hitblowupSelectorY -= (pow(2, hitblowupLoopCount) /
                            (hitblowupTargetLen * hitblowupTargetLen + hitblowupSelectorLen)) *
                          0.05;
  }
  color = LIGHT_BLACK;
  rect(0, hitblowupSelectorY, 99, 99 - hitblowupSelectorY, &scratch);
  float sx = 50 - ((hitblowupSelectorLen - 1) / 2.0) * 10;
  TIMES(hitblowupSelectorLen, i) {
    color = hitblowupColors[hitblowupSelector[i]];
    character("a", sx, hitblowupSelectorY + 3, &scratch);
    sx += 10;
  }
  if (hitblowupNextStageTicks < 0 && input.isJustPressed) {
    int si = (int)floor((input.pos.x - 50) / 10 + hitblowupSelectorLen / 2.0);
    if (si >= 0 && si < hitblowupSelectorLen) {
      play(SELECT);
      hitblowupCurrent[hitblowupCurrentIndex] = hitblowupSelector[si];
      hitblowupCurrentIndex++;
      if (hitblowupCurrentIndex == hitblowupTargetLen) {
        int hit = 0;
        int blow = 0;
        TIMES(hitblowupTargetLen, ti) {
          if (hitblowupTarget[ti] == hitblowupCurrent[ti]) {
            hit++;
          } else {
            bool foundInCurrent = false;
            TIMES(hitblowupTargetLen, cj) {
              if (hitblowupCurrent[cj] == hitblowupTarget[ti]) {
                foundInCurrent = true;
              }
            }
            if (foundInCurrent) {
              blow++;
            }
          }
        }
        if (hitblowupHistCount < HITBLOWUP_MAX_HIST_COUNT) {
          HitblowupHistEntry* he = &hitblowupHist[hitblowupHistCount];
          TIMES(hitblowupTargetLen, ci) { he->colorsArr[ci] = hitblowupCurrent[ci]; }
          he->len = hitblowupTargetLen;
          he->hit = hit;
          he->blow = blow;
          hitblowupHistCount++;
        }
        hitblowupHitBlowTicks = 60;
        TIMES(hitblowupTargetLen, ci) { hitblowupCurrent[ci] = -1; }
        hitblowupCurrentIndex = 0;
        if (hit == hitblowupTargetLen) {
          addScore((hitblowupSelectorY - hitblowupHistCount * 6) * (hitblowupLoopCount + 1), 70,
                   (hitblowupSelectorY - hitblowupHistCount * 6) / 2 + 9);
          hitblowupNextStageTicks = 60;
        }
        hitblowupHitCount = hit;
        hitblowupBlowCount = blow;
      }
    }
  }
  float hy = hitblowupSelectorY - 3;
  TIMES(hitblowupHistCount, hi) {
    HitblowupHistEntry* hs = &hitblowupHist[hi];
    float hx = 50 - ((hs->len - 1) / 2.0) * 7;
    TIMES(hs->len, ci) {
      color = hitblowupColors[hs->colorsArr[ci]];
      character("a", hx, hy, &scratch);
      hx += 7;
    }
    color = BLACK;
    text(intToChar(hs->hit), 3, hy, &scratch);
    text(intToChar(hs->blow), 96, hy, &scratch);
    hy -= 6;
  }
  hitblowupHitBlowTicks--;
  if (hitblowupHitBlowTicks > 0) {
    text("HIT", 10, hy + 6, &scratch);
    text("BLOW", 70, hy + 6, &scratch);
    if ((int)hitblowupHitBlowTicks % 10 == 0) {
      if (hitblowupHitCount > 0) {
        play(POWER_UP);
        hitblowupHitCount--;
      } else if (hitblowupBlowCount > 0) {
        play(COIN);
        hitblowupBlowCount--;
      }
    }
  }
  hitblowupNextStageTicks--;
  if (hitblowupNextStageTicks >= 0) {
    color = BLACK;
    rect(50, 0, 1, hy + 3, &scratch);
    rect(48, 0, 5, 1, &scratch);
    rect(48, hy + 2, 5, 1, &scratch);
    hitblowupDrawAnswer();
    if (hitblowupNextStageTicks == 0) {
      hitblowupStageCount++;
      hitblowupIsGoingNextStage = true;
    }
    return;
  }
  float cx = 50 - ((hitblowupTargetLen - 1) / 2.0) * 7;
  TIMES(hitblowupTargetLen, ci) {
    if (hitblowupCurrent[ci] < 0) {
      color = LIGHT_BLACK;
      character("b", cx, hy, &scratch);
    } else {
      color = hitblowupColors[hitblowupCurrent[ci]];
      character("a", cx, hy, &scratch);
    }
    cx += 7;
  }
  if (hitblowupNextStageTicks < 0 && hy < 3) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    hitblowupDrawAnswer();
    gameOver();
  }
}

void addGameHitblowup() {
  addGame(hitblowupTitle, hitblowupDescription, hitblowupCharacters,
          hitblowupCharactersCount, &hitblowupOptions, true, &hitblowupUpdate);
}
