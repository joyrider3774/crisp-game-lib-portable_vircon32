#include "../cglp.h"

int* periscopepingTitle = "PERISCOPE PING";
int* periscopepingDescription = "[Hold] Ping\n[Tap]  Fire & Turn";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] periscopepingCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int periscopepingCharactersCount = 0;

Options periscopepingOptions = {100, 100, 7, true};

#define PERISCOPEPING_PLAY_AREA_RADIUS 50
#define PERISCOPEPING_ROTATION_SPEED 0.04
#define PERISCOPEPING_OBJECT_MOVE_SPEED 0.1
#define PERISCOPEPING_SHOT_SPEED 1
#define PERISCOPEPING_TYPE_CIRCLE 0
#define PERISCOPEPING_TYPE_TRIANGLE 1

struct PeriscopepingPeriscope {
  float angle;
  float pingRadius;
  float angleVel;
};
PeriscopepingPeriscope periscopepingPeriscope;
Vector periscopepingTowerPos;

struct PeriscopepingObject {
  Vector pos;
  bool isHidden;
  int type;
  float angle;
  bool isAlive;
};
// Concurrent objects ~= life/spawn-interval = (450/sqrt(difficulty)) /
// (199/difficulty) = ~2.26*sqrt(difficulty) - unbounded growth; already
// exceeds 16 by ~difficulty 50 (under an hour of play).
#define PERISCOPEPING_MAX_OBJECT_COUNT 256
PeriscopepingObject[PERISCOPEPING_MAX_OBJECT_COUNT] periscopepingObjects;
int periscopepingObjectIndex;

struct PeriscopepingShot {
  Vector pos;
  float angle;
  float speed;
  bool isAlive;
};
#define PERISCOPEPING_MAX_SHOT_COUNT 16
PeriscopepingShot[PERISCOPEPING_MAX_SHOT_COUNT] periscopepingShots;
int periscopepingShotIndex;

// Vircon32 port note: the JS version never resets this in its "if
// (!ticks)" block either (only initialized once at module scope) - kept
// as a persistent global that's simply never reset on replay, matching
// upstream behavior exactly rather than "fixing" what looks like a minor
// oversight in the original.
float periscopepingSpawnTimer;

void periscopepingFireShot(float x, float y, float angle) {
  ASSIGN_ARRAY_ITEM(periscopepingShots, periscopepingShotIndex, PeriscopepingShot, ns);
  vectorSet(&ns->pos, x, y);
  ns->angle = angle;
  ns->speed = PERISCOPEPING_SHOT_SPEED;
  ns->isAlive = true;
  periscopepingShotIndex = cgl_wrap(periscopepingShotIndex + 1, 0, PERISCOPEPING_MAX_SHOT_COUNT);
}

void periscopepingSpawnObject() {
  float angle = rnd(0, CGLP_PI * 2);
  Vector pos;
  pos = periscopepingTowerPos;
  vectorAdd(&pos, cos(angle) * PERISCOPEPING_PLAY_AREA_RADIUS,
            sin(angle) * PERISCOPEPING_PLAY_AREA_RADIUS);
  ASSIGN_ARRAY_ITEM(periscopepingObjects, periscopepingObjectIndex, PeriscopepingObject, no);
  no->pos = pos;
  no->isHidden = true;
  if (rnd(0, 1) < 0.5) {
    no->type = PERISCOPEPING_TYPE_CIRCLE;
  } else {
    no->type = PERISCOPEPING_TYPE_TRIANGLE;
  }
  no->angle = angle + CGLP_PI + rnd(0, 0.5) * RNDPM();
  no->isAlive = true;
  periscopepingObjectIndex =
      cgl_wrap(periscopepingObjectIndex + 1, 0, PERISCOPEPING_MAX_OBJECT_COUNT);
}

