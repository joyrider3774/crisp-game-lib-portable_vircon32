#include "../cglp.h"

int* zoomioTitle = "ZOOM IO";
int* zoomioDescription = "[Hold]\n Zoom &\n Go forward";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] zoomioCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int zoomioCharactersCount = 1;

Options zoomioOptions = {100, 100, 3, false};

#define ZOOMIO_TYPE_PLAYER 0
#define ZOOMIO_TYPE_ENEMY 1
#define ZOOMIO_TYPE_BONUS 2

struct ZoomioArrow {
  Vector pos;
  float angle;
  float speed;
  int type;
  bool isAlive;
};
// index 0 is always reserved for the player arrow and never overwritten;
// enemy/bonus arrows cycle through indices 1..MAX-1.
// Spawn interval shrinks as difficulty^-0.5 but speed grows as difficulty^1, so concurrent count ~sqrt(d)/(d+1.1) peaks right at game start (d~1) at ~60+ arrows crossing the 70-radius disk - 48 overflowed almost immediately.
#define ZOOMIO_MAX_ARROW_COUNT 128
ZoomioArrow[ZOOMIO_MAX_ARROW_COUNT] zoomioArrows;
int zoomioNextSpawnIndex;

float zoomioNextArrowTicks;
int zoomioNextArrowCount;
float[2] zoomioNextAngles;
float zoomioZoom;
float[20] zoomioLines;
Vector zoomioScr;
float zoomioMultiplier;

void zoomioUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(zoomioArrows);
    vectorSet(&zoomioArrows[0].pos, 50, 50);
    zoomioArrows[0].angle = -CGLP_PI / 2;
    zoomioArrows[0].speed = 0;
    zoomioArrows[0].type = ZOOMIO_TYPE_PLAYER;
    zoomioArrows[0].isAlive = true;
    zoomioNextSpawnIndex = 1;
    zoomioNextArrowTicks = 0;
    zoomioNextArrowCount = 0;
    zoomioNextAngles[0] = rnd(0, CGLP_PI * 2);
    zoomioNextAngles[1] = rnd(0, CGLP_PI * 2);
    zoomioZoom = 1;
    TIMES(20, i) { zoomioLines[i] = (i % 10) * 10; }
    vectorSet(&zoomioScr, 0, 0);
    zoomioMultiplier = 1;
  }
  color = LIGHT_CYAN;
  for (int i = 0; i < 20; i++) {
    if (i < 10) {
      zoomioLines[i] = cgl_wrap(zoomioLines[i] + zoomioScr.x, 0, 100);
      rect((zoomioLines[i] - 50) * zoomioZoom + 50, 0, 1, 100, &scratch);
    } else {
      zoomioLines[i] = cgl_wrap(zoomioLines[i] + zoomioScr.y, 0, 100);
      rect(0, (zoomioLines[i] - 50) * zoomioZoom + 50, 100, 1, &scratch);
    }
  }
  zoomioNextArrowTicks--;
  if (zoomioNextArrowTicks < 0) {
    if (rnd(0, 1) < 0.1) {
      float na = rnd(0, CGLP_PI * 2);
      if (zoomioNextArrowCount % 2 == 1 && zoomioZoom > 5) {
        na = zoomioArrows[0].angle;
      }
      zoomioNextAngles[zoomioNextArrowCount % 2] = na;
    }
    Vector pos;
    vectorSet(&pos, 50, 50);
    addWithAngle(&pos, zoomioNextAngles[zoomioNextArrowCount % 2], 70);
    Vector tp;
    vectorSet(&tp, 50, 50);
    addWithAngle(&tp, rnd(0, CGLP_PI * 2), rnd(20, 40));
    ZoomioArrow* na2 = &zoomioArrows[zoomioNextSpawnIndex];
    na2->pos = pos;
    na2->angle = angleTo(&pos, tp.x, tp.y);
    na2->speed = rnd(1, difficulty + 0.1) * 0.1;
    if (zoomioNextArrowCount % 2 == 0) {
      na2->type = ZOOMIO_TYPE_BONUS;
    } else {
      na2->type = ZOOMIO_TYPE_ENEMY;
    }
    na2->isAlive = true;
    zoomioNextSpawnIndex = cgl_wrap(zoomioNextSpawnIndex + 1, 1, ZOOMIO_MAX_ARROW_COUNT);
    zoomioNextArrowTicks = 20 / sqrt(difficulty);
    zoomioNextArrowCount++;
  }
  if (input.isJustPressed || input.isJustReleased) {
    play(LASER);
  }
  if (input.isPressed) {
    zoomioZoom = clamp(zoomioZoom + 0.05 * sqrt(difficulty), 1, 9);
    zoomioMultiplier += zoomioZoom * 0.1 * sqrt(difficulty);
  } else {
    zoomioZoom += (1 - zoomioZoom) * (0.03 * sqrt(difficulty));
  }
  if (zoomioZoom < 2) {
    zoomioMultiplier += (0.5 - zoomioMultiplier) * 0.02;
  }
  FOR_EACH(zoomioArrows, i) {
    ASSIGN_ARRAY_ITEM(zoomioArrows, i, ZoomioArrow, a);
    SKIP_IS_NOT_ALIVE(a);
    if (a->type == ZOOMIO_TYPE_PLAYER) {
      if (!input.isPressed) {
        a->angle += (sqrt(difficulty) * 0.1) / zoomioZoom;
      }
      a->speed += (sqrt(difficulty) * (zoomioZoom - 1) * 0.1 - a->speed) * 0.05;
      vectorSet(&zoomioScr, 0, 0);
      addWithAngle(&zoomioScr, a->angle, -a->speed);
      color = CYAN;
    } else {
      addWithAngle(&a->pos, a->angle, a->speed);
      vectorAdd(&a->pos, zoomioScr.x, zoomioScr.y);
      if (a->type == ZOOMIO_TYPE_ENEMY) {
        color = RED;
      } else {
        color = YELLOW;
      }
    }
    Vector p;
    vectorSet(&p, a->pos.x, a->pos.y);
    vectorAdd(&p, -50, -50);
    vectorMul(&p, zoomioZoom);
    vectorAdd(&p, 50, 50);
    float d = distanceTo(&a->pos, 50, 50);
    bool pInRect = p.x >= 0 && p.x < 99 && p.y >= 0 && p.y < 99;
    if (a->type == ZOOMIO_TYPE_PLAYER ||
        (sqrt(zoomioZoom) > 0.5 + d * 0.03 && pInRect)) {
      Vector bp;
      vectorSet(&bp, p.x, p.y);
      addWithAngle(&bp, a->angle, -1 * zoomioZoom);
      thickness = 3 * zoomioZoom;
      barCenterPosRatio = 0.5;
      Collision c1;
      bar(p.x, p.y, 5 * zoomioZoom, a->angle, &c1);
      Vector bp1;
      vectorSet(&bp1, bp.x, bp.y);
      addWithAngle(&bp1, a->angle + CGLP_PI / 2, 2 * zoomioZoom);
      Collision c2;
      box(bp1.x, bp1.y, 2 * zoomioZoom, 2 * zoomioZoom, &c2);
      Vector bp2;
      vectorSet(&bp2, bp.x, bp.y);
      addWithAngle(&bp2, a->angle - CGLP_PI / 2, 2 * zoomioZoom);
      Collision c3;
      box(bp2.x, bp2.y, 2 * zoomioZoom, 2 * zoomioZoom, &c3);
      if (a->type != ZOOMIO_TYPE_PLAYER &&
          (c1.isColliding.rect[CYAN] || c2.isColliding.rect[CYAN] ||
           c3.isColliding.rect[CYAN])) {
        if (a->type == ZOOMIO_TYPE_BONUS) {
          play(POWER_UP);
          particle(p.x, p.y, 5 * zoomioZoom, sqrt(zoomioZoom), 0, CGLP_PI * 2);
          addScore(ceil(zoomioMultiplier), 50, 50);
          a->isAlive = false;
          continue;
        } else {
          play(EXPLOSION);
          gameOver();
        }
      }
    } else {
      thickness = 1;
      barCenterPosRatio = 0;
      Vector cpVec;
      vectorSet(&cpVec, 50, 50);
      float ang = angleTo(&cpVec, p.x, p.y);
      bar(50, 50, 30, ang, &scratch);
    }
    a->isAlive = d <= 70;
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "+");
  strcat(multText, intToChar((int)ceil(zoomioMultiplier)));
  text(multText, 3, 9, &scratch);
}

void addGameZoomio() {
  addGame(zoomioTitle, zoomioDescription, zoomioCharacters,
          zoomioCharactersCount, &zoomioOptions, false, &zoomioUpdate);
}
