#include "../cglp.h"

int* bolathreadTitle = "BOLA THREAD";
int* bolathreadDescription = "[Hold]\n Contract\n[Release]\n Expand";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bolathreadCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int bolathreadCharactersCount = 0;

Options bolathreadOptions = {100, 100, 7, false};

Vector bolathreadPlayerPos;
Vector[2] bolathreadBalls;
float bolathreadAng;
float bolathreadR;
float bolathreadNextWallX;
float bolathreadNextBeaconT;
int bolathreadHitstop;
float bolathreadShakeMag;
int bolathreadShakeT;

struct BolathreadObstacle {
  Vector pos;
  float gapY;
  float gapH;
  bool nub;
  bool passed;
  bool isAlive;
};
#define BOLATHREAD_MAX_OBSTACLE_COUNT 20
BolathreadObstacle[BOLATHREAD_MAX_OBSTACLE_COUNT] bolathreadObstacles;
int bolathreadObstacleIndex;

struct BolathreadItem {
  Vector pos;
  bool isAlive;
};
#define BOLATHREAD_MAX_ITEM_COUNT 12
BolathreadItem[BOLATHREAD_MAX_ITEM_COUNT] bolathreadItems;
int bolathreadItemIndex;

void bolathreadUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&bolathreadPlayerPos, 30, 50);
    vectorSet(&bolathreadBalls[0], 0, 0);
    vectorSet(&bolathreadBalls[1], 0, 0);
    INIT_UNALIVED_ARRAY_FAST(bolathreadObstacles);
    bolathreadObstacleIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(bolathreadItems);
    bolathreadItemIndex = 0;
    bolathreadAng = 0;
    bolathreadR = 20;
    bolathreadNextWallX = 60;
    bolathreadNextBeaconT = 100;
    bolathreadHitstop = 0;
    bolathreadShakeMag = 0;
    bolathreadShakeT = 0;
  }

  bool frozen = bolathreadHitstop > 0;
  if (frozen) {
    bolathreadHitstop--;
  }

  float spd = 0.45 + difficulty * 0.1;
  float dAngNow = 0.02 + difficulty * 0.002;
  float dr = 0;
  float dAng = 0;
  if (!frozen) {
    float prevR = bolathreadR;
    if (input.isPressed) {
      bolathreadR = fmax(6, bolathreadR - 0.6);
    } else {
      bolathreadR = fmin(26, bolathreadR + 0.45);
    }
    dr = bolathreadR - prevR;
    dAng = dAngNow;
    bolathreadAng += dAng;
  }

  bolathreadBalls[0].x = 30 + cos(bolathreadAng) * bolathreadR;
  bolathreadBalls[0].y = 50 + sin(bolathreadAng) * bolathreadR;
  bolathreadBalls[1].x = 30 - cos(bolathreadAng) * bolathreadR;
  bolathreadBalls[1].y = 50 - sin(bolathreadAng) * bolathreadR;

  if (!frozen) {
    bolathreadNextWallX -= spd;
    if (bolathreadNextWallX < 0) {
      bool nub = rnd(0, 1) < 0.36;
      float ticksToArrival = (108 - 30) / spd;
      float predictedAng = bolathreadAng + dAngNow * ticksToArrival;
      bool nubSafe = fabs(sin(predictedAng)) > 0.8;
      bool finalNub = nub && nubSafe;
      ASSIGN_ARRAY_ITEM(bolathreadObstacles, bolathreadObstacleIndex, BolathreadObstacle, no);
      vectorSet(&no->pos, 108, 50);
      no->gapY = rnd(42, 58);
      if (finalNub) {
        no->gapH = rnd(42, 54);
      } else {
        no->gapH = rnd(25, 40);
      }
      no->nub = finalNub;
      no->passed = false;
      no->isAlive = true;
      bolathreadObstacleIndex = cgl_wrap(bolathreadObstacleIndex + 1, 0, BOLATHREAD_MAX_OBSTACLE_COUNT);
      bolathreadNextWallX = rnd(65, 90);
    }
    bolathreadNextBeaconT--;
    if (bolathreadNextBeaconT < 0) {
      ASSIGN_ARRAY_ITEM(bolathreadItems, bolathreadItemIndex, BolathreadItem, ni);
      vectorSet(&ni->pos, 108, rnd(20, 80));
      ni->isAlive = true;
      bolathreadItemIndex = cgl_wrap(bolathreadItemIndex + 1, 0, BOLATHREAD_MAX_ITEM_COUNT);
      bolathreadNextBeaconT = rnd(90, 150);
    }
  }

  float shakeX = 0;
  float shakeY = 0;
  if (bolathreadShakeT > 0) {
    bolathreadShakeT--;
    float m = bolathreadShakeMag * (bolathreadShakeT / 8.0);
    shakeX = rnd(-m, m);
    shakeY = rnd(-m, m);
  }

  color = LIGHT_BLACK;
  thickness = 1;
  line(bolathreadBalls[0].x + shakeX, bolathreadBalls[0].y + shakeY,
       bolathreadBalls[1].x + shakeX, bolathreadBalls[1].y + shakeY, &scratch);
  color = LIGHT_BLUE;
  box(bolathreadPlayerPos.x + shakeX, bolathreadPlayerPos.y + shakeY, 3, 3, &scratch);

  float tvx = dr * cos(bolathreadAng) - bolathreadR * dAng * sin(bolathreadAng);
  float tvy = dr * sin(bolathreadAng) + bolathreadR * dAng * cos(bolathreadAng);
  if (tvx * tvx + tvy * tvy > 1) {
    color = LIGHT_CYAN;
    box(bolathreadBalls[0].x - tvx * 1.4, bolathreadBalls[0].y - tvy * 1.4, 3, 3, &scratch);
    box(bolathreadBalls[1].x + tvx * 1.4, bolathreadBalls[1].y + tvy * 1.4, 3, 3, &scratch);
    color = LIGHT_BLACK;
    box(bolathreadBalls[0].x - tvx * 2.8, bolathreadBalls[0].y - tvy * 2.8, 2, 2, &scratch);
    box(bolathreadBalls[1].x + tvx * 2.8, bolathreadBalls[1].y + tvy * 2.8, 2, 2, &scratch);
  }

  color = CYAN;
  box(bolathreadBalls[0].x, bolathreadBalls[0].y, 4, 4, &scratch);
  box(bolathreadBalls[1].x, bolathreadBalls[1].y, 4, 4, &scratch);

  FOR_EACH(bolathreadObstacles, oi) {
    ASSIGN_ARRAY_ITEM(bolathreadObstacles, oi, BolathreadObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    if (!frozen) {
      o->pos.x -= spd;
    }
    float x = o->pos.x;
    color = RED;
    Collision top;
    rect(x - 2, 0, 4, o->gapY - o->gapH / 2, &top);
    Collision bot;
    rect(x - 2, o->gapY + o->gapH / 2, 4, 100 - o->gapY - o->gapH / 2, &bot);
    bool hit = top.isColliding.rect[CYAN] || bot.isColliding.rect[CYAN];
    if (o->nub) {
      Collision nb;
      box(x, 50, 4, 8, &nb);
      if (nb.isColliding.rect[CYAN]) {
        hit = true;
      }
    }
    if (hit) {
      play(EXPLOSION);
      particle(x, 50, 25, 3, 0, CGLP_PI * 2);
      gameOver();
    }
    if (!o->passed && x < 30) {
      o->passed = true;
      int bonus = max((int)floor((bolathreadR - 6) / 3), 1);
      addScore(bonus, x, o->gapY);
      play(SELECT);
      int tier;
      if (bonus >= 5) {
        tier = 2;
      } else if (bonus >= 3) {
        tier = 1;
      } else {
        tier = 0;
      }
      bolathreadHitstop = max(bolathreadHitstop, (int)clamp(2 + tier * 2, 2, 6));
      bolathreadShakeMag = fmax(bolathreadShakeMag, clamp(1 + tier, 1, 3));
      bolathreadShakeT = 8;
      if (tier == 2) {
        color = YELLOW;
      } else if (tier == 1) {
        color = CYAN;
      } else {
        color = LIGHT_CYAN;
      }
      particle(x, o->gapY, 8 + tier * 6, 1.5 + tier * 0.5, 0, CGLP_PI * 2);
    }
    o->isAlive = x >= -5;
  }

  FOR_EACH(bolathreadItems, ii) {
    ASSIGN_ARRAY_ITEM(bolathreadItems, ii, BolathreadItem, g);
    SKIP_IS_NOT_ALIVE(g);
    if (!frozen) {
      g->pos.x -= spd;
    }
    color = YELLOW;
    Collision c;
    box(g->pos.x, g->pos.y, 5, 5, &c);
    if (c.isColliding.rect[CYAN]) {
      addScore(8, g->pos.x, g->pos.y);
      play(COIN);
      particle(g->pos.x, g->pos.y, 12, 2, 0, CGLP_PI * 2);
      g->isAlive = false;
      continue;
    }
    g->isAlive = g->pos.x >= -5;
  }
}

void addGameBolathread() {
  addGame(bolathreadTitle, bolathreadDescription, bolathreadCharacters,
          bolathreadCharactersCount, &bolathreadOptions, false, &bolathreadUpdate);
}
