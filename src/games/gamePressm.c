#include "../cglp.h"

int* pressmTitle = "PRESS M";
int* pressmDescription = "[Slide] Move";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pressmCharacters = {
    {
        "llllll",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "llllll",
        "ll  ll",
    },
};
int pressmCharactersCount = 2;

Options pressmOptions = {100, 100, 7, false};

#define PRESSM_MODE_PRESS 0
#define PRESSM_MODE_RETURN 1

struct PressmWall {
  Vector pos;
  float sy;
  float ey;
  float ney;
};
#define PRESSM_WALL_COUNT 20
PressmWall[PRESSM_WALL_COUNT] pressmWalls;

int pressmWallMode;
float pressmWallTicks;
float pressmWallModeInterval;
int pressmHitWallIndex;
float pressmPx;
int pressmPmx;

struct PressmCoin {
  Vector pos;
  int wallIndex;
  float wallOy;
  bool isAlive;
};
#define PRESSM_MAX_COIN_COUNT 20
PressmCoin[PRESSM_MAX_COIN_COUNT] pressmCoins;
int pressmCoinCount;

struct PressmInhalingCoin {
  Vector pos;
  float speed;
  bool isAlive;
};
#define PRESSM_MAX_INHALING_COUNT 64
PressmInhalingCoin[PRESSM_MAX_INHALING_COUNT] pressmInhalingCoins;
int pressmInhalingCoinIndex;

int pressmMultiplier;

void pressmUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(PRESSM_WALL_COUNT, wi) {
      int xi = wi % 10;
      int yi = wi / 10;
      float ey;
      if (yi == 0) {
        ey = 40;
      } else {
        ey = 60;
      }
      vectorSet(&pressmWalls[wi].pos, xi * 10, ey);
      pressmWalls[wi].sy = 0;
      pressmWalls[wi].ey = ey;
      pressmWalls[wi].ney = 0;
    }
    pressmWallTicks = 0;
    pressmWallMode = PRESSM_MODE_PRESS;
    pressmPx = 50;
    pressmPmx = 1;
    pressmHitWallIndex = -1;
    INIT_UNALIVED_ARRAY_FAST(pressmInhalingCoins);
    pressmInhalingCoinIndex = 0;
    pressmMultiplier = 1;
  }
  if (pressmWallMode == PRESSM_MODE_PRESS && pressmWallTicks < 5) {
    color = RED;
  } else {
    color = PURPLE;
  }
  if (pressmWallTicks == 0) {
    if (pressmWallMode == PRESSM_MODE_PRESS) {
      if (pressmMultiplier > 1) {
        pressmMultiplier--;
      }
      if (pressmHitWallIndex >= 0) {
        play(EXPLOSION);
        particle(pressmWalls[pressmHitWallIndex].pos.x, pressmWalls[pressmHitWallIndex].pos.y, 30,
                 2, 0, CGLP_PI * 2);
      }
      int wi = 0;
      TIMES(2, yi) {
        float a = rnd(0, CGLP_PI * 2);
        float av = rnd(0.4, 1) * RNDPM();
        float r = rnd(10, 20);
        float cy;
        if (yi == 0) {
          cy = rnd(15, 25);
        } else {
          cy = rnd(75, 85);
        }
        TIMES(10, xi) {
          PressmWall* w = &pressmWalls[wi];
          w->sy = clamp(sin(a) * r + cy, 2, 97);
          av += rnd(0, 0.1) * RNDPM();
          r += rnd(0, 1) * RNDPM();
          a += av;
          cy += rnd(0, 1) * RNDPM();
          wi++;
        }
      }
      float mw = 99;
      TIMES(10, xi2) {
        float wdiff = pressmWalls[xi2 + 10].sy - pressmWalls[xi2].sy;
        if (wdiff < mw) {
          mw = wdiff;
          pressmHitWallIndex = xi2;
        }
      }
      mw /= 2;
      TIMES(PRESSM_WALL_COUNT, wi3) {
        if (wi3 < 10) {
          pressmWalls[wi3].ney = pressmWalls[wi3].sy + mw;
        } else {
          pressmWalls[wi3].ney = pressmWalls[wi3].sy - mw;
        }
      }
      bool hasSpace = false;
      TIMES(10, xi3) {
        PressmWall* w1 = &pressmWalls[xi3];
        PressmWall* w2 = &pressmWalls[xi3 + 10];
        if (w1->ney < 40 && w2->ney > 60) {
          hasSpace = true;
        }
      }
      if (!hasSpace) {
        TIMES(10, xi4) {
          if (xi4 != pressmHitWallIndex) {
            PressmWall* w1 = &pressmWalls[xi4];
            PressmWall* w2 = &pressmWalls[xi4 + 10];
            if (w1->ney > 40) {
              float wamt = w1->ney - 40;
              w1->ney -= wamt;
              w1->sy -= wamt;
            }
            if (w2->ney < 60) {
              float wamt2 = 60 - w2->ney;
              w2->ney += wamt2;
              w2->sy += wamt2;
            }
          }
        }
      }
      INIT_UNALIVED_ARRAY_FAST(pressmCoins);
      pressmCoinCount = 0;
      TIMES(PRESSM_WALL_COUNT, wi4b) {
        if (rnd(0, 1) < 0.2) {
          PressmCoin* c = &pressmCoins[pressmCoinCount];
          c->wallIndex = wi4b;
          if (wi4b < 10) {
            c->wallOy = 4;
          } else {
            c->wallOy = -4;
          }
          c->isAlive = true;
          pressmCoinCount++;
        }
      }
      pressmWallMode = PRESSM_MODE_RETURN;
      pressmWallModeInterval = ceil(60 / sqrt(difficulty));
      pressmWallTicks = pressmWallModeInterval;
    } else {
      TIMES(PRESSM_WALL_COUNT, wi5) { pressmWalls[wi5].ey = pressmWalls[wi5].ney; }
      pressmWallMode = PRESSM_MODE_PRESS;
      pressmWallModeInterval = ceil(20 / sqrt(difficulty));
      pressmWallTicks = pressmWallModeInterval;
    }
  }
  pressmWallTicks--;
  TIMES(PRESSM_WALL_COUNT, wi6) {
    PressmWall* w = &pressmWalls[wi6];
    float factor;
    if (pressmWallMode == PRESSM_MODE_PRESS) {
      if (pressmWallTicks < 5) {
        factor = 1 - pressmWallTicks / 5.0;
      } else {
        factor = (1 - pressmWallTicks / pressmWallModeInterval) * 0.2;
      }
      w->pos.y = w->sy + (w->ey - w->sy) * factor;
    } else {
      w->pos.y = w->ey + (w->sy - w->ey) * (1 - (pressmWallTicks + 1) / pressmWallModeInterval);
    }
    if (wi6 < 10) {
      rect(w->pos.x, 0, 9, w->pos.y, &scratch);
    } else {
      rect(w->pos.x, w->pos.y, 9, 99 - w->pos.y, &scratch);
    }
  }
  Vector p;
  vectorSet(&p, clamp(input.pos.x, 3, 96), 50);
  if (p.x < pressmPx - 2) {
    pressmPmx = -1;
  } else if (p.x > pressmPx + 2) {
    pressmPmx = 1;
  }
  pressmPx = p.x;
  color = BLACK;
  int[2] pc;
  pc[0] = 'a' + (ticks % 60) / 30;
  pc[1] = 0;
  characterOptions.isMirrorX = pressmPmx < 0;
  Collision pcoll;
  character(pc, p.x, p.y, &pcoll);
  characterOptions.isMirrorX = false;
  if (pcoll.isColliding.rect[RED]) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  if (pressmWallMode == PRESSM_MODE_PRESS) {
    color = YELLOW;
  } else {
    color = LIGHT_YELLOW;
  }
  TIMES(pressmCoinCount, ci) {
    PressmCoin* c = &pressmCoins[ci];
    SKIP_IS_NOT_ALIVE(c);
    vectorSet(&c->pos, pressmWalls[c->wallIndex].pos.x + 5,
              pressmWalls[c->wallIndex].pos.y + c->wallOy);
    if (pressmWallMode == PRESSM_MODE_PRESS && distanceTo(&c->pos, p.x, p.y) < 30) {
      ASSIGN_ARRAY_ITEM(pressmInhalingCoins, pressmInhalingCoinIndex, PressmInhalingCoin, ic);
      ic->pos = c->pos;
      ic->speed = rnd(0.1, 0.3);
      ic->isAlive = true;
      pressmInhalingCoinIndex = cgl_wrap(pressmInhalingCoinIndex + 1, 0, PRESSM_MAX_INHALING_COUNT);
      c->isAlive = false;
      continue;
    }
    text("o", c->pos.x, c->pos.y, &scratch);
  }
  color = YELLOW;
  FOR_EACH(pressmInhalingCoins, ii) {
    ASSIGN_ARRAY_ITEM(pressmInhalingCoins, ii, PressmInhalingCoin, ic2);
    SKIP_IS_NOT_ALIVE(ic2);
    ic2->pos.x += (p.x - ic2->pos.x) * ic2->speed;
    ic2->pos.y += (p.y - ic2->pos.y) * ic2->speed;
    Collision cc;
    text("o", ic2->pos.x, ic2->pos.y, &cc);
    if (cc.isColliding.character['a'] || cc.isColliding.character['b']) {
      play(COIN);
      addScore(pressmMultiplier, ic2->pos.x, ic2->pos.y);
      pressmMultiplier++;
      ic2->isAlive = false;
      continue;
    }
  }
}

void addGamePressm() {
  addGame(pressmTitle, pressmDescription, pressmCharacters, pressmCharactersCount,
          &pressmOptions, true, &pressmUpdate);
}
