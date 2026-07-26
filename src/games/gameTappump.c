#include "../cglp.h"

int* tappumpTitle = "TAPPUMP";
int* tappumpDescription = "[Tap]\n Jump\n[Hold]\n Pump";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tappumpCharacters = {
    {
        "r r r ",
        " r r  ",
        "rrRrr ",
        " r r  ",
        "r r r ",
    },
    {
        " yyyy ",
        "y yyYy",
        "y yyYy",
        "y yyYy",
        "y yyYy",
        " yyyy ",
    },
};
int tappumpCharactersCount = 2;

Options tappumpOptions = {100, 100, 7, false};

struct TappumpPlayer {
  Vector pos;
  Vector vel;
  float radius;
  float rv;
};
TappumpPlayer tappumpPlayer;

struct TappumpSpike {
  Vector pos;
  bool isAlive;
};
#define TAPPUMP_MAX_SPIKE_COUNT 32
TappumpSpike[TAPPUMP_MAX_SPIKE_COUNT] tappumpSpikes;
int tappumpSpikeIndex;
float tappumpNextSpikeDist;

struct TappumpCoin {
  Vector pos;
  bool isAlive;
};
#define TAPPUMP_MAX_COIN_COUNT 32
TappumpCoin[TAPPUMP_MAX_COIN_COUNT] tappumpCoins;
int tappumpCoinIndex;
float tappumpCoinY;
float tappumpCoinVy;
float tappumpNextCoinDist;

void tappumpUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&tappumpPlayer.pos, 10, 20);
    vectorSet(&tappumpPlayer.vel, 0, 0);
    tappumpPlayer.radius = 1;
    tappumpPlayer.rv = 0;
    INIT_UNALIVED_ARRAY_FAST(tappumpSpikes);
    tappumpSpikeIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(tappumpCoins);
    tappumpCoinIndex = 0;
    tappumpCoinY = 50;
    tappumpCoinVy = 0;
    tappumpNextSpikeDist = 40;
    tappumpNextCoinDist = 0;
  }
  float scr;
  if (tappumpPlayer.pos.x > 20) {
    scr = (tappumpPlayer.pos.x - 20) * 0.2;
  } else {
    scr = 0;
  }
  tappumpNextSpikeDist -= scr;
  if (tappumpNextSpikeDist < 0) {
    play(LASER);
    ASSIGN_ARRAY_ITEM(tappumpSpikes, tappumpSpikeIndex, TappumpSpike, sp);
    vectorSet(&sp->pos, 103, rnd(0, 99));
    sp->isAlive = true;
    tappumpSpikeIndex = cgl_wrap(tappumpSpikeIndex + 1, 0, TAPPUMP_MAX_SPIKE_COUNT);
    tappumpNextSpikeDist += rnd(40, 140);
  }
  color = BLACK;
  FOR_EACH(tappumpSpikes, i) {
    ASSIGN_ARRAY_ITEM(tappumpSpikes, i, TappumpSpike, sp);
    SKIP_IS_NOT_ALIVE(sp);
    sp->pos.x -= scr;
    character("a", sp->pos.x, sp->pos.y, &scratch);
    sp->isAlive = sp->pos.x >= -2;
  }
  color = GREEN;
  tappumpPlayer.vel.x = difficulty;
  if (input.isJustPressed) {
    play(SELECT);
    tappumpPlayer.vel.y -= sqrt(difficulty) * 2;
  }
  if (input.isPressed) {
    tappumpPlayer.vel.y -= sqrt(difficulty) * 0.03;
  } else {
    tappumpPlayer.vel.y -= sqrt(difficulty) * -0.12;
  }
  if (input.isPressed) {
    tappumpPlayer.rv += difficulty * 0.08;
    tappumpPlayer.radius += tappumpPlayer.rv;
  } else {
    tappumpPlayer.radius += (1 - tappumpPlayer.radius) * 0.04 * difficulty;
    tappumpPlayer.rv = 0;
  }
  vectorAdd(&tappumpPlayer.pos, tappumpPlayer.vel.x, tappumpPlayer.vel.y);
  tappumpPlayer.pos.x -= scr;
  thickness = 5;
  Collision ac;
  arc(tappumpPlayer.pos.x, tappumpPlayer.pos.y, tappumpPlayer.radius, 0,
      CGLP_PI * 2, &ac);
  if (ac.isColliding.character['a'] ||
      tappumpPlayer.pos.y < -5 - tappumpPlayer.radius ||
      tappumpPlayer.pos.y > 105 + tappumpPlayer.radius) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLACK;
  tappumpNextCoinDist -= scr;
  tappumpCoinVy += rnd(0, 0.1) * RNDPM();
  tappumpCoinVy *= 0.98;
  tappumpCoinY += tappumpCoinVy;
  if ((tappumpCoinY < 10 && tappumpCoinVy < 0) ||
      (tappumpCoinY > 90 && tappumpCoinVy > 0)) {
    tappumpCoinVy *= -1;
  }
  if (tappumpNextCoinDist < 0) {
    ASSIGN_ARRAY_ITEM(tappumpCoins, tappumpCoinIndex, TappumpCoin, co);
    vectorSet(&co->pos, 103, tappumpCoinY + rnd(0, 9) * RNDPM());
    co->isAlive = true;
    tappumpCoinIndex = cgl_wrap(tappumpCoinIndex + 1, 0, TAPPUMP_MAX_COIN_COUNT);
    tappumpNextCoinDist += rnd(6, 9);
  }
  FOR_EACH(tappumpCoins, i) {
    ASSIGN_ARRAY_ITEM(tappumpCoins, i, TappumpCoin, co);
    SKIP_IS_NOT_ALIVE(co);
    co->pos.x -= scr;
    Collision cl;
    character("b", co->pos.x, co->pos.y, &cl);
    if (cl.isColliding.rect[GREEN]) {
      int sc = ceil(tappumpPlayer.radius);
      if (sc < 20) {
        play(COIN);
      } else {
        play(POWER_UP);
      }
      addScore(sc, co->pos.x, co->pos.y);
      co->isAlive = false;
      continue;
    }
    co->isAlive = !(cl.isColliding.character['a'] || co->pos.x < -3);
  }
}

void addGameTappump() {
  addGame(tappumpTitle, tappumpDescription, tappumpCharacters,
          tappumpCharactersCount, &tappumpOptions, false, &tappumpUpdate);
}
