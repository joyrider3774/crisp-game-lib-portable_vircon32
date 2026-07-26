#include "../cglp.h"

int* snake1Title = "SNAKE 1";
int* snake1Description = "[Tap]\n Turn";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] snake1Characters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int snake1CharactersCount = 1;

Options snake1Options = {100, 100, 9, true};

struct Snake1Head {
  Vector pos;
  float angle;
  float rotation;
};
Snake1Head snake1Head;
int snake1HeadMoveTicks;
bool snake1IsHeadGettingDollar;
bool snake1IsHeadTurning;
#define SNAKE1_MAX_BODY_COUNT 200
Vector[SNAKE1_MAX_BODY_COUNT] snake1Bodies;
int snake1BodyCount;
// Each dollar eaten removes 1 but spawns up to 2 replacements (net +1/eat) and dollars never expire
// on their own - a direct (non-wrapping) array write, so this must outsize any realistic eat count.
#define SNAKE1_MAX_DOLLAR_COUNT 256
Vector[SNAKE1_MAX_DOLLAR_COUNT] snake1Dollars;
int snake1DollarCount;
int[4][2] snake1AngleOfs = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
int* snake1HeadChars = ">v<^";
int* snake1WallChars = "################";
int* snake1EdgeWallChars = "#              #";

void snake1ShiftBodiesDown() {
  memcpy(&snake1Bodies[0], &snake1Bodies[1],
         (snake1BodyCount - 1) * sizeof(snake1Bodies[0]));
  snake1BodyCount--;
}

void snake1ShiftDollarsDown(int removedIndex) {
  memcpy(&snake1Dollars[removedIndex], &snake1Dollars[removedIndex + 1],
         (snake1DollarCount - 1 - removedIndex) * sizeof(snake1Dollars[0]));
  snake1DollarCount--;
}

void snake1Update() {
  Collision scratch;
  if (!ticks) {
    color = GREEN;
    vectorSet(&snake1Head.pos, 8, 8);
    snake1Head.angle = 0;
    snake1Head.rotation = 1;
    snake1HeadMoveTicks = 0;
    snake1IsHeadGettingDollar = false;
    snake1IsHeadTurning = false;
    snake1BodyCount = 0;
    TIMES(4, i) {
      vectorSet(&snake1Bodies[snake1BodyCount], 4 + i, 8);
      snake1BodyCount++;
    }
    snake1DollarCount = 0;
    vectorSet(&snake1Dollars[snake1DollarCount], 12, 8);
    snake1DollarCount++;
  }
  text(snake1WallChars, 3, 9, &scratch);
  for (int y = 1; y <= 13; y++) {
    text(snake1EdgeWallChars, 3, 9 + y * 6, &scratch);
  }
  text(snake1WallChars, 3, 9 + 14 * 6, &scratch);
  if (!snake1IsHeadTurning && input.isJustPressed) {
    play(SELECT);
    snake1IsHeadTurning = true;
  }
  snake1HeadMoveTicks--;
  if (snake1HeadMoveTicks < 0) {
    play(LASER);
    if (!snake1IsHeadGettingDollar) {
      snake1ShiftBodiesDown();
    } else {
      snake1IsHeadGettingDollar = false;
    }
    snake1Bodies[snake1BodyCount] = snake1Head.pos;
    snake1BodyCount++;
    if (snake1IsHeadTurning) {
      snake1Head.angle = cgl_wrap(snake1Head.angle + snake1Head.rotation, 0, 4);
      snake1IsHeadTurning = false;
    }
    int angleIndex = (int)snake1Head.angle;
    vectorAdd(&snake1Head.pos, snake1AngleOfs[angleIndex][0], snake1AngleOfs[angleIndex][1]);
    snake1HeadMoveTicks = 20 / difficulty;
  }
  for (int i = 0; i < snake1BodyCount; i++) {
    text("o", snake1Bodies[i].x * 6 + 3, snake1Bodies[i].y * 6 + 3, &scratch);
  }
  int[2] hc;
  hc[0] = snake1HeadChars[(int)cgl_wrap(snake1Head.angle + snake1Head.rotation, 0, 4)];
  hc[1] = 0;
  text(hc, snake1Head.pos.x * 6 + 3, snake1Head.pos.y * 6 + 3, &scratch);
  if (scratch.isColliding.text['o'] || scratch.isColliding.text['#']) {
    play(EXPLOSION);
    color = WHITE;
    rect(snake1Head.pos.x * 6, snake1Head.pos.y * 6, 6, 6, &scratch);
    color = GREEN;
    text("X", snake1Head.pos.x * 6 + 3, snake1Head.pos.y * 6 + 3, &scratch);
    gameOver();
  }
  bool ig = false;
  int di = 0;
  while (di < snake1DollarCount) {
    text("$", snake1Dollars[di].x * 6 + 3, snake1Dollars[di].y * 6 + 3, &scratch);
    if (scratch.isColliding.text['v'] || scratch.isColliding.text['>'] ||
        scratch.isColliding.text['<'] || scratch.isColliding.text['^']) {
      ig = true;
      snake1ShiftDollarsDown(di);
    } else {
      di++;
    }
  }
  if (ig) {
    play(COIN);
    addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    snake1IsHeadGettingDollar = true;
    snake1Head.rotation *= -1;
    color = TRANSPARENT;
    TIMES(2, i) {
      TIMES(99, j) {
        int x = rndi(2, 14);
        int y = rndi(3, 14);
        text("$", x * 6 + 3, y * 6 + 3, &scratch);
        if (scratch.isColliding.text['v'] || scratch.isColliding.text['>'] ||
            scratch.isColliding.text['<'] || scratch.isColliding.text['^'] ||
            scratch.isColliding.text['o']) {
        } else {
          vectorSet(&snake1Dollars[snake1DollarCount], x, y);
          snake1DollarCount++;
          break;
        }
      }
    }
    color = GREEN;
  }
}

void addGameSnake1() {
  addGame(snake1Title, snake1Description, snake1Characters,
          snake1CharactersCount, &snake1Options, false, &snake1Update);
}
