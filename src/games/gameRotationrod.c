#include "../cglp.h"

int* rotationrodTitle = "ROTATION ROD";
int* rotationrodDescription = "[Tap]\n Turn";

// Vircon32 port note: upstream's characters array is empty (this game draws
// only bar()/box()/line()/text(), never character()) - a single blank dummy
// entry with charactersCount = 0 matches the convention already used by
// other character-less ports in this project (see gamePinClimb.c/
// gameJujump.c).
int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rotationrodCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int rotationrodCharactersCount = 0;

Options rotationrodOptions = {100, 100, 7, false};

struct RotationrodPlayer {
  Vector center;
  float length;
  float angle;
  float rotationSpeed;
};
RotationrodPlayer rotationrodPlayer;

struct RotationrodObstacle {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define ROTATIONROD_MAX_OBSTACLE_COUNT 16
RotationrodObstacle[ROTATIONROD_MAX_OBSTACLE_COUNT] rotationrodObstacles;
int rotationrodObstacleIndex;

struct RotationrodCollectible {
  Vector pos;
  Vector vel;
  bool isAlive;
};
// Spawn rate scales with difficulty while lifetime only shrinks by sqrt(difficulty), so the
// number alive at once keeps growing over a long session (~25 * sqrt(difficulty)) - sized
// generously rather than tightly to avoid the ring buffer overwriting still-alive collectibles.
#define ROTATIONROD_MAX_COLLECTIBLE_COUNT 256
RotationrodCollectible[ROTATIONROD_MAX_COLLECTIBLE_COUNT] rotationrodCollectibles;
int rotationrodCollectibleIndex;

float rotationrodNextObstacleTicks;
// Vircon32 port note: upstream's lastObstacleSpawn is the {pos, vel} object
// most recently produced by spawnAtEdge(), reused later (unchanged) to spawn
// a collectible at the exact same edge point/velocity. A plain pair of
// globals captures that with no array/reference needed.
Vector rotationrodLastSpawnPos;
Vector rotationrodLastSpawnVel;
float rotationrodNextCollectibleTicks;
float rotationrodMultiplier;

// Vircon32 port note: upstream's spawnAtEdge() returns a {pos, vel} object -
// over the one-word function-return limit, so it becomes an out-pointer
// pair. The original switch(side) is rewritten as an if/else chain (no
// switch statement in this dialect).
void rotationrodSpawnAtEdge(float speed, Vector* pos, Vector* vel) {
  int side = rndi(0, 4);
  if (side == 0) {
    if (rnd(0, 1) < 0.5) {
      vectorSet(pos, rnd(20, 40), -3);
    } else {
      vectorSet(pos, rnd(60, 80), -3);
    }
    vectorSet(vel, 0, speed);
  } else if (side == 1) {
    if (rnd(0, 1) < 0.5) {
      vectorSet(pos, 103, rnd(20, 40));
    } else {
      vectorSet(pos, 103, rnd(60, 80));
    }
    vectorSet(vel, -speed, 0);
  } else if (side == 2) {
    if (rnd(0, 1) < 0.5) {
      vectorSet(pos, rnd(20, 40), 103);
    } else {
      vectorSet(pos, rnd(60, 80), 103);
    }
    vectorSet(vel, 0, -speed);
  } else {
    if (rnd(0, 1) < 0.5) {
      vectorSet(pos, -3, rnd(20, 40));
    } else {
      vectorSet(pos, -3, rnd(60, 80));
    }
    vectorSet(vel, speed, 0);
  }
}

void rotationrodUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&rotationrodPlayer.center, 50, 50);
    rotationrodPlayer.length = 48;
    rotationrodPlayer.angle = 0;
    rotationrodPlayer.rotationSpeed = 0.05;
    INIT_UNALIVED_ARRAY_FAST(rotationrodObstacles);
    rotationrodObstacleIndex = 0;
    rotationrodNextObstacleTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(rotationrodCollectibles);
    rotationrodCollectibleIndex = 0;
    rotationrodNextCollectibleTicks = 0;
    rotationrodMultiplier = 1;
  }
  float sd = sqrt(difficulty);
  if (input.isJustPressed) {
    rotationrodPlayer.rotationSpeed *= -1;
    play(SELECT);
  }
  if (!input.isPressed) {
    rotationrodPlayer.angle += rotationrodPlayer.rotationSpeed * sd;
  }
  color = BLUE;
  thickness = 2;
  bar(rotationrodPlayer.center.x, rotationrodPlayer.center.y, rotationrodPlayer.length,
      rotationrodPlayer.angle, &scratch);
  Vector ap1;
  ap1 = rotationrodPlayer.center;
  addWithAngle(&ap1, rotationrodPlayer.angle, rotationrodPlayer.length * 0.48);
  Vector ap2;
  ap2 = rotationrodPlayer.center;
  float ap2Angle;
  if (rotationrodPlayer.rotationSpeed > 0) {
    ap2Angle = rotationrodPlayer.angle + 0.2;
  } else {
    ap2Angle = rotationrodPlayer.angle - 0.2;
  }
  addWithAngle(&ap2, ap2Angle, rotationrodPlayer.length * 0.42);
  thickness = 3;
  line(ap1.x, ap1.y, ap2.x, ap2.y, &scratch);

  rotationrodNextObstacleTicks -= sd;
  if (rotationrodNextObstacleTicks < 0) {
    play(LASER);
    rotationrodSpawnAtEdge(0.5 * sd, &rotationrodLastSpawnPos, &rotationrodLastSpawnVel);
    ASSIGN_ARRAY_ITEM(rotationrodObstacles, rotationrodObstacleIndex, RotationrodObstacle, no);
    no->pos = rotationrodLastSpawnPos;
    no->vel = rotationrodLastSpawnVel;
    no->isAlive = true;
    rotationrodObstacleIndex =
        cgl_wrap(rotationrodObstacleIndex + 1, 0, ROTATIONROD_MAX_OBSTACLE_COUNT);
    rotationrodNextObstacleTicks = 99;
    rotationrodNextCollectibleTicks = 9;
  }
  color = RED;
  FOR_EACH(rotationrodObstacles, oi) {
    ASSIGN_ARRAY_ITEM(rotationrodObstacles, oi, RotationrodObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    vectorAdd(&o->pos, o->vel.x, o->vel.y);
    box(o->pos.x, o->pos.y, 5, 5, &scratch);
    if (scratch.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
    o->isAlive = o->pos.x >= -5 && o->pos.x <= 110 && o->pos.y >= -5 && o->pos.y <= 110;
  }

  rotationrodNextCollectibleTicks -= difficulty;
  if (rotationrodNextCollectibleTicks < 0) {
    ASSIGN_ARRAY_ITEM(rotationrodCollectibles, rotationrodCollectibleIndex, RotationrodCollectible, nc);
    nc->pos = rotationrodLastSpawnPos;
    nc->vel = rotationrodLastSpawnVel;
    nc->isAlive = true;
    rotationrodCollectibleIndex =
        cgl_wrap(rotationrodCollectibleIndex + 1, 0, ROTATIONROD_MAX_COLLECTIBLE_COUNT);
    rotationrodNextCollectibleTicks = 9;
  }
  color = YELLOW;
  FOR_EACH(rotationrodCollectibles, ci) {
    ASSIGN_ARRAY_ITEM(rotationrodCollectibles, ci, RotationrodCollectible, c);
    SKIP_IS_NOT_ALIVE(c);
    vectorAdd(&c->pos, c->vel.x, c->vel.y);
    box(c->pos.x, c->pos.y, 3, 3, &scratch);
    bool isColliding = scratch.isColliding.rect[BLUE];
    if (isColliding) {
      play(COIN);
      addScore(ceil(rotationrodMultiplier), c->pos.x, c->pos.y);
      rotationrodMultiplier += 10;
    }
    bool inRect = c->pos.x >= -5 && c->pos.x <= 110 && c->pos.y >= -5 && c->pos.y <= 110;
    c->isAlive = !isColliding && inRect;
  }

  rotationrodMultiplier *= 0.99;
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar((int)ceil(rotationrodMultiplier)));
  text(multText, 2, 10, &scratch);
}

void addGameRotationrod() {
  addGame(rotationrodTitle, rotationrodDescription, rotationrodCharacters,
          rotationrodCharactersCount, &rotationrodOptions, false, &rotationrodUpdate);
}
