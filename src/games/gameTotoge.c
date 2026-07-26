#include "../cglp.h"

int* totogeTitle = "TOTOGE";
int* totogeDescription = "[Tap]\n Jump";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] totogeCharacters = {
    {
        " r r  ",
        " lll l",
        "yl lll",
        " lllll",
        "  llyl",
        "     y",
    },
    {
        " r r  ",
        " lll  ",
        "yl ll ",
        " lllll",
        " yl ll",
        "y     ",
    },
    {
        "  rr  ",
        "  rr  ",
        " rRRr ",
        " rRRr ",
        "rRRRRr",
        "rrrrrr",
    },
};
int totogeCharactersCount = 3;

Options totogeOptions = {100, 100, 5, false};

struct TotogeSpike {
  Vector pos;
  int width;
  bool isAlive;
};
#define TOTOGE_MAX_SPIKE_COUNT 32
TotogeSpike[TOTOGE_MAX_SPIKE_COUNT] totogeSpikes;
int totogeSpikeIndex;
float totogeSpikeAddDist;
float totogeNextSpikeX;
float totogeNextSpikeVx;

struct TotogeWall {
  Vector pos;
  bool isAlive;
};
#define TOTOGE_MAX_WALL_COUNT 128
TotogeWall[TOTOGE_MAX_WALL_COUNT] totogeWalls;
int totogeWallIndex;
float totogeNextWallY;

Vector totogePos;
Vector totogeVel;
float totogeScr;

void totogeAddWall(float x, float y) {
  ASSIGN_ARRAY_ITEM(totogeWalls, totogeWallIndex, TotogeWall, w);
  vectorSet(&w->pos, x, y);
  w->isAlive = true;
  totogeWallIndex = cgl_wrap(totogeWallIndex + 1, 0, TOTOGE_MAX_WALL_COUNT);
}

void totogeUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(totogeSpikes);
    totogeSpikeIndex = 0;
    totogeSpikeAddDist = 0;
    totogeNextSpikeX = 10;
    totogeNextSpikeVx = 1;
    vectorSet(&totogePos, 50, 9);
    vectorSet(&totogeVel, 0.33, 0);
    totogeScr = 0;
    INIT_UNALIVED_ARRAY_FAST(totogeWalls);
    totogeWallIndex = 0;
    for (int i = -10; i < 10; i++) {
      totogeAddWall(5, i * 10 + 5);
      totogeAddWall(95, i * 10 + 5);
    }
    totogeNextWallY = 105;
  }
  while (totogeNextWallY < 105) {
    addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    totogeAddWall(5, totogeNextWallY);
    totogeAddWall(95, totogeNextWallY);
    totogeNextWallY += 10;
  }
  totogeNextWallY -= totogeScr;
  color = LIGHT_BLUE;
  FOR_EACH(totogeWalls, i) {
    ASSIGN_ARRAY_ITEM(totogeWalls, i, TotogeWall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.y -= totogeScr;
    box(w->pos.x, w->pos.y, 8, 8, &scratch);
    w->isAlive = w->pos.y > -99;
  }
  totogeSpikeAddDist -= totogeScr;
  if (totogeSpikeAddDist < 0) {
    play(LASER);
    int width = rndi(2, 5);
    ASSIGN_ARRAY_ITEM(totogeSpikes, totogeSpikeIndex, TotogeSpike, sp);
    vectorSet(&sp->pos, clamp(totogeNextSpikeX, 12, 93 - width * 6), 199);
    sp->width = width;
    sp->isAlive = true;
    totogeSpikeIndex = cgl_wrap(totogeSpikeIndex + 1, 0, TOTOGE_MAX_SPIKE_COUNT);
    totogeSpikeAddDist = rnd(99, 199);
    totogeNextSpikeX += totogeNextSpikeVx * width * 6;
    if ((totogeNextSpikeX < 9 && totogeNextSpikeVx < 0) ||
        (totogeNextSpikeX > 90 && totogeNextSpikeVx > 0)) {
      totogeNextSpikeVx *= -1;
    }
  }
  color = BLACK;
  FOR_EACH(totogeSpikes, i) {
    ASSIGN_ARRAY_ITEM(totogeSpikes, i, TotogeSpike, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.y -= totogeScr;
    int[5] spikeChars;
    TIMES(s->width, k) { spikeChars[k] = 'c'; }
    spikeChars[s->width] = 0;
    character(spikeChars, s->pos.x, clamp(s->pos.y, -99, 99), &scratch);
    s->isAlive = s->pos.y > -99;
  }
  color = TRANSPARENT;
  Vector cp;
  vectorSet(&cp, totogePos.x, totogePos.y);
  int iterCount = (int)clamp(totogeVel.y / 6, 1, 99);
  TIMES(iterCount, i) {
    Collision cc;
    character("a", cp.x, cp.y, &cc);
    if (cc.isColliding.character['c']) {
      play(RANDOM);
      gameOver();
    }
    cp.y -= 6;
  }
  color = BLACK;
  characterOptions.isMirrorX = totogeVel.x > 0;
  if (totogeVel.y > 0) {
    character("a", totogePos.x, totogePos.y, &scratch);
  } else {
    character("b", totogePos.x, totogePos.y, &scratch);
  }
  characterOptions.isMirrorX = false;
  totogePos.x += totogeVel.x * difficulty;
  totogePos.y += totogeVel.y * difficulty - totogeScr;
  if (totogePos.y < 10) {
    totogeScr = (totogePos.y - 10) * 0.5;
  } else if (totogePos.y > 30) {
    totogeScr = (totogePos.y - 30) * 0.5;
  }
  totogeVel.y += 0.05;
  if ((totogePos.x < 9 && totogeVel.x < 0) || (totogePos.x > 90 && totogeVel.x > 0)) {
    totogeVel.x *= -1;
  }
  if (totogeVel.y > 0 && totogeNextWallY < 150 && input.isJustPressed) {
    play(JUMP);
    float s = totogeVel.y * totogeVel.y * difficulty * difficulty;
    if (s > 10) {
      addScore(s, totogePos.x, totogePos.y);
    }
    totogeVel.y = -1;
  }
}

void addGameTotoge() {
  addGame(totogeTitle, totogeDescription, totogeCharacters,
          totogeCharactersCount, &totogeOptions, false, &totogeUpdate);
}
