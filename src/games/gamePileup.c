#include "../cglp.h"

int* pileupTitle = "PILEUP";
int* pileupDescription = "[Slide] Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pileupCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int pileupCharactersCount = 0;

Options pileupOptions = {100, 100, 3, false};

struct PileupBoard {
  Vector pos;
  float vy;
};
PileupBoard pileupBoard;
bool pileupHasBoard;

struct PileupBox {
  Vector pos;
  bool isFixed;
  bool isAlive;
};
// Fixed boxes never expire (only removed by the rare falling-board sweep); floor
// is ~14 columns wide (99/7) x ~11 stack rows (80/7) before "by" hits 0 and ends
// the run, so ~150 concurrent fixed boxes is realistic - 32 was far too small.
#define PILEUP_MAX_BOX_COUNT 256
PileupBox[PILEUP_MAX_BOX_COUNT] pileupBoxes;
int pileupBoxIndex;
float pileupBoxAddTicks;
Vector pileupPp;

void pileupUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(pileupBoxes);
    pileupBoxIndex = 0;
    pileupBoxAddTicks = 0;
    vectorSet(&pileupPp, 0, 0);
    pileupHasBoard = false;
  }
  if (pileupHasBoard) {
    pileupBoard.vy += difficulty * 0.1;
    pileupBoard.pos.y += pileupBoard.vy;
    color = RED;
    box(pileupBoard.pos.x, pileupBoard.pos.y, 20, 5, &scratch);
    if (pileupBoard.pos.y > 99) {
      pileupHasBoard = false;
      play(SELECT);
      addScore(99 - pileupPp.y, pileupPp.x, pileupPp.y);
    }
  } else {
    vectorSet(&pileupBoard.pos, rnd(0, 99), 0);
    pileupBoard.vy = 0;
    pileupHasBoard = true;
  }
  float by = 80;
  color = BLACK;
  rect(0, 95, 99, 5, &scratch);
  FOR_EACH(pileupBoxes, i) {
    ASSIGN_ARRAY_ITEM(pileupBoxes, i, PileupBox, b);
    SKIP_IS_NOT_ALIVE(b);
    if (b->isFixed) {
      if (b->pos.y - 9 < by) {
        by = b->pos.y - 9;
      }
    } else {
      b->pos.y += difficulty;
    }
    if (b->isFixed) {
      color = BLACK;
    } else {
      color = LIGHT_BLACK;
    }
    box(b->pos.x, b->pos.y, 7, 7, &scratch);
    if (!b->isFixed) {
      if (scratch.isColliding.rect[BLACK]) {
        b->isFixed = true;
      }
    }
    if (scratch.isColliding.rect[RED]) {
      play(HIT);
      b->isAlive = false;
      continue;
    }
  }
  color = GREEN;
  vectorSet(&pileupPp, clamp(input.pos.x, 0, 99), by);
  box(pileupPp.x, pileupPp.y, 7, 7, &scratch);
  if (scratch.isColliding.rect[RED] || pileupPp.y < 0) {
    play(RANDOM);
    gameOver();
  }
  pileupBoxAddTicks--;
  if (pileupBoxAddTicks < 0) {
    ASSIGN_ARRAY_ITEM(pileupBoxes, pileupBoxIndex, PileupBox, nb);
    nb->pos = pileupPp;
    nb->isFixed = false;
    nb->isAlive = true;
    pileupBoxIndex = cgl_wrap(pileupBoxIndex + 1, 0, PILEUP_MAX_BOX_COUNT);
    pileupBoxAddTicks += 10 / difficulty;
  }
}

void addGamePileup() {
  addGame(pileupTitle, pileupDescription, pileupCharacters, pileupCharactersCount,
          &pileupOptions, true, &pileupUpdate);
}
