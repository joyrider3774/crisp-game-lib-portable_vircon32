#include "../cglp.h"

int* twholsTitle = "TWHOLS";
int* twholsDescription = "[Press] Turn";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] twholsCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int twholsCharactersCount = 0;

Options twholsOptions = {100, 100, 0, false};

struct TwholsWall {
  Vector p;
  float v;
};
#define TWHOLS_WALL_COUNT 2
TwholsWall[TWHOLS_WALL_COUNT] twholsWalls;

Vector twholsP;
Vector twholsV;

void twholsUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&twholsP, 50, 50);
    vectorSet(&twholsV, 1, 1);
    TIMES(TWHOLS_WALL_COUNT, i) {
      vectorSet(&twholsWalls[i].p, 0, 9 + i * 70);
      twholsWalls[i].v = 1;
    }
  }
  color = BLUE;
  TIMES(TWHOLS_WALL_COUNT, wi) {
    TwholsWall* w = &twholsWalls[wi];
    rect(w->p.x, w->p.y, -99, 5, &scratch);
    rect(w->p.x + 40, w->p.y, 99, 5, &scratch);
    w->p.x -= w->v;
    if (w->p.x < -40) {
      w->p.x = 99;
      w->v = rnd(1, 2) * difficulty;
    }
  }
  color = GREEN;
  Vector dv;
  vectorSet(&dv, twholsV.x, twholsV.y);
  vectorMul(&dv, difficulty);
  vectorAdd(&twholsP, dv.x, dv.y);
  Collision c;
  box(twholsP.x, twholsP.y, 5, 5, &c);
  if (c.isColliding.rect[BLUE]) {
    play(HIT);
    if (twholsP.y < 50) {
      twholsV.y = 1;
    } else {
      twholsV.y = -1;
    }
    score++;
  }
  if (input.isJustPressed || twholsP.x < 0 || twholsP.x > 99) {
    twholsV.x *= -1;
  }
  if (twholsP.y < 0 || twholsP.y > 99) {
    play(RANDOM);
    gameOver();
  }
}

void addGameTwhols() {
  addGame(twholsTitle, twholsDescription, twholsCharacters,
          twholsCharactersCount, &twholsOptions, false, &twholsUpdate);
}
