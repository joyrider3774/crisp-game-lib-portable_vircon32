#include "../cglp.h"

int* numberlineTitle = "NUMBER LINE";
int* numberlineDescription = "[Tap] Sum";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] numberlineCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int numberlineCharactersCount = 1;

Options numberlineOptions = {100, 100, 60, false};

int[5] numberlineNumberColors = {BLUE, CYAN, GREEN, PURPLE, RED};

struct NumberlineLine {
  float y;
  float ty;
  int index;
};
#define NUMBERLINE_LINE_COUNT 2
NumberlineLine[NUMBERLINE_LINE_COUNT] numberlineLines;

struct NumberlineNextNumber {
  float nextTicks;
  int lineIndex;
};
NumberlineNextNumber[NUMBERLINE_LINE_COUNT] numberlineNextNumbers;

struct NumberlineNumber {
  float value;
  float x;
  float vx;
  int lineIndex;
  bool isSummed;
  bool isAlive;
};
#define NUMBERLINE_MAX_NUMBER_COUNT 64
NumberlineNumber[NUMBERLINE_MAX_NUMBER_COUNT] numberlineNumbers;
int numberlineNumberIndex;

float numberlineSummedTicks;

void numberlineUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - the tappable
  // range and number-merging are direct position/fabs() distance checks
  // (see "n->x > 3 && n->x < 97" and "fabs(an->x - n->x) < 6" below), so
  // the engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is
  // pure waste here. Restored automatically when the next real game
  // starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    numberlineLines[0].y = 12;
    numberlineLines[0].ty = 12;
    numberlineLines[0].index = 0;
    numberlineLines[1].y = 94;
    numberlineLines[1].ty = 94;
    numberlineLines[1].index = 1;
    INIT_UNALIVED_ARRAY_FAST(numberlineNumbers);
    numberlineNumberIndex = 0;
    numberlineNextNumbers[0].nextTicks = 0;
    numberlineNextNumbers[0].lineIndex = 0;
    numberlineNextNumbers[1].nextTicks = 0;
    numberlineNextNumbers[1].lineIndex = 1;
    numberlineSummedTicks = -1;
  }
  TIMES(NUMBERLINE_LINE_COUNT, i) {
    NumberlineLine* l = &numberlineLines[i];
    l->ty += (1 - l->index * 2) * difficulty * 0.01;
    l->y += (l->ty - l->y) * 0.2;
    if (l->index == 0 && l->y < 12) {
      play(COIN);
      l->ty++;
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    }
    if (l->index == 1 && l->y > 94) {
      l->ty--;
    }
    if (l->index == 0) {
      color = BLUE;
    } else {
      color = RED;
    }
    rect(0, l->y, 100, 1, &scratch);
  }
  if (numberlineLines[0].y >= numberlineLines[1].y) {
    play(RANDOM);
    gameOver();
  }
  if (numberlineSummedTicks < 0) {
    if (input.isJustPressed) {
      play(SELECT);
      FOR_EACH(numberlineNumbers, i) {
        ASSIGN_ARRAY_ITEM(numberlineNumbers, i, NumberlineNumber, n);
        SKIP_IS_NOT_ALIVE(n);
        if (n->x > 3 && n->x < 97) {
          n->isSummed = true;
        }
      }
      numberlineSummedTicks = 0;
    }
  } else {
    numberlineSummedTicks++;
  }
  TIMES(NUMBERLINE_LINE_COUNT, i) {
    numberlineNextNumbers[i].nextTicks--;
    if (numberlineNextNumbers[i].nextTicks < 0) {
      ASSIGN_ARRAY_ITEM(numberlineNumbers, numberlineNumberIndex, NumberlineNumber, nn);
      nn->value = rndi(1, 10);
      nn->x = -3;
      nn->vx = rnd(1, difficulty) * 0.7;
      nn->lineIndex = numberlineNextNumbers[i].lineIndex;
      nn->isSummed = false;
      nn->isAlive = true;
      numberlineNumberIndex = cgl_wrap(numberlineNumberIndex + 1, 0, NUMBERLINE_MAX_NUMBER_COUNT);
      numberlineNextNumbers[i].nextTicks = rnd(10, 30) / difficulty;
    }
  }
  float[2] numberlineSums = {0, 0};
  FOR_EACH(numberlineNumbers, i) {
    ASSIGN_ARRAY_ITEM(numberlineNumbers, i, NumberlineNumber, n);
    SKIP_IS_NOT_ALIVE(n);
    NumberlineLine* l = &numberlineLines[n->lineIndex];
    if (n->isSummed) {
      if (numberlineSummedTicks < 0) {
        n->isAlive = false;
        continue;
      }
      bool merged = false;
      for (int j = 0; j < i; j++) {
        NumberlineNumber* an = &numberlineNumbers[j];
        if (!an->isAlive) {
          continue;
        }
        if (an->isSummed && an->lineIndex == n->lineIndex && fabs(an->x - n->x) < 6) {
          an->value += n->value;
          merged = true;
          break;
        }
      }
      if (merged) {
        n->isAlive = false;
        continue;
      }
      n->x += (50 - n->x) * 0.1;
      float x;
      if (l->index == 0) {
        x = n->x;
      } else {
        x = 100 - n->x;
      }
      float summedTicksOfs = 0;
      if (numberlineSummedTicks > 0) {
        summedTicksOfs = 10;
      }
      float y = l->y + l->index * 8 - 4 +
                (numberlineSummedTicks + summedTicksOfs) *
                (1 - l->index * 2) * 0.2 * sqrt(difficulty);
      color = BLACK;
      float textX = x;
      if (n->value > 9) {
        textX -= 3;
      }
      text(intToChar((int)n->value), textX, y, &scratch);
      numberlineSums[l->index] = n->value;
      continue;
    }
    n->x += n->vx;
    float x2;
    if (l->index == 0) {
      x2 = n->x;
    } else {
      x2 = 100 - n->x;
    }
    float y2 = l->y + l->index * 6 - 3;
    color = numberlineNumberColors[(int)((n->value - 1) / 2)];
    text(intToChar((int)n->value), x2, y2, &scratch);
    n->isAlive = n->x <= 103;
  }
  if (numberlineSummedTicks > 60 / sqrt(difficulty)) {
    numberlineSummedTicks = -1;
    float s = numberlineSums[0] - numberlineSums[1];
    if (s > 0) {
      play(POWER_UP);
    } else if (s < 0) {
      play(EXPLOSION);
    }
    addScore(s, 50, 50);
    numberlineLines[0].ty -= s * sqrt(difficulty);
    numberlineLines[1].ty += s * sqrt(difficulty);
  }
}

void addGameNumberline() {
  addGame(numberlineTitle, numberlineDescription, numberlineCharacters,
          numberlineCharactersCount, &numberlineOptions, false,
          &numberlineUpdate);
}
