#include "../cglp.h"

char* balltourTitle = "BALL TOUR";
char* balltourDescription = "[Hold]\n Move forward";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] balltourCharacters = {
    {
        "llllll",
        "l l ll",
        "l l ll",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "      ",
        "llllll",
        "l l ll",
        "l l ll",
        "llllll",
        "ll  ll",
    },
    {
        "  lll ",
        " ll ll",
        " l lll",
        " lllll",
        "  lll ",
        "  ll  ",
    }};
int balltourCharactersCount = 3;

Options balltourOptions = {100, 100, 4, true};

struct BalltourPlayer {
  Vector pos;
  float yAngle;
  float vx;
  float ticks;
  bool isAlive;
};
BalltourPlayer balltourPlayer;
struct Spike {
  Vector pos;
  float vy;
  bool isAlive;
};
#define BALL_TOUR_MAX_SPIKE_COUNT 32
Spike[BALL_TOUR_MAX_SPIKE_COUNT] balltourSpikes;
int balltourSpikeIndex;
float balltourNextSpikeDist;
struct Ball {
  Vector pos;
  bool isAlive;
};
#define BALL_TOUR_MAX_BALL_COUNT 16
Ball[BALL_TOUR_MAX_BALL_COUNT] balltourBalls;
int balltourBallIndex;
float balltourNextBallDist;
float balltourMultiplier;

void balltourUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&balltourPlayer.pos, 90, 50);
    balltourPlayer.yAngle = 0;
    balltourPlayer.vx = 0;
    balltourPlayer.ticks = 0;
    INIT_UNALIVED_ARRAY_FAST(balltourSpikes);
    balltourSpikeIndex = 0;
    balltourNextSpikeDist = 0;
    INIT_UNALIVED_ARRAY_FAST(balltourBalls);
    balltourBallIndex = 0;
    balltourNextBallDist = 9;
    balltourMultiplier = 1;
  }
  color = BLUE;
  rect(0, 90, 100, 9, &scratch);
  balltourNextSpikeDist -= balltourPlayer.vx;
  if (balltourNextSpikeDist < 0) {
    ASSIGN_ARRAY_ITEM(balltourSpikes, balltourSpikeIndex, Spike, s);
    vectorSet(&s->pos, -9, rnd(10, 80));
    if (rnd(0, 1) < 0.2) {
      s->vy = rnd(1, difficulty) * RNDPM() * 0.3;
    } else {
      s->vy = 0;
    }
    s->isAlive = true;
    balltourNextSpikeDist += rnd(9, 49);
    balltourSpikeIndex = cgl_wrap(balltourSpikeIndex + 1, 0, BALL_TOUR_MAX_SPIKE_COUNT);
  }
  color = BLACK;
  FOR_EACH(balltourSpikes, i) {
    ASSIGN_ARRAY_ITEM(balltourSpikes, i, Spike, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x += balltourPlayer.vx;
    s->pos.y += s->vy;
    if (s->pos.y < 10 || s->pos.y > 80) {
      s->vy *= -1;
    }
    text("*", s->pos.x, s->pos.y, &scratch);
    s->isAlive = s->pos.x <= 103;
  }
  float py = balltourPlayer.pos.y;
  balltourPlayer.yAngle += difficulty * 0.05;
  balltourPlayer.pos.y = sin(balltourPlayer.yAngle) * 30 + 50;
  balltourPlayer.ticks += clamp((py - balltourPlayer.pos.y) * 9 + 1, 0, 9);
  if (input.isJustPressed) {
    play(HIT);
  }
  if (input.isPressed) {
    balltourPlayer.vx = 1 * difficulty;
  } else {
    balltourPlayer.vx = 0.1 * difficulty;
  }
  int[2] pc;
  pc[0] = 'a' + (int)(balltourPlayer.ticks / 50) % 2;
  pc[1] = 0;
  character(pc, balltourPlayer.pos.x, balltourPlayer.pos.y, &scratch);
  color = RED;
  character("c", balltourPlayer.pos.x, balltourPlayer.pos.y - 6, &scratch);
  if (scratch.isColliding.text['*']) {
    play(EXPLOSION);
    gameOver();
  }
  color = GREEN;
  FOR_EACH(balltourBalls, i) {
    ASSIGN_ARRAY_ITEM(balltourBalls, i, Ball, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x += balltourPlayer.vx;
    character("c", b->pos.x, b->pos.y, &scratch);
    bool* c = scratch.isColliding.character;
    if (c['a'] || c['b'] || c['c']) {
      addScore((int)balltourMultiplier, balltourPlayer.pos.x, balltourPlayer.pos.y);
      balltourMultiplier += 10;
      play(SELECT);
      b->isAlive = false;
      continue;
    }
    b->isAlive = b->pos.x <= 103;
  }
  balltourNextBallDist -= balltourPlayer.vx;
  if (balltourNextBallDist < 0) {
    Vector p;
    vectorSet(&p, -3, rnd(20, 70));
    color = TRANSPARENT;
    character("c", p.x, p.y, &scratch);
    if (scratch.isColliding.text['*']) {
      balltourNextBallDist += 9;
    } else {
      ASSIGN_ARRAY_ITEM(balltourBalls, balltourBallIndex, Ball, b);
      b->pos = p;
      b->isAlive = true;
      balltourNextBallDist += rnd(25, 64);
      balltourBallIndex = cgl_wrap(balltourBallIndex + 1, 0, BALL_TOUR_MAX_BALL_COUNT);
    }
  }
  balltourMultiplier = clamp(balltourMultiplier - 0.02 * difficulty, 1, 999);
  color = BLACK;
  text("x", 3, 9, &scratch);
  text(intToChar((int)balltourMultiplier), 9, 9, &scratch);
}

void addGameBallTour() {
  addGame(balltourTitle, balltourDescription, balltourCharacters,
          balltourCharactersCount, &balltourOptions, false, &balltourUpdate);
}
