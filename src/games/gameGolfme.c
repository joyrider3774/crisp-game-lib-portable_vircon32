#include "../cglp.h"

int* golfmeTitle = "GOLFME";
int* golfmeDescription = "[Hold]\n Change angle\n[Release]\n Jump";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] golfmeCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int golfmeCharactersCount = 0;

Options golfmeOptions = {200, 100, 2, false};

Vector golfmeP;
Vector golfmeV;
bool golfmeIsJumping;
float golfmeAngle;
float golfmeWidth;
float golfmeSpace;

void golfmeUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&golfmeP, 50, 85);
    golfmeIsJumping = false;
    golfmeAngle = 0;
    golfmeWidth = 0;
    golfmeSpace = 0;
  }
  if (golfmeWidth + golfmeSpace < 0) {
    golfmeWidth = 200;
    golfmeSpace = rnd(50, 150);
  }
  color = BLUE;
  rect(0, 90, golfmeWidth, 9, &scratch);
  rect(golfmeWidth + golfmeSpace, 90, 200, 9, &scratch);
  color = GREEN;
  box(golfmeP.x, golfmeP.y, 9, 9, &scratch);
  if (golfmeP.x < 0 || golfmeP.y > 99) {
    play(RANDOM);
    gameOver();
  }
  if (golfmeIsJumping) {
    vectorAdd(&golfmeP, golfmeV.x, golfmeV.y);
    golfmeV.y += 0.1;
    if (scratch.isColliding.rect[BLUE]) {
      golfmeIsJumping = false;
      golfmeAngle = 0;
      golfmeP.y = 85;
    }
  } else {
    if (input.isPressed) {
      golfmeAngle -= 0.05;
      thickness = 3;
      barCenterPosRatio = 0;
      bar(golfmeP.x, golfmeP.y, 20, golfmeAngle, &scratch);
    }
    if (input.isJustReleased) {
      play(JUMP);
      golfmeIsJumping = true;
      vectorSet(&golfmeV, 4, 0);
      rotate(&golfmeV, golfmeAngle);
    }
  }
  float scr = clamp(golfmeP.x - 50, 0, 99) * 0.1 + difficulty;
  golfmeP.x -= scr;
  golfmeWidth -= scr;
  score += scr;
}

void addGameGolfme() {
  addGame(golfmeTitle, golfmeDescription, golfmeCharacters,
          golfmeCharactersCount, &golfmeOptions, false, &golfmeUpdate);
}
