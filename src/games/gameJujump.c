#include "../cglp.h"

int* jujumpTitle = "JUJUMP";
int* jujumpDescription = "[Press]\n Jump";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] jujumpCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int jujumpCharactersCount = 0;

Options jujumpOptions = {100, 100, 0, false};

Vector jujumpP;
Vector jujumpV;

struct JujumpFloor {
  Vector pos;
  bool isAlive;
};
#define JUJUMP_MAX_FLOOR_COUNT 16
JujumpFloor[JUJUMP_MAX_FLOOR_COUNT] jujumpFloors;
int jujumpFloorIndex;

float jujumpJumpWay;
float jujumpJumpPower;
float jujumpFloorAppDist;

void jujumpUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&jujumpP, 50, 50);
    vectorSet(&jujumpV, 0, 0);
    INIT_UNALIVED_ARRAY_FAST(jujumpFloors);
    vectorSet(&jujumpFloors[0].pos, 50, 70);
    jujumpFloors[0].isAlive = true;
    jujumpFloorIndex = 1;
    jujumpJumpWay = 1;
    jujumpJumpPower = 1;
    jujumpFloorAppDist = 1;
  }
  vectorAdd(&jujumpP, jujumpV.x, jujumpV.y);
  if (input.isPressed) {
    jujumpV.y += 0.05;
  } else {
    jujumpV.y += 0.1;
  }
  float scr;
  if (jujumpP.y < 30) {
    scr = (30 - jujumpP.y) * 0.1;
  } else {
    scr = 0;
  }
  scr += difficulty * 0.1;
  score += scr;
  jujumpFloorAppDist -= scr;
  if (jujumpFloorAppDist < 0) {
    jujumpFloorAppDist = rnd(0, 99);
    ASSIGN_ARRAY_ITEM(jujumpFloors, jujumpFloorIndex, JujumpFloor, nf);
    vectorSet(&nf->pos, rnd(0, 99), -9);
    nf->isAlive = true;
    jujumpFloorIndex = cgl_wrap(jujumpFloorIndex + 1, 0, JUJUMP_MAX_FLOOR_COUNT);
  }
  jujumpP.y += scr;
  color = BLUE;
  FOR_EACH(jujumpFloors, i) {
    ASSIGN_ARRAY_ITEM(jujumpFloors, i, JujumpFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->pos.y += scr;
    box(f->pos.x, f->pos.y, 33, 7, &scratch);
    if (f->pos.y >= 99) {
      f->isAlive = false;
    }
  }
  color = TRANSPARENT;
  while (true) {
    box(jujumpP.x, jujumpP.y, 7, 7, &scratch);
    if (!scratch.isColliding.rect[BLUE]) {
      break;
    }
    jujumpP.y--;
    vectorSet(&jujumpV, 0, 0);
    jujumpJumpPower = 1;
  }
  color = GREEN;
  box(jujumpP.x, jujumpP.y, 7, 7, &scratch);
  if (input.isJustPressed) {
    play(JUMP);
    jujumpJumpWay *= -1;
    jujumpV.x = jujumpJumpWay;
    jujumpV.y = -3 * jujumpJumpPower;
    jujumpJumpPower *= 0.7;
  }
  if (jujumpP.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameJujump() {
  addGame(jujumpTitle, jujumpDescription, jujumpCharacters,
          jujumpCharactersCount, &jujumpOptions, false, &jujumpUpdate);
}
