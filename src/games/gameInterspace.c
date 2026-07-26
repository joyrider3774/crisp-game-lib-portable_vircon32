#include "../cglp.h"

int* interspaceTitle = "INTERSPACE";
int* interspaceDescription = "[Slide]\n Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] interspaceCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int interspaceCharactersCount = 0;

Options interspaceOptions = {100, 100, 0, false};

Vector interspaceP;
Vector interspaceS;
Vector interspaceWp;

void interspaceUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&interspaceP, 50, 10);
    vectorSet(&interspaceS, 7, 7);
    vectorSet(&interspaceWp, 50, 99);
  }
  float sc = difficulty * 0.5 + (interspaceS.y - 7) * 0.3;
  interspaceWp.y -= sc;
  addScore(sc, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  if (interspaceWp.y < 0) {
    interspaceWp.x = cgl_wrap(interspaceWp.x + rnd(-9, 9), 9, 90);
    interspaceWp.y = 99;
    play(COIN);
  }
  color = RED;
  rect(0, interspaceWp.y, interspaceWp.x - 7, 9, &scratch);
  rect(interspaceWp.x + 7, interspaceWp.y, 99, 9, &scratch);
  interspaceP.x = clamp(input.pos.x, 0, 99);
  color = TRANSPARENT;
  box(interspaceP.x, interspaceP.y, 7, 99, &scratch);
  if (scratch.isColliding.rect[RED]) {
    interspaceP.y += (10 - interspaceP.y) * 0.1;
    interspaceS.y += (7 - interspaceS.y) * 0.4;
  } else {
    interspaceP.y += (30 - interspaceP.y) * 0.1;
    interspaceS.y += (30 - interspaceS.y) * 0.1;
  }
  color = GREEN;
  box(interspaceP.x, interspaceP.y, interspaceS.x, interspaceS.y, &scratch);
  if (scratch.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameInterspace() {
  addGame(interspaceTitle, interspaceDescription, interspaceCharacters,
          interspaceCharactersCount, &interspaceOptions, true,
          &interspaceUpdate);
}
