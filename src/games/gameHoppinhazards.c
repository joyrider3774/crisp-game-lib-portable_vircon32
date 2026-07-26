#include "../cglp.h"

int* hoppinhazardsTitle = "HOPPIN' \nHAZARDS";
int* hoppinhazardsDescription = "[Hold] Hop";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] hoppinhazardsCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int hoppinhazardsCharactersCount = 0;

Options hoppinhazardsOptions = {100, 100, 80, false};

#define HOPPINHAZARDS_MIN_FROG_SIZE 10
#define HOPPINHAZARDS_MAX_FROG_SIZE 30
#define HOPPINHAZARDS_OBSTACLE_SPEED 1
#define HOPPINHAZARDS_SHAPE_BIRD 0
#define HOPPINHAZARDS_SHAPE_SNAKE 1

struct HoppinhazardsFrog {
  Vector pos;
  float size;
};
HoppinhazardsFrog hoppinhazardsFrog;

struct HoppinhazardsObstacle {
  Vector pos;
  float vx;
  float vy;
  int shape;
  int color;
  bool isAlive;
};
#define HOPPINHAZARDS_MAX_OBSTACLE_COUNT 32
HoppinhazardsObstacle[HOPPINHAZARDS_MAX_OBSTACLE_COUNT] hoppinhazardsObstacles;
int hoppinhazardsObstacleIndex;
float hoppinhazardsNextObstacleTicks;

void hoppinhazardsUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&hoppinhazardsFrog.pos, 50, 90);
    hoppinhazardsFrog.size = HOPPINHAZARDS_MIN_FROG_SIZE;
    INIT_UNALIVED_ARRAY_FAST(hoppinhazardsObstacles);
    hoppinhazardsObstacleIndex = 0;
    hoppinhazardsNextObstacleTicks = 0;
  }
  color = BLUE;
  rect(0, 95, 100, 5, &scratch);
  hoppinhazardsFrog.pos.x = 50;
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    float newSize = hoppinhazardsFrog.size + difficulty;
    if (newSize > HOPPINHAZARDS_MAX_FROG_SIZE) {
      newSize = HOPPINHAZARDS_MAX_FROG_SIZE;
    }
    hoppinhazardsFrog.size = newSize;
    float newY = hoppinhazardsFrog.pos.y - difficulty;
    if (newY < 10) {
      newY = 10;
    }
    hoppinhazardsFrog.pos.y = newY;
  } else {
    float newSize = hoppinhazardsFrog.size - difficulty * 2;
    if (newSize < HOPPINHAZARDS_MIN_FROG_SIZE) {
      newSize = HOPPINHAZARDS_MIN_FROG_SIZE;
    }
    hoppinhazardsFrog.size = newSize;
    float newY = hoppinhazardsFrog.pos.y + difficulty * 2;
    if (newY > 90) {
      newY = 90;
    }
    hoppinhazardsFrog.pos.y = newY;
  }
  addScore((hoppinhazardsFrog.size - HOPPINHAZARDS_MIN_FROG_SIZE) / 100, SCORE_NO_POPUP_X,
           SCORE_NO_POPUP_Y);
  color = GREEN;
  box(hoppinhazardsFrog.pos.x, hoppinhazardsFrog.pos.y, hoppinhazardsFrog.size,
      hoppinhazardsFrog.size, &scratch);
  hoppinhazardsNextObstacleTicks -= difficulty;
  if (hoppinhazardsNextObstacleTicks < 0) {
    float x;
    if (rndi(0, 2) == 0) {
      x = -10;
    } else {
      x = 110;
    }
    int shape;
    if (rndi(0, 2) == 0) {
      shape = HOPPINHAZARDS_SHAPE_BIRD;
    } else {
      shape = HOPPINHAZARDS_SHAPE_SNAKE;
    }
    float vx;
    if (x < 50) {
      vx = HOPPINHAZARDS_OBSTACLE_SPEED + rnd(0, difficulty - 1);
    } else {
      vx = -(HOPPINHAZARDS_OBSTACLE_SPEED + rnd(0, difficulty - 1));
    }
    int col;
    if (rndi(0, 2) == 0) {
      col = BLUE;
    } else {
      col = RED;
    }
    ASSIGN_ARRAY_ITEM(hoppinhazardsObstacles, hoppinhazardsObstacleIndex, HoppinhazardsObstacle, no);
    vectorSet(&no->pos, x, rnd(10, 90));
    no->vx = vx;
    no->vy = 0;
    no->shape = shape;
    no->color = col;
    no->isAlive = true;
    hoppinhazardsObstacleIndex =
        cgl_wrap(hoppinhazardsObstacleIndex + 1, 0, HOPPINHAZARDS_MAX_OBSTACLE_COUNT);
    if (shape == HOPPINHAZARDS_SHAPE_BIRD) {
      play(HIT);
    } else {
      play(CLICK);
    }
    hoppinhazardsNextObstacleTicks = rnd(45, 120) / sqrt(difficulty);
  }
  FOR_EACH(hoppinhazardsObstacles, i) {
    ASSIGN_ARRAY_ITEM(hoppinhazardsObstacles, i, HoppinhazardsObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.x += o->vx;
    o->pos.y += o->vy;
    color = o->color;
    if (o->shape == HOPPINHAZARDS_SHAPE_BIRD) {
      float angle;
      if (o->vx > 0) {
        angle = 0;
      } else {
        angle = CGLP_PI;
      }
      thickness = 3;
      barCenterPosRatio = 0.5;
      bar(o->pos.x, o->pos.y, 10, angle, &scratch);
      if (scratch.isColliding.rect[GREEN]) {
        play(EXPLOSION);
        gameOver();
      }
    } else if (o->shape == HOPPINHAZARDS_SHAPE_SNAKE) {
      float angleTo2;
      if (o->vx > 0) {
        angleTo2 = CGLP_PI_2 - CGLP_PI;
      } else {
        angleTo2 = CGLP_PI_2 + CGLP_PI;
      }
      thickness = 3;
      arc(o->pos.x, o->pos.y, 5, CGLP_PI_2, angleTo2, &scratch);
      if (scratch.isColliding.rect[GREEN]) {
        play(EXPLOSION);
        gameOver();
      }
    }
    if (o->pos.x < -10 || o->pos.x > 110) {
      o->isAlive = false;
      continue;
    }
  }
}

void addGameHoppinhazards() {
  addGame(hoppinhazardsTitle, hoppinhazardsDescription, hoppinhazardsCharacters,
          hoppinhazardsCharactersCount, &hoppinhazardsOptions, false,
          &hoppinhazardsUpdate);
}
