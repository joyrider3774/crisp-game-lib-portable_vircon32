#include "../cglp.h"

int* squarebarTitle = "SQUARE BAR";
int* squarebarDescription = "[Hold]\n Grow";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] squarebarCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int squarebarCharactersCount = 0;

Options squarebarOptions = {100, 100, 3, true};

#define SQUAREBAR_SIZE 40
int[4][2] squarebarAngleToXy = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

#define SQUAREBAR_TYPE_PLAYER 0
#define SQUAREBAR_TYPE_ENEMY 1

struct SquarebarBar {
  Vector pos;
  float length;
  float spRatio;
  float angle;
  int tAngle;
  int type;
};
// Exactly one player bar plus exactly one enemy bar exist at all times (a
// destroyed enemy is always immediately replaced) - sized with headroom.
#define SQUAREBAR_MAX_BAR_COUNT 32
SquarebarBar[SQUAREBAR_MAX_BAR_COUNT] squarebarBars;
int squarebarBarCount;

void squarebarRemoveBar(int index) {
  memcpy(&squarebarBars[index], &squarebarBars[index + 1],
         (squarebarBarCount - 1 - index) * sizeof(squarebarBars[0]));
  squarebarBarCount--;
}

#define SQUAREBAR_ITEM_SPIKE 0
#define SQUAREBAR_ITEM_GOLD 1
struct SquarebarItem {
  Vector pos;
  int type;
  float ticks;
  float size;
  bool isAlive;
};
#define SQUAREBAR_MAX_ITEM_COUNT 32
SquarebarItem[SQUAREBAR_MAX_ITEM_COUNT] squarebarItems;
int squarebarItemIndex;

int squarebarMultiplier;

void squarebarAddItem(int type) {
  Vector pos;
  TIMES(99, ai) {
    float rx = rnd(50 - SQUAREBAR_SIZE / 2.0 - 1 - 20, 50 + SQUAREBAR_SIZE / 2.0);
    float ryHigh;
    if (type == SQUAREBAR_ITEM_SPIKE) {
      ryHigh = 50 - SQUAREBAR_SIZE / 2.0 - 15;
    } else {
      ryHigh = 50 - SQUAREBAR_SIZE / 2.0 - 4;
    }
    float ry = rnd(50 - SQUAREBAR_SIZE / 2.0 - 1 - 20, ryHigh);
    vectorSet(&pos, rx, ry);
    vectorAdd(&pos, -50, -50);
    rotate(&pos, rndi(0, 4) * CGLP_PI / 2);
    vectorAdd(&pos, 50, 50);
    if (distanceTo(&pos, squarebarBars[0].pos.x, squarebarBars[0].pos.y) > 36) {
      break;
    }
  }
  ASSIGN_ARRAY_ITEM(squarebarItems, squarebarItemIndex, SquarebarItem, ni);
  ni->pos = pos;
  ni->type = type;
  ni->ticks = floor(rnd(777, 999) / sqrt(difficulty));
  ni->size = 0;
  ni->isAlive = true;
  squarebarItemIndex = cgl_wrap(squarebarItemIndex + 1, 0, SQUAREBAR_MAX_ITEM_COUNT);
}

