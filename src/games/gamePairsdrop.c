#include "../cglp.h"

int* pairsdropTitle = "PAIRS DROP";
int* pairsdropDescription = "[Tap]\n Open";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pairsdropCharacters = {
    {
        "l  l  ",
        "l l l ",
        "l l l ",
        "l l l ",
        "l  l  ",
    },
    {
        "l l l ",
        " l l  ",
        "l l l ",
        " l l  ",
        "l l l ",
    },
};
int pairsdropCharactersCount = 2;

Options pairsdropOptions = {100, 100, 101, false};

struct PairsdropCard {
  Vector pos;
  int n;
  bool isOpen;
  float vy;
  bool isAlive;
};
#define PAIRSDROP_MAX_CARD_COUNT 128
PairsdropCard[PAIRSDROP_MAX_CARD_COUNT] pairsdropCards;
int pairsdropCardIndex;

Vector pairsdropNextCardPos;
float pairsdropScr;
#define PAIRSDROP_MAX_OPENED_COUNT 2
int[PAIRSDROP_MAX_OPENED_COUNT] pairsdropOpenedCardIndices;
int pairsdropOpenedCardCount;
float pairsdropOpenedTicks;
int pairsdropMultiplier;

void pairsdropFallCards(float px, float py) {
  FOR_EACH(pairsdropCards, i) {
    ASSIGN_ARRAY_ITEM(pairsdropCards, i, PairsdropCard, c);
    SKIP_IS_NOT_ALIVE(c);
    if (c->pos.x == px && c->pos.y >= py) {
      c->vy = 1;
    }
  }
}

void pairsdropAddCards() {
  int[26] ns;
  TIMES(26, i) { ns[i] = i % 13; }
  TIMES(99, k) {
    int i1 = rndi(0, 26);
    int i2 = rndi(0, 26);
    int tn = ns[i1];
    ns[i1] = ns[i2];
    ns[i2] = tn;
  }
  TIMES(26, i) {
    ASSIGN_ARRAY_ITEM(pairsdropCards, pairsdropCardIndex, PairsdropCard, nc);
    nc->pos = pairsdropNextCardPos;
    nc->n = ns[i];
    nc->isOpen = false;
    nc->vy = 0;
    nc->isAlive = true;
    pairsdropCardIndex = cgl_wrap(pairsdropCardIndex + 1, 0, PAIRSDROP_MAX_CARD_COUNT);
    pairsdropNextCardPos.x += 10;
    if (pairsdropNextCardPos.x > 80) {
      pairsdropNextCardPos.x = 20;
      pairsdropNextCardPos.y -= 12;
    }
  }
}

void pairsdropUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - a card tap is
  // a direct fabs() distance check against input.pos (see the
  // "fabs(c->pos.x - input.pos.x) < 5" comparison below), so the engine's
  // own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure waste
  // here. Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(pairsdropCards);
    pairsdropCardIndex = 0;
    vectorSet(&pairsdropNextCardPos, 20, -6);
    pairsdropScr = 0;
    pairsdropOpenedCardCount = 0;
    pairsdropOpenedTicks = 0;
    pairsdropMultiplier = 1;
  }
  float minY = 99;
  float maxY = 0;
  pairsdropOpenedTicks--;
  if (pairsdropOpenedTicks == 0) {
    TIMES(pairsdropOpenedCardCount, i) {
      pairsdropCards[pairsdropOpenedCardIndices[i]].isOpen = false;
    }
    pairsdropOpenedCardCount = 0;
  }
  bool isOpening = false;
  FOR_EACH(pairsdropCards, i) {
    ASSIGN_ARRAY_ITEM(pairsdropCards, i, PairsdropCard, c);
    SKIP_IS_NOT_ALIVE(c);
    int cl;
    if (c->isOpen) {
      if (pairsdropOpenedTicks > 0) {
        cl = GREEN;
      } else {
        cl = CYAN;
      }
    } else {
      cl = BLUE;
    }
    color = cl;
    rect(c->pos.x - 4, c->pos.y - 5, 9, 11, &scratch);
    color = WHITE;
    rect(c->pos.x - 3, c->pos.y - 4, 7, 9, &scratch);
    color = cl;
    c->pos.y += pairsdropScr + c->vy;
    if (c->vy > 0) {
      c->vy += 0.1;
    }
    if (c->isOpen) {
      if (c->n == 0) {
        text("A", c->pos.x, c->pos.y, &scratch);
      } else if (c->n < 9) {
        text(intToChar(c->n + 1), c->pos.x, c->pos.y, &scratch);
      } else if (c->n == 9) {
        character("a", c->pos.x, c->pos.y, &scratch);
      } else {
        int[2] fc;
        if (c->n == 10) {
          fc[0] = 'J';
        } else if (c->n == 11) {
          fc[0] = 'Q';
        } else {
          fc[0] = 'K';
        }
        fc[1] = 0;
        text(fc, c->pos.x, c->pos.y, &scratch);
      }
    } else {
      character("b", c->pos.x, c->pos.y, &scratch);
      if (c->vy == 0 && input.isJustPressed && fabs(c->pos.x - input.pos.x) < 5 &&
          fabs(c->pos.y - input.pos.y) < 6) {
        play(HIT);
        if (pairsdropOpenedTicks >= 0) {
          TIMES(pairsdropOpenedCardCount, k) {
            pairsdropCards[pairsdropOpenedCardIndices[k]].isOpen = false;
          }
          pairsdropOpenedCardCount = 0;
          pairsdropOpenedTicks = 0;
        }
        isOpening = true;
        c->isOpen = true;
        if (pairsdropOpenedCardCount < PAIRSDROP_MAX_OPENED_COUNT) {
          pairsdropOpenedCardIndices[pairsdropOpenedCardCount] = i;
          pairsdropOpenedCardCount++;
        }
      }
    }
    if (c->pos.y < minY) {
      minY = c->pos.y;
    }
    if (c->pos.y > maxY) {
      maxY = c->pos.y;
    }
    if (c->vy == 0 && c->pos.y > 95) {
      play(EXPLOSION);
      color = RED;
      text("X", c->pos.x, c->pos.y, &scratch);
      gameOver();
    }
    if (c->pos.y > 99) {
      play(COIN);
      addScore(pairsdropMultiplier, c->pos.x, c->pos.y);
      pairsdropMultiplier++;
      c->isAlive = false;
      continue;
    }
  }
  if (pairsdropOpenedTicks < 0 && pairsdropOpenedCardCount == 2) {
    if (pairsdropCards[pairsdropOpenedCardIndices[0]].n ==
        pairsdropCards[pairsdropOpenedCardIndices[1]].n) {
      play(POWER_UP);
      pairsdropFallCards(pairsdropCards[pairsdropOpenedCardIndices[0]].pos.x,
                         pairsdropCards[pairsdropOpenedCardIndices[0]].pos.y);
      pairsdropFallCards(pairsdropCards[pairsdropOpenedCardIndices[1]].pos.x,
                         pairsdropCards[pairsdropOpenedCardIndices[1]].pos.y);
      pairsdropOpenedCardCount = 0;
      pairsdropMultiplier = 1;
    } else {
      play(LASER);
    }
    pairsdropOpenedTicks = 60;
  }
  pairsdropNextCardPos.y += pairsdropScr;
  float openBonus;
  if (isOpening) {
    openBonus = 0.15;
  } else {
    openBonus = 0;
  }
  pairsdropScr += (difficulty * 0.0015 - pairsdropScr) * 0.2 + openBonus;
  if (maxY < 50) {
    pairsdropScr += (50 - maxY) * 0.01;
  }
  if (minY > -6) {
    pairsdropAddCards();
  }
}

void addGamePairsdrop() {
  addGame(pairsdropTitle, pairsdropDescription, pairsdropCharacters,
          pairsdropCharactersCount, &pairsdropOptions, true, &pairsdropUpdate);
}
