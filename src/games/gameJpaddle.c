#include "../cglp.h"

int* jpaddleTitle = "JPADDLE";
int* jpaddleDescription = "[Tap]  Jump/Turn\n[Hold] Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] jpaddleCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int jpaddleCharactersCount = 0;

Options jpaddleOptions = {100, 100, 4, false};

#define JPADDLE_JUMP_HEIGHT 12
#define JPADDLE_MAX_JUMPS 99
#define JPADDLE_PADDLE_SIZE 6
#define JPADDLE_BALL_SPEED 1
#define JPADDLE_SPAWN_INTERVAL 60

Vector jpaddlePaddle;
int jpaddleJumpCounter;
int jpaddleJumpToggle;

struct JpaddleBall {
  Vector pos;
  Vector vel;
  bool isAlive;
};
// Balls only die by falling past the paddle (a miss); a skilled player who
// keeps bouncing every ball never removes any, while spawn interval
// (60/difficulty) shrinks as difficulty grows forever, so live count grows
// quadratically with play time (~90 within the first minute) - size for a
// long session instead of a handful.
#define JPADDLE_MAX_BALL_COUNT 1024
JpaddleBall[JPADDLE_MAX_BALL_COUNT] jpaddleBalls;
int jpaddleBallIndex;

void jpaddleUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&jpaddlePaddle, 50, 90);
    INIT_UNALIVED_ARRAY_FAST(jpaddleBalls);
    jpaddleBallIndex = 0;
    jpaddleJumpCounter = 0;
    jpaddleJumpToggle = 0;
  }
  if (input.isJustPressed && jpaddleJumpCounter < JPADDLE_MAX_JUMPS) {
    jpaddlePaddle.y -= JPADDLE_JUMP_HEIGHT / (jpaddleJumpCounter + 1);
    jpaddleJumpCounter++;
    jpaddleJumpToggle++;
    play(JUMP);
  } else {
    jpaddlePaddle.y += difficulty * 0.1;
  }
  if (input.isPressed) {
    if (jpaddleJumpCounter > 0) {
      float dir;
      if (jpaddleJumpToggle % 2 == 1) {
        dir = 1;
      } else {
        dir = -1;
      }
      jpaddlePaddle.x += difficulty * 0.5 * dir;
    }
  }
  jpaddlePaddle.x = clamp(jpaddlePaddle.x, 3, 97);
  jpaddlePaddle.y = clamp(jpaddlePaddle.y, 30, 110);
  color = CYAN;
  rect(jpaddlePaddle.x - JPADDLE_PADDLE_SIZE / 2.0, jpaddlePaddle.y - 2, JPADDLE_PADDLE_SIZE, 4,
       &scratch);
  color = BLACK;
  FOR_EACH(jpaddleBalls, i) {
    ASSIGN_ARRAY_ITEM(jpaddleBalls, i, JpaddleBall, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x += b->vel.x * JPADDLE_BALL_SPEED * difficulty;
    b->pos.y += b->vel.y * JPADDLE_BALL_SPEED * difficulty;
    if (b->pos.x < 0 || b->pos.x > 97) {
      b->vel.x *= -1;
      play(HIT);
    }
    thickness = 3;
    arc(b->pos.x, b->pos.y, 3, 0, CGLP_PI * 2, &scratch);
    if (scratch.isColliding.rect[CYAN] && b->vel.y > 0) {
      b->pos.y = jpaddlePaddle.y - 3;
      b->vel.y *= -1;
      addScore(99 - b->pos.y, b->pos.x, b->pos.y);
      play(POWER_UP);
      jpaddleJumpCounter -= 9;
      if (jpaddleJumpCounter < 0) {
        jpaddleJumpCounter = 0;
      }
    }
    if (b->pos.y > 100) {
      b->isAlive = false;
      continue;
    }
  }
  if (ticks % (int)floor(JPADDLE_SPAWN_INTERVAL / difficulty) == 0) {
    ASSIGN_ARRAY_ITEM(jpaddleBalls, jpaddleBallIndex, JpaddleBall, nb);
    vectorSet(&nb->pos, rnd(10, 90), 0);
    Vector v;
    vectorSet(&v, rnd(-1, 1), 1);
    float len = vectorLength(&v);
    if (len > 0) {
      vectorMul(&v, 1.0 / len);
    }
    nb->vel = v;
    nb->isAlive = true;
    jpaddleBallIndex = cgl_wrap(jpaddleBallIndex + 1, 0, JPADDLE_MAX_BALL_COUNT);
    play(LASER);
  }
  if (jpaddlePaddle.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameJpaddle() {
  addGame(jpaddleTitle, jpaddleDescription, jpaddleCharacters,
          jpaddleCharactersCount, &jpaddleOptions, false, &jpaddleUpdate);
}
