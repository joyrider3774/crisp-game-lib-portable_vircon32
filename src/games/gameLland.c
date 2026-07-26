#include "../cglp.h"

int* llandTitle = "LLAND";
int* llandDescription = "[Hold] Thrust up";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] llandCharacters = {{
    " llll ",
    "l    l",
    " llll ",
    " l  l ",
    "l ll l",
    "ll  ll",
}};
int llandCharactersCount = 1;

Options llandOptions = {100, 100, 6, false};

struct LlandMountain {
  float y;
  int c;
};
LlandMountain[9] llandMountains;

float llandShipY;
float llandShipV;
float llandOffset;
float llandMountainAppDist;
int llandMountainIndex;
int llandLandingIndex;
bool llandLanding;
float llandLandY;
bool llandIsFirstLanded;

void llandUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(9, i) {
      if (i == 4) {
        llandLandY = 49;
        llandMountains[i].y = llandLandY;
        llandMountains[i].c = CYAN;
      } else {
        llandMountains[i].y = 90 - i;
        llandMountains[i].c = RED;
      }
    }
    llandShipY = 30;
    llandShipV = 0;
    llandOffset = 0;
    llandMountainAppDist = 0;
    llandMountainIndex = 0;
    llandLanding = false;
    llandIsFirstLanded = false;
    llandLandingIndex = 7;
  }
  TIMES(9, i) {
    LlandMountain* m = &llandMountains[i];
    color = m->c;
    rect(cgl_wrap(i * 13 + llandOffset - 13, -13, 104), m->y, 13, 99, &scratch);
  }
  color = GREEN;
  Collision shipCollision;
  character("a", 25, llandShipY, &shipCollision);
  if (llandLanding) {
    if (input.isJustPressed) {
      llandLanding = false;
    } else {
      return;
    }
  }
  llandOffset -= difficulty;
  llandMountainAppDist -= difficulty;
  if (llandMountainAppDist < 0) {
    LlandMountain* m = &llandMountains[(int)cgl_wrap(llandMountainIndex, 0, 9)];
    if (llandLandingIndex > 7 || llandLandingIndex == 1) {
      m->y = rnd(70, 90);
    } else if (llandLandingIndex == 0) {
      llandLandY = rnd(40, 70);
      m->y = llandLandY;
    } else {
      m->y = rnd(40, 90);
    }
    llandLandingIndex--;
    if (llandLandingIndex < 0) {
      m->c = CYAN;
      llandLandingIndex = 9;
    } else {
      m->c = RED;
    }
    llandMountainIndex++;
    llandMountainAppDist += 13;
  }
  if (llandIsFirstLanded) {
    if (input.isJustPressed) {
      play(LASER);
      llandShipV -= 0.4;
    }
    if (input.isPressed) {
      llandShipV -= 0.2;
      particle(24.5, llandShipY + 2, 1, 1, CGLP_PI_2, 1);
    }
  }
  llandShipV += 0.1;
  llandShipV *= 0.99;
  if (llandShipY < 0 && llandShipV < 0) {
    llandShipV *= -1;
  }
  llandShipY += llandShipV * difficulty;
  if (shipCollision.isColliding.rect[CYAN]) {
    play(SELECT);
    particle(24.5, llandShipY, 9, 1, 0, CGLP_PI * 2);
    score++;
    llandLanding = true;
    llandShipV = 0;
    llandShipY = llandLandY - 3;
    TIMES(9, i) { llandMountains[i].c = RED; }
    llandIsFirstLanded = true;
  }
  if (shipCollision.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  Collision leftEdge;
  rect(-1, 0, 1, 99, &leftEdge);
  if (leftEdge.isColliding.rect[CYAN]) {
    color = RED;
    for (float y = llandLandY - 4; y < 99; y += 7) {
      text("X", 2, y, &scratch);
    }
    play(EXPLOSION);
    gameOver();
  }
}

void addGameLland() {
  addGame(llandTitle, llandDescription, llandCharacters, llandCharactersCount,
          &llandOptions, false, &llandUpdate);
}
