#include "../cglp.h"

int* turtletideTitle = "TURTLE TIDE";
int* turtletideDescription = "[Hold] Dive & Speed up";
int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] turtletideCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int turtletideCharactersCount = 1;

Options turtletideOptions = {100, 100, 1, false};

#define TURTLETIDE_TURTLE_SIZE 2
#define TURTLETIDE_WATER_COLOR LIGHT_BLUE
#define TURTLETIDE_TURTLE_COLOR GREEN
#define TURTLETIDE_OBSTACLE_COLOR_ABOVE RED
#define TURTLETIDE_OBSTACLE_COLOR_BELOW PURPLE
#define TURTLETIDE_SCROLL_SPEED 1

struct TurtletideTurtle {
  Vector pos;
  bool isUnderwater;
  float speed;
};
TurtletideTurtle turtletideTurtle;

struct TurtletideWater {
  float waterLevel;
  float waveAmplitude;
  float waveFrequency;
};
TurtletideWater turtletideWater;

struct TurtletideObstacle {
  Vector pos;
  bool isUnderwater;
  Vector size;
  bool isAlive;
};
#define TURTLETIDE_MAX_OBSTACLE_COUNT 24
TurtletideObstacle[TURTLETIDE_MAX_OBSTACLE_COUNT] turtletideObstacles;
int turtletideObstacleIndex;
float turtletideNextObstacleDist;

void turtletideInitGame() {
  vectorSet(&turtletideTurtle.pos, 20, 50);
  turtletideTurtle.isUnderwater = false;
  turtletideTurtle.speed = 1;
  turtletideWater.waterLevel = 50;
  turtletideWater.waveAmplitude = 10;
  turtletideWater.waveFrequency = 0.05;
  INIT_UNALIVED_ARRAY_FAST(turtletideObstacles);
  turtletideObstacleIndex = 0;
  turtletideNextObstacleDist = 60;
  ticks = 0;
  score = 0;
}

void turtletideUpdateWater() {
  Collision scratch;
  turtletideWater.waveFrequency = clamp(
      turtletideWater.waveFrequency + rnd(0, 0.0001 * sqrt(difficulty)) * RNDPM(), 0, 0.05 * sqrt(difficulty));
  turtletideWater.waveAmplitude = clamp(
      turtletideWater.waveAmplitude + rnd(0, 0.01 * sqrt(difficulty)) * RNDPM(), 0, 10 * sqrt(difficulty));
  float wl = 50 + sin(ticks * turtletideWater.waveFrequency) * turtletideWater.waveAmplitude;
  turtletideWater.waterLevel += (wl - turtletideWater.waterLevel) * 0.1;
  color = TURTLETIDE_WATER_COLOR;
  rect(0, turtletideWater.waterLevel, 100, 100 - turtletideWater.waterLevel, &scratch);
}

void turtletideUpdateTurtle() {
  Collision turtleCollision;
  if (input.isJustPressed) {
    play(POWER_UP);
  }
  if (input.isJustReleased) {
    play(JUMP);
  }
  if (input.isPressed) {
    turtletideTurtle.isUnderwater = true;
    turtletideTurtle.speed = clamp(turtletideTurtle.speed + 0.01, 1, 9);
  } else {
    turtletideTurtle.isUnderwater = false;
    turtletideTurtle.speed += (1 - turtletideTurtle.speed) * 0.1;
  }
  float targetY;
  if (turtletideTurtle.isUnderwater) {
    targetY = turtletideWater.waterLevel + 15;
  } else {
    targetY = turtletideWater.waterLevel - 5;
  }
  turtletideTurtle.pos.y += (targetY - turtletideTurtle.pos.y) * 0.1 * difficulty;
  color = TURTLETIDE_TURTLE_COLOR;
  box(turtletideTurtle.pos.x, turtletideTurtle.pos.y, TURTLETIDE_TURTLE_SIZE * turtletideTurtle.speed,
      TURTLETIDE_TURTLE_SIZE, &turtleCollision);
  if (turtleCollision.isColliding.rect[TURTLETIDE_WATER_COLOR] && !turtletideTurtle.isUnderwater) {
    turtletideTurtle.pos.y = turtletideWater.waterLevel - TURTLETIDE_TURTLE_SIZE / 2;
  }
}

void turtletideUpdateObstacles() {
  Collision scratch;
  float ss = TURTLETIDE_SCROLL_SPEED * difficulty * turtletideTurtle.speed;
  addScore(ss * 0.1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  FOR_EACH(turtletideObstacles, oi) {
    ASSIGN_ARRAY_ITEM(turtletideObstacles, oi, TurtletideObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.x -= ss;
    if (o->pos.x < -10) {
      o->isAlive = false;
      continue;
    }
  }
  turtletideNextObstacleDist -= ss;
  if (turtletideNextObstacleDist < 0) {
    play(CLICK);
    bool isUnderwater = rnd(0, 1) < 0.5;
    ASSIGN_ARRAY_ITEM(turtletideObstacles, turtletideObstacleIndex, TurtletideObstacle, no);
    float oy;
    if (isUnderwater) {
      oy = rnd(turtletideWater.waterLevel + 5, 95);
    } else {
      oy = rnd(5, turtletideWater.waterLevel - 5);
    }
    vectorSet(&no->pos, 110, oy);
    no->isUnderwater = isUnderwater;
    vectorSet(&no->size, rnd(5, 10), rnd(5, 10));
    no->isAlive = true;
    turtletideObstacleIndex = cgl_wrap(turtletideObstacleIndex + 1, 0, TURTLETIDE_MAX_OBSTACLE_COUNT);
    turtletideNextObstacleDist = rnd(99, 120);
  }
  FOR_EACH(turtletideObstacles, oi2) {
    ASSIGN_ARRAY_ITEM(turtletideObstacles, oi2, TurtletideObstacle, o2);
    SKIP_IS_NOT_ALIVE(o2);
    if (o2->isUnderwater) {
      color = TURTLETIDE_OBSTACLE_COLOR_BELOW;
    } else {
      color = TURTLETIDE_OBSTACLE_COLOR_ABOVE;
    }
    box(o2->pos.x, o2->pos.y, o2->size.x, o2->size.y, &scratch);
    if (scratch.isColliding.rect[TURTLETIDE_TURTLE_COLOR]) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void turtletideUpdate() {
  if (!ticks) {
    turtletideInitGame();
  }
  turtletideUpdateWater();
  turtletideUpdateTurtle();
  turtletideUpdateObstacles();
}

void addGameTurtletide() {
  addGame(turtletideTitle, turtletideDescription, turtletideCharacters, turtletideCharactersCount,
          &turtletideOptions, false, &turtletideUpdate);
}
