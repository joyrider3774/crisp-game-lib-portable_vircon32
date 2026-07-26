#include "../cglp.h"

int* tornadotwisterTitle = "TORNADO TWISTER";
int* tornadotwisterDescription = "[Tap]\n Change direction\n[Hold]\n Shrink & slow down";

// Vircon32 port note: upstream's characters array is empty (only box() is
// used to draw everything, no character() calls) - blank dummy entry,
// charactersCount = 0, matching the convention in gamePinClimb.c etc.
int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tornadotwisterCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int tornadotwisterCharactersCount = 0;

// "shapeDark" theme -> isDarkColor = true.
Options tornadotwisterOptions = {150, 100, 6, true};

#define TORNADOTWISTER_PARTICLE_COUNT 20
struct TornadotwisterTornado {
  Vector pos;
  float size;
  float currentSize;
  float direction;
  float speed;
  Vector[TORNADOTWISTER_PARTICLE_COUNT] particles;
};
TornadotwisterTornado tornadotwisterTornado;

#define TORNADOTWISTER_TYPE_TREE 0
#define TORNADOTWISTER_TYPE_HOUSE 1
#define TORNADOTWISTER_TYPE_CAR 2

struct TornadotwisterSuckable {
  Vector pos;
  int type;
  float size;
  float angle;
  float distance;
  bool isAlive;
};
#define TORNADOTWISTER_MAX_SUCKABLE_COUNT 16
TornadotwisterSuckable[TORNADOTWISTER_MAX_SUCKABLE_COUNT] tornadotwisterSuckableObjects;
int tornadotwisterSuckableIndex;

struct TornadotwisterObstacle {
  Vector pos;
  float width;
  float height;
  bool isAlive;
};
#define TORNADOTWISTER_MAX_OBSTACLE_COUNT 16
TornadotwisterObstacle[TORNADOTWISTER_MAX_OBSTACLE_COUNT] tornadotwisterObstacles;
int tornadotwisterObstacleIndex;

float tornadotwisterNextObstacleTicks;
float tornadotwisterScrollSpeed;
// Vircon32 port note: upstream's growthRate is assigned at init but never
// read anywhere else in update() - dead state in the original game itself,
// kept here only so this port's init block mirrors upstream's 1:1.
float tornadotwisterGrowthRate;
float tornadotwisterSpawnTimer;

void tornadotwisterUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&tornadotwisterTornado.pos, 20, 75);
    tornadotwisterTornado.size = 7;
    tornadotwisterTornado.currentSize = 7;
    tornadotwisterTornado.direction = 1;
    tornadotwisterTornado.speed = 1;
    TIMES(TORNADOTWISTER_PARTICLE_COUNT, particleIdx) {
      vectorSet(&tornadotwisterTornado.particles[particleIdx], rnd(0, 10) * RNDPM(), rnd(0, 10) * RNDPM());
    }
    INIT_UNALIVED_ARRAY_FAST(tornadotwisterSuckableObjects);
    tornadotwisterSuckableIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(tornadotwisterObstacles);
    tornadotwisterObstacleIndex = 0;
    tornadotwisterNextObstacleTicks = 0;
    tornadotwisterScrollSpeed = 1;
    tornadotwisterGrowthRate = 0.01;
    tornadotwisterSpawnTimer = 0;
  }

  float sd = sqrt(difficulty);
  tornadotwisterScrollSpeed = sd;
  if (input.isJustPressed) {
    tornadotwisterTornado.direction *= -1;
    play(LASER);
  }
  if (input.isPressed) {
    tornadotwisterTornado.currentSize += (3 - tornadotwisterTornado.currentSize) * 0.1;
    tornadotwisterTornado.speed += (0.1 - tornadotwisterTornado.speed) * 0.1;
  } else {
    tornadotwisterTornado.currentSize +=
        (tornadotwisterTornado.size - tornadotwisterTornado.currentSize) * 0.03;
    tornadotwisterTornado.speed += (1 - tornadotwisterTornado.speed) * 0.1;
  }
  tornadotwisterTornado.pos.y += tornadotwisterTornado.direction * tornadotwisterTornado.speed * sd;
  if (tornadotwisterTornado.pos.y <= tornadotwisterTornado.currentSize &&
      tornadotwisterTornado.direction < 0) {
    tornadotwisterTornado.direction = 1;
  }
  if (tornadotwisterTornado.pos.y >= 100 - tornadotwisterTornado.currentSize &&
      tornadotwisterTornado.direction > 0) {
    tornadotwisterTornado.direction = -1;
  }

  color = CYAN;
  TIMES(TORNADOTWISTER_PARTICLE_COUNT, pi2) {
    rotate(&tornadotwisterTornado.particles[pi2], 0.15 * (1 - (float)pi2 / TORNADOTWISTER_PARTICLE_COUNT) * sd);
    Vector ppos;
    ppos = tornadotwisterTornado.particles[pi2];
    vectorMul(&ppos, tornadotwisterTornado.currentSize / 10);
    vectorAdd(&ppos, tornadotwisterTornado.pos.x, tornadotwisterTornado.pos.y);
    box(ppos.x, ppos.y, sqrt(tornadotwisterTornado.currentSize), sqrt(tornadotwisterTornado.currentSize),
        &scratch);
  }

  tornadotwisterSpawnTimer -= sd;
  if (tornadotwisterSpawnTimer <= 0) {
    int type;
    float size;
    if (rnd(0, 1) < 0.5) {
      type = TORNADOTWISTER_TYPE_TREE;
      size = 3;
    } else if (rnd(0, 1) < 0.7) {
      type = TORNADOTWISTER_TYPE_HOUSE;
      size = 5;
    } else {
      type = TORNADOTWISTER_TYPE_CAR;
      size = 4;
    }
    ASSIGN_ARRAY_ITEM(tornadotwisterSuckableObjects, tornadotwisterSuckableIndex, TornadotwisterSuckable, ns);
    vectorSet(&ns->pos, 160, rnd(10, 90));
    ns->type = type;
    ns->size = size;
    ns->angle = 0;
    ns->distance = 0;
    ns->isAlive = true;
    tornadotwisterSuckableIndex =
        cgl_wrap(tornadotwisterSuckableIndex + 1, 0, TORNADOTWISTER_MAX_SUCKABLE_COUNT);
    tornadotwisterSpawnTimer = rnd(30, 50);
  }

  FOR_EACH(tornadotwisterSuckableObjects, si) {
    ASSIGN_ARRAY_ITEM(tornadotwisterSuckableObjects, si, TornadotwisterSuckable, obj);
    SKIP_IS_NOT_ALIVE(obj);
    if (obj->distance > 0) {
      obj->distance -= 1;
      obj->angle += 0.2;
      Vector offset;
      vectorSet(&offset, obj->distance, 0);
      rotate(&offset, obj->angle);
      obj->pos = tornadotwisterTornado.pos;
      vectorAdd(&obj->pos, offset.x, offset.y);
      if (obj->distance <= 0) {
        tornadotwisterTornado.size += obj->size * 0.05;
        addScore(floor(obj->size * tornadotwisterTornado.currentSize), tornadotwisterTornado.pos.x,
                  tornadotwisterTornado.pos.y);
        play(POWER_UP);
        obj->isAlive = false;
        continue;
      }
    } else {
      obj->pos.x -= tornadotwisterScrollSpeed;
    }
    if (obj->type == TORNADOTWISTER_TYPE_TREE) {
      color = GREEN;
    } else if (obj->type == TORNADOTWISTER_TYPE_HOUSE) {
      color = YELLOW;
    } else {
      color = RED;
    }
    Collision oc;
    box(obj->pos.x, obj->pos.y, obj->size, obj->size, &oc);
    bool isCollidingTornado = oc.isColliding.rect[CYAN];
    if (isCollidingTornado && obj->distance == 0) {
      obj->distance = distanceTo(&obj->pos, tornadotwisterTornado.pos.x, tornadotwisterTornado.pos.y) * 1.5;
      obj->angle = angleTo(&tornadotwisterTornado.pos, obj->pos.x, obj->pos.y);
    }
    obj->isAlive = obj->pos.x >= -obj->size;
  }

  tornadotwisterNextObstacleTicks -= sd;
  if (tornadotwisterNextObstacleTicks < 0) {
    float width = rnd(20, 30);
    ASSIGN_ARRAY_ITEM(tornadotwisterObstacles, tornadotwisterObstacleIndex, TornadotwisterObstacle, no);
    vectorSet(&no->pos, 150 + width / 2, rnd(10, 90));
    no->width = width;
    no->height = rnd(10, 20);
    no->isAlive = true;
    tornadotwisterObstacleIndex =
        cgl_wrap(tornadotwisterObstacleIndex + 1, 0, TORNADOTWISTER_MAX_OBSTACLE_COUNT);
    tornadotwisterNextObstacleTicks += rnd(50, 150);
  }

  FOR_EACH(tornadotwisterObstacles, oi) {
    ASSIGN_ARRAY_ITEM(tornadotwisterObstacles, oi, TornadotwisterObstacle, obs);
    SKIP_IS_NOT_ALIVE(obs);
    obs->pos.x -= tornadotwisterScrollSpeed;
    color = BLACK;
    Collision obc;
    box(obs->pos.x, obs->pos.y, obs->width, obs->height, &obc);
    if (obc.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    }
    obs->isAlive = obs->pos.x > -obs->width;
  }
}

void addGameTornadotwister() {
  addGame(tornadotwisterTitle, tornadotwisterDescription, tornadotwisterCharacters,
          tornadotwisterCharactersCount, &tornadotwisterOptions, false, &tornadotwisterUpdate);
}
