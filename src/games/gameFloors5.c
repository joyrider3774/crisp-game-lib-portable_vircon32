#include "../cglp.h"

int* floors5Title = "FLOORS 5";
int* floors5Description = "[Tap]  Jump / Double Jump\n[Hold] Fly";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] floors5Characters = {
    {
        "      ",
        "      ",
        "      ",
        " l  l ",
        "l ll l",
        " l  l ",
    },
    {
        " lll  ",
        "l l ll",
        "llllll",
        "l ll l",
    },
    {
        " llll ",
        "llllll",
        "llllll",
        "llllll",
        "llllll",
        " llll ",
    },
    {
        " llll ",
        "l    l",
        "l    l",
        "l    l",
        "l    l",
        " llll ",
    },
};
int floors5CharactersCount = 4;

Options floors5Options = {200, 100, 7, false};

struct Floors5Floor {
  Vector pos;
  float width;
  int index;
  float paintFrom;
  float paintTo;
};
#define FLOORS5_FLOOR_COUNT 5
Floors5Floor[FLOORS5_FLOOR_COUNT] floors5Floors;
int[FLOORS5_FLOOR_COUNT] floors5ColorList;

struct Floors5Car {
  Vector pos;
  Vector vel;
  int floorIndex;
  float by;
  float bvy;
  float fallTicks;
  int jumpCount;
};
Floors5Car floors5Car;

bool[5] floors5LandedColors;
int floors5Multiplier;

void floors5AddFloorScore(Floors5Floor* f) {
  play(POWER_UP);
  float w = f->paintTo - f->paintFrom;
  int m;
  if (w >= f->width) {
    m = 3;
  } else {
    m = 1;
  }
  float y = f->pos.y;
  if (m > 1) {
    y -= 7;
  }
  int s = (int)clamp(floor(w) * floors5Multiplier, 0, 999);
  TIMES(m, i) {
    addScore(s, f->pos.x + f->width + 15, y);
    y += 7;
  }
}

