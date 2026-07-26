#include "../cglp.h"

int* baserunnerdashTitle = "BASERUNNER DASH";
int* baserunnerdashDescription = "[Hold] Run forward\n[Release] Run back";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] baserunnerdashCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int baserunnerdashCharactersCount = 0;

Options baserunnerdashOptions = {100, 100, 60, false};

#define BASERUNNERDASH_BASE_RADIUS 3
#define BASERUNNERDASH_RUNNER_RADIUS 4
#define BASERUNNERDASH_BALL_RADIUS 2

Vector[4] baserunnerdashBasePositions;

struct BaserunnerdashRunner {
  Vector pos;
  int targetBase;
  int currentBase;
  float speed;
  bool isOnBase;
};
BaserunnerdashRunner baserunnerdashRunner;

struct BaserunnerdashBall {
  Vector pos;
  int targetBase;
  int currentBase;
  float speed;
  bool isThrown;
};
BaserunnerdashBall baserunnerdashBall;

void baserunnerdashMoveToward(Vector* p, float tx, float ty, float speed) {
  float dx = tx - p->x;
  float dy = ty - p->y;
  float len = sqrt(dx * dx + dy * dy);
  if (len > 0) {
    vectorAdd(p, dx / len * speed, dy / len * speed);
  }
}

void baserunnerdashUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&baserunnerdashBasePositions[0], 50, 90);
    vectorSet(&baserunnerdashBasePositions[1], 90, 50);
    vectorSet(&baserunnerdashBasePositions[2], 50, 10);
    vectorSet(&baserunnerdashBasePositions[3], 10, 50);
    baserunnerdashRunner.pos = baserunnerdashBasePositions[1];
    baserunnerdashRunner.targetBase = 2;
    baserunnerdashRunner.currentBase = 1;
    baserunnerdashRunner.speed = 0.6;
    baserunnerdashRunner.isOnBase = true;
    baserunnerdashBall.pos = baserunnerdashBasePositions[0];
    baserunnerdashBall.targetBase = 2;
    baserunnerdashBall.currentBase = 0;
    baserunnerdashBall.speed = 0.3;
    baserunnerdashBall.isThrown = true;
  }
  Vector currentBase = baserunnerdashBasePositions[baserunnerdashRunner.currentBase];
  Vector nextBase = baserunnerdashBasePositions[baserunnerdashRunner.targetBase];
  float runnerSpeed = baserunnerdashRunner.speed * difficulty;
  if (input.isJustPressed || input.isJustReleased) {
    play(CLICK);
  }
  if (input.isPressed) {
    baserunnerdashMoveToward(&baserunnerdashRunner.pos, nextBase.x, nextBase.y, runnerSpeed);
    baserunnerdashRunner.isOnBase = false;
    if (distanceTo(&baserunnerdashRunner.pos, nextBase.x, nextBase.y) < runnerSpeed) {
      play(COIN);
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      baserunnerdashRunner.pos = nextBase;
      baserunnerdashRunner.currentBase = baserunnerdashRunner.targetBase;
      baserunnerdashRunner.targetBase = (baserunnerdashRunner.targetBase + 1) % 4;
      baserunnerdashRunner.isOnBase = true;
    }
  } else {
    if (distanceTo(&baserunnerdashRunner.pos, currentBase.x, currentBase.y) < runnerSpeed) {
      baserunnerdashRunner.pos = currentBase;
      if (!baserunnerdashRunner.isOnBase) {
        play(HIT);
      }
      baserunnerdashRunner.isOnBase = true;
    } else {
      baserunnerdashMoveToward(&baserunnerdashRunner.pos, currentBase.x, currentBase.y, runnerSpeed);
    }
  }
  Vector targetPos = baserunnerdashBasePositions[baserunnerdashBall.targetBase];
  float thrownMul;
  if (baserunnerdashBall.isThrown) {
    thrownMul = 3;
  } else {
    thrownMul = 1;
  }
  float ballSpeed = baserunnerdashBall.speed * thrownMul * difficulty;
  baserunnerdashMoveToward(&baserunnerdashBall.pos, targetPos.x, targetPos.y, ballSpeed);
  if (distanceTo(&baserunnerdashBall.pos, targetPos.x, targetPos.y) < ballSpeed) {
    baserunnerdashBall.isThrown = false;
    baserunnerdashBall.pos = targetPos;
    baserunnerdashBall.currentBase = baserunnerdashBall.targetBase;
    if (baserunnerdashBall.targetBase == baserunnerdashRunner.currentBase) {
      baserunnerdashBall.targetBase = baserunnerdashRunner.targetBase;
    } else {
      baserunnerdashBall.targetBase = baserunnerdashRunner.currentBase;
    }
  } else if (!baserunnerdashBall.isThrown && rnd(0, 1) < 0.05 * difficulty) {
    play(POWER_UP);
    baserunnerdashBall.isThrown = true;
    if (baserunnerdashBall.currentBase != baserunnerdashRunner.currentBase &&
        baserunnerdashBall.currentBase != baserunnerdashRunner.targetBase) {
      baserunnerdashBall.targetBase = baserunnerdashRunner.targetBase;
    }
  }
  color = LIGHT_BLACK;
  TIMES(4, i) {
    box(baserunnerdashBasePositions[i].x, baserunnerdashBasePositions[i].y,
        BASERUNNERDASH_BASE_RADIUS * 2, BASERUNNERDASH_BASE_RADIUS * 2, &scratch);
  }
  if (baserunnerdashRunner.isOnBase) {
    color = LIGHT_BLUE;
  } else {
    color = BLUE;
  }
  box(baserunnerdashRunner.pos.x, baserunnerdashRunner.pos.y,
      BASERUNNERDASH_RUNNER_RADIUS * 2, BASERUNNERDASH_RUNNER_RADIUS * 2, &scratch);
  if (baserunnerdashBall.isThrown) {
    color = LIGHT_RED;
  } else {
    color = RED;
  }
  box(baserunnerdashBall.pos.x, baserunnerdashBall.pos.y,
      BASERUNNERDASH_BALL_RADIUS * 2, BASERUNNERDASH_BALL_RADIUS * 2, &scratch);
  if (scratch.isColliding.rect[BLUE] && !baserunnerdashBall.isThrown) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameBaserunnerdash() {
  addGame(baserunnerdashTitle, baserunnerdashDescription,
          baserunnerdashCharacters, baserunnerdashCharactersCount,
          &baserunnerdashOptions, false, &baserunnerdashUpdate);
}
