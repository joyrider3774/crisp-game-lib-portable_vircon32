#include "../cglp.h"

int* refbalsTitle = "REFBALS";
int* refbalsDescription = "[Hold] Accel";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] refbalsCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int refbalsCharactersCount = 0;

Options refbalsOptions = {100, 100, 0, false};

#define REFBALS_WALL_COUNT 5
Vector[REFBALS_WALL_COUNT] refbalsWalls;

// Vircon32 port note: upstream's `balls` is a plain JS array that only ever
// grows (push(), never spliced/filtered) - the game always ends the instant
// any single ball reaches the bottom (see gameOver() below), so in practice
// only a handful of balls are ever alive at once even over a long play
// session. A fixed ring buffer with generous headroom reproduces the same
// observable behavior as the upstream unbounded array.
struct RefbalsBall {
  Vector p;
  float v;
  bool isAlive;
};
#define REFBALS_MAX_BALL_COUNT 64
RefbalsBall[REFBALS_MAX_BALL_COUNT] refbalsBalls;
int refbalsBallIndex;

void refbalsUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(refbalsBalls);
    refbalsBallIndex = 0;
    TIMES(REFBALS_WALL_COUNT, i) {
      vectorSet(&refbalsWalls[i], i * -29, -9);
    }
  }
  if (ticks % 99 == 0) {
    ASSIGN_ARRAY_ITEM(refbalsBalls, refbalsBallIndex, RefbalsBall, nb);
    vectorSet(&nb->p, rnd(0, 50), 0);
    nb->v = 0;
    nb->isAlive = true;
    refbalsBallIndex = cgl_wrap(refbalsBallIndex + 1, 0, REFBALS_MAX_BALL_COUNT);
  }
  color = BLUE;
  TIMES(REFBALS_WALL_COUNT, wi) {
    Vector* w = &refbalsWalls[wi];
    if (input.isPressed) {
      w->x -= 2;
    } else {
      w->x -= 1;
    }
    box(w->x, w->y, 36, 3, &scratch);
    if (w->x < -19) {
      w->x += rnd(130, 150);
      w->y = rnd(50, 90);
    }
  }
  color = PURPLE;
  FOR_EACH(refbalsBalls, bi) {
    ASSIGN_ARRAY_ITEM(refbalsBalls, bi, RefbalsBall, b);
    SKIP_IS_NOT_ALIVE(b);
    b->v += 0.03;
    b->p.y += b->v;
    if (b->p.y > 99) {
      play(EXPLOSION);
      gameOver();
    }
    Collision bc;
    box(b->p.x, b->p.y, 5, 5, &bc);
    if (bc.isColliding.rect[BLUE]) {
      play(SELECT);
      score++;
      b->v *= -1;
      b->p.y += b->v * 2;
    }
  }
}

void addGameRefbals() {
  addGame(refbalsTitle, refbalsDescription, refbalsCharacters,
          refbalsCharactersCount, &refbalsOptions, false, &refbalsUpdate);
}
