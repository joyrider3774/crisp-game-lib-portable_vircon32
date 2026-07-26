#include "../cglp.h"

int* holesTitle = "HOLES";
int* holesDescription = "[Tap]\n Change holes";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] holesCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int holesCharactersCount = 0;

Options holesOptions = {100, 100, 9, false};

#define HOLES_BALL_RADIUS 1
#define HOLES_WALL_ANGLE 0.2
#define HOLES_HOLE_LENGTH 12

struct HolesBall {
  Vector pos;
  Vector vel;
  float angle;
  bool isAlive;
};
#define HOLES_MAX_BALL_COUNT 16
HolesBall[HOLES_MAX_BALL_COUNT] holesBalls;
int holesBallIndex;

// `side` is JS's `angle` field on wall objects: despite the name, it only
// ever holds 0 or 1 there - a left/right side flag, not a radian angle.
struct HolesWall {
  Vector pos;
  float length;
  int side;
  int index;
  bool isAlive;
};
#define HOLES_MAX_WALL_COUNT 512
HolesWall[HOLES_MAX_WALL_COUNT] holesWalls;
int holesWallIndex;

Vector holesCoinPos;
int holesCoinWallIndex;

int holesHoleIndex;
int holesCurrentIndex;
float holesScr;
float holesScrBaseY;

void holesReflect(HolesBall* b, float a, int c, bool hasColor) {
  float oa = cgl_wrap(vectorAngle(&b->vel) - a - CGLP_PI, -CGLP_PI, CGLP_PI);
  if (fabs(oa) < CGLP_PI / 2) {
    addWithAngle(&b->vel, a, vectorLength(&b->vel) * cos(oa) * 1.7);
  }
  if (hasColor) {
    color = TRANSPARENT;
    TIMES(9, i) {
      addWithAngle(&b->pos, a, 1);
      Collision rc;
      arc(b->pos.x, b->pos.y, HOLES_BALL_RADIUS, 0, CGLP_PI * 2, &rc);
      if (!rc.isColliding.rect[c]) {
        break;
      }
    }
  }
}

void holesSetCoin(float y, int a) {
  float cx;
  if (a == 0) {
    cx = 10;
  } else {
    cx = 89;
  }
  vectorSet(&holesCoinPos, cx, y);
  float wx;
  if (a == 0) {
    wx = 7;
  } else {
    wx = 93;
  }
  ASSIGN_ARRAY_ITEM(holesWalls, holesWallIndex, HolesWall, w);
  vectorSet(&w->pos, wx, y + 9);
  w->length = 9;
  w->side = a;
  w->index = -1;
  w->isAlive = true;
  holesCoinWallIndex = holesWallIndex;
  holesWallIndex = cgl_wrap(holesWallIndex + 1, 0, HOLES_MAX_WALL_COUNT);
}

