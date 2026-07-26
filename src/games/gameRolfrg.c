#include "../cglp.h"

int* rolfrgTitle = "ROLFRG";
int* rolfrgDescription = "[Press] Turn";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rolfrgCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int rolfrgCharactersCount = 0;

Options rolfrgOptions = {100, 100, 1, false};

struct RolfrgFrog {
  Vector p;
  float a;
};
#define ROLFRG_FROG_COUNT 5
RolfrgFrog[ROLFRG_FROG_COUNT] rolfrgFrogs;

float rolfrgX;
float rolfrgW;

void rolfrgUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(ROLFRG_FROG_COUNT, i) {
      vectorSet(&rolfrgFrogs[i].p, 0, -i * 9);
      rolfrgFrogs[i].a = rnd(0, 9);
    }
    rolfrgX = 50;
    rolfrgW = 1;
  }
  color = GREEN;
  score++;
  int period = (int)ceil(30 / difficulty);
  float l;
  if (ticks % period) {
    l = 4;
  } else {
    l = 30;
  }
  TIMES(ROLFRG_FROG_COUNT, fi) {
    RolfrgFrog* f = &rolfrgFrogs[fi];
    if (f->p.y < 0) {
      f->p.x = rnd(0, 99);
      f->p.y += rnd(99, 120);
    }
    thickness = 3;
    barCenterPosRatio = 0;
    bar(f->p.x, f->p.y, l, f->a, &scratch);
    f->a += difficulty / 9;
    f->p.y -= difficulty / 2;
    if (l > 9) {
      play(LASER);
      addWithAngle(&f->p, f->a, 30);
    }
  }
  color = YELLOW;
  rolfrgX += rolfrgW * difficulty;
  if (input.isJustPressed || rolfrgX < 0 || rolfrgX > 99) {
    rolfrgW *= -1;
  }
  Collision c;
  box(rolfrgX, 36, 3, 3, &c);
  if (c.isColliding.rect[GREEN]) {
    play(RANDOM);
    gameOver();
  }
}

void addGameRolfrg() {
  addGame(rolfrgTitle, rolfrgDescription, rolfrgCharacters,
          rolfrgCharactersCount, &rolfrgOptions, false, &rolfrgUpdate);
}
