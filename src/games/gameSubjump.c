#include "../cglp.h"

int* subjumpTitle = "SUB JUMP";
int* subjumpDescription = "[Hold]\n Go up & \n Speed up";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] subjumpCharacters = {
    {
        "   ll ",
        "   l  ",
        "  lll ",
        "l l l ",
        "llllll",
        "l lll ",
    },
    {
        "   ll ",
        "   l  ",
        "  lll ",
        "  l l ",
        "llllll",
        "  lll ",
    },
    {
        " llll ",
        "ll lll",
        "ll lll",
        "ll lll",
        "ll lll",
        " llll ",
    },
};
int subjumpCharactersCount = 3;

Options subjumpOptions = {100, 100, 4, true};

#define SUBJUMP_LANDFORM_SEA 0
#define SUBJUMP_LANDFORM_LAND 1

Vector[12] subjumpPoints;
int subjumpLandForm;
float subjumpNextLandFormDist;
float subjumpLvy;

struct SubjumpSub {
  Vector pos;
  Vector vel;
};
SubjumpSub subjumpSub;

struct SubjumpCoin {
  Vector pos;
  bool isAlive;
};
#define SUBJUMP_MAX_COIN_COUNT 64
SubjumpCoin[SUBJUMP_MAX_COIN_COUNT] subjumpCoins;
int subjumpCoinIndex;
float subjumpNextCoinDist;
int subjumpMultiplier;

void subjumpUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(12, ptIdx) { vectorSet(&subjumpPoints[ptIdx], ptIdx * 10 - 10, rnd(75, 85)); }
    subjumpLandForm = SUBJUMP_LANDFORM_SEA;
    subjumpNextLandFormDist = 50;
    subjumpLvy = 0;
    vectorSet(&subjumpSub.pos, 5, 60);
    vectorSet(&subjumpSub.vel, 0, 0);
    INIT_UNALIVED_ARRAY_FAST(subjumpCoins);
    subjumpCoinIndex = 0;
    subjumpNextCoinDist = 0;
    subjumpMultiplier = 1;
  }
  float sd = sqrt(difficulty);
  float scr = 0;
  if (subjumpSub.pos.x > 10) {
    scr += (subjumpSub.pos.x - 10) * 0.2;
  }
  subjumpNextLandFormDist -= scr;
  int gpIndex = 11;
  TIMES(12, pi2) {
    Vector* p = &subjumpPoints[pi2];
    p->x -= scr;
    int ppIndex = (int)cgl_wrap(pi2 - 1, 0, 12);
    Vector* pp = &subjumpPoints[ppIndex];
    if (p->x < -10) {
      if (subjumpNextLandFormDist < 0) {
        if (subjumpLandForm == SUBJUMP_LANDFORM_LAND) {
          subjumpLandForm = SUBJUMP_LANDFORM_SEA;
        } else {
          subjumpLandForm = SUBJUMP_LANDFORM_LAND;
        }
        float divisor;
        if (subjumpLandForm == SUBJUMP_LANDFORM_LAND) {
          divisor = 7 / sqrt(difficulty);
        } else {
          divisor = 1;
        }
        subjumpNextLandFormDist = rnd(200, 300) / divisor;
      }
      p->x += 120;
      subjumpLvy += rnd(0, sd) * RNDPM() * 2;
      subjumpLvy *= 0.95;
      if (subjumpLandForm == SUBJUMP_LANDFORM_SEA) {
        if (pp->y < 55) {
          subjumpLvy += 5;
        } else if (pp->y < 65) {
          subjumpLvy += 3;
        } else if ((pp->y < 65 && subjumpLvy < 0) || (pp->y > 90 && subjumpLvy > 0)) {
          subjumpLvy *= -0.5;
        }
        p->y = pp->y + subjumpLvy;
        if (subjumpNextLandFormDist < 60) {
          subjumpLvy += 4;
        }
      } else {
        if (pp->y > 50) {
          subjumpLvy -= 5;
        } else if ((pp->y < 40 && subjumpLvy < 0) || (pp->y > 45 && subjumpLvy > 0)) {
          subjumpLvy *= -0.5;
        }
        p->y = pp->y + subjumpLvy / 3;
      }
      p->y = clamp(p->y, 35, 95) + rnd(0, 5) * RNDPM();
    }
    if (pp->x < p->x) {
      if (pp->y < 50 || p->y < 50) {
        color = GREEN;
      } else {
        color = PURPLE;
      }
      thickness = 2;
      line(pp->x, pp->y, p->x, p->y, &scratch);
    } else {
      gpIndex = ppIndex;
    }
  }
  color = BLUE;
  rect(0, 50, 100, 2, &scratch);
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    if (subjumpSub.pos.y > 50) {
      subjumpSub.vel.y -= sd * 0.06;
      subjumpSub.vel.x += (1 * sd - subjumpSub.vel.x) * 0.1;
    } else {
      subjumpSub.vel.y += sd * 0.01;
      subjumpSub.vel.x += (1 * sd - subjumpSub.vel.x) * 0.1;
    }
  } else {
    if (subjumpSub.pos.y > 50) {
      subjumpSub.vel.y += sd * 0.03;
      subjumpSub.vel.x += (0.5 * sd - subjumpSub.vel.x) * 0.1;
    } else {
      subjumpSub.vel.y += sd * 0.05;
      subjumpSub.vel.x += (0.5 * sd - subjumpSub.vel.x) * 0.1;
    }
  }
  float py = subjumpSub.pos.y;
  float velMul;
  if (subjumpSub.pos.y > 50) {
    velMul = 0.95;
  } else {
    velMul = 0.99;
  }
  vectorMul(&subjumpSub.vel, velMul);
  vectorAdd(&subjumpSub.pos, subjumpSub.vel.x, subjumpSub.vel.y);
  subjumpSub.pos.x -= scr;
  color = BLUE;
  if (py > 50 && subjumpSub.pos.y < 50) {
    play(JUMP);
    particle(subjumpSub.pos.x, 50, 9, 1, -CGLP_PI / 2, CGLP_PI);
  }
  if (py < 50 && subjumpSub.pos.y > 50) {
    play(HIT);
    particle(subjumpSub.pos.x, 50, 9, 0.5, -CGLP_PI / 2, CGLP_PI);
  }
  color = BLACK;
  int[2] sc;
  if (subjumpSub.pos.y < 50) {
    sc[0] = 'a';
  } else {
    sc[0] = 'a' + (ticks / 7) % 2;
  }
  sc[1] = 0;
  Collision scoll;
  character(sc, subjumpSub.pos.x, subjumpSub.pos.y, &scoll);
  if (scoll.isColliding.rect[PURPLE] || scoll.isColliding.rect[GREEN]) {
    play(EXPLOSION);
    gameOver();
  }
  if (subjumpSub.pos.y > 55) {
    color = BLUE;
    particle(subjumpSub.pos.x - 3, subjumpSub.pos.y + 1, 0.3, 1, CGLP_PI, 0.1);
  }
  subjumpNextCoinDist -= scr;
  if (subjumpNextCoinDist < 0) {
    ASSIGN_ARRAY_ITEM(subjumpCoins, subjumpCoinIndex, SubjumpCoin, nc);
    float gpY = subjumpPoints[gpIndex].y;
    float coinYOfs;
    if (gpY < 50) {
      coinYOfs = rnd(5, 10);
    } else {
      coinYOfs = rnd(10, 20);
    }
    vectorSet(&nc->pos, subjumpPoints[gpIndex].x, gpY - coinYOfs);
    nc->isAlive = true;
    subjumpCoinIndex = cgl_wrap(subjumpCoinIndex + 1, 0, SUBJUMP_MAX_COIN_COUNT);
    subjumpNextCoinDist = rnd(10, 40);
  }
  color = YELLOW;
  FOR_EACH(subjumpCoins, ci) {
    ASSIGN_ARRAY_ITEM(subjumpCoins, ci, SubjumpCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= scr;
    Collision cc;
    character("c", c->pos.x, c->pos.y, &cc);
    if (cc.isColliding.character['a'] || cc.isColliding.character['b']) {
      play(COIN);
      addScore(subjumpMultiplier, c->pos.x, c->pos.y);
      subjumpMultiplier++;
      c->isAlive = false;
      continue;
    }
    if (c->pos.x < -3) {
      if (subjumpMultiplier > 1) {
        subjumpMultiplier--;
      }
      c->isAlive = false;
      continue;
    }
  }
}

void addGameSubjump() {
  addGame(subjumpTitle, subjumpDescription, subjumpCharacters, subjumpCharactersCount,
          &subjumpOptions, false, &subjumpUpdate);
}
