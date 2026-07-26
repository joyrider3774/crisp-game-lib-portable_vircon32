#include "../cglp.h"

int* paintballTitle = "PAINT BALL";
int* paintballDescription = "[Tap] Throw";

// Vircon32 port note: drawn with color=BLACK, activating this engine's
// per-pixel multi-color glyph rendering (see gameCount.c's port note and
// colorGridChars/setColorGrid() in cglp.c) - 'y' renders yellow, 'c'/'b'
// render cyan/blue directly from the template letters.
int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] paintballCharacters = {
    {
        " llll ",
        "lyyyyl",
        "lyyyyl",
        "lyyyyl",
        "lyyyyl",
        " llll ",
    },
    {
        " cccc ",
        "cbbbbc",
        "cbbbbc",
        "cbbbbc",
        "cbbbbc",
        " cccc ",
    },
};
int paintballCharactersCount = 2;

Options paintballOptions = {100, 100, 5, false};

#define PAINTBALL_GRID_COUNT 12
#define PAINTBALL_GRID_ROWS 15

int[PAINTBALL_GRID_COUNT][PAINTBALL_GRID_ROWS] paintballGrid;
float paintballGridY;

struct PaintballBall {
  Vector pos;
  Vector vel;
  int color;
  float paintingCount;
  bool isAlive;
};
#define PAINTBALL_MAX_BALL_COUNT 32
PaintballBall[PAINTBALL_MAX_BALL_COUNT] paintballBalls;
int paintballBallIndex;
float paintballNextBallTicks;

struct PaintballWaitingBall {
  Vector pos;
  float angle;
  float va;
};
PaintballWaitingBall paintballWaitingBall;
bool paintballHasWaitingBall;

int paintballMultiplier;

void paintballUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(PAINTBALL_GRID_COUNT, x) {
      TIMES(PAINTBALL_GRID_ROWS, y) { paintballGrid[x][y] = 0; }
    }
    paintballGridY = 0;
    INIT_UNALIVED_ARRAY_FAST(paintballBalls);
    paintballBallIndex = 0;
    paintballNextBallTicks = 0;
    vectorSet(&paintballWaitingBall.pos, 9, 70);
    paintballWaitingBall.angle = 0;
    paintballWaitingBall.va = -1;
    paintballHasWaitingBall = true;
    paintballMultiplier = 1;
  }
  float scr = sqrt(difficulty) * 0.03;
  color = LIGHT_BLACK;
  box(4, 50, 4, 100, &scratch);
  box(95, 50, 4, 100, &scratch);
  paintballGridY += scr;
  if (paintballGridY > 4) {
    paintballGridY -= 7;
    TIMES(PAINTBALL_GRID_COUNT, x) {
      for (int y = 0; y < PAINTBALL_GRID_COUNT + 2; y++) {
        paintballGrid[x][PAINTBALL_GRID_COUNT + 2 - y] =
            paintballGrid[x][PAINTBALL_GRID_COUNT + 1 - y];
      }
      paintballGrid[x][0] = 0;
    }
  }
  TIMES(PAINTBALL_GRID_COUNT, x) {
    TIMES(PAINTBALL_GRID_ROWS, y) {
      int g = paintballGrid[x][y];
      if (g == 1) {
        color = YELLOW;
      } else if (g == 2) {
        color = BLUE;
      } else {
        color = LIGHT_BLACK;
      }
      box(50 + (x - (PAINTBALL_GRID_COUNT - 1) / 2.0) * 7,
          50 + (y - (PAINTBALL_GRID_COUNT + 2) / 2.0) * 7 + paintballGridY, 6, 6, &scratch);
    }
  }
  color = BLACK;
  if (paintballHasWaitingBall) {
    paintballWaitingBall.pos.y += scr;
    if (paintballWaitingBall.pos.y > 95) {
      paintballWaitingBall.pos.y = 95;
    }
    paintballWaitingBall.angle += paintballWaitingBall.va * sqrt(difficulty) * 0.02;
    if ((paintballWaitingBall.va < 0 && paintballWaitingBall.angle < -CGLP_PI / 4) ||
        (paintballWaitingBall.va > 0 && paintballWaitingBall.angle > CGLP_PI / 4)) {
      paintballWaitingBall.va *= -1;
    }
    float a;
    if (paintballWaitingBall.pos.x < 50) {
      a = paintballWaitingBall.angle;
    } else {
      a = CGLP_PI - paintballWaitingBall.angle;
    }
    thickness = 2;
    barCenterPosRatio = 0;
    bar(paintballWaitingBall.pos.x, paintballWaitingBall.pos.y, 20, a, &scratch);
    character("a", paintballWaitingBall.pos.x, paintballWaitingBall.pos.y, &scratch);
    if (input.isJustPressed) {
      play(SELECT);
      ASSIGN_ARRAY_ITEM(paintballBalls, paintballBallIndex, PaintballBall, nb);
      nb->pos = paintballWaitingBall.pos;
      Vector v;
      vectorSet(&v, sqrt(difficulty) * 2, 0);
      rotate(&v, a);
      nb->vel = v;
      nb->color = 1;
      nb->paintingCount = 0;
      nb->isAlive = true;
      paintballBallIndex = cgl_wrap(paintballBallIndex + 1, 0, PAINTBALL_MAX_BALL_COUNT);
      paintballHasWaitingBall = false;
      paintballMultiplier = 1;
    }
  }
  paintballNextBallTicks--;
  if (paintballNextBallTicks < 0) {
    Vector vel;
    vectorSet(&vel, sqrt(difficulty) * 0.1, 0);
    rotate(&vel, rnd(CGLP_PI / 8, CGLP_PI / 8 * 7));
    ASSIGN_ARRAY_ITEM(paintballBalls, paintballBallIndex, PaintballBall, nb2);
    vectorSet(&nb2->pos, rnd(20, 80), -3);
    nb2->vel = vel;
    nb2->color = 2;
    nb2->paintingCount = 0;
    nb2->isAlive = true;
    paintballBallIndex = cgl_wrap(paintballBallIndex + 1, 0, PAINTBALL_MAX_BALL_COUNT);
    paintballNextBallTicks = 150 / difficulty;
  }
  FOR_EACH(paintballBalls, i) {
    ASSIGN_ARRAY_ITEM(paintballBalls, i, PaintballBall, b);
    SKIP_IS_NOT_ALIVE(b);
    int gx = (int)floor((b->pos.x + 3 - (50 - (PAINTBALL_GRID_COUNT - 1) / 2.0 * 7)) / 7);
    int gy = (int)floor(
        (b->pos.y + 3 - (50 - (PAINTBALL_GRID_COUNT + 2) / 2.0 * 7) - paintballGridY) / 7);
    float sp = 1;
    if (b->color == 2 && gx >= 0 && gx < PAINTBALL_GRID_COUNT && gy >= 0 &&
        gy < PAINTBALL_GRID_COUNT + 3) {
      if (paintballGrid[gx][gy] == 1) {
        b->paintingCount++;
        sp = 0.1;
      } else {
        b->paintingCount = 999;
      }
    }
    vectorAdd(&b->pos, b->vel.x * sp, b->vel.y * sp);
    b->pos.y += scr;
    bool removed = false;
    if ((b->pos.x <= 9 && b->vel.x < 0) || (b->pos.x >= 90 && b->vel.x > 0)) {
      if (b->color == 1) {
        paintballWaitingBall.pos = b->pos;
        paintballWaitingBall.angle = 0;
        paintballWaitingBall.va = -1;
        paintballHasWaitingBall = true;
        paintballMultiplier = 1;
        b->isAlive = false;
        removed = true;
      } else {
        b->vel.x *= -1;
      }
    }
    if (removed) {
      continue;
    }
    int[2] bc;
    if (b->color == 1) {
      bc[0] = 'a';
    } else {
      bc[0] = 'b';
    }
    bc[1] = 0;
    character(bc, b->pos.x, b->pos.y, &scratch);
    if (b->color == 2 && b->pos.y > 99) {
      play(EXPLOSION);
      color = RED;
      text("X", b->pos.x, 97, &scratch);
      color = BLACK;
      gameOver();
    }
    if ((b->pos.y < 3 && b->vel.y < 0) || (b->pos.y > 99 && b->vel.y > 0)) {
      b->vel.y *= -1;
    }
    if (gx >= 0 && gx < PAINTBALL_GRID_COUNT && gy >= 0 && gy < PAINTBALL_GRID_COUNT + 3) {
      if (b->color == 1 || b->paintingCount > 99 / sqrt(difficulty)) {
        if (b->color == 1 && paintballGrid[gx][gy] != 1) {
          if (paintballGrid[gx][gy] == 2) {
            play(LASER);
            paintballMultiplier++;
          } else {
            play(HIT);
          }
          addScore(paintballMultiplier, b->pos.x, b->pos.y);
        }
        paintballGrid[gx][gy] = b->color;
        b->paintingCount = 0;
      }
    }
  }
  FOR_EACH(paintballBalls, i) {
    ASSIGN_ARRAY_ITEM(paintballBalls, i, PaintballBall, b);
    SKIP_IS_NOT_ALIVE(b);
    color = TRANSPARENT;
    character("b", b->pos.x, b->pos.y, &scratch);
    if (b->color == 2 && scratch.isColliding.character['a']) {
      play(POWER_UP);
      paintballMultiplier++;
      color = CYAN;
      particle(b->pos.x, b->pos.y, 9, 1, 0, CGLP_PI * 2);
      addScore(paintballMultiplier, b->pos.x, b->pos.y);
      b->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(paintballBalls, aliveBallCount);
  if (aliveBallCount == 0) {
    paintballNextBallTicks = 0;
  }
}

void addGamePaintball() {
  addGame(paintballTitle, paintballDescription, paintballCharacters,
          paintballCharactersCount, &paintballOptions, false, &paintballUpdate);
}
