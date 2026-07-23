#include "../cglp.h"

char* pakupakuTitle = "PAKU PAKU";
char* pakupakuDescription = "[Tap]   Turn\n[Arrow] Move";

int[10][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pakupakuCharacters = {
    {
        "  llll",
        " lll  ",
        "lll   ",
        "lll   ",
        " lll  ",
        "  llll",
    },
    {
        "  lll ",
        " lllll",
        "lll   ",
        "lll   ",
        " lllll",
        "  lll ",
    },
    {
        "  ll  ",
        " llll ",
        "llllll",
        "llllll",
        " llll ",
        "  ll  ",
    },
    {
        "  lll ",
        " l l l",
        " llll ",
        " llll ",
        "llll  ",
        "l l l ",
    },
    {
        "  lll ",
        " l l l",
        " llll ",
        " llll ",
        " llll ",
        " l l  ",
    },
    {
        "      ",
        "      ",
        "  ll  ",
        "  ll  ",
        "      ",
        "      ",
    },
    {
        "      ",
        "  ll  ",
        " llll ",
        " llll ",
        "  ll  ",
        "      ",
    },
    {
        "      ",
        "   l l",
        "      ",
        "      ",
        "      ",
        "      ",
    },
    {
        "      ",
        "  lll ",
        " l   l",
        " llll ",
        "llll  ",
        "l l   ",
    },
    {
        "      ",
        "  lll ",
        " l   l",
        " llll ",
        " llll ",
        " l l  ",
    },
};
int pakupakuCharactersCount = 10;

Options pakupakuOptions = {100, 50, 4, true};

struct PakupakuPlayer {
  float x;
  float vx;
};
PakupakuPlayer pakupakuPlayer;

struct PakupakuEnemy {
  float x;
  float eyeVx;
};
PakupakuEnemy pakupakuEnemy;

struct PakupakuDot {
  float x;
  bool isPower;
  bool isAlive;
};
#define PAKU_PAKU_MAX_DOT_COUNT 16
PakupakuDot[PAKU_PAKU_MAX_DOT_COUNT] pakupakuDots;
float pakupakuPowerTicks;
float pakupakuAnimTicks;
int pakupakuMultiplier;

void pakupakuAddDots() {
  int pIndex;
  if (pakupakuPlayer.x > 50) {
    pIndex = rndi(1, 6);
  } else {
    pIndex = rndi(10, 15);
  }
  for (int i = 0; i < 16; i++) {
    pakupakuDots[i].x = i * 6 + 5;
    pakupakuDots[i].isPower = i == pIndex;
    pakupakuDots[i].isAlive = true;
  }
  pakupakuMultiplier++;
}

void pakupakuUpdate() {
  Collision scratch;
  if (!ticks) {
    pakupakuPlayer.x = 40;
    pakupakuPlayer.vx = 1;
    pakupakuEnemy.x = 100;
    pakupakuEnemy.eyeVx = 0;
    pakupakuMultiplier = 1;
    pakupakuAddDots();
    pakupakuPowerTicks = pakupakuAnimTicks = 0;
  }
  pakupakuAnimTicks += difficulty;
  color = BLACK;
  text("x", 3, 9, &scratch);
  text(intToChar(pakupakuMultiplier), 9, 9, &scratch);
  bool isJustPressed = input.isJustPressed;
  if (isJustPressed && ((input.left.isJustPressed && pakupakuPlayer.vx < 0) ||
                        (input.right.isJustPressed && pakupakuPlayer.vx > 0))) {
    isJustPressed = false;
  }
  if (isJustPressed) {
    pakupakuPlayer.vx *= -1;
  }
  pakupakuPlayer.x += pakupakuPlayer.vx * 0.5 * difficulty;
  if (pakupakuPlayer.x < -3) {
    pakupakuPlayer.x = 103;
  } else if (pakupakuPlayer.x > 103) {
    pakupakuPlayer.x = -3;
  }
  color = BLUE;
  rect(0, 23, 100, 1, &scratch);
  rect(0, 25, 100, 1, &scratch);
  rect(0, 34, 100, 1, &scratch);
  rect(0, 36, 100, 1, &scratch);
  color = GREEN;
  int ai = ((int)pakupakuAnimTicks / 7) % 4;
  characterOptions.isMirrorX = pakupakuPlayer.vx < 0;
  int[2] pc;
  if (ai == 3) {
    pc[0] = 'a' + 1;
  } else {
    pc[0] = 'a' + ai;
  }
  pc[1] = 0;
  character(pc, pakupakuPlayer.x, 30, &scratch);
  characterOptions.isMirrorX = false;
  FOR_EACH(pakupakuDots, i) {
    ASSIGN_ARRAY_ITEM(pakupakuDots, i, PakupakuDot, d);
    SKIP_IS_NOT_ALIVE(d);
    if (d->isPower && ((int)pakupakuAnimTicks / 7) % 2 == 0) {
      color = TRANSPARENT;
    } else {
      color = YELLOW;
    }
    int[2] dc;
    if (d->isPower) {
      dc[0] = 'g';
    } else {
      dc[0] = 'f';
    }
    dc[1] = 0;
    Collision dotCl;
    character(dc, d->x, 30, &dotCl);
    if (dotCl.isColliding.character['a'] || dotCl.isColliding.character['b'] ||
        dotCl.isColliding.character['c']) {
      if (d->isPower) {
        play(JUMP);
        if (pakupakuEnemy.eyeVx == 0) {
          pakupakuPowerTicks = 120;
        }
      } else {
        play(HIT);
      }
      score += pakupakuMultiplier;
      d->isAlive = false;
    }
  }
  float evx;
  if (pakupakuEnemy.eyeVx != 0) {
    evx = pakupakuEnemy.eyeVx;
  } else {
    float dirSign;
    if (pakupakuPlayer.x > pakupakuEnemy.x) {
      dirSign = 1;
    } else {
      dirSign = -1;
    }
    float powerSign;
    if (pakupakuPowerTicks > 0) {
      powerSign = -1;
    } else {
      powerSign = 1;
    }
    evx = dirSign * powerSign;
  }
  float speedFactor;
  if (pakupakuPowerTicks > 0) {
    speedFactor = 0.25;
  } else if (pakupakuEnemy.eyeVx != 0) {
    speedFactor = 0.75;
  } else {
    speedFactor = 0.55;
  }
  pakupakuEnemy.x = clamp(pakupakuEnemy.x + evx * speedFactor * difficulty, 0, 100);
  if ((pakupakuEnemy.eyeVx < 0 && pakupakuEnemy.x < 1) ||
      (pakupakuEnemy.eyeVx > 0 && pakupakuEnemy.x > 99)) {
    pakupakuEnemy.eyeVx = 0;
  }
  if (pakupakuEnemy.eyeVx != 0) {
    color = BLACK;
  } else if (pakupakuPowerTicks <= 0) {
    color = RED;
  } else {
    color = BLUE;
    if (pakupakuPowerTicks < 30 && (int)pakupakuPowerTicks % 10 < 5) {
      color = TRANSPARENT;
    }
  }
  characterOptions.isMirrorX = evx < 0;
  int ecBase;
  if (pakupakuEnemy.eyeVx != 0) {
    ecBase = 'h';
  } else {
    int baseChar;
    if (pakupakuPowerTicks > 0) {
      baseChar = 'i';
    } else {
      baseChar = 'd';
    }
    ecBase = baseChar + ((int)pakupakuAnimTicks / 7) % 2;
  }
  int[2] ec;
  ec[0] = ecBase;
  ec[1] = 0;
  Collision enemyCl;
  character(ec, pakupakuEnemy.x, 30, &enemyCl);
  characterOptions.isMirrorX = false;
  if (pakupakuEnemy.eyeVx == 0 &&
      (enemyCl.isColliding.character['a'] || enemyCl.isColliding.character['b'] ||
       enemyCl.isColliding.character['c'])) {
    if (pakupakuPowerTicks > 0) {
      play(POWER_UP);
      addScore(10 * pakupakuMultiplier, pakupakuEnemy.x, 30);
      if (pakupakuPlayer.x > 50) {
        pakupakuEnemy.eyeVx = -1;
      } else {
        pakupakuEnemy.eyeVx = 1;
      }
      pakupakuPowerTicks = 0;
      pakupakuMultiplier++;
    } else {
      play(EXPLOSION);
      gameOver();
    }
  }
  pakupakuPowerTicks -= difficulty;
  COUNT_IS_ALIVE(pakupakuDots, dc2);
  if (dc2 == 0) {
    play(COIN);
    pakupakuAddDots();
  }
}

void addGamePakuPaku() {
  addGame(pakupakuTitle, pakupakuDescription, pakupakuCharacters, pakupakuCharactersCount,
          &pakupakuOptions, false, &pakupakuUpdate);
}
