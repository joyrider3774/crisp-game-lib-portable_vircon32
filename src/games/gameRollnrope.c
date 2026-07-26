#include "../cglp.h"

int* rollnropeTitle = "ROLLNROPE";
int* rollnropeDescription = "[Tap] Jump / Run";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rollnropeCharacters = {
    {
        "  llll",
        "  llll",
        "  llll",
        "  ll  ",
        "llllll",
        "llllll",
    },
    {
        " lll  ",
        "  lll ",
        "   ll ",
        "  ll  ",
        "  ll  ",
        " ll   ",
    },
    {
        "  lll ",
        "  l ll",
        " ll ll",
        " ll ll",
        "ll  ll",
        "ll ll ",
    },
};
int rollnropeCharactersCount = 3;

Options rollnropeOptions = {100, 100, 9, false};

#define ROLLNROPE_MODE_STOP 0
#define ROLLNROPE_MODE_RUN 1
#define ROLLNROPE_MODE_STAND 2
#define ROLLNROPE_MODE_JUMP 3

struct RollnropeRope {
  float x;
  float angle;
  float radius;
  float size;
  float speed;
  int count;
};
RollnropeRope rollnropeRope;
bool rollnropeRopeActive;

struct RollnropePlayer {
  Vector pos;
  float vy;
  int mode;
};
RollnropePlayer rollnropePlayer;

void rollnropeUpdate() {
  Collision scratch;
  if (!ticks) {
    rollnropeRopeActive = false;
    vectorSet(&rollnropePlayer.pos, 5, 84);
    rollnropePlayer.vy = 0;
    rollnropePlayer.mode = ROLLNROPE_MODE_STOP;
  }
  if (!rollnropeRopeActive) {
    rollnropeRope.x = 150;
    rollnropeRope.angle = rnd(0, CGLP_PI * 2);
    rollnropeRope.radius = rnd(30, 45);
    rollnropeRope.size = rnd(5, 10);
    rollnropeRope.speed = rnd(1, 2) * difficulty;
    if (!ticks) {
      rollnropeRope.count = 2;
    } else {
      rollnropeRope.count = rndi(2, 6);
    }
    if (rnd(0, 1) < 0.2) {
      rollnropeRope.speed *= -1;
    }
    rollnropeRopeActive = true;
  }
  rect(0, 90, 99, 9, &scratch);
  rollnropeRope.x -= (rollnropeRope.x - 50) * 0.1;
  float pa = cgl_wrap(rollnropeRope.angle - CGLP_PI / 2, -CGLP_PI, CGLP_PI);
  rollnropeRope.angle += rollnropeRope.speed * 0.05;
  float a = cgl_wrap(rollnropeRope.angle - CGLP_PI / 2, -CGLP_PI, CGLP_PI);
  float ry = 91 - rollnropeRope.size / 2 - rollnropeRope.radius +
             sin(rollnropeRope.angle) * rollnropeRope.radius;
  box(rollnropeRope.x + cos(rollnropeRope.angle) * rollnropeRope.radius, ry,
      rollnropeRope.size, rollnropeRope.size, &scratch);
  text(intToChar(rollnropeRope.count), 50, 10, &scratch);
  if (pa * a < 0 && fabs(a) < CGLP_PI / 2) {
    if (rollnropePlayer.mode == ROLLNROPE_MODE_JUMP) {
      play(POWER_UP);
      rollnropeRope.count--;
      float s = 85 - rollnropeRope.size - rollnropePlayer.pos.y;
      if (s < 0) {
        s = 1;
      }
      s = floor(100 / s);
      addScore(s, rollnropePlayer.pos.x, rollnropePlayer.pos.y);
      if (rollnropeRope.count == 0) {
        play(COIN);
        rollnropeRopeActive = false;
        rollnropePlayer.mode = ROLLNROPE_MODE_STOP;
      }
    } else {
      play(HIT);
    }
  }
  if (rollnropePlayer.mode == ROLLNROPE_MODE_STOP) {
    rollnropePlayer.pos.x += (5 - rollnropePlayer.pos.x) * 0.1;
    if (rollnropePlayer.pos.y == 84 && input.isJustPressed) {
      play(SELECT);
      rollnropePlayer.mode = ROLLNROPE_MODE_RUN;
    }
  }
  if (rollnropePlayer.mode == ROLLNROPE_MODE_RUN) {
    rollnropePlayer.pos.x += difficulty * 2;
    if (rollnropePlayer.pos.x > 50) {
      rollnropePlayer.pos.x = 50;
      rollnropePlayer.mode = ROLLNROPE_MODE_STAND;
    }
  }
  if (rollnropePlayer.mode == ROLLNROPE_MODE_STAND) {
    if (input.isPressed) {
      play(JUMP);
      rollnropePlayer.vy = -3;
      rollnropePlayer.mode = ROLLNROPE_MODE_JUMP;
    }
  }
  if (rollnropePlayer.mode == ROLLNROPE_MODE_JUMP ||
      rollnropePlayer.mode == ROLLNROPE_MODE_STOP) {
    float dvy;
    if (input.isPressed) {
      dvy = 0.1;
    } else {
      dvy = 0.2;
    }
    rollnropePlayer.vy += dvy * sqrt(difficulty);
    rollnropePlayer.pos.y += rollnropePlayer.vy;
    if (rollnropePlayer.pos.y > 84) {
      rollnropePlayer.pos.y = 84;
      if (rollnropePlayer.mode == ROLLNROPE_MODE_JUMP) {
        rollnropePlayer.mode = ROLLNROPE_MODE_STAND;
      }
    }
  }
  Collision c1;
  character("a", rollnropePlayer.pos.x, rollnropePlayer.pos.y - 3, &c1);
  int[2] legChar;
  if (rollnropePlayer.mode == ROLLNROPE_MODE_JUMP ||
      rollnropePlayer.mode == ROLLNROPE_MODE_RUN) {
    legChar[0] = 'c';
  } else {
    legChar[0] = 'b';
  }
  legChar[1] = 0;
  Collision c2;
  character(legChar, rollnropePlayer.pos.x, rollnropePlayer.pos.y + 3, &c2);
  if (c1.isColliding.rect[BLACK] || c2.isColliding.rect[BLACK]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameRollnrope() {
  addGame(rollnropeTitle, rollnropeDescription, rollnropeCharacters,
          rollnropeCharactersCount, &rollnropeOptions, false,
          &rollnropeUpdate);
}
