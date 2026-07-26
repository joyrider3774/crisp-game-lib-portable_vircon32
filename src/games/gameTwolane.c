#include "../cglp.h"

int* twolaneTitle = "TWO LANE";
int* twolaneDescription = "[Tap]\n Change Lane\n[Hold] \n Accel";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] twolaneCharacters = {
    {
        "ll  ll",
        "llrrll",
        "  rr  ",
        " rppr ",
        "llrrll",
        "ll  ll",
    },
    {
        " yyyy ",
        "yyYYyy",
        "yyYYyy",
        "yyYYyy",
        "yyYYyy",
        " yyyy ",
    },
};
int twolaneCharactersCount = 2;

Options twolaneOptions = {100, 100, 10, false};

#define TWOLANE_TYPE_LEFT 0
#define TWOLANE_TYPE_RIGHT 1
#define TWOLANE_TYPE_BOTH 2
#define TWOLANE_COIN_NONE 0
#define TWOLANE_COIN_LEFT 1
#define TWOLANE_COIN_RIGHT 2

struct TwolaneRoad {
  float y;
  int type;
  int coin;
  int index;
};
#define TWOLANE_ROAD_COUNT 12
TwolaneRoad[TWOLANE_ROAD_COUNT] twolaneRoads;

float twolaneSpeed;
float twolaneX;
float twolaneTargetX;
int twolaneCurrentType;
float twolaneLastScore;
int twolaneMultiplier;
int twolaneLastMultiplier;
bool twolaneIsOutOfCourse;

void twolaneUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(TWOLANE_ROAD_COUNT, i) {
      twolaneRoads[i].y = i * 10 - 10;
      twolaneRoads[i].type = TWOLANE_TYPE_BOTH;
      twolaneRoads[i].coin = TWOLANE_COIN_NONE;
      twolaneRoads[i].index = i;
    }
    twolaneSpeed = 1;
    twolaneX = 30;
    twolaneTargetX = 30;
    twolaneCurrentType = TWOLANE_TYPE_BOTH;
    twolaneLastScore = 0;
    twolaneMultiplier = 1;
    twolaneLastMultiplier = 1;
    twolaneIsOutOfCourse = false;
  }
  color = LIGHT_GREEN;
  rect(0, 0, 100, 100, &scratch);
  if (input.isJustPressed) {
    play(LASER);
    if (twolaneTargetX == 30) {
      twolaneTargetX = 70;
    } else {
      twolaneTargetX = 30;
    }
  }
  if (twolaneIsOutOfCourse) {
    twolaneSpeed *= 1 - 0.1 * sqrt(difficulty);
    if (twolaneSpeed < 0.1) {
      play(RANDOM);
      gameOver();
    }
  } else if (input.isPressed) {
    twolaneSpeed += difficulty * 0.02;
  } else {
    twolaneSpeed += (difficulty - twolaneSpeed) * 0.1;
  }
  float sl = clamp(difficulty * 3, 1, 9);
  if (twolaneSpeed > sl) {
    twolaneSpeed = sl;
  }
  twolaneX += (twolaneTargetX - twolaneX) * (0.15 * sqrt(twolaneSpeed));
  color = BLACK;
  character("a", twolaneX, 90, &scratch);
  TIMES(TWOLANE_ROAD_COUNT, i) {
    TwolaneRoad* r = &twolaneRoads[i];
    float py = r->y;
    r->y = cgl_wrap(r->y + twolaneSpeed, -10, 110);
    if (r->y < py) {
      if (rnd(0, 1) < 0.1) {
        if (twolaneCurrentType == TWOLANE_TYPE_BOTH) {
          if (rnd(0, 1) < 0.5) {
            r->type = TWOLANE_TYPE_LEFT;
          } else {
            r->type = TWOLANE_TYPE_RIGHT;
          }
        } else {
          r->type = TWOLANE_TYPE_BOTH;
        }
        twolaneCurrentType = r->type;
      } else {
        r->type = twolaneCurrentType;
      }
      if (r->coin != TWOLANE_COIN_NONE) {
        if (twolaneMultiplier > 1) {
          twolaneMultiplier--;
        }
      }
      if (rnd(0, 1) < 0.2) {
        if (r->type == TWOLANE_TYPE_BOTH) {
          if (rnd(0, 1) < 0.5) {
            r->coin = TWOLANE_COIN_LEFT;
          } else {
            r->coin = TWOLANE_COIN_RIGHT;
          }
        } else if (r->type == TWOLANE_TYPE_LEFT) {
          r->coin = TWOLANE_COIN_LEFT;
        } else {
          r->coin = TWOLANE_COIN_RIGHT;
        }
      } else {
        r->coin = TWOLANE_COIN_NONE;
      }
    }
    color = LIGHT_BLACK;
    float rx;
    if (r->type == TWOLANE_TYPE_RIGHT) {
      rx = 50;
    } else {
      rx = 10;
    }
    float rw;
    if (r->type == TWOLANE_TYPE_BOTH) {
      rw = 80;
    } else {
      rw = 40;
    }
    rect(rx, r->y, rw, 10, &scratch);
    color = WHITE;
    if (r->index % 2 == 0) {
      if (r->type == TWOLANE_TYPE_LEFT || r->type == TWOLANE_TYPE_BOTH) {
        rect(11, r->y, 5, 10, &scratch);
      }
      if (r->type == TWOLANE_TYPE_RIGHT || r->type == TWOLANE_TYPE_BOTH) {
        rect(84, r->y, 5, 10, &scratch);
      }
    } else {
      float lx;
      if (r->type == TWOLANE_TYPE_RIGHT) {
        lx = 50;
      } else {
        lx = 47;
      }
      float lw;
      if (r->type == TWOLANE_TYPE_BOTH) {
        lw = 6;
      } else {
        lw = 3;
      }
      rect(lx, r->y, lw, 10, &scratch);
    }
    if (r->coin != TWOLANE_COIN_NONE) {
      color = BLACK;
      float cx;
      if (r->coin == TWOLANE_COIN_LEFT) {
        cx = 30;
      } else {
        cx = 70;
      }
      Collision cc;
      character("b", cx, r->y + 5, &cc);
      if (cc.isColliding.character['a']) {
        play(COIN);
        twolaneLastScore = floor(twolaneSpeed * twolaneSpeed + 1);
        twolaneLastMultiplier = twolaneMultiplier;
        addScore(twolaneLastScore * twolaneMultiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        if (twolaneMultiplier < 32) {
          twolaneMultiplier++;
        }
        r->coin = TWOLANE_COIN_NONE;
      }
    }
  }
  color = BLACK;
  Collision c;
  character("a", twolaneX, 90, &c);
  if (!c.isColliding.rect[LIGHT_BLACK] && !c.isColliding.rect[WHITE]) {
    play(HIT);
    twolaneIsOutOfCourse = true;
    twolaneMultiplier = 1;
  } else {
    twolaneIsOutOfCourse = false;
  }
  if (twolaneLastScore > 0) {
    color = BLACK;
    int[16] scoreText;
    strcpy(scoreText, "+");
    strcat(scoreText, intToChar((int)twolaneLastScore));
    strcat(scoreText, "x");
    strcat(scoreText, intToChar(twolaneLastMultiplier));
    text(scoreText, 3, 9, &scratch);
  }
}

void addGameTwolane() {
  addGame(twolaneTitle, twolaneDescription, twolaneCharacters,
          twolaneCharactersCount, &twolaneOptions, false, &twolaneUpdate);
}
