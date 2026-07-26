#include "../cglp.h"

int* boardingTitle = "BOARDING";
int* boardingDescription = "[Hold] Boarding";

int[8][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] boardingCharacters = {
    {
        " lll  ",
        "l l ll",
        " llll ",
        " l  l ",
        "ll  ll",
        "      ",
    },
    {
        " lll  ",
        "l l ll",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
        "      ",
    },
    {
        "     l",
        "    ll",
        "   ll ",
        "  ll  ",
        " ll   ",
        "ll    ",
    },
    {
        "l     ",
        "ll    ",
        " ll   ",
        "  ll  ",
        "   ll ",
        "    ll",
    },
    {
        "  l   ",
        " l    ",
        "lllll ",
        " l    ",
        "  l   ",
    },
    {
        "  l   ",
        "   l  ",
        "lllll ",
        "   l  ",
        "  l   ",
    },
};
int boardingCharactersCount = 8;

Options boardingOptions = {100, 100, 6, false};

struct BoardingBoard {
  Vector pos;
  int angle;
  bool isAlive;
};
#define BOARDING_MAX_BOARD_COUNT 64
BoardingBoard[BOARDING_MAX_BOARD_COUNT] boardingBoards;
int boardingBoardIndex;

struct BoardingPassenger {
  Vector pos;
  Vector vel;
  Vector prevPos;
  int type;
  int bc;
  bool isAlive;
};
#define BOARDING_MAX_PASSENGER_COUNT 32
BoardingPassenger[BOARDING_MAX_PASSENGER_COUNT] boardingPassengers;
int boardingPassengerIndex;

float boardingPx;
int boardingPt;
float boardingPi;
float boardingPvx;
float boardingGy;
float boardingTgy;
float boardingBi;
float boardingScr;
bool boardingIsFirstPressing;

void boardingAddBoard() {
  Vector pos;
  vectorSet(&pos, rnd(9, 90), boardingGy - 3);
  bool tooClose = false;
  FOR_EACH(boardingBoards, i) {
    ASSIGN_ARRAY_ITEM(boardingBoards, i, BoardingBoard, b);
    SKIP_IS_NOT_ALIVE(b);
    if (distanceTo(&b->pos, pos.x, pos.y) < 16) {
      tooClose = true;
      break;
    }
  }
  if (tooClose) {
    return;
  }
  ASSIGN_ARRAY_ITEM(boardingBoards, boardingBoardIndex, BoardingBoard, nb);
  nb->pos = pos;
  nb->angle = rndi(0, 2);
  nb->isAlive = true;
  boardingBoardIndex = cgl_wrap(boardingBoardIndex + 1, 0, BOARDING_MAX_BOARD_COUNT);
}

void boardingUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(boardingBoards);
    boardingBoardIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(boardingPassengers);
    boardingPassengerIndex = 0;
    boardingPx = 50;
    boardingPt = -1;
    boardingPi = 0;
    boardingPvx = -1;
    boardingGy = 50;
    boardingBi = 0;
    boardingScr = 0.1;
    boardingIsFirstPressing = true;
    while (boardingGy < 91) {
      boardingAddBoard();
      boardingGy += 2;
    }
    boardingGy = boardingTgy = 91;
  }
  color = BLACK;
  FOR_EACH(boardingBoards, i) {
    ASSIGN_ARRAY_ITEM(boardingBoards, i, BoardingBoard, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y -= boardingScr * difficulty;
    int[2] boardChar;
    boardChar[0] = 'e' + b->angle;
    boardChar[1] = 0;
    character(boardChar, b->pos.x, b->pos.y, &scratch);
    if (!(b->pos.y > 15 && b->pos.y < boardingGy)) {
      b->isAlive = false;
    }
  }
  boardingTgy -= boardingScr * difficulty;
  if (boardingTgy < 11) {
    boardingTgy = 11;
  }
  boardingGy += (boardingTgy - boardingGy) * 0.1;
  color = RED;
  rect(0, boardingGy, 50, 8, &scratch);
  color = BLUE;
  rect(50, boardingGy, 50, 8, &scratch);
  color = WHITE;
  character("a", 5, boardingGy + 4, &scratch);
  character("g", 12, boardingGy + 4, &scratch);
  text("GATE1", 20, boardingGy + 4, &scratch);
  character("c", 95, boardingGy + 4, &scratch);
  character("h", 88, boardingGy + 4, &scratch);
  text("GATE2", 55, boardingGy + 4, &scratch);
  if (boardingBi < 0) {
    boardingAddBoard();
    boardingBi += 4;
  }
  if (boardingPt < 0) {
    color = RED;
  } else {
    color = BLUE;
  }
  int[2] playerChar;
  playerChar[0] = 'a' + boardingPt + 1 + ((int)(ticks / 30) % 2);
  playerChar[1] = 0;
  character(playerChar, boardingPx, 9, &scratch);
  if (boardingGy <= 12) {
    play(EXPLOSION);
    gameOver();
  }
  boardingPi--;
  float speed = 1;
  if (input.isPressed && !boardingIsFirstPressing) {
    if (boardingPi < 0) {
      ASSIGN_ARRAY_ITEM(boardingPassengers, boardingPassengerIndex, BoardingPassenger, p);
      vectorSet(&p->pos, boardingPx, 9);
      vectorSet(&p->vel, 0, 0);
      p->type = boardingPt;
      p->prevPos = p->pos;
      p->bc = 0;
      p->isAlive = true;
      boardingPassengerIndex =
          cgl_wrap(boardingPassengerIndex + 1, 0, BOARDING_MAX_PASSENGER_COUNT);
      boardingPi = 9;
    }
    speed = 0.1;
  }
  boardingPx += boardingPvx * difficulty * speed;
  if ((boardingPx < 10 && boardingPvx < 0) || (boardingPx > 90 && boardingPvx > 0)) {
    boardingPvx *= -1;
  }
  if (input.isJustReleased) {
    if (boardingIsFirstPressing) {
      boardingIsFirstPressing = false;
    } else {
      boardingPt *= -1;
    }
  }
  FOR_EACH(boardingPassengers, i) {
    ASSIGN_ARRAY_ITEM(boardingPassengers, i, BoardingPassenger, p);
    SKIP_IS_NOT_ALIVE(p);
    p->vel.y += 0.2;
    vectorMul(&p->vel, 0.9);
    p->prevPos = p->pos;
    vectorAdd(&p->pos, p->vel.x, p->vel.y);
    if (p->type < 0) {
      color = RED;
    } else {
      color = BLUE;
    }
    int[2] pChar;
    pChar[0] = 'a' + p->type + 1 + ((int)(ticks / 30) % 2);
    pChar[1] = 0;
    character(pChar, p->pos.x, p->pos.y, &scratch);
    bool ce = scratch.isColliding.character['e'];
    bool cf = scratch.isColliding.character['f'];
    if (ce || cf) {
      float a = cgl_wrap(angleTo(&p->pos, p->prevPos.x, p->prevPos.y), -CGLP_PI, CGLP_PI);
      float ra;
      if (ce) {
        if (a < -CGLP_PI / 4 || a > CGLP_PI / 4 * 3) {
          ra = -CGLP_PI / 4 * 3;
        } else {
          ra = CGLP_PI / 4;
        }
      } else {
        if (a < -CGLP_PI / 4 * 3 || a > CGLP_PI / 4) {
          ra = CGLP_PI / 4 * 3;
        } else {
          ra = -CGLP_PI / 4;
        }
      }
      Vector v;
      vectorSet(&v, vectorLength(&p->vel) * 2, 0);
      rotate(&v, ra);
      vectorAdd(&p->vel, v.x, v.y);
      vectorAdd(&p->pos, p->vel.x, p->vel.y);
      vectorAdd(&p->pos, p->vel.x, p->vel.y);
      p->bc++;
    }
    if (p->pos.y < 0 && p->vel.y < 0) {
      p->vel.y *= -0.5;
    }
    if ((p->pos.x < 0 && p->vel.x < 0) || (p->pos.x > 99 && p->vel.x > 0)) {
      p->vel.x *= -1;
    }
    if (p->pos.y > boardingGy) {
      bool isOk = (p->pos.x - 50) * p->type > 0;
      if (isOk) {
        if (p->bc > 0) {
          play(POWER_UP);
          addScore(p->bc, p->pos.x, p->pos.y);
          float oy = p->bc * 2 * difficulty;
          boardingTgy += oy;
          boardingBi -= oy;
          if (boardingTgy > 91) {
            boardingTgy = 91;
          }
        } else {
          addScore(0, p->pos.x, p->pos.y);
        }
        p->isAlive = false;
        continue;
      } else {
        play(HIT);
        addScore(-1 - p->bc, p->pos.x, p->pos.y);
        float oy2 = sqrt(1 + p->bc) * difficulty;
        if (oy2 > 20) {
          oy2 = 20;
        }
        boardingTgy -= oy2;
        p->isAlive = false;
        continue;
      }
    }
  }
}

void addGameBoarding() {
  addGame(boardingTitle, boardingDescription, boardingCharacters,
          boardingCharactersCount, &boardingOptions, false, &boardingUpdate);
}
