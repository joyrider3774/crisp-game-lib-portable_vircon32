// Vircon32 port note: see gamePinClimb.c for the full explanation of the
// changes applied here (out-pointer Collision calls, no ternary, no
// static, prefixed names, etc).
#include "../cglp.h"

char* hexminTitle = "HEXMIN";
char* hexminDescription = "[Tap]   Roll\n[Arrow] Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] hexminCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int hexminCharactersCount = 1;

Options hexminOptions = {100, 100, 9, false};

#define HEXMIN_LANE_COUNT 6
int[HEXMIN_LANE_COUNT] hexminLanes;
float hexminLaneChangeTicks;
float hexminNextLaneAddingCount;
int hexminArrowAngle;
float hexminMultiplier;
float[6] hexminSs = {4, 6, 8, 10, 10, 10};
float[5] hexminRs = {49, 42, 33, 22, 11};
float hexminKeyRepeatTicks;

void hexminDrawHex(Vector* cp, float s) {
  Vector p1;
  Vector p2;
  Collision scratch;
  vectorSet(&p1, cp->x, cp->y);
  addWithAngle(&p1, (5.5 * M_PI) / 3, s);
  for (int i = 0; i < HEXMIN_LANE_COUNT; i++) {
    vectorSet(&p2, p1.x, p1.y);
    addWithAngle(&p1, ((i + 0.5) * M_PI) / 3, s);
    line(p1.x, p1.y, p2.x, p2.y, &scratch);
  }
}

void hexminUpdate() {
  Collision scratch;
  if (!ticks) {
    hasCollision = false;
    memset(hexminLanes, 0, sizeof(hexminLanes));
    hexminLaneChangeTicks = 0;
    hexminNextLaneAddingCount = 0;
    hexminArrowAngle = 0;
    hexminMultiplier = 1;
    hexminKeyRepeatTicks = 0;
  }
  if (input.isJustPressed || (hexminKeyRepeatTicks < 0 && input.isPressed)) {
    hexminKeyRepeatTicks = 15 / sqrt(difficulty);
    play(LASER);
    int oa;
    if (input.left.isPressed) {
      oa = -1;
    } else {
      oa = 1;
    }
    hexminArrowAngle = cgl_wrap(hexminArrowAngle + oa, 0, 6);
    hexminMultiplier = ceil(hexminMultiplier * 0.9) - 1;
    if (hexminMultiplier < 1) {
      hexminMultiplier = 1;
    }
  }
  hexminKeyRepeatTicks--;
  Vector p;
  for (int i = 0; i < HEXMIN_LANE_COUNT; i++) {
    int oa = cgl_wrap(i - hexminArrowAngle, 0, 6);
    vectorSet(&p, 50, 50);
    addWithAngle(&p, (i * M_PI) / 3, hexminRs[4]);
    if (oa == 0 || oa == 2) {
      color = RED;
      thickness = 3;
    } else {
      color = LIGHT_BLACK;
      thickness = 1;
    }
    hexminDrawHex(&p, hexminSs[4] / 2);
  }
  hexminLaneChangeTicks--;
  if (hexminLaneChangeTicks < 0) {
    for (int i = 0; i < HEXMIN_LANE_COUNT; i++) {
      if (hexminLanes[i] > 0) {
        play(HIT);
        hexminLanes[i]++;
        if (hexminLanes[i] > 4) {
          play(EXPLOSION);
          gameOver();
        }
      }
    }
    hexminNextLaneAddingCount--;
    if (hexminNextLaneAddingCount < 0) {
      int li = rndi(0, 6);
      for (int i = 0; i < 6; i++) {
        if (hexminLanes[li] == 0) {
          hexminLanes[li] = 1;
          break;
        }
        li = cgl_wrap(li + 1, 0, 6);
      }
      hexminNextLaneAddingCount = floor(rnd(0, 1 + 3 / difficulty));
    }
    hexminLaneChangeTicks += 30 / sqrt(difficulty);
  }
  for (int j = 0; j < HEXMIN_LANE_COUNT; j++) {
    for (int i = 0; i < 5; i++) {
      if (hexminLanes[j] <= i) {
        break;
      }
      vectorSet(&p, 50, 50);
      addWithAngle(&p, (j * M_PI) / 3, hexminRs[i]);
      if (i < 3) {
        color = GREEN;
        thickness = 2;
      } else {
        color = BLUE;
        thickness = 3;
      }
      hexminDrawHex(&p, hexminSs[i] / 2);
    }
    if (hexminLanes[j] == 4) {
      int oa = cgl_wrap(j - hexminArrowAngle, 0, 6);
      if (oa == 0 || oa == 2) {
        play(COIN);
        addScore((int)hexminMultiplier, 50, 50);
        hexminMultiplier += 6;
        hexminLanes[j] = 0;
      }
    }
  }
  color = BLACK;
  text("x", 3, 9, &scratch);
  text(intToChar(hexminMultiplier), 9, 9, &scratch);
}

void addGameHexmin() {
  addGame(hexminTitle, hexminDescription, hexminCharacters,
          hexminCharactersCount, &hexminOptions, false, &hexminUpdate);
}
