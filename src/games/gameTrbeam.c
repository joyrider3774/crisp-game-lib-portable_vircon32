#include "../cglp.h"

int* trbeamTitle = "TR BEAM";
int* trbeamDescription = "[Hold]\n Tractor beam";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] trbeamCharacters = {
    {
        "y cc y",
        " cccc ",
        "llllll",
        "lllll ",
        " l l  ",
    },
    {
        "y cc y",
        " cccc ",
        "llllll",
        " lllll",
        "  l l ",
    },
};
int trbeamCharactersCount = 2;

Options trbeamOptions = {100, 100, 4, true};

struct TrbeamUfo {
  Vector pos;
  float angle;
  float trLength;
};
TrbeamUfo trbeamUfo;

struct TrbeamBall {
  Vector pos;
  Vector vel;
  float radius;
  bool isRed;
  bool isBeamed;
  bool isAlive;
};
#define TRBEAM_MAX_BALL_COUNT 64
TrbeamBall[TRBEAM_MAX_BALL_COUNT] trbeamBalls;
int trbeamBallIndex;
float trbeamNextBallTicks;
int trbeamNextRedBallCount;

int trbeamMultiplier;

void trbeamUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&trbeamUfo.pos, 0, 9);
    trbeamUfo.angle = -CGLP_PI * 0.3;
    trbeamUfo.trLength = 0;
    INIT_UNALIVED_ARRAY_FAST(trbeamBalls);
    trbeamBallIndex = 0;
    trbeamNextBallTicks = 0;
    trbeamNextRedBallCount = 9;
    trbeamMultiplier = 1;
  }
  trbeamUfo.angle += sqrt(difficulty) * 0.03;
  trbeamUfo.pos.x = sin(trbeamUfo.angle) * 40 + 50;
  if (input.isJustPressed) {
    play(SELECT);
    trbeamMultiplier = 1;
  }
  if (input.isPressed) {
    play(LASER);
    trbeamUfo.trLength = clamp(trbeamUfo.trLength + difficulty * 2, 0, 82);
  } else {
    trbeamUfo.trLength *= 1 - clamp(sqrt(difficulty), 1, 3) * 0.2;
  }
  float ta = ((float)(ticks % 10) / 10) * (CGLP_PI / 4);
  if (trbeamUfo.trLength > 1) {
    color = BLUE;
    thickness = 2;
    TIMES(4, li) {
      line(4 * cos(ta) + trbeamUfo.pos.x, trbeamUfo.pos.y + 5, 9 * cos(ta) + trbeamUfo.pos.x,
           trbeamUfo.pos.y + 5 + trbeamUfo.trLength, &scratch);
      ta += CGLP_PI / 4;
    }
  }
  color = BLACK;
  int[2] uc;
  uc[0] = 'a' + (ticks / 30) % 2;
  uc[1] = 0;
  character(uc, trbeamUfo.pos.x, trbeamUfo.pos.y, &scratch);
  trbeamNextBallTicks--;
  if (trbeamNextBallTicks < 0) {
    trbeamNextRedBallCount--;
    bool isRed = false;
    if (trbeamNextRedBallCount < 0) {
      isRed = true;
      trbeamNextRedBallCount = rndi(9, 12);
    }
    float radius = rnd(6, 12);
    ASSIGN_ARRAY_ITEM(trbeamBalls, trbeamBallIndex, TrbeamBall, nb);
    vectorSet(&nb->pos, rnd(10, 90), 105 + radius);
    vectorSet(&nb->vel, 0, 0);
    nb->radius = radius;
    nb->isRed = isRed;
    nb->isBeamed = false;
    nb->isAlive = true;
    trbeamBallIndex = cgl_wrap(trbeamBallIndex + 1, 0, TRBEAM_MAX_BALL_COUNT);
    trbeamNextBallTicks = rnd(20, 25) / sqrt(difficulty);
  }
  FOR_EACH(trbeamBalls, bi) {
    ASSIGN_ARRAY_ITEM(trbeamBalls, bi, TrbeamBall, b);
    SKIP_IS_NOT_ALIVE(b);
    if (b->pos.y > 99) {
      b->vel.y -= sqrt(difficulty);
      vectorMul(&b->vel, 0.5);
    } else if (b->pos.x < 0 || b->pos.x > 99) {
      vectorMul(&b->vel, 0.1);
    } else {
      vectorMul(&b->vel, 0.9);
      if (b->isRed) {
        b->vel.y *= 0.9;
      }
    }
    b->vel.y += sqrt(difficulty) * 0.1;
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    FOR_EACH(trbeamBalls, abi) {
      if (abi == bi) {
        continue;
      }
      ASSIGN_ARRAY_ITEM(trbeamBalls, abi, TrbeamBall, ab);
      SKIP_IS_NOT_ALIVE(ab);
      float d = distanceTo(&ab->pos, b->pos.x, b->pos.y) - ab->radius - b->radius;
      if (d < 0) {
        addWithAngle(&b->vel, angleTo(&ab->pos, b->pos.x, b->pos.y), -d / sqrt(b->radius));
      }
    }
    if (b->isRed) {
      color = RED;
    } else {
      color = BLACK;
    }
    thickness = 3;
    Collision c;
    arc(b->pos.x, b->pos.y, b->radius, 0, CGLP_PI * 2, &c);
    if (c.isColliding.character['a'] || c.isColliding.character['b']) {
      if (b->isRed) {
        play(EXPLOSION);
        gameOver();
      } else {
        play(COIN);
        addScore(trbeamMultiplier, b->pos.x, b->pos.y);
        trbeamMultiplier++;
      }
      b->isAlive = false;
      continue;
    } else if (c.isColliding.rect[BLUE]) {
      addWithAngle(&b->vel, angleTo(&b->pos, trbeamUfo.pos.x, trbeamUfo.pos.y),
                   sqrt(difficulty) / sqrt(b->radius));
      b->vel.x += (trbeamUfo.pos.x - b->pos.x) * clamp(sqrt(difficulty), 1, 5) * 0.01;
      b->isBeamed = true;
      if (b->isRed) {
        play(HIT);
      }
    } else if (b->isBeamed) {
      b->vel.y *= 0.1;
      b->vel.x *= 5;
      b->isBeamed = false;
    }
    bool inStripe = b->pos.x >= 10 && b->pos.x < 90 && b->pos.y >= 12 + b->radius &&
                    b->pos.y < 17 + b->radius;
    if (b->isRed && !b->isBeamed && (b->pos.x < b->radius || b->pos.x > 99 - b->radius)) {
      play(POWER_UP);
      b->isRed = false;
    } else if (!b->isBeamed && inStripe && b->vel.y > 0) {
      b->isRed = true;
    }
    if (b->pos.x < -20 || b->pos.x > 120) {
      b->isAlive = false;
      continue;
    }
  }
  if (trbeamUfo.trLength > 1) {
    color = CYAN;
    thickness = 3;
    TIMES(4, li2) {
      line(4 * cos(ta) + trbeamUfo.pos.x, trbeamUfo.pos.y + 5, 9 * cos(ta) + trbeamUfo.pos.x,
           trbeamUfo.pos.y + 5 + trbeamUfo.trLength, &scratch);
      ta += CGLP_PI / 4;
    }
  }
}

void addGameTrbeam() {
  addGame(trbeamTitle, trbeamDescription, trbeamCharacters, trbeamCharactersCount,
          &trbeamOptions, false, &trbeamUpdate);
}
