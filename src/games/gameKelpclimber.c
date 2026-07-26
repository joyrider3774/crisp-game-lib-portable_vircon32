#include "../cglp.h"

int* kelpclimberTitle = "KELP CLIMBER";
int* kelpclimberDescription = "[Tap]\n Grab kelp";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] kelpclimberCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int kelpclimberCharactersCount = 0;

Options kelpclimberOptions = {100, 100, 1, false};

#define KELPCLIMBER_SCREEN_WIDTH 100
#define KELPCLIMBER_SCREEN_HEIGHT 100
#define KELPCLIMBER_OTTER_SIZE 5

struct KelpclimberKelp {
  Vector pos;
  float length;
  bool isAlive;
};
#define KELPCLIMBER_MAX_KELP_COUNT 16
KelpclimberKelp[KELPCLIMBER_MAX_KELP_COUNT] kelpclimberKelpStrands;
int kelpclimberKelpIndex;
float kelpclimberNextKelpStandDist;

struct KelpclimberObstacle {
  Vector pos;
  Vector size;
  bool isAlive;
};
#define KELPCLIMBER_MAX_OBSTACLE_COUNT 16
KelpclimberObstacle[KELPCLIMBER_MAX_OBSTACLE_COUNT] kelpclimberObstacles;
int kelpclimberObstacleIndex;
float kelpclimberNextObstacleDist;

struct KelpclimberOtter {
  Vector pos;
  int grabbedKelpIndex;
};
KelpclimberOtter kelpclimberOtter;

void kelpclimberUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(kelpclimberKelpStrands);
    vectorSet(&kelpclimberKelpStrands[0].pos, 25, 40);
    kelpclimberKelpStrands[0].length = KELPCLIMBER_SCREEN_HEIGHT;
    kelpclimberKelpStrands[0].isAlive = true;
    vectorSet(&kelpclimberKelpStrands[1].pos, 50, 50);
    kelpclimberKelpStrands[1].length = KELPCLIMBER_SCREEN_HEIGHT;
    kelpclimberKelpStrands[1].isAlive = true;
    vectorSet(&kelpclimberKelpStrands[2].pos, 75, 45);
    kelpclimberKelpStrands[2].length = KELPCLIMBER_SCREEN_HEIGHT;
    kelpclimberKelpStrands[2].isAlive = true;
    kelpclimberKelpIndex = 3;
    kelpclimberNextKelpStandDist = 0;
    vectorSet(&kelpclimberOtter.pos, kelpclimberKelpStrands[1].pos.x, 90);
    kelpclimberOtter.grabbedKelpIndex = 1;
    INIT_UNALIVED_ARRAY_FAST(kelpclimberObstacles);
    kelpclimberObstacleIndex = 0;
    kelpclimberNextObstacleDist = 0;
  }
  float scrollSpeed = 0.5 * difficulty;
  int nearestKelpIndex = -1;
  float minDistance = 99;
  FOR_EACH(kelpclimberKelpStrands, i) {
    ASSIGN_ARRAY_ITEM(kelpclimberKelpStrands, i, KelpclimberKelp, kelp);
    SKIP_IS_NOT_ALIVE(kelp);
    if (i != kelpclimberOtter.grabbedKelpIndex && kelp->pos.y > 40) {
      float distance = fabs(kelp->pos.x - kelpclimberOtter.pos.x);
      if (distance < minDistance) {
        minDistance = distance;
        nearestKelpIndex = i;
      }
    }
  }
  if (input.isJustPressed) {
    if (nearestKelpIndex >= 0) {
      KelpclimberKelp* nearestKelp = &kelpclimberKelpStrands[nearestKelpIndex];
      play(JUMP);
      addScore(ceil(fabs(nearestKelp->pos.x - kelpclimberOtter.pos.x)), kelpclimberOtter.pos.x,
               kelpclimberOtter.pos.y);
      kelpclimberKelpStrands[kelpclimberOtter.grabbedKelpIndex].pos.y = 199;
      kelpclimberOtter.grabbedKelpIndex = nearestKelpIndex;
      kelpclimberOtter.pos.x = nearestKelp->pos.x;
    }
  }
  FOR_EACH(kelpclimberKelpStrands, i) {
    ASSIGN_ARRAY_ITEM(kelpclimberKelpStrands, i, KelpclimberKelp, kelp);
    SKIP_IS_NOT_ALIVE(kelp);
    kelp->pos.y += scrollSpeed;
    if (kelp->pos.y > KELPCLIMBER_SCREEN_HEIGHT + 50) {
      kelp->isAlive = false;
    }
  }
  kelpclimberNextKelpStandDist -= scrollSpeed;
  if (kelpclimberNextKelpStandDist < 0) {
    play(CLICK);
    ASSIGN_ARRAY_ITEM(kelpclimberKelpStrands, kelpclimberKelpIndex, KelpclimberKelp, nk);
    vectorSet(&nk->pos, rnd(10, 90), -50);
    nk->length = KELPCLIMBER_SCREEN_HEIGHT;
    nk->isAlive = true;
    kelpclimberKelpIndex = cgl_wrap(kelpclimberKelpIndex + 1, 0, KELPCLIMBER_MAX_KELP_COUNT);
    kelpclimberNextKelpStandDist += rnd(30, 50);
  }
  FOR_EACH(kelpclimberObstacles, i) {
    ASSIGN_ARRAY_ITEM(kelpclimberObstacles, i, KelpclimberObstacle, obstacle);
    SKIP_IS_NOT_ALIVE(obstacle);
    obstacle->pos.y += scrollSpeed;
    if (obstacle->pos.y > KELPCLIMBER_SCREEN_HEIGHT + 10) {
      obstacle->isAlive = false;
    }
  }
  kelpclimberNextObstacleDist -= scrollSpeed;
  if (kelpclimberNextObstacleDist < 0) {
    play(HIT);
    ASSIGN_ARRAY_ITEM(kelpclimberObstacles, kelpclimberObstacleIndex, KelpclimberObstacle, nobs);
    vectorSet(&nobs->pos, rnd(10, 90), -10);
    vectorSet(&nobs->size, rnd(5, 15), rnd(5, 15));
    nobs->isAlive = true;
    kelpclimberObstacleIndex =
        cgl_wrap(kelpclimberObstacleIndex + 1, 0, KELPCLIMBER_MAX_OBSTACLE_COUNT);
    kelpclimberNextObstacleDist += rnd(40, 50);
  }
  color = GREEN;
  FOR_EACH(kelpclimberKelpStrands, i) {
    ASSIGN_ARRAY_ITEM(kelpclimberKelpStrands, i, KelpclimberKelp, kelp);
    SKIP_IS_NOT_ALIVE(kelp);
    box(kelp->pos.x, kelp->pos.y, 3, kelp->length, &scratch);
  }
  color = RED;
  FOR_EACH(kelpclimberObstacles, i) {
    ASSIGN_ARRAY_ITEM(kelpclimberObstacles, i, KelpclimberObstacle, obstacle);
    SKIP_IS_NOT_ALIVE(obstacle);
    box(obstacle->pos.x, obstacle->pos.y, obstacle->size.x, obstacle->size.y, &scratch);
  }
  color = YELLOW;
  box(kelpclimberOtter.pos.x, kelpclimberOtter.pos.y, KELPCLIMBER_OTTER_SIZE,
      KELPCLIMBER_OTTER_SIZE, &scratch);
  if (scratch.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  if (nearestKelpIndex >= 0) {
    color = LIGHT_CYAN;
    box(kelpclimberKelpStrands[nearestKelpIndex].pos.x, 90, KELPCLIMBER_OTTER_SIZE - 2,
        KELPCLIMBER_OTTER_SIZE - 2, &scratch);
  }
}

void addGameKelpclimber() {
  addGame(kelpclimberTitle, kelpclimberDescription, kelpclimberCharacters,
          kelpclimberCharactersCount, &kelpclimberOptions, false,
          &kelpclimberUpdate);
}
