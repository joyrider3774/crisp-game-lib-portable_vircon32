#include "../cglp.h"

int* futurewakeTitle = "FUTURE WAKE";
int* futurewakeDescription = "[Slide]\n Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] futurewakeCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int futurewakeCharactersCount = 1;

Options futurewakeOptions = {100, 100, 7, true};

#define FUTUREWAKE_BALL_SIZE 3

struct FuturewakeBallRoot {
  Vector pos;
  Vector vel;
  float speed;
};
FuturewakeBallRoot futurewakeBallRoot;

struct FuturewakeBallWake {
  Vector pos;
  Vector vel;
};
FuturewakeBallWake futurewakeBallWake;

float futurewakeBallDist;
int futurewakeWakeColor;
float futurewakeWakeDist;
int futurewakeWakeCount;
Vector futurewakeBallPos;
bool futurewakeIsHittingRacket;
float futurewakeMultiplier;
float futurewakeRx;

void futurewakeUpdateBall() {
  futurewakeWakeDist += FUTUREWAKE_BALL_SIZE;
  if (futurewakeWakeColor == LIGHT_BLACK && futurewakeWakeDist >= futurewakeBallDist) {
    float r = (futurewakeWakeDist - futurewakeBallDist) / FUTUREWAKE_BALL_SIZE;
    vectorSet(&futurewakeBallPos,
              futurewakeBallWake.pos.x + futurewakeBallWake.vel.x * (1 - r),
              futurewakeBallWake.pos.y + futurewakeBallWake.vel.y * (1 - r));
    futurewakeWakeColor = LIGHT_CYAN;
  }
  if (futurewakeWakeColor == LIGHT_CYAN &&
      (futurewakeBallWake.pos.y < 7 || futurewakeBallWake.pos.y > 93)) {
    futurewakeWakeColor = LIGHT_RED;
    futurewakeWakeCount = 5;
  }
  vectorAdd(&futurewakeBallWake.pos, futurewakeBallWake.vel.x, futurewakeBallWake.vel.y);
  color = futurewakeWakeColor;
  Collision c;
  box(futurewakeBallWake.pos.x, futurewakeBallWake.pos.y, FUTUREWAKE_BALL_SIZE,
      FUTUREWAKE_BALL_SIZE, &c);
  if (c.isColliding.rect[LIGHT_BLUE]) {
    if ((futurewakeBallWake.pos.x < 50 && futurewakeBallWake.vel.x < 0) ||
        (futurewakeBallWake.pos.x > 50 && futurewakeBallWake.vel.x > 0)) {
      futurewakeBallWake.vel.x *= -1;
    }
  }
  if (c.isColliding.rect[BLUE] &&
      ((futurewakeBallWake.pos.y < 50 && futurewakeBallWake.vel.y < 0) ||
       (futurewakeBallWake.pos.y > 50 && futurewakeBallWake.vel.y > 0))) {
    float d = (futurewakeBallWake.pos.x - futurewakeRx) / 10;
    if (fabs(d) < 0.6) {
      futurewakeBallWake.vel.y *= -1;
    } else {
      float a;
      if (d > 0) {
        a = CGLP_PI / 4 + (1 - d);
      } else {
        a = (CGLP_PI / 4) * 3 - (1 + d);
      }
      if (futurewakeBallWake.pos.y > 50) {
        a *= -1;
      }
      vectorSet(&futurewakeBallWake.vel, 0, 0);
      addWithAngle(&futurewakeBallWake.vel, a, FUTUREWAKE_BALL_SIZE);
    }
    if (futurewakeWakeDist < futurewakeBallDist) {
      play(HIT);
      futurewakeBallRoot.pos = futurewakeBallWake.pos;
      futurewakeBallRoot.vel = futurewakeBallWake.vel;
      futurewakeBallPos = futurewakeBallWake.pos;
      futurewakeIsHittingRacket = true;
    }
    futurewakeWakeColor = LIGHT_GREEN;
    futurewakeWakeCount = 9;
  }
}

void futurewakeUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&futurewakeBallRoot.pos, 50, 50);
    vectorSet(&futurewakeBallRoot.vel, 0, 0);
    addWithAngle(&futurewakeBallRoot.vel, CGLP_PI / 4, FUTUREWAKE_BALL_SIZE);
    futurewakeBallRoot.speed = 1;
    vectorSet(&futurewakeBallWake.pos, 0, 0);
    vectorSet(&futurewakeBallWake.vel, 0, 0);
    futurewakeWakeColor = LIGHT_BLACK;
    futurewakeBallDist = 0;
    futurewakeWakeDist = 0;
    futurewakeWakeCount = 0;
    vectorSet(&futurewakeBallPos, 0, 0);
    futurewakeMultiplier = 1;
    futurewakeIsHittingRacket = false;
  }
  color = LIGHT_BLUE;
  rect(0, 0, 7, 100, &scratch);
  rect(93, 0, 7, 100, &scratch);
  futurewakeRx = clamp(input.pos.x, 17, 83);
  color = BLUE;
  box(futurewakeRx, 5, 20, 5, &scratch);
  box(futurewakeRx, 95, 20, 5, &scratch);
  futurewakeWakeColor = LIGHT_BLACK;
  futurewakeWakeDist = 0;
  futurewakeWakeCount = 99;
  futurewakeBallDist += futurewakeBallRoot.speed;
  futurewakeBallWake.pos = futurewakeBallRoot.pos;
  futurewakeBallWake.vel = futurewakeBallRoot.vel;
  futurewakeIsHittingRacket = false;
  while (futurewakeWakeCount > 0) {
    futurewakeUpdateBall();
    futurewakeWakeCount--;
  }
  if (futurewakeWakeColor == LIGHT_GREEN) {
    futurewakeBallRoot.speed += difficulty;
  } else {
    futurewakeBallRoot.speed = difficulty;
  }
  color = BLACK;
  box(futurewakeBallPos.x, futurewakeBallPos.y, FUTUREWAKE_BALL_SIZE, FUTUREWAKE_BALL_SIZE, &scratch);
  if (futurewakeBallPos.y < 0 || futurewakeBallPos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  if (futurewakeIsHittingRacket) {
    futurewakeBallDist = 0;
    addScore(floor(futurewakeMultiplier), futurewakeBallPos.x,
             clamp(futurewakeBallPos.y, 30, 80));
    futurewakeMultiplier += 10;
    if (futurewakeMultiplier > 100) {
      futurewakeBallRoot.vel.x += rnd(0, 1) * RNDPM();
      float len = vectorLength(&futurewakeBallRoot.vel);
      vectorMul(&futurewakeBallRoot.vel, 1.0 / len);
      vectorMul(&futurewakeBallRoot.vel, 3);
    }
  }
  futurewakeMultiplier = clamp(futurewakeMultiplier * 0.99, 1, 100.99);
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(futurewakeMultiplier)));
  text(multText, 3, 9, &scratch);
}

void addGameFuturewake() {
  addGame(futurewakeTitle, futurewakeDescription, futurewakeCharacters,
          futurewakeCharactersCount, &futurewakeOptions, true,
          &futurewakeUpdate);
}