void floors5Update() {
  Collision scratch;
  if (!ticks) {
    floors5ColorList[0] = RED;
    floors5ColorList[1] = GREEN;
    floors5ColorList[2] = BLUE;
    floors5ColorList[3] = YELLOW;
    floors5ColorList[4] = PURPLE;
    int[5] initX = {25, 52, 105, 160, 220};
    int[5] initY = {30, 50, 70, 60, 40};
    int[5] initW = {10, 35, 30, 30, 50};
    TIMES(FLOORS5_FLOOR_COUNT, i) {
      vectorSet(&floors5Floors[i].pos, initX[i], initY[i]);
      floors5Floors[i].width = initW[i];
      floors5Floors[i].index = i;
      floors5Floors[i].paintFrom = 0;
      floors5Floors[i].paintTo = 0;
    }
    vectorSet(&floors5Car.pos, 10, 10);
    vectorSet(&floors5Car.vel, 1, 0);
    floors5Car.floorIndex = -1;
    floors5Car.by = 0;
    floors5Car.bvy = 0;
    floors5Car.fallTicks = -99;
    floors5Car.jumpCount = 0;
    TIMES(5, i) { floors5LandedColors[i] = false; }
    floors5Multiplier = 1;
  }
  TIMES(FLOORS5_FLOOR_COUNT, i) {
    Floors5Floor* f = &floors5Floors[i];
    if (f->pos.x + f->width < 0) {
      vectorSet(&f->pos, rnd(200, 250), rnd(30, 90));
      f->width = rnd(20, 60);
      f->paintFrom = 0;
      f->paintTo = 0;
    }
    color = floors5ColorList[i];
    f->pos.x -= floors5Car.vel.x;
    rect(f->pos.x, f->pos.y, f->width, 6, &scratch);
    color = WHITE;
    rect(f->pos.x + 1, f->pos.y + 1, f->width - 2, 4, &scratch);
    color = floors5ColorList[i];
    rect(f->pos.x + f->paintFrom, f->pos.y + 1, f->paintTo - f->paintFrom, 4, &scratch);
  }
  floors5Car.vel.x += difficulty * 0.02;
  if (floors5Car.floorIndex < 0) {
    if (input.isPressed) {
      floors5Car.vel.y += 0.03;
    } else {
      floors5Car.vel.y += 0.18;
    }
  }
  floors5Car.pos.y += floors5Car.vel.y;
  floors5Car.bvy -= floors5Car.by * 0.1;
  floors5Car.by += floors5Car.bvy;
  floors5Car.by *= 0.9;
  floors5Car.fallTicks--;
  color = BLACK;
  Collision crColl;
  character("a", floors5Car.pos.x, clamp(floors5Car.pos.y, 0, 999), &crColl);
  Collision crbColl;
  character("b", floors5Car.pos.x, floors5Car.pos.y + floors5Car.by, &crbColl);
  if (floors5Car.floorIndex < 0) {
    TIMES(5, ci) {
      int cclr = floors5ColorList[ci];
      if (crColl.isColliding.rect[cclr] || crbColl.isColliding.rect[cclr]) {
        if (floors5Car.vel.y >= 0) {
          play(SELECT);
          floors5Car.floorIndex = ci;
          floors5Car.pos.y = floors5Floors[ci].pos.y - 3;
          floors5Car.vel.y = 0;
          floors5Car.vel.x = sqrt(difficulty);
          floors5Floors[ci].paintFrom =
              clamp(floors5Car.pos.x - 5 - floors5Floors[ci].pos.x, 0, 999);
          floors5Car.jumpCount = 0;
          floors5LandedColors[ci] = true;
        } else {
          play(HIT);
          floors5Car.pos.y = floors5Floors[ci].pos.y + 9 - floors5Car.by;
          floors5Car.vel.y *= -0.7;
        }
      }
    }
    if (floors5Car.floorIndex < 0 &&
        (floors5Car.fallTicks > -9 || floors5Car.jumpCount < 2) && input.isJustPressed) {
      play(JUMP);
      floors5Car.vel.y = -2;
      floors5Car.vel.x = sqrt(difficulty);
      floors5Car.bvy = -2;
      floors5Car.jumpCount++;
    }
  } else {
    Floors5Floor* cf = &floors5Floors[floors5Car.floorIndex];
    if (input.isJustPressed) {
      play(JUMP);
      floors5AddFloorScore(cf);
      cf->pos.x = -999;
      floors5Car.floorIndex = -1;
      floors5Car.vel.y = -2;
      floors5Car.vel.x = sqrt(difficulty);
      floors5Car.bvy = -2;
      floors5Car.jumpCount++;
    } else if (cf->pos.x + cf->width < floors5Car.pos.x - 3) {
      floors5AddFloorScore(cf);
      cf->pos.x = -999;
      floors5Car.floorIndex = -1;
      floors5Car.vel.x = sqrt(difficulty);
      floors5Car.fallTicks = 0;
      floors5Car.jumpCount = 0;
    } else {
      cf->paintTo = clamp(floors5Car.pos.x + 5 - cf->pos.x, 0, cf->width);
    }
  }
  bool isAll = true;
  TIMES(5, i) {
    color = floors5ColorList[i];
    if (floors5LandedColors[i]) {
      character("c", i * 7 + 3, 96, &scratch);
    } else {
      character("d", i * 7 + 3, 96, &scratch);
    }
    isAll = floors5LandedColors[i] && isAll;
  }
  if (isAll) {
    play(COIN);
    floors5Multiplier++;
    TIMES(5, i) { floors5LandedColors[i] = false; }
  }
  if (floors5Multiplier > 1) {
    color = BLACK;
    int[16] multText;
    strcpy(multText, "x");
    strcat(multText, intToChar(floors5Multiplier));
    text(multText, 45, 96, &scratch);
  }
  if (floors5Car.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameFloors5() {
  addGame(floors5Title, floors5Description, floors5Characters,
          floors5CharactersCount, &floors5Options, false, &floors5Update);
}
