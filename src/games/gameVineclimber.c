#include "../cglp.h"

int* vineclimberTitle = "VINE CLIMBER";
int* vineclimberDescription = "[Hold] Climb\n[Release] Slide";
int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] vineclimberCharacters = {
    {
        "  ll  ",
        "l ll l",
        " llll ",
        "  ll  ",
        " l  l ",
        "l    l",
    },
    {
        "  ll  ",
        "  ll  ",
        " llll ",
        "l ll l",
        " l  l ",
        " l  l ",
    },
    {
        " lll  ",
        "  l   ",
        " lll  ",
        "l lll ",
        " lll  ",
        "      ",
    },
    {
        " llll ",
        "l llll",
        "l llll",
        "l llll",
        "l llll",
        " llll ",
    },
};
int vineclimberCharactersCount = 4;

Options vineclimberOptions = {100, 100, 7, false};

struct VineclimberVine {
  float centerX;
  float ticks;
  float amplitude;
  float frequency;
  float bending;
  float targetAmplitude;
  float targetFrequency;
  float targetBending;
};
VineclimberVine vineclimberVine;

struct VineclimberPlayer {
  Vector pos;
  float speed;
};
VineclimberPlayer vineclimberPlayer;

struct VineclimberObstacle {
  Vector pos;
  float speed;
  bool isAlive;
};
#define VINECLIMBER_MAX_OBSTACLE_COUNT 32
VineclimberObstacle[VINECLIMBER_MAX_OBSTACLE_COUNT] vineclimberObstacles;
int vineclimberObstacleIndex;
float vineclimberNextObstacleTicks;

struct VineclimberCoin {
  Vector pos;
  float vx;
};
VineclimberCoin vineclimberCoin;
bool vineclimberCoinExists;

float vineclimberMultiplier;

// Shared helper for the upstream vine x-position formula, used both for the
// player's own x position and for drawing the vine's body.
float vineclimberVineX(float y) {
  return vineclimberVine.centerX +
         pow(sin((vineclimberVine.ticks + y * vineclimberVine.bending) * vineclimberVine.frequency / 2), 2) *
             vineclimberVine.amplitude * 2 -
         vineclimberVine.amplitude;
}

void vineclimberUpdate() {
  Collision scratch;
  if (!ticks) {
    vineclimberVine.centerX = 50;
    vineclimberVine.ticks = 0;
    vineclimberVine.amplitude = 30;
    vineclimberVine.frequency = 0.03;
    vineclimberVine.bending = 0.5;
    vineclimberVine.targetAmplitude = 30;
    vineclimberVine.targetFrequency = 0.03;
    vineclimberVine.targetBending = 0.5;
    vectorSet(&vineclimberPlayer.pos, 50, 80);
    vineclimberPlayer.speed = 1;
    INIT_UNALIVED_ARRAY_FAST(vineclimberObstacles);
    vineclimberObstacleIndex = 0;
    vineclimberNextObstacleTicks = 0;
    vineclimberCoinExists = false;
    vineclimberMultiplier = 1;
  }

  int animIndex = 1;
  if (input.isPressed) {
    vineclimberPlayer.pos.y -= vineclimberPlayer.speed * 0.8 * difficulty;
    animIndex = (int)floor(ticks / 20) % 2;
    if (ticks % 20 == 0) {
      play(CLICK);
    }
  } else {
    if (input.isJustReleased) {
      play(HIT);
    }
    vineclimberPlayer.pos.y += vineclimberPlayer.speed * 1.2 * difficulty;
  }
  vineclimberPlayer.pos.y = clamp(vineclimberPlayer.pos.y, 0, 99);
  vineclimberPlayer.pos.x = vineclimberVineX(vineclimberPlayer.pos.y);
  vineclimberVine.ticks++;
  vineclimberVine.amplitude += (vineclimberVine.targetAmplitude - vineclimberVine.amplitude) * 0.01;
  vineclimberVine.frequency += (vineclimberVine.targetFrequency - vineclimberVine.frequency) * 0.01;
  vineclimberVine.bending += (vineclimberVine.targetBending - vineclimberVine.bending) * 0.01;
  if (rnd(0, 1) < 0.005) {
    vineclimberVine.targetAmplitude = rnd(25, 40);
    vineclimberVine.targetFrequency = rnd(0.025, 0.04);
    vineclimberVine.targetBending = rnd(0.45, 0.6);
  }

  color = GREEN;
  for (float y = 0; y <= 100; y += 5) {
    float x = vineclimberVineX(y);
    box(x, y, 2, 5, &scratch);
  }
  if (vineclimberVine.ticks * vineclimberVine.frequency >= CGLP_PI * 2) {
    vineclimberVine.ticks = 0;
  }

  color = RED;
  FOR_EACH(vineclimberObstacles, oi) {
    ASSIGN_ARRAY_ITEM(vineclimberObstacles, oi, VineclimberObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->speed += 0.01 * sqrt(difficulty);
    o->pos.y += o->speed;
    character("c", o->pos.x, o->pos.y, &scratch);
    if (o->pos.y > 105) {
      o->isAlive = false;
      continue;
    }
  }
  vineclimberNextObstacleTicks--;
  if (vineclimberNextObstacleTicks < 0) {
    ASSIGN_ARRAY_ITEM(vineclimberObstacles, vineclimberObstacleIndex, VineclimberObstacle, no);
    vectorSet(&no->pos, rnd(10, 90), 0);
    no->speed = rnd(0, 0.5) * difficulty;
    no->isAlive = true;
    vineclimberObstacleIndex = cgl_wrap(vineclimberObstacleIndex + 1, 0, VINECLIMBER_MAX_OBSTACLE_COUNT);
    vineclimberNextObstacleTicks = rnd(70, 90) / difficulty;
  }

  color = BLUE;
  int[2] playerChar;
  playerChar[0] = 'a' + animIndex;
  playerChar[1] = 0;
  character(playerChar, vineclimberPlayer.pos.x, vineclimberPlayer.pos.y, &scratch);
  if (scratch.isColliding.character['c']) {
    play(EXPLOSION);
    gameOver();
  }

  if (!vineclimberCoinExists) {
    float vx = rnd(0.18, 0.25) * RNDPM() * difficulty;
    float cx;
    if (vx > 0) {
      cx = -10;
    } else {
      cx = 110;
    }
    vectorSet(&vineclimberCoin.pos, cx, rnd(10, 90));
    vineclimberCoin.vx = vx;
    vineclimberCoinExists = true;
  }
  color = YELLOW;
  vineclimberCoin.pos.x += vineclimberCoin.vx;
  Collision coinCollision;
  character("d", vineclimberCoin.pos.x, vineclimberCoin.pos.y, &coinCollision);
  if (coinCollision.isColliding.character['a'] || coinCollision.isColliding.character['b']) {
    play(COIN);
    addScore(vineclimberMultiplier, vineclimberCoin.pos.x, vineclimberCoin.pos.y);
    vineclimberMultiplier = clamp(vineclimberMultiplier + 1, 1, 9);
    vineclimberCoinExists = false;
  } else if (vineclimberCoin.pos.x < -10 || vineclimberCoin.pos.x > 110) {
    vineclimberMultiplier = 1;
    vineclimberCoinExists = false;
  }
}

void addGameVineclimber() {
  addGame(vineclimberTitle, vineclimberDescription, vineclimberCharacters, vineclimberCharactersCount,
          &vineclimberOptions, false, &vineclimberUpdate);
}
