#include "../cglp.h"

int* pillars3dTitle = "PILLARS 3D";
int* pillars3dDescription = "[Slide]\n Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pillars3dCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int pillars3dCharactersCount = 1;

Options pillars3dOptions = {100, 100, 20, false};

struct Pillars3dPillar {
  float x;
  float z;
  Vector size;
  int color;
  bool isAlive;
};
#define PILLARS3D_MAX_PILLAR_COUNT 32
Pillars3dPillar[PILLARS3D_MAX_PILLAR_COUNT] pillars3dPillars;
int pillars3dPillarIndex;
int pillars3dNextPillarTicks;
int pillars3dNextYellowPillar;
Vector pillars3dPos;
float pillars3dVy;
int pillars3dMultiplier;

void pillars3dUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(pillars3dPillars);
    pillars3dPillarIndex = 0;
    ASSIGN_ARRAY_ITEM(pillars3dPillars, pillars3dPillarIndex, Pillars3dPillar, p0);
    p0->x = 0;
    p0->z = 20;
    vectorSet(&p0->size, 100, 100);
    p0->color = YELLOW;
    p0->isAlive = true;
    pillars3dPillarIndex = cgl_wrap(pillars3dPillarIndex + 1, 0, PILLARS3D_MAX_PILLAR_COUNT);
    pillars3dNextPillarTicks = 9;
    pillars3dNextYellowPillar = 9;
    vectorSet(&pillars3dPos, 50, 10);
    pillars3dVy = 0;
    pillars3dMultiplier = 1;
  }
  pillars3dNextPillarTicks--;
  if (pillars3dNextPillarTicks < 0) {
    pillars3dNextYellowPillar--;
    ASSIGN_ARRAY_ITEM(pillars3dPillars, pillars3dPillarIndex, Pillars3dPillar, np);
    np->x = rnd(60, 160) * RNDPM();
    np->z = 20;
    vectorSet(&np->size, rnd(50, 100), rnd(70, 180));
    if (pillars3dNextYellowPillar < 0) {
      np->color = YELLOW;
    } else {
      np->color = BLACK;
    }
    np->isAlive = true;
    pillars3dPillarIndex = cgl_wrap(pillars3dPillarIndex + 1, 0, PILLARS3D_MAX_PILLAR_COUNT);
    pillars3dNextPillarTicks = 20 / difficulty;
    if (pillars3dNextYellowPillar < 0) {
      pillars3dNextYellowPillar = 9;
    }
  }
  color = LIGHT_BLACK;
  rect(0, 60, 100, 1, &scratch);
  color = BLACK;
  pillars3dPos.x = clamp(input.pos.x, 6, 93);
  pillars3dPos.y += pillars3dVy;
  pillars3dVy += 0.1 * difficulty;
  text("TT", pillars3dPos.x - 3, pillars3dPos.y, &scratch);
  if (pillars3dPos.y > 95) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(pillars3dPillars, i) {
    ASSIGN_ARRAY_ITEM(pillars3dPillars, i, Pillars3dPillar, p);
    SKIP_IS_NOT_ALIVE(p);
    color = p->color;
    Collision pc;
    box(p->x / p->z + 50, p->size.y / 3 / p->z + 60, p->size.x / p->z,
        p->size.y / p->z, &pc);
    if (pc.isColliding.text['T']) {
      float ty = p->size.y / 3 / p->z + 60 - p->size.y / p->z / 2;
      if (pillars3dVy > 0) {
        play(LASER);
        pillars3dVy = -2.5 * sqrt(difficulty);
        if (pillars3dPos.y > ty) {
          pillars3dPos.y = ty;
        }
      }
      addScore(pillars3dMultiplier, p->x / p->z + 50, ty);
      if (p->color == YELLOW) {
        play(SELECT);
        pillars3dMultiplier++;
      }
      p->isAlive = false;
      continue;
    }
    p->z -= difficulty * 0.2;
    p->isAlive = p->z > 1;
  }
}

void addGamePillars3d() {
  addGame(pillars3dTitle, pillars3dDescription, pillars3dCharacters,
          pillars3dCharactersCount, &pillars3dOptions, true, &pillars3dUpdate);
}
