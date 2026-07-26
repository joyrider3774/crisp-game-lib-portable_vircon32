#include "../cglp.h"

int* dangoTitle = "DANGO";
int* dangoDescription = "[Tap] Stretch";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] dangoCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int dangoCharactersCount = 0;

Options dangoOptions = {100, 100, 4, false};

#define DANGO_WIDTH 12

struct DangoDango {
  float x;
  float angle;
  float angleVel;
  float radius;
  float height;
  int[8] exp;
  bool isSticked;
  bool isAlive;
};
#define DANGO_MAX_DANGO_COUNT 16
DangoDango[DANGO_MAX_DANGO_COUNT] dangoDangos;
int dangoDangoIndex;

float dangoNextDangoX;
float dangoStickTicks;
int dangoStickLeft;
int dangoStickAdd;
int[64] dangoExpStr;
float dangoExpAdd;
float dangoOffsetX;

// Vircon32 port note: replaces the JS version's
// `Function("return " + expStr + ";")()` dynamic eval - not possible in
// static C. expStr is always a flat chain of single digits separated by
// '+'/'*' (built one 2-char token at a time by this same file, see the
// dango-creation code below), so a plain two-pass evaluate (collapse '*'
// pairs left-to-right, then sum the remaining '+'-separated terms)
// reproduces the exact same operator precedence JS would have applied.
float dangoEvalExpr(int* expr) {
  int len = strlen(expr);
  if (len == 0) {
    return 0;
  }
  float[32] terms;
  int termCount = 0;
  float current = expr[0] - '0';
  int i = 1;
  while (i < len) {
    int op = expr[i];
    float digit = expr[i + 1] - '0';
    if (op == '*') {
      current *= digit;
    } else {
      terms[termCount] = current;
      termCount++;
      current = digit;
    }
    i += 2;
  }
  terms[termCount] = current;
  termCount++;
  float sum = 0;
  TIMES(termCount, k) { sum += terms[k]; }
  return sum;
}

void dangoUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(dangoDangos);
    dangoDangoIndex = 0;
    dangoNextDangoX = 0;
    dangoStickTicks = 0;
    dangoStickLeft = 9;
    dangoExpStr[0] = 0;
    dangoExpAdd = 0;
    dangoOffsetX = 0;
  }
  float sd = sqrt(difficulty);
  if (dangoStickTicks == 0 && input.isJustPressed) {
    play(LASER);
    dangoStickTicks = 1;
    dangoExpStr[0] = 0;
    dangoStickAdd = 0;
    dangoStickLeft--;
  }
  if (dangoStickTicks > 0) {
    dangoStickTicks += sd;
    if (dangoStickTicks < 10) {
      color = BLUE;
    } else {
      color = LIGHT_BLUE;
    }
    float x;
    if (dangoStickTicks < 10) {
      x = 80 - dangoStickTicks * 10;
    } else {
      x = 80 - (10 - dangoStickTicks) * 10;
    }
    rect(x, 75, 120, 2, &scratch);
    if (dangoStickTicks > 19) {
      dangoStickTicks = 0;
      if (dangoExpStr[0] == '+') {
        int len = strlen(dangoExpStr);
        TIMES(len, k) { dangoExpStr[k] = dangoExpStr[k + 1]; }
      } else if (dangoExpStr[0] == '*') {
        int len = strlen(dangoExpStr);
        for (int k = len; k >= 0; k--) {
          dangoExpStr[k + 1] = dangoExpStr[k];
        }
        dangoExpStr[0] = '0';
      }
      if (strlen(dangoExpStr) == 0) {
        dangoExpAdd = 0;
      } else {
        dangoExpAdd = dangoEvalExpr(dangoExpStr);
      }
      addScore(dangoExpAdd, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      int sl = dangoStickLeft;
      dangoStickLeft = (int)clamp(dangoStickLeft + floor(dangoExpAdd / 100), -1, 9);
      dangoStickAdd = dangoStickLeft - sl;
      if (dangoStickAdd > 0) {
        play(POWER_UP);
      } else if (dangoExpAdd > 0) {
        play(COIN);
      }
    }
    dangoOffsetX *= 0.1;
  } else if (dangoStickLeft >= 0) {
    color = CYAN;
    rect(90, 75, 120, 2, &scratch);
  } else {
    gameOver();
    return;
  }
  if (dangoStickLeft > 0) {
    color = BLUE;
    TIMES(dangoStickLeft, i) { rect(92, 70 - i * 3, 8, 1, &scratch); }
  }
  if (dangoStickAdd > 0) {
    int[16] sa;
    strcpy(sa, "+");
    strcat(sa, intToChar(dangoStickAdd));
    text(sa, 90, 30, &scratch);
  }
  dangoOffsetX += 0.002 * sqrt(difficulty);
  float targetX = 80;
  color = BLACK;
  FOR_EACH(dangoDangos, i) {
    ASSIGN_ARRAY_ITEM(dangoDangos, i, DangoDango, d);
    SKIP_IS_NOT_ALIVE(d);
    if (d->isSticked) {
      if (dangoStickTicks > 10) {
        d->x += 20 * sqrt(difficulty);
      }
      if (d->x > 99) {
        dangoNextDangoX -= DANGO_WIDTH;
        int[64] tmpExp;
        strcpy(tmpExp, d->exp);
        strcat(tmpExp, dangoExpStr);
        strcpy(dangoExpStr, tmpExp);
        d->isAlive = false;
        targetX -= DANGO_WIDTH;
        continue;
      }
    } else {
      d->x += (targetX - d->x) * 0.1;
      d->angle += d->angleVel;
    }
    float x = d->x + dangoOffsetX;
    float y = 50 + sin(d->angle) * d->radius;
    color = BLACK;
    box(x, y, 13, d->height, &scratch);
    if (scratch.isColliding.rect[BLUE]) {
      play(HIT);
      d->isSticked = true;
    } else if (scratch.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    }
    color = WHITE;
    box(x, y, 11, d->height - 2, &scratch);
    color = BLACK;
    text(d->exp, x - 3.5, y, &scratch);
    targetX -= DANGO_WIDTH;
  }
  while (targetX > DANGO_WIDTH / 2.0) {
    ASSIGN_ARRAY_ITEM(dangoDangos, dangoDangoIndex, DangoDango, nd);
    nd->x = -DANGO_WIDTH / 2.0;
    nd->angle = rnd(0, CGLP_PI * 2);
    nd->angleVel = rnd(0.02, 0.05) * RNDPM() * sd;
    nd->radius = 30;
    nd->height = rnd(15, 30);
    int opChar;
    if (rnd(0, 1) < 0.5) {
      opChar = '+';
    } else {
      opChar = '*';
    }
    int digit = rndi(2, 10);
    nd->exp[0] = opChar;
    nd->exp[1] = '0' + digit;
    nd->exp[2] = 0;
    nd->isSticked = false;
    nd->isAlive = true;
    dangoDangoIndex = cgl_wrap(dangoDangoIndex + 1, 0, DANGO_MAX_DANGO_COUNT);
    targetX -= DANGO_WIDTH;
  }
  color = BLACK;
  text(dangoExpStr, 3, 96, &scratch);
  if (dangoExpAdd > 0) {
    int[16] ea;
    strcpy(ea, "+");
    strcat(ea, intToChar((int)dangoExpAdd));
    text(ea, 102 - strlen(ea) * 6, 90, &scratch);
  }
}

void addGameDango() {
  addGame(dangoTitle, dangoDescription, dangoCharacters, dangoCharactersCount,
          &dangoOptions, false, &dangoUpdate);
}