void holesAddWalls(int wallIdx) {
  HolesWall* w = &holesWalls[wallIdx];
  COUNT_IS_ALIVE(holesBalls, aliveBallCount);
  if (aliveBallCount < 9) {
    ASSIGN_ARRAY_ITEM(holesBalls, holesBallIndex, HolesBall, nb);
    vectorSet(&nb->pos, rnd(30, 70), 0);
    vectorSet(&nb->vel, 0, sqrt(difficulty));
    nb->angle = rnd(0, CGLP_PI * 2);
    nb->isAlive = true;
    holesBallIndex = cgl_wrap(holesBallIndex + 1, 0, HOLES_MAX_BALL_COUNT);
  }
  float y = w->pos.y;
  int a = w->side;
  float[5] holeXs;
  int holeXsCount = 0;
  float xl = 80;
  float lp = HOLES_HOLE_LENGTH + 9;
  TIMES(5, i) {
    float hx = rnd(15, xl - lp / 2 - 5);
    bool isValid = true;
    TIMES(holeXsCount, k) {
      if (fabs(hx - holeXs[k]) < HOLES_HOLE_LENGTH + 9) {
        isValid = false;
      }
    }
    if (isValid) {
      holeXs[holeXsCount] = hx;
      holeXsCount++;
    }
  }
  TIMES(holeXsCount, i) {
    for (int j = i + 1; j < holeXsCount; j++) {
      if (holeXs[j] < holeXs[i]) {
        float t = holeXs[i];
        holeXs[i] = holeXs[j];
        holeXs[j] = t;
      }
    }
  }
  Vector p;
  if (a == 0) {
    vectorSet(&p, 7, y);
  } else {
    vectorSet(&p, 93, y);
  }
  float wa;
  if (a == 0) {
    wa = HOLES_WALL_ANGLE;
  } else {
    wa = CGLP_PI - HOLES_WALL_ANGLE;
  }
  bool hasPhx = false;
  float phx = 0;
  TIMES(holeXsCount, k) {
    float hx = holeXs[k];
    float l;
    if (!hasPhx) {
      l = hx - HOLES_HOLE_LENGTH / 2.0;
    } else {
      l = hx - HOLES_HOLE_LENGTH / 2.0 -
          (phx + HOLES_HOLE_LENGTH / 2.0) * (1 / cos(HOLES_WALL_ANGLE));
    }
    ASSIGN_ARRAY_ITEM(holesWalls, holesWallIndex, HolesWall, seg1);
    seg1->pos = p;
    seg1->length = l - 5;
    seg1->side = a;
    seg1->index = -1;
    seg1->isAlive = true;
    holesWallIndex = cgl_wrap(holesWallIndex + 1, 0, HOLES_MAX_WALL_COUNT);
    addWithAngle(&p, wa, l);
    float hl = HOLES_HOLE_LENGTH * (1 / cos(HOLES_WALL_ANGLE));
    ASSIGN_ARRAY_ITEM(holesWalls, holesWallIndex, HolesWall, seg2);
    seg2->pos = p;
    seg2->length = hl - 5;
    seg2->side = a;
    seg2->index = holesHoleIndex;
    seg2->isAlive = true;
    holesWallIndex = cgl_wrap(holesWallIndex + 1, 0, HOLES_MAX_WALL_COUNT);
    if (rnd(0, 1) < 0.7) {
      if (holesHoleIndex == 0) {
        holesHoleIndex = 1;
      } else {
        holesHoleIndex = 0;
      }
    }
    addWithAngle(&p, wa, hl);
    phx = hx;
    hasPhx = true;
  }
  float l2 = (xl - (phx + HOLES_HOLE_LENGTH / 2.0)) * (1 / cos(HOLES_WALL_ANGLE));
  ASSIGN_ARRAY_ITEM(holesWalls, holesWallIndex, HolesWall, seg3);
  seg3->pos = p;
  seg3->length = l2 - 5;
  seg3->side = a;
  seg3->index = -1;
  seg3->isAlive = true;
  holesWallIndex = cgl_wrap(holesWallIndex + 1, 0, HOLES_MAX_WALL_COUNT);
  w->pos.y = -99;
  int nextSide;
  if (a == 0) {
    nextSide = 1;
  } else {
    nextSide = 0;
  }
  holesSetCoin(y + 80 * sin(HOLES_WALL_ANGLE) + 9, nextSide);
}

void holesUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(holesBalls);
    holesBallIndex = 0;
    ASSIGN_ARRAY_ITEM(holesBalls, holesBallIndex, HolesBall, b0);
    vectorSet(&b0->pos, 10, 0);
    vectorSet(&b0->vel, 0, 0);
    b0->angle = 0;
    b0->isAlive = true;
    holesBallIndex = cgl_wrap(holesBallIndex + 1, 0, HOLES_MAX_BALL_COUNT);
    INIT_UNALIVED_ARRAY_FAST(holesWalls);
    holesWallIndex = 0;
    vectorSet(&holesCoinPos, 0, 0);
    holesCoinWallIndex = -1;
    holesSetCoin(20, 0);
    holesHoleIndex = 0;
    holesCurrentIndex = 0;
    holesScr = 0;
    holesScrBaseY = 60;
  }
  if (input.isJustPressed) {
    play(LASER);
    if (holesCurrentIndex == 0) {
      holesCurrentIndex = 1;
    } else {
      holesCurrentIndex = 0;
    }
    holesScrBaseY -= sqrt(difficulty) * 9;
  }
  color = LIGHT_YELLOW;
  rect(0, 0, 5, 99, &scratch);
  rect(95, 0, 5, 99, &scratch);
  color = YELLOW;
  holesCoinPos.y -= holesScr;
  COUNT_IS_ALIVE(holesBalls, aliveBallCountForScore);
  addScore(holesScr * aliveBallCountForScore, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  thickness = 7;
  line(holesCoinPos.x, holesCoinPos.y, holesCoinPos.x + 1, holesCoinPos.y, &scratch);
  FOR_EACH(holesWalls, i) {
    ASSIGN_ARRAY_ITEM(holesWalls, i, HolesWall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.y -= holesScr;
    if (w->pos.y < 9) {
      w->pos.y--;
    }
    if (w->index < 0 || w->index == holesCurrentIndex) {
      bool isLight = w->pos.y < 9;
      if (w->side == 0) {
        if (isLight) {
          color = LIGHT_GREEN;
        } else {
          color = GREEN;
        }
      } else {
        if (isLight) {
          color = LIGHT_CYAN;
        } else {
          color = CYAN;
        }
      }
      float wallAngleVal;
      if (w->side == 0) {
        wallAngleVal = HOLES_WALL_ANGLE;
      } else {
        wallAngleVal = CGLP_PI - HOLES_WALL_ANGLE;
      }
      thickness = 4;
      barCenterPosRatio = 0;
      bar(w->pos.x, w->pos.y, w->length, wallAngleVal, &scratch);
    }
    if (w->pos.y < -w->length * sin(HOLES_WALL_ANGLE)) {
      w->isAlive = false;
      continue;
    }
  }
  float maxY = 0;
  FOR_EACH(holesBalls, i) {
    ASSIGN_ARRAY_ITEM(holesBalls, i, HolesBall, b);
    SKIP_IS_NOT_ALIVE(b);
    b->vel.y += 0.2 * sqrt(difficulty);
    vectorMul(&b->vel, 1 - 0.02 * sqrt(difficulty));
    vectorAdd(&b->pos, b->vel.x * sqrt(difficulty) * 0.5, b->vel.y * sqrt(difficulty) * 0.5);
    b->pos.y -= holesScr;
    if (b->pos.y < holesScrBaseY + 20 && b->pos.y > maxY) {
      maxY = b->pos.y;
    }
    b->angle += b->vel.x * 0.03 + b->vel.y * 0.02;
    color = RED;
    Collision bc;
    arc(b->pos.x, b->pos.y, HOLES_BALL_RADIUS, b->angle, b->angle + CGLP_PI * 2, &bc);
    if (bc.isColliding.rect[YELLOW]) {
      play(COIN);
      addScore(aliveBallCountForScore * 10, b->pos.x, b->pos.y);
      holesAddWalls(holesCoinWallIndex);
    }
    if (bc.isColliding.rect[LIGHT_YELLOW]) {
      float a;
      if (b->pos.x < 50) {
        a = 0;
      } else {
        a = CGLP_PI;
      }
      holesReflect(b, a, LIGHT_YELLOW, true);
    }
    if (bc.isColliding.rect[GREEN]) {
      holesReflect(b, HOLES_WALL_ANGLE - CGLP_PI / 2, GREEN, true);
    }
    if (bc.isColliding.rect[CYAN]) {
      holesReflect(b, CGLP_PI - HOLES_WALL_ANGLE + CGLP_PI / 2, CYAN, true);
    }
    if (b->pos.y > 99 + HOLES_BALL_RADIUS) {
      play(HIT);
      b->isAlive = false;
      continue;
    }
    if (b->pos.y < -HOLES_BALL_RADIUS) {
      b->isAlive = false;
      continue;
    }
  }
  if (maxY > holesScrBaseY) {
    holesScr = (maxY - holesScrBaseY) * 0.1;
  } else {
    holesScr = 0;
  }
  holesScrBaseY += (60 - holesScrBaseY) * 0.01;
  COUNT_IS_ALIVE(holesBalls, aliveBallCountAfter);
  if (aliveBallCountAfter == 0) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(holesBalls, i) {
    ASSIGN_ARRAY_ITEM(holesBalls, i, HolesBall, b);
    SKIP_IS_NOT_ALIVE(b);
    FOR_EACH(holesBalls, j) {
      ASSIGN_ARRAY_ITEM(holesBalls, j, HolesBall, ab);
      SKIP_IS_NOT_ALIVE(ab);
      if (i == j || distanceTo(&ab->pos, b->pos.x, b->pos.y) > HOLES_BALL_RADIUS * 2) {
        continue;
      }
      holesReflect(b, angleTo(&ab->pos, b->pos.x, b->pos.y), 0, false);
    }
  }
}

void addGameHoles() {
  addGame(holesTitle, holesDescription, holesCharacters, holesCharactersCount,
          &holesOptions, false, &holesUpdate);
}
