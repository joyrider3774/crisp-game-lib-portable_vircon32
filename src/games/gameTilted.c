#include "../cglp.h"

int* tiltedTitle = "TILTED";
int* tiltedDescription = "[Tap]\n Jump\n Jump & Turn";

// Index 4 is a genuine gap in the upstream character array (elided, never
// drawn) - kept blank here so index 5 ("e") still lines up correctly.
int[6][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tiltedCharacters = {
    {
        "  lll ",
        "  lll ",
        " lll  ",
        "  l   ",
        " l ll ",
        "l    l",
    },
    {
        "  lll ",
        "  lll ",
        "llllll",
        "  l   ",
        " l l  ",
        " l l  ",
    },
    {
        "  lll ",
        "l lll ",
        " ll   ",
        "  l   ",
        " l lll",
        "l     ",
    },
    {
        " llll ",
        "l llll",
        "llllll",
        "llllll",
        "llllll",
        " llll ",
    },
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    },
    {
        " llll ",
        "l llll",
        "llllll",
        "llllll",
        "llllll",
        " llll ",
    },
};
int tiltedCharactersCount = 6;

Options tiltedOptions = {100, 100, 3000, false};

struct TiltedBar {
  Vector pos;
  float angle;
  float length;
  bool isAlive;
};
#define TILTED_MAX_BAR_COUNT 128
TiltedBar[TILTED_MAX_BAR_COUNT] tiltedBars;
int tiltedBarIndex;
float tiltedNextBarDist;
int tiltedBarCount;

struct TiltedBall {
  Vector pos;
  Vector vel;
  float ticks;
  bool isAlive;
};
// Spawn interval shrinks as ~1/difficulty (rnd(60,80)/difficulty) while
// each ball's accelerating fall (vel.y ramps up every tick) only shrinks
// its lifetime roughly as difficulty^-0.25, so concurrent balls grow like
// difficulty^0.75 - already exceeds 32 within ~1-2 hours of continuous
// play, so sized with real headroom.
#define TILTED_MAX_BALL_COUNT 512
TiltedBall[TILTED_MAX_BALL_COUNT] tiltedBalls;
int tiltedBallIndex;
float tiltedNextBallTicks;

struct TiltedPlayer {
  Vector pos;
  Vector vel;
  bool hasBar;
  int barIndex;
  int jumpCount;
};
TiltedPlayer tiltedPlayer;

void tiltedUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(tiltedBars);
    tiltedBarIndex = 0;
    ASSIGN_ARRAY_ITEM(tiltedBars, tiltedBarIndex, TiltedBar, initBar);
    vectorSet(&initBar->pos, 50, 0);
    initBar->angle = 0;
    initBar->length = 90;
    initBar->isAlive = true;
    int initialBarIndex = tiltedBarIndex;
    tiltedBarIndex = cgl_wrap(tiltedBarIndex + 1, 0, TILTED_MAX_BAR_COUNT);
    tiltedNextBarDist = 10;
    tiltedBarCount = 0;
    INIT_UNALIVED_ARRAY_FAST(tiltedBalls);
    tiltedBallIndex = 0;
    tiltedNextBallTicks = 60;
    vectorSet(&tiltedPlayer.pos, 50, -6);
    vectorSet(&tiltedPlayer.vel, 1, 0);
    tiltedPlayer.hasBar = true;
    tiltedPlayer.barIndex = initialBarIndex;
    tiltedPlayer.jumpCount = 0;
  }
  float scr = difficulty * 0.02;
  if (tiltedPlayer.pos.y < 60) {
    scr += (60 - tiltedPlayer.pos.y) * 0.1;
  }
  addScore(scr, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  int[2] pc;
  if (tiltedPlayer.hasBar) {
    TiltedBar* b = &tiltedBars[tiltedPlayer.barIndex];
    Vector tmp;
    vectorSet(&tmp, tiltedPlayer.pos.x, tiltedPlayer.pos.y + 5);
    float d = distanceTo(&tmp, b->pos.x, b->pos.y);
    if (tiltedPlayer.pos.x < b->pos.x) {
      d = -d;
    }
    d += (tiltedPlayer.vel.x / 2) * sqrt(difficulty);
    if (fabs(d) > b->length / 2 + 6) {
      tiltedPlayer.hasBar = false;
      tiltedPlayer.jumpCount = 0;
    }
    Vector newPos;
    vectorSet(&newPos, b->pos.x, b->pos.y);
    addWithAngle(&newPos, b->angle, d);
    vectorAdd(&newPos, 0, -5);
    tiltedPlayer.pos = newPos;
    if (input.isJustPressed) {
      play(JUMP);
      tiltedPlayer.hasBar = false;
      tiltedPlayer.vel.y = -2;
      tiltedPlayer.pos.y -= 2 * sqrt(difficulty);
      tiltedPlayer.jumpCount = 1;
    }
    pc[0] = 'a' + (ticks / 15) % 2;
    pc[1] = 0;
  } else {
    if (input.isJustPressed) {
      play(JUMP);
      if (tiltedPlayer.jumpCount > 0) {
        tiltedPlayer.vel.x *= -1;
      } else {
        tiltedPlayer.jumpCount = 1;
      }
      tiltedPlayer.vel.y = -2.0 / tiltedPlayer.jumpCount / tiltedPlayer.jumpCount;
      tiltedPlayer.jumpCount++;
    }
    tiltedPlayer.pos.y += scr;
    float dvy;
    if (input.isPressed) {
      dvy = 0.05;
    } else {
      dvy = 0.1;
    }
    tiltedPlayer.vel.y += dvy;
    vectorAdd(&tiltedPlayer.pos, tiltedPlayer.vel.x * sqrt(difficulty) / 2,
              tiltedPlayer.vel.y * sqrt(difficulty) / 2);
    pc[0] = 'c';
    pc[1] = 0;
  }
  if ((tiltedPlayer.pos.x < 8 && tiltedPlayer.vel.x < 0) ||
      (tiltedPlayer.pos.x > 92 && tiltedPlayer.vel.x > 0)) {
    tiltedPlayer.vel.x *= -1;
  }
  if (tiltedPlayer.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLACK;
  characterOptions.isMirrorX = tiltedPlayer.vel.x <= 0;
  character(pc, tiltedPlayer.pos.x, tiltedPlayer.pos.y, &scratch);
  characterOptions.isMirrorX = false;
  tiltedNextBallTicks--;
  if (tiltedNextBallTicks < 0) {
    ASSIGN_ARRAY_ITEM(tiltedBalls, tiltedBallIndex, TiltedBall, nball);
    vectorSet(&nball->pos, rnd(20, 80), -40);
    vectorSet(&nball->vel, rnd(0.5, 1) * RNDPM(), 0);
    nball->ticks = 0;
    nball->isAlive = true;
    tiltedBallIndex = cgl_wrap(tiltedBallIndex + 1, 0, TILTED_MAX_BALL_COUNT);
    tiltedNextBallTicks += rnd(60, 80) / difficulty;
  }
  FOR_EACH(tiltedBalls, bli2) {
    ASSIGN_ARRAY_ITEM(tiltedBalls, bli2, TiltedBall, bl2);
    SKIP_IS_NOT_ALIVE(bl2);
    bl2->pos.y += scr;
    bl2->vel.y += 0.05;
    vectorMul(&bl2->vel, 0.995);
    vectorAdd(&bl2->pos, bl2->vel.x * sqrt(difficulty) / 2, bl2->vel.y * sqrt(difficulty) / 2);
    if ((bl2->pos.x < 8 && bl2->vel.x < 0) || (bl2->pos.x > 92 && bl2->vel.x > 0)) {
      bl2->vel.x *= -1;
    }
    color = YELLOW;
    Collision bc;
    character("d", bl2->pos.x, bl2->pos.y, &bc);
    if (bc.isColliding.character['a'] || bc.isColliding.character['b'] ||
        bc.isColliding.character['c']) {
      play(HIT);
      if (tiltedPlayer.hasBar) {
        tiltedBars[tiltedPlayer.barIndex].isAlive = false;
        tiltedPlayer.hasBar = false;
        tiltedPlayer.vel.y += 1;
      } else {
        tiltedPlayer.vel.y += 4;
      }
      particle(bl2->pos.x, bl2->pos.y, 20, 3, CGLP_PI / 2, CGLP_PI / 3);
      bl2->isAlive = false;
      continue;
    }
    if (bc.isColliding.character['d']) {
      bl2->ticks = 999;
      character("e", bl2->pos.x, bl2->pos.y, &scratch);
    }
    bl2->ticks++;
    if (bl2->ticks > 500) {
      particle(bl2->pos.x, bl2->pos.y, 16, 1, 0, CGLP_PI * 2);
      bl2->isAlive = false;
      continue;
    }
    if (bl2->pos.y > 103) {
      bl2->isAlive = false;
      continue;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(tiltedBalls, bli3) {
    ASSIGN_ARRAY_ITEM(tiltedBalls, bli3, TiltedBall, bl3);
    SKIP_IS_NOT_ALIVE(bl3);
    Collision ec;
    character("d", bl3->pos.x, bl3->pos.y, &ec);
    if (ec.isColliding.character['e']) {
      bl3->ticks = 999;
    }
  }
  color = LIGHT_BLACK;
  rect(0, 0, 5, 100, &scratch);
  rect(95, 0, 5, 100, &scratch);
  FOR_EACH(tiltedBars, bai) {
    ASSIGN_ARRAY_ITEM(tiltedBars, bai, TiltedBar, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y += scr;
    thickness = 3;
    Collision bc2;
    bar(b->pos.x, b->pos.y, b->length, b->angle, &bc2);
    if (!tiltedPlayer.hasBar && (bc2.isColliding.character['a'] || bc2.isColliding.character['b'] ||
                                 bc2.isColliding.character['c'])) {
      play(LASER);
      if (tiltedPlayer.vel.y <= 0) {
        tiltedPlayer.vel.y *= -0.5;
        tiltedPlayer.pos.y += tiltedPlayer.vel.y * 3 * difficulty;
      } else {
        tiltedPlayer.hasBar = true;
        tiltedPlayer.barIndex = bai;
      }
    }
    if (bc2.isColliding.character['d']) {
      bool hasHb = false;
      int hbIndex = -1;
      float dHb = 99;
      FOR_EACH(tiltedBalls, blj) {
        ASSIGN_ARRAY_ITEM(tiltedBalls, blj, TiltedBall, bl4);
        SKIP_IS_NOT_ALIVE(bl4);
        if (!hasHb) {
          hasHb = true;
          hbIndex = blj;
        }
        float bd = distanceTo(&bl4->pos, b->pos.x, b->pos.y);
        if (bd < dHb) {
          dHb = bd;
          hbIndex = blj;
        }
      }
      if (hasHb) {
        TiltedBall* hb = &tiltedBalls[hbIndex];
        float dd = distanceTo(&hb->pos, b->pos.x, b->pos.y);
        if (dd > b->length / 2) {
          vectorMul(&hb->vel, -1);
        } else {
          float a = vectorAngle(&hb->vel) - (vectorAngle(&hb->vel) - b->angle) * 2;
          float s = vectorLength(&hb->vel);
          vectorSet(&hb->vel, 0, 0);
          addWithAngle(&hb->vel, a, s);
        }
        vectorAdd(&hb->pos, hb->vel.x * sqrt(difficulty), hb->vel.y * sqrt(difficulty));
        hb->ticks += 30;
      }
    }
    if (b->pos.y > 120) {
      b->isAlive = false;
      continue;
    }
  }
  tiltedNextBarDist -= scr;
  while (tiltedNextBarDist < 0) {
    bool isSide = tiltedBarCount % 2;
    float length = rnd(15, 22);
    float x;
    if (isSide) {
      x = 50 + rnd(25, 50 - length / 2) * RNDPM();
    } else {
      x = rnd(35, 65);
    }
    float angleThreshold;
    if (isSide) {
      angleThreshold = 0.2;
    } else {
      angleThreshold = 0.7;
    }
    float bAngle;
    if (rnd(0, 1) < angleThreshold) {
      bAngle = 0;
    } else {
      bAngle = rnd(0, CGLP_PI / 5) * RNDPM();
    }
    ASSIGN_ARRAY_ITEM(tiltedBars, tiltedBarIndex, TiltedBar, newB);
    vectorSet(&newB->pos, x, -9 - tiltedNextBarDist);
    newB->angle = bAngle;
    newB->length = length;
    newB->isAlive = true;
    float newBY = newB->pos.y;
    tiltedBarIndex = cgl_wrap(tiltedBarIndex + 1, 0, TILTED_MAX_BAR_COUNT);
    if (isSide) {
      ASSIGN_ARRAY_ITEM(tiltedBars, tiltedBarIndex, TiltedBar, newB2);
      vectorSet(&newB2->pos, 100 - x, newBY);
      newB2->angle = -bAngle;
      newB2->length = length;
      newB2->isAlive = true;
      tiltedBarIndex = cgl_wrap(tiltedBarIndex + 1, 0, TILTED_MAX_BAR_COUNT);
    }
    tiltedNextBarDist += rnd(15, 20);
    tiltedBarCount++;
  }
}

void addGameTilted() {
  addGame(tiltedTitle, tiltedDescription, tiltedCharacters, tiltedCharactersCount,
          &tiltedOptions, false, &tiltedUpdate);
}
