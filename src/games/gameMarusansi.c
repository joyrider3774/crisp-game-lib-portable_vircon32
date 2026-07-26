#include "../cglp.h"

int* marusansiTitle = "MARUSANSI";
int* marusansiDescription = "   Tap\nto start";

int[8][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] marusansiCharacters = {
    {
        " rrr  ",
        "rRRRr ",
        "rRRRr ",
        "rRRRr ",
        " rrr  ",
    },
    {
        "  g   ",
        " gGg  ",
        " gGg  ",
        "gGGGg ",
        "ggggg ",
    },
    {
        "bbbbb ",
        "bBBBb ",
        "bBBBb ",
        "bBBBb ",
        "bbbbb ",
    },
    {
        " RRR  ",
        "R   R ",
        "R   R ",
        "R   R ",
        " RRR  ",
    },
    {
        "  G   ",
        " G G  ",
        " G G  ",
        "G   G ",
        "GGGGG ",
    },
    {
        "BBBBB ",
        "B   B ",
        "B   B ",
        "B   B ",
        "BBBBB ",
    },
    {
        "  l   ",
        " lll  ",
        "l l l ",
        "  l   ",
        "  l   ",
    },
    {
        "  l   ",
        "  l   ",
        "l l l ",
        " lll  ",
        "  l   ",
    },
};
int marusansiCharactersCount = 8;

// Vircon32 port note: this engine has no rewind/replay system (see the
// FOOSAN/FOOT LASER port notes elsewhere) - the JS "startTicks =
// isReplaying ? 0 : 270" always takes the non-replaying branch here.
Options marusansiOptions = {80, 80, 9, false};

#define MARUSANSI_GRID_WIDTH 6
#define MARUSANSI_GRID_HEIGHT 12

int[MARUSANSI_GRID_WIDTH][MARUSANSI_GRID_HEIGHT] marusansiGrid;
int[MARUSANSI_GRID_WIDTH] marusansiGridHeight;
int[MARUSANSI_GRID_WIDTH][MARUSANSI_GRID_HEIGHT] marusansiHGrid;
int[MARUSANSI_GRID_WIDTH][MARUSANSI_GRID_HEIGHT] marusansiVGrid;
bool[MARUSANSI_GRID_WIDTH][MARUSANSI_GRID_HEIGHT] marusansiSGrid;
int[2] marusansiBlocks;
int[MARUSANSI_GRID_WIDTH] marusansiNextRow;
float marusansiNextRowTicks;
float marusansiChainingTicks;
float marusansiFallingTicks;
int marusansiMultiplier;
int[32] marusansiMessage;
float marusansiMessageTicks;
float marusansiStartTicks;

void marusansiCalcPixelPosition(int x, int y, Vector* out) {
  out->x = 40 - MARUSANSI_GRID_WIDTH * 6 / 2 + x * 6 + 3;
  out->y = MARUSANSI_GRID_HEIGHT * 6 - y * 6 - 3;
}

void marusansiCalcGridHeight() {
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    int y = 0;
    for (; y < MARUSANSI_GRID_HEIGHT; y++) {
      if (marusansiGrid[x][y] == 0) {
        break;
      }
    }
    marusansiGridHeight[x] = y;
  }
}

void marusansiCheckGridSequence() {
  bool existsSequence = false;
  int count = 0;
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      int g = marusansiGrid[x][y];
      if (g == 0) {
        continue;
      }
      if (marusansiHGrid[x][y] == 0 && x < MARUSANSI_GRID_WIDTH - 2 &&
          marusansiGrid[x + 1][y] == g && marusansiGrid[x + 2][y] == g) {
        existsSequence = true;
        int i = g;
        for (int gx = x; gx < MARUSANSI_GRID_WIDTH; gx++) {
          i++;
          if ((i - 1) % 3 == g - 1) {
            i++;
          }
          if (marusansiGrid[gx][y] != g) {
            break;
          }
          marusansiHGrid[gx][y] = i;
          count++;
        }
      }
      if (marusansiVGrid[x][y] == 0 && y < MARUSANSI_GRID_HEIGHT - 2 &&
          marusansiGrid[x][y + 1] == g && marusansiGrid[x][y + 2] == g) {
        existsSequence = true;
        int vi = g;
        for (int gy = y; gy < MARUSANSI_GRID_HEIGHT; gy++) {
          vi++;
          if ((vi - 1) % 3 == g - 1) {
            vi++;
          }
          if (marusansiGrid[x][gy] != g) {
            break;
          }
          marusansiVGrid[x][gy] = vi;
          count++;
        }
      }
    }
  }
  if (existsSequence) {
    int sc = count * marusansiMultiplier;
    strcpy(marusansiMessage, intToChar(count));
    strcat(marusansiMessage, "x");
    strcat(marusansiMessage, intToChar(marusansiMultiplier));
    strcat(marusansiMessage, "=");
    strcat(marusansiMessage, intToChar(sc));
    marusansiMessageTicks = 60;
    addScore(sc, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    marusansiChainingTicks = 1;
    marusansiMultiplier++;
    play(POWER_UP);
  }
}