void periscopepingUpdate() {
  Collision scratch;
  if (!ticks) {
    periscopepingPeriscope.angle = 0;
    periscopepingPeriscope.pingRadius = 0;
    periscopepingPeriscope.angleVel = 1;
    vectorSet(&periscopepingTowerPos, 50, 50);
    INIT_UNALIVED_ARRAY_FAST(periscopepingObjects);
    periscopepingObjectIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(periscopepingShots);
    periscopepingShotIndex = 0;
  }
  color = LIGHT_BLACK;
  thickness = 3;
  arc(periscopepingTowerPos.x, periscopepingTowerPos.y, PERISCOPEPING_PLAY_AREA_RADIUS, 0,
      CGLP_PI * 2, &scratch);
  periscopepingPeriscope.angle +=
      PERISCOPEPING_ROTATION_SPEED * periscopepingPeriscope.angleVel * sqrt(difficulty);
  Vector periscopePos;
  periscopePos = periscopepingTowerPos;
  vectorAdd(&periscopePos, cos(periscopepingPeriscope.angle) * 10,
            sin(periscopepingPeriscope.angle) * 10);
  color = BLUE;
  box(periscopePos.x, periscopePos.y, 3, 3, &scratch);
  if (input.isJustPressed) {
    play(LASER);
    periscopepingPeriscope.pingRadius = 0;
    periscopepingPeriscope.angleVel *= -1;
    periscopepingFireShot(periscopePos.x, periscopePos.y, periscopepingPeriscope.angle);
  }
  if (input.isPressed) {
    periscopepingPeriscope.pingRadius += 1;
    color = LIGHT_BLUE;
    thickness = 5;
    arc(periscopePos.x, periscopePos.y, periscopepingPeriscope.pingRadius, 0, CGLP_PI * 2,
        &scratch);
  }
  color = BLACK;
  box(periscopepingTowerPos.x, periscopepingTowerPos.y, 10, 10, &scratch);
  FOR_EACH(periscopepingShots, i) {
    ASSIGN_ARRAY_ITEM(periscopepingShots, i, PeriscopepingShot, shot);
    SKIP_IS_NOT_ALIVE(shot);
    addWithAngle(&shot->pos, shot->angle, shot->speed * sqrt(difficulty));
    color = CYAN;
    thickness = 3;
    barCenterPosRatio = 0.5;
    bar(shot->pos.x, shot->pos.y, 3, shot->angle, &scratch);
    if (distanceTo(&shot->pos, periscopepingTowerPos.x, periscopepingTowerPos.y) >
        PERISCOPEPING_PLAY_AREA_RADIUS) {
      shot->isAlive = false;
      continue;
    }
  }
  FOR_EACH(periscopepingObjects, i) {
    ASSIGN_ARRAY_ITEM(periscopepingObjects, i, PeriscopepingObject, obj);
    SKIP_IS_NOT_ALIVE(obj);
    addWithAngle(&obj->pos, obj->angle, PERISCOPEPING_OBJECT_MOVE_SPEED * sqrt(difficulty));
    if (distanceTo(&obj->pos, periscopepingTowerPos.x, periscopepingTowerPos.y) < 5) {
      obj->isAlive = false;
      continue;
    }
    color = TRANSPARENT;
    if (obj->type == PERISCOPEPING_TYPE_CIRCLE) {
      thickness = 3;
      arc(obj->pos.x, obj->pos.y, 2, 0, CGLP_PI * 2, &scratch);
    } else {
      thickness = 4;
      barCenterPosRatio = 0.5;
      bar(obj->pos.x, obj->pos.y, 4, (ticks * 10 % 360) * CGLP_PI / 180, &scratch);
    }
    if (scratch.isColliding.rect[CYAN]) {
      color = RED;
      play(HIT);
      particle(obj->pos.x, obj->pos.y, 20, 1, 0, CGLP_PI * 2);
      addScore(floor(distanceTo(&obj->pos, periscopepingTowerPos.x, periscopepingTowerPos.y)),
               obj->pos.x, obj->pos.y);
      obj->isAlive = false;
      continue;
    } else if (scratch.isColliding.rect[LIGHT_BLUE] || scratch.isColliding.rect[BLUE] ||
               scratch.isColliding.rect[BLACK]) {
      color = RED;
      if (obj->type == PERISCOPEPING_TYPE_CIRCLE) {
        arc(obj->pos.x, obj->pos.y, 2, 0, CGLP_PI * 2, &scratch);
      } else {
        bar(obj->pos.x, obj->pos.y, 4, (ticks * 10 % 360) * CGLP_PI / 180, &scratch);
      }
      if (obj->isHidden) {
        play(COIN);
        obj->isHidden = false;
      }
      if (scratch.isColliding.rect[BLUE] || scratch.isColliding.rect[BLACK]) {
        play(EXPLOSION);
        gameOver();
      }
    } else {
      obj->isHidden = true;
    }
  }
  periscopepingSpawnTimer += difficulty;
  COUNT_IS_ALIVE(periscopepingObjects, aliveObjectCount);
  if (aliveObjectCount == 0 || periscopepingSpawnTimer > 199) {
    periscopepingSpawnTimer = 0;
    periscopepingSpawnObject();
  }
}

void addGamePeriscopeping() {
  addGame(periscopepingTitle, periscopepingDescription, periscopepingCharacters,
          periscopepingCharactersCount, &periscopepingOptions, false,
          &periscopepingUpdate);
}
