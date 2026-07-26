#include "../cglp.h"

int* pfitTitle = "P FIT";
int* pfitDescription = "[Slide]\n Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pfitCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int pfitCharactersCount = 1;

Options pfitOptions = {100, 100, 22, false};

#define PFIT_TYPE_FIX 0
#define PFIT_TYPE_MOVE 1
#define PFIT_TYPE_END 2

struct PfitWall {
  Vector pos;
  float width;
  float height;
  float targetY;
  int type;
};
#define PFIT_MAX_WALL_COUNT 40
PfitWall[PFIT_MAX_WALL_COUNT] pfitWalls;
int pfitWallCount;

bool pfitIsAddingWall;
float pfitWallX;
float pfitMatchTicks;
float pfitNextScore;
int pfitPairCount;
float pfitWallWidth;

void pfitUpdate() {
  Collision scratch;
  if (!ticks) {
    pfitWallCount = 0;
    pfitIsAddingWall = true;
    pfitWallX = 0;
    pfitMatchTicks = 0;
    pfitNextScore = 0;
  }
  if (pfitIsAddingWall) {
    pfitPairCount = (int)clamp(floor(6 * difficulty * rnd(0.5, 1)), 6, 20);
    pfitWallWidth = 100.0 / (pfitPairCount - 1);
    float h = rnd(22, 32);
    float[20] hs;
    TIMES(pfitPairCount, i) {
      h += rnd(10, 50) / pfitPairCount;
      hs[i] = h;
    }
    TIMES(99, k) {
      int i1 = rndi(0, pfitPairCount);
      int i2 = rndi(0, pfitPairCount);
      float t = hs[i1];
      hs[i1] = hs[i2];
      hs[i2] = t;
    }
    pfitWallCount = pfitPairCount * 2;
    TIMES(pfitWallCount, i) {
      PfitWall* w = &pfitWalls[i];
      float px = (i % pfitPairCount) * pfitWallWidth - pfitWallWidth / 2;
      float py;
      float ty;
      float wh;
      int wtype;
      if (i < pfitPairCount) {
        py = 75;
        ty = -25;
        wh = hs[i];
        wtype = PFIT_TYPE_FIX;
      } else {
        py = 225;
        ty = 125;
        wh = hs[i - pfitPairCount] - 99;
        wtype = PFIT_TYPE_MOVE;
      }
      vectorSet(&w->pos, px, py);
      w->targetY = ty;
      w->width = pfitWallWidth;
      w->height = wh;
      w->type = wtype;
    }
    pfitWallX = -rnd(10, 90);
    TIMES(9, k) {
      float wrapped = fabs(cgl_wrap(pfitWallX + input.pos.x, (-pfitWallWidth * pfitPairCount) / 2,
                                     (pfitWallWidth * pfitPairCount) / 2));
      if (wrapped > 20) {
        break;
      }
      pfitWallX = -rnd(10, 90);
    }
    pfitNextScore = 300;
    pfitIsAddingWall = false;
  }
  int wi2 = 0;
  while (wi2 < pfitWallCount) {
    PfitWall* w = &pfitWalls[wi2];
    w->pos.y += (w->targetY - w->pos.y) * 0.1;
    bool removed = false;
    if (w->type == PFIT_TYPE_END) {
      float hVal;
      if (w->height > 0) {
        hVal = w->height;
      } else {
        hVal = 0;
      }
      if (w->pos.y + hVal < 1) {
        removed = true;
      }
    } else {
      float dir;
      if (w->type == PFIT_TYPE_FIX) {
        dir = 1;
      } else {
        dir = -1;
      }
      w->targetY += dir * difficulty * 0.02;
    }
    if (!removed) {
      if (w->type == PFIT_TYPE_MOVE && pfitMatchTicks < 30) {
        color = BLUE;
      } else {
        color = GREEN;
      }
      float x;
      if (w->type == PFIT_TYPE_MOVE && pfitMatchTicks <= 30) {
        x = cgl_wrap(w->pos.x + pfitWallX + input.pos.x, -pfitWallWidth, 100);
      } else {
        x = w->pos.x;
      }
      Collision wc;
      rect(x, w->pos.y, ceil(w->width), w->height, &wc);
      if (wc.isColliding.rect[GREEN] && pfitMatchTicks < 30 && w->type == PFIT_TYPE_MOVE) {
        color = RED;
        text("x", x + pfitWallWidth / 2, cgl_wrap(w->pos.y + w->height, -pfitWallWidth, 100),
             &scratch);
        color = BLUE;
        play(EXPLOSION);
        gameOver();
      }
    }
    if (removed) {
      memcpy(&pfitWalls[wi2], &pfitWalls[wi2 + 1], (pfitWallCount - 1 - wi2) * sizeof(pfitWalls[0]));
      pfitWallCount--;
    } else {
      wi2++;
    }
  }
  if (pfitMatchTicks > 30) {
    pfitMatchTicks++;
    if (pfitMatchTicks == 60) {
      TIMES(pfitWallCount, i) {
        PfitWall* w = &pfitWalls[i];
        if (w->type == PFIT_TYPE_FIX) {
          w->targetY = -100;
        } else {
          w->targetY = 0;
        }
        w->type = PFIT_TYPE_END;
      }
      pfitIsAddingWall = true;
      pfitMatchTicks = 0;
    }
  } else if (fabs(cgl_wrap(pfitWallX + input.pos.x, (-pfitWallWidth * pfitPairCount) / 2,
                            (pfitWallWidth * pfitPairCount) / 2)) < 3) {
    pfitMatchTicks++;
    if (pfitMatchTicks > 30) {
      play(POWER_UP);
      TIMES(pfitWallCount, i) {
        PfitWall* w = &pfitWalls[i];
        if (w->type == PFIT_TYPE_FIX) {
          w->targetY = 1;
        } else {
          w->targetY = 99;
        }
      }
      addScore(pfitNextScore, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    }
  } else {
    pfitMatchTicks = 0;
  }
  color = BLACK;
  int[16] scoreText;
  strcpy(scoreText, "+");
  strcat(scoreText, intToChar((int)pfitNextScore));
  text(scoreText, 3, 10, &scratch);
  if (pfitMatchTicks <= 30 && pfitNextScore > 0) {
    pfitNextScore--;
  }
}

void addGamePfit() {
  addGame(pfitTitle, pfitDescription, pfitCharacters, pfitCharactersCount,
          &pfitOptions, true, &pfitUpdate);
}