void marusansiChangeSequence() {
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      int vh;
      if (marusansiVGrid[x][y] > 0) {
        vh = marusansiVGrid[x][y];
      } else {
        vh = marusansiHGrid[x][y];
      }
      if (vh > 0) {
        marusansiGrid[x][y] = (vh - 1) % 3 + 1;
        marusansiSGrid[x][y] = true;
        marusansiVGrid[x][y] = 0;
        marusansiHGrid[x][y] = 0;
      }
    }
  }
}

void marusansiClearSequence() {
  play(JUMP);
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      if (marusansiSGrid[x][y]) {
        if (marusansiGrid[x][y] == 1) {
          color = RED;
        } else if (marusansiGrid[x][y] == 2) {
          color = GREEN;
        } else {
          color = BLUE;
        }
        Vector p;
        marusansiCalcPixelPosition(x, y, &p);
        particle(p.x, p.y, 9, 1, 0, CGLP_PI * 2);
        marusansiGrid[x][y] = 0;
      }
    }
  }
  color = BLACK;
  marusansiFallingTicks = 1;
}

bool marusansiFallBlocks() {
  bool isFalling = false;
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      if (marusansiSGrid[x][y]) {
        isFalling = true;
        for (int gy = y; gy < MARUSANSI_GRID_HEIGHT - 1; gy++) {
          marusansiGrid[x][gy] = marusansiGrid[x][gy + 1];
          marusansiSGrid[x][gy] = marusansiSGrid[x][gy + 1];
        }
        marusansiGrid[x][MARUSANSI_GRID_HEIGHT - 1] = 0;
        marusansiSGrid[x][MARUSANSI_GRID_HEIGHT - 1] = false;
      }
    }
  }
  return isFalling;
}

void marusansiSetNextBlock() {
  int pb0 = marusansiBlocks[0];
  int pb1 = marusansiBlocks[1];
  TIMES(2, i) { marusansiBlocks[i] = rndi(1, 4); }
  if (marusansiBlocks[0] == pb0 && marusansiBlocks[1] == pb1) {
    marusansiBlocks[0] = marusansiBlocks[0] % 3 + 1;
  }
}

void marusansiSetNextRow() {
  play(LASER);
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    int[3] cs;
    int csCount = 0;
    TIMES(3, i) {
      int c = i + 1;
      if (marusansiGrid[x][0] == c && marusansiGrid[x][1] == c) {
        continue;
      }
      if (x > 1 && marusansiNextRow[x - 1] == c && marusansiNextRow[x - 2] == c) {
        continue;
      }
      cs[csCount] = c;
      csCount++;
    }
    if (csCount == 0) {
      cs[0] = rndi(1, 4);
      csCount = 1;
    }
    marusansiNextRow[x] = cs[rndi(0, csCount)];
  }
}

void marusansiAddNextRow() {
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    for (int y = MARUSANSI_GRID_HEIGHT - 1; y > 0; y--) {
      marusansiGrid[x][y] = marusansiGrid[x][y - 1];
    }
    marusansiGrid[x][0] = marusansiNextRow[x];
  }
  marusansiSetNextRow();
  marusansiCalcGridHeight();
  marusansiNextRowTicks = 0;
}

void marusansiUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - the tapped
  // column/next-row-bar is computed directly from input.pos via grid
  // math (see "isSelected"/"isNextRowSelected" below), so the engine's
  // own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure waste
  // here. Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    TIMES(MARUSANSI_GRID_WIDTH, x) {
      TIMES(MARUSANSI_GRID_HEIGHT, y) {
        marusansiGrid[x][y] = 0;
        marusansiHGrid[x][y] = 0;
        marusansiVGrid[x][y] = 0;
        marusansiSGrid[x][y] = false;
      }
      marusansiGridHeight[x] = 0;
      marusansiNextRow[x] = 0;
    }
    marusansiBlocks[0] = 1;
    marusansiBlocks[1] = 2;
    marusansiNextRowTicks = 0;
    marusansiCalcGridHeight();
    marusansiSetNextRow();
    marusansiChainingTicks = 0;
    marusansiFallingTicks = 0;
    marusansiMultiplier = 1;
    marusansiMessage[0] = 0;
    marusansiMessageTicks = 0;
    marusansiStartTicks = 270;
  }
  color = LIGHT_BLACK;
  rect(20, MARUSANSI_GRID_HEIGHT * 6, 40, 1, &scratch);
  color = BLACK;
  int chainingIndex;
  if (marusansiChainingTicks > 0) {
    chainingIndex = (int)floor(marusansiChainingTicks / 5);
  } else {
    chainingIndex = 0;
  }
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      int g = marusansiGrid[x][y];
      if (g == 0) {
        continue;
      }
      Vector p;
      marusansiCalcPixelPosition(x, y, &p);
      int vh;
      if (marusansiVGrid[x][y] > 0) {
        vh = marusansiVGrid[x][y];
      } else {
        vh = marusansiHGrid[x][y];
      }
      if (marusansiSGrid[x][y] || vh > 0) {
        color = YELLOW;
        box(p.x, p.y, 6, 6, &scratch);
        color = BLACK;
      }
      int[2] cc;
      cc[1] = 0;
      if (vh > 0) {
        if (vh == chainingIndex) {
          play(COIN);
        } else if (vh < chainingIndex) {
          int cg = (vh - 1) % 3 + 1;
          cc[0] = 'a' + cg - 1;
          character(cc, p.x, p.y, &scratch);
        } else {
          cc[0] = 'd' + g - 1;
          character(cc, p.x, p.y, &scratch);
        }
      } else {
        cc[0] = 'a' + g - 1;
        character(cc, p.x, p.y, &scratch);
      }
    }
  }
  if (marusansiMessageTicks > 0) {
    marusansiMessageTicks--;
    text(marusansiMessage, 3, 9, &scratch);
  }
  if (marusansiFallingTicks > 0) {
    marusansiFallingTicks++;
    if ((int)marusansiFallingTicks % 5 == 0) {
      if (!marusansiFallBlocks()) {
        marusansiFallingTicks = 0;
        marusansiNextRowTicks = 0;
        marusansiCalcGridHeight();
        marusansiCheckGridSequence();
        marusansiSetNextRow();
      }
    }
    return;
  }
  if (marusansiChainingTicks > 0) {
    marusansiChainingTicks++;
    if (marusansiChainingTicks > 50) {
      marusansiChainingTicks = 0;
      marusansiChangeSequence();
      marusansiCheckGridSequence();
      if (marusansiChainingTicks == 0 || marusansiMultiplier >= 32) {
        marusansiChainingTicks = 0;
        marusansiClearSequence();
      }
    }
    return;
  }
  int bx = -1;
  bool hasPlace = false;
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    int h = marusansiGridHeight[x];
    if (h < MARUSANSI_GRID_HEIGHT - 1) {
      hasPlace = true;
      Vector p;
      marusansiCalcPixelPosition(x, h + 1, &p);
      bool isSelected = input.pos.x >= p.x - 3 && input.pos.x < p.x + 3 && input.pos.y >= p.y - 3 &&
                         input.pos.y < p.y + 9;
      TIMES(2, i) {
        if (!isSelected) {
          color = LIGHT_BLACK;
        }
        int[2] cc;
        cc[0] = 'd' + marusansiBlocks[i] - 1;
        cc[1] = 0;
        character(cc, p.x, p.y + i * 6, &scratch);
        color = BLACK;
      }
      if (isSelected) {
        bx = x;
      }
    }
  }
  if (!hasPlace) {
    play(EXPLOSION);
    color = WHITE;
    rect(20, 0, 40, 6, &scratch);
    rect(20, 30, 40, 20, &scratch);
    color = BLACK;
    gameOver();
    return;
  }
  bool isNextRowSelected = input.pos.y >= MARUSANSI_GRID_HEIGHT * 6;
  color = LIGHT_PURPLE;
  marusansiNextRowTicks += sqrt(sqrt(difficulty));
  rect(20, 80, 40, -marusansiNextRowTicks / 50, &scratch);
  color = BLACK;
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    Vector p;
    marusansiCalcPixelPosition(x, -1, &p);
    int[2] cc;
    if (isNextRowSelected) {
      cc[0] = 'a' + marusansiNextRow[x] - 1;
    } else {
      cc[0] = 'd' + marusansiNextRow[x] - 1;
    }
    cc[1] = 0;
    character(cc, p.x, p.y, &scratch);
  }
  if (marusansiNextRowTicks > 400) {
    marusansiAddNextRow();
    return;
  }
  if (input.isJustPressed) {
    if (isNextRowSelected) {
      marusansiAddNextRow();
    } else if (bx >= 0) {
      play(SELECT);
      int y = marusansiGridHeight[bx];
      marusansiGrid[bx][y + 1] = marusansiBlocks[0];
      marusansiGrid[bx][y] = marusansiBlocks[1];
      marusansiMultiplier = 1;
      marusansiSetNextBlock();
      marusansiCalcGridHeight();
      marusansiCheckGridSequence();
    } else {
      play(CLICK);
      int tb = marusansiBlocks[0];
      marusansiBlocks[0] = marusansiBlocks[1];
      marusansiBlocks[1] = tb;
    }
  }
  marusansiStartTicks--;
  if (marusansiStartTicks > 0) {
    text("Tap\nto", 3, 40, &scratch);
    text("rotate", 26, 30, &scratch);
    text("place", 26, 66, &scratch);
    text("add", 26, 76, &scratch);
    text("blo\ncks", 65, 40, &scratch);
  }
}

void addGameMarusansi() {
  addGame(marusansiTitle, marusansiDescription, marusansiCharacters,
          marusansiCharactersCount, &marusansiOptions, true, &marusansiUpdate);
}
