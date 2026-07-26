#include "../cglp.h"

int* fromtwosidesTitle = "FROM TWO SIDES";
int* fromtwosidesDescription = "[Slide]\n Move";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] fromtwosidesCharacters = {
    {
        "rrrrrr",
        "rRRRRr",
        " rRRr ",
    },
    {
        "rRRr  ",
        " rr   ",
        " rr   ",
    },
    {
        " G    ",
        "GgG   ",
        " G    ",
    },
};
int fromtwosidesCharactersCount = 3;

Options fromtwosidesOptions = {100, 100, 9, false};

struct FromtwosidesArrow {
  Vector pos;
  float vy;
  int wy;
  bool isAlive;
};
#define FROMTWOSIDES_MAX_ARROW_COUNT 64
FromtwosidesArrow[FROMTWOSIDES_MAX_ARROW_COUNT] fromtwosidesArrows;
int fromtwosidesArrowIndex;
int[2] fromtwosidesNextArrowTicks;

struct FromtwosidesSafe {
  float x;
  float vx;
};
#define FROMTWOSIDES_SAFE_COUNT 2
FromtwosidesSafe[FROMTWOSIDES_SAFE_COUNT] fromtwosidesSafes;

void fromtwosidesUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(fromtwosidesArrows);
    fromtwosidesArrowIndex = 0;
    fromtwosidesNextArrowTicks[0] = 0;
    fromtwosidesNextArrowTicks[1] = 60;
    fromtwosidesSafes[0].x = 50;
    fromtwosidesSafes[0].vx = -1;
    fromtwosidesSafes[1].x = 50;
    fromtwosidesSafes[1].vx = 1;
  }
  TIMES(FROMTWOSIDES_SAFE_COUNT, i) {
    ASSIGN_ARRAY_ITEM(fromtwosidesSafes, i, FromtwosidesSafe, s);
    s->x += s->vx;
    if ((s->x < 9 && s->vx < 0) || (s->x > 90 && s->vx > 0)) {
      s->vx *= -1;
    }
    s->vx += rnd(0, 0.5) * RNDPM();
    s->vx *= 0.98;
  }
  TIMES(2, i) {
    fromtwosidesNextArrowTicks[i]--;
    if (fromtwosidesNextArrowTicks[i] < 0) {
      play(EXPLOSION);
      float w = rnd(10, 40) / sqrt(difficulty) + 10;
      TIMES(17, xi) {
        float x = xi * 6 + 2;
        bool isSafe = false;
        TIMES(FROMTWOSIDES_SAFE_COUNT, si) {
          ASSIGN_ARRAY_ITEM(fromtwosidesSafes, si, FromtwosidesSafe, s);
          if (fabs(x - s->x) < w / 2) {
            isSafe = true;
          }
        }
        if (!isSafe) {
          float ay;
          int wy;
          if (i == 0) {
            ay = -3;
            wy = 1;
          } else {
            ay = 103;
            wy = -1;
          }
          ASSIGN_ARRAY_ITEM(fromtwosidesArrows, fromtwosidesArrowIndex, FromtwosidesArrow, a);
          vectorSet(&a->pos, x, ay);
          a->vy = 0;
          a->wy = wy;
          a->isAlive = true;
          fromtwosidesArrowIndex = cgl_wrap(fromtwosidesArrowIndex + 1, 0, FROMTWOSIDES_MAX_ARROW_COUNT);
          addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        }
      }
      fromtwosidesNextArrowTicks[i] = rnd(60, 90) / sqrt(difficulty);
    }
  }
  float s = difficulty * 2;
  FOR_EACH(fromtwosidesArrows, i) {
    ASSIGN_ARRAY_ITEM(fromtwosidesArrows, i, FromtwosidesArrow, a);
    SKIP_IS_NOT_ALIVE(a);
    a->vy += (a->wy * s - a->vy) * 0.05;
    a->pos.y += a->vy;
    if (a->wy > 0) {
      character("a", a->pos.x, a->pos.y - 1, &scratch);
      character("b", a->pos.x, a->pos.y + 2, &scratch);
    } else {
      characterOptions.isMirrorY = true;
      character("a", a->pos.x, a->pos.y + 1, &scratch);
      character("b", a->pos.x, a->pos.y - 2, &scratch);
      characterOptions.isMirrorY = false;
    }
    color = RED;
    particle(a->pos.x, a->pos.y, 0.4, -fabs(a->vy), (CGLP_PI / 2) * a->wy, 0.3);
    color = BLACK;
    a->isAlive = !(a->pos.y < -3 || a->pos.y > 103);
  }
  float x = clamp(input.pos.x, 1, 98);
  Collision cc;
  character("c", x, 50, &cc);
  if (cc.isColliding.character['a'] || cc.isColliding.character['b']) {
    play(POWER_UP);
    gameOver();
  }
}

void addGameFromtwosides() {
  addGame(fromtwosidesTitle, fromtwosidesDescription, fromtwosidesCharacters,
          fromtwosidesCharactersCount, &fromtwosidesOptions, true,
          &fromtwosidesUpdate);
}