void squarebarUpdate() {
  Collision scratch;
  if (!ticks) {
    squarebarBarCount = 2;
    vectorSet(&squarebarBars[0].pos, 50, 50 - SQUAREBAR_SIZE / 2.0 - 1);
    squarebarBars[0].length = 9;
    squarebarBars[0].spRatio = 0;
    squarebarBars[0].angle = 3;
    squarebarBars[0].tAngle = 0;
    squarebarBars[0].type = SQUAREBAR_TYPE_PLAYER;
    vectorSet(&squarebarBars[1].pos, 50, 50 + SQUAREBAR_SIZE / 2.0 + 1);
    squarebarBars[1].length = 12;
    squarebarBars[1].spRatio = 0;
    squarebarBars[1].angle = 1;
    squarebarBars[1].tAngle = 2;
    squarebarBars[1].type = SQUAREBAR_TYPE_ENEMY;
    INIT_UNALIVED_ARRAY_FAST(squarebarItems);
    squarebarItemIndex = 0;
    TIMES(3, ii) { squarebarAddItem(SQUAREBAR_ITEM_SPIKE); }
    TIMES(7, ii2) { squarebarAddItem(SQUAREBAR_ITEM_GOLD); }
    squarebarMultiplier = 1;
  }
  color = LIGHT_BLACK;
  box(50, 50, SQUAREBAR_SIZE, SQUAREBAR_SIZE, &scratch);
  bool isChased = false;
  Vector cpTmp;
  vectorSet(&cpTmp, 50, 50);
  float a0 = angleTo(&cpTmp, squarebarBars[0].pos.x, squarebarBars[0].pos.y);
  float a1 = angleTo(&cpTmp, squarebarBars[1].pos.x, squarebarBars[1].pos.y);
  if (cgl_wrap(a0 - a1, -CGLP_PI, CGLP_PI) > 0) {
    isChased = true;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  int bi = 0;
  while (bi < squarebarBarCount) {
    SquarebarBar* b = &squarebarBars[bi];
    bool removed = false;
    if (b->type == SQUAREBAR_TYPE_PLAYER) {
      if (input.isPressed) {
        b->length += (20 + sqrt(difficulty) * 9 - b->length) * 0.1;
      } else {
        b->length += (9 - b->length) * 0.2;
      }
    } else {
      b->length = 11 + sqrt(difficulty) * 4;
    }
    b->angle = cgl_wrap(b->angle + 0.03 * difficulty, 0, 4);
    if (b->angle >= b->tAngle && b->angle < b->tAngle + CGLP_PI / 4) {
      play(HIT);
      b->angle = b->tAngle;
      int dirIdx = (int)b->angle;
      Vector np;
      vectorSet(&np, squarebarAngleToXy[dirIdx][0], squarebarAngleToXy[dirIdx][1]);
      vectorMul(&np, b->length * (1 - b->spRatio));
      vectorAdd(&np, b->pos.x, b->pos.y);
      if (b->angle == 0) {
        if (np.x > 50 + SQUAREBAR_SIZE / 2.0 + 1) {
          b->tAngle = 1;
          b->spRatio = 1 - (np.x - (50 + SQUAREBAR_SIZE / 2.0 + 1)) / b->length;
          vectorSet(&b->pos, 50 + SQUAREBAR_SIZE / 2.0 + 1, 50 - SQUAREBAR_SIZE / 2.0 - 1);
        } else {
          b->angle = 2;
          b->spRatio = 0;
          b->pos = np;
        }
      } else if (b->angle == 1) {
        if (np.y > 50 + SQUAREBAR_SIZE / 2.0 + 1) {
          b->tAngle = 2;
          b->spRatio = 1 - (np.y - (50 + SQUAREBAR_SIZE / 2.0 + 1)) / b->length;
          vectorSet(&b->pos, 50 + SQUAREBAR_SIZE / 2.0 + 1, 50 + SQUAREBAR_SIZE / 2.0 + 1);
        } else {
          b->angle = 3;
          b->spRatio = 0;
          b->pos = np;
        }
      } else if (b->angle == 2) {
        if (np.x < 50 - SQUAREBAR_SIZE / 2.0 - 1) {
          b->tAngle = 3;
          b->spRatio = 1 - (50 - SQUAREBAR_SIZE / 2.0 - 1 - np.x) / b->length;
          vectorSet(&b->pos, 50 - SQUAREBAR_SIZE / 2.0 - 1, 50 + SQUAREBAR_SIZE / 2.0 + 1);
        } else {
          b->angle = 0;
          b->spRatio = 0;
          b->pos = np;
        }
      } else if (b->angle == 3) {
        if (np.y < 50 - SQUAREBAR_SIZE / 2.0 - 1) {
          b->tAngle = 0;
          b->spRatio = 1 - (50 - SQUAREBAR_SIZE / 2.0 - 1 - np.y) / b->length;
          vectorSet(&b->pos, 50 - SQUAREBAR_SIZE / 2.0 - 1, 50 - SQUAREBAR_SIZE / 2.0 - 1);
        } else {
          b->angle = 1;
          b->spRatio = 0;
          b->pos = np;
        }
      }
    }
    if (b->type == SQUAREBAR_TYPE_PLAYER) {
      color = GREEN;
    } else if (isChased) {
      color = RED;
    } else {
      color = YELLOW;
    }
    thickness = 3;
    barCenterPosRatio = b->spRatio;
    Collision bc;
    bar(b->pos.x, b->pos.y, b->length, (b->angle * CGLP_PI) / 2, &bc);
    if (b->type == SQUAREBAR_TYPE_ENEMY && bc.isColliding.rect[GREEN]) {
      if (isChased) {
        play(EXPLOSION);
        gameOver();
      } else {
        play(POWER_UP);
        addScore(10 * squarebarMultiplier, b->pos.x, b->pos.y);
        squarebarMultiplier++;
        int ta = (int)cgl_wrap(squarebarBars[0].tAngle + 2, 0, 4);
        Vector chosenPs;
        if (ta == 0) {
          vectorSet(&chosenPs, 50, 50 - SQUAREBAR_SIZE / 2.0 - 1);
        } else if (ta == 1) {
          vectorSet(&chosenPs, 50 + SQUAREBAR_SIZE / 2.0 + 1, 50);
        } else if (ta == 2) {
          vectorSet(&chosenPs, 50, 50 + SQUAREBAR_SIZE / 2.0 - 1);
        } else {
          vectorSet(&chosenPs, 50 - SQUAREBAR_SIZE / 2.0 + 1, 50);
        }
        SquarebarBar* newBar = &squarebarBars[squarebarBarCount];
        newBar->pos = chosenPs;
        newBar->length = 12;
        newBar->spRatio = 0;
        newBar->angle = cgl_wrap(ta - 1, 0, 4);
        newBar->tAngle = ta;
        newBar->type = SQUAREBAR_TYPE_ENEMY;
        squarebarBarCount++;
        removed = true;
      }
    }
    if (removed) {
      squarebarRemoveBar(bi);
    } else {
      bi++;
    }
  }
  FOR_EACH(squarebarItems, ii3) {
    ASSIGN_ARRAY_ITEM(squarebarItems, ii3, SquarebarItem, it);
    SKIP_IS_NOT_ALIVE(it);
    if (it->ticks < 60) {
      if ((int)it->ticks % 15 == 0) {
        it->size--;
      }
    } else if (it->size < 4 && (int)it->ticks % 15 == 0) {
      it->size++;
    }
    if (it->type == SQUAREBAR_ITEM_SPIKE) {
      color = RED;
    } else {
      color = YELLOW;
    }
    Collision ic;
    box(it->pos.x, it->pos.y, it->size, it->size, &ic);
    if (ic.isColliding.rect[GREEN]) {
      if (it->type == SQUAREBAR_ITEM_SPIKE) {
        if (it->size == 4) {
          play(EXPLOSION);
          gameOver();
        }
      } else {
        play(COIN);
        addScore(squarebarMultiplier, it->pos.x, it->pos.y);
        squarebarAddItem(SQUAREBAR_ITEM_GOLD);
        it->isAlive = false;
        continue;
      }
    }
    it->ticks--;
    if (it->ticks < 0 || ic.isColliding.rect[RED] || ic.isColliding.rect[YELLOW]) {
      squarebarAddItem(it->type);
      it->isAlive = false;
      continue;
    }
  }
}

void addGameSquarebar() {
  addGame(squarebarTitle, squarebarDescription, squarebarCharacters, squarebarCharactersCount,
          &squarebarOptions, false, &squarebarUpdate);
}
