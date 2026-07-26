#include "../cglp.h"

int* bmathTitle = "B MATH";
int* bmathDescription = "[Tap]\n Answer";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bmathCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int bmathCharactersCount = 0;

Options bmathOptions = {100, 100, 1, false};

#define BMATH_OP_ADD 0
#define BMATH_OP_SUB 1
#define BMATH_OP_MUL 2
#define BMATH_OP_DIV 3
int*[4] bmathOpChars = {"+", "-", "X", "/"};

int[2] bmathValues;
int bmathOperator;
int[5] bmathAnss;
int bmathAns;
float bmathLeftTime;
float bmathTargetTime;
float bmathAnsTicks;
int bmathAnsIndex;
int bmathMultiplier;

void bmathNextQuestion() {
  float cd = clamp((difficulty - 1) * 2, 0, 1);
  if (rnd(0, cd) > 0.9) {
    bmathOperator = BMATH_OP_DIV;
  } else if (rnd(0, cd) > 0.7) {
    bmathOperator = BMATH_OP_MUL;
  } else if (rnd(0, cd) > 0.5) {
    bmathOperator = BMATH_OP_SUB;
  } else {
    bmathOperator = BMATH_OP_ADD;
  }
  if (bmathOperator == BMATH_OP_DIV || bmathOperator == BMATH_OP_MUL) {
    bmathValues[1] = rndi(2, 10);
    bmathAns = floor(rnd(10, 100) / bmathValues[1]);
    bmathValues[0] = bmathValues[1] * bmathAns;
    if (bmathOperator == BMATH_OP_MUL) {
      int a = bmathAns;
      bmathAns = bmathValues[0];
      bmathValues[0] = a;
      if (rnd(0, 1) < 0.5) {
        int v0 = bmathValues[0];
        bmathValues[0] = bmathValues[1];
        bmathValues[1] = v0;
      }
    }
  } else {
    bmathAns = rndi(10, 100);
    bmathValues[0] = rndi(1, bmathAns);
    bmathValues[1] = bmathAns - bmathValues[0];
    if (bmathOperator == BMATH_OP_SUB) {
      int v = bmathValues[0];
      bmathValues[0] = bmathAns;
      bmathAns = bmathValues[1];
      bmathValues[1] = v;
    }
  }
  int ci = rndi(0, 5);
  TIMES(5, i) {
    if (i == ci) {
      bmathAnss[i] = bmathAns;
      continue;
    }
    TIMES(9, j) {
      float a = bmathAns + rnd(0, 5) * RNDPM();
      if (rnd(0, 1) < 0.3) {
        a += a * (rnd(0.1, 0.2) * RNDPM());
      }
      a = clamp(round(a), 1, 99);
      bmathAnss[i] = (int)a;
      bool iv = (int)a != bmathAns;
      TIMES(i, k) {
        if (bmathAnss[k] == (int)a) {
          iv = false;
        }
      }
      if (iv) {
        break;
      }
    }
  }
}

void bmathUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - the tapped
  // answer index is computed directly from input.pos via grid math (see
  // "floor(input.pos.x / 20)" below), so the engine's own O(n^2) hitbox
  // scan (see checkHitBox() in cglp.c) is pure waste here. Restored
  // automatically when the next real game starts, via resetDrawState() in
  // initInGame().
  hasCollision = false;
  float sd = sqrt(difficulty);
  if (!ticks) {
    bmathValues[0] = 0;
    bmathValues[1] = 0;
    TIMES(5, i) { bmathAnss[i] = 0; }
    bmathNextQuestion();
    bmathLeftTime = CGLP_PI * 2;
    bmathAnsTicks = 0;
    bmathAnsIndex = 0;
    bmathMultiplier = 1;
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(bmathMultiplier));
  text(multText, 3, 9, &scratch);
  float v0Ofs;
  if (bmathValues[0] > 9) {
    v0Ofs = 6;
  } else {
    v0Ofs = 0;
  }
  text(intToChar(bmathValues[0]), 50 - v0Ofs, 36, &scratch);
  float v1Ofs;
  if (bmathValues[1] > 9) {
    v1Ofs = 6;
  } else {
    v1Ofs = 0;
  }
  text(intToChar(bmathValues[1]), 50 - v1Ofs, 45, &scratch);
  text(bmathOpChars[bmathOperator], 35, 41, &scratch);
  rect(25, 50, 40, 1, &scratch);
  if (bmathAnsTicks > 0) {
    float ansOfs;
    if (bmathAns > 9) {
      ansOfs = 6;
    } else {
      ansOfs = 0;
    }
    text(intToChar(bmathAns), 50 - ansOfs, 55, &scratch);
    bmathLeftTime += (bmathTargetTime - bmathLeftTime) * 0.1;
    bmathAnsTicks--;
    if (bmathAnsTicks <= 0) {
      bmathNextQuestion();
    }
  } else {
    bmathAnsIndex = (int)floor(input.pos.x / 20);
    if (input.isJustPressed) {
      if (bmathAnsIndex >= 0 && bmathAnsIndex < 5) {
        bmathAnsTicks = 30 / sd;
        addScore(ceil(bmathLeftTime * 9 * bmathMultiplier), SCORE_NO_POPUP_X,
                  SCORE_NO_POPUP_Y);
        float dir;
        if (bmathAnss[bmathAnsIndex] == bmathAns) {
          dir = 2;
        } else {
          dir = -1;
        }
        float tt = bmathLeftTime + CGLP_PI_2 * dir;
        if (bmathAnss[bmathAnsIndex] == bmathAns) {
          if (tt >= CGLP_PI * 2) {
            bmathMultiplier++;
            play(POWER_UP);
          } else {
            play(COIN);
          }
        } else {
          if (bmathMultiplier > 1) {
            bmathMultiplier--;
            play(EXPLOSION);
          }
        }
        bmathTargetTime = clamp(tt, 0, CGLP_PI * 2);
      }
    }
  }
  bmathLeftTime -= sd * 0.01;
  if (bmathLeftTime < 0) {
    bmathLeftTime = 0;
    float ansOfs2;
    if (bmathAns > 9) {
      ansOfs2 = 6;
    } else {
      ansOfs2 = 0;
    }
    text(intToChar(bmathAns), 50 - ansOfs2, 55, &scratch);
    play(EXPLOSION);
    gameOver();
  }
  thickness = 3;
  arc(45, 45, 33, CGLP_PI_2 + bmathLeftTime, CGLP_PI_2, &scratch);
  TIMES(5, i) {
    color = BLACK;
    if (bmathAnsIndex == i) {
      box(i * 20 + 10, 85, 18, 8, &scratch);
      if (bmathAnsTicks > 0) {
        if (bmathAnss[i] == bmathAns) {
          text("OK", i * 20 + 7, 92, &scratch);
        } else {
          text("NG", i * 20 + 7, 92, &scratch);
        }
      }
      color = WHITE;
    }
    float aOfs;
    if (bmathAnss[i] > 9) {
      aOfs = 6;
    } else {
      aOfs = 0;
    }
    text(intToChar(bmathAnss[i]), i * 20 + 13 - aOfs, 85, &scratch);
  }
}

void addGameBmath() {
  addGame(bmathTitle, bmathDescription, bmathCharacters, bmathCharactersCount,
          &bmathOptions, true, &bmathUpdate);
}
