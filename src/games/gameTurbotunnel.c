#include "../cglp.h"

int* turbotunnelTitle = "TURBO TUNNEL";
int* turbotunnelDescription = "[Tap]\n Turn";
int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] turbotunnelCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int turbotunnelCharactersCount = 1;

Options turbotunnelOptions = {100, 100, 2, false};

#define TURBOTUNNEL_TUNNEL_RADIUS 30
#define TURBOTUNNEL_TUNNEL_WIDTH 40

struct TurbotunnelCar {
  float angle;
  Vector pos;
  float radius;
  float vr;
};
TurbotunnelCar turbotunnelCar;

struct TurbotunnelObstacle {
  float angle;
  float va;
  float radius;
  float destroyedTicks;
  Vector pos;
  bool isAlive;
};
#define TURBOTUNNEL_MAX_OBSTACLE_COUNT 64
TurbotunnelObstacle[TURBOTUNNEL_MAX_OBSTACLE_COUNT] turbotunnelObstacles;
int turbotunnelObstacleIndex;

float turbotunnelMultiplier;

// Shared helper for the repeated upstream `vec(50, 50).add(vec(radius,
// 0).rotate(angle))` idiom - a point at `radius` from the tunnel's (50, 50)
// center, at `angle`.
void turbotunnelPosOnRadius(float radius, float angle, Vector* out) {
  vectorSet(out, radius, 0);
  rotate(out, angle);
  vectorAdd(out, 50, 50);
}

void turbotunnelUpdate() {
  Collision scratch;
  if (!ticks) {
    turbotunnelCar.angle = CGLP_PI / 2;
    vectorSet(&turbotunnelCar.pos, 0, 0);
    turbotunnelCar.radius = TURBOTUNNEL_TUNNEL_RADIUS;
    turbotunnelCar.vr = 1;
    INIT_UNALIVED_ARRAY_FAST(turbotunnelObstacles);
    turbotunnelObstacleIndex = 0;
    turbotunnelMultiplier = 1;
  }
  bool isSpawningObstacle = false;
  if (input.isJustPressed) {
    play(LASER);
    turbotunnelCar.vr *= -1;
    isSpawningObstacle = true;
  }
  turbotunnelCar.angle += 0.015 * difficulty / (turbotunnelCar.radius * 0.03);
  turbotunnelCar.radius += turbotunnelCar.vr * 0.4;
  turbotunnelPosOnRadius(turbotunnelCar.radius, turbotunnelCar.angle, &turbotunnelCar.pos);
  float distanceFromCenter = distanceTo(&turbotunnelCar.pos, 50, 50);
  if ((distanceFromCenter > TURBOTUNNEL_TUNNEL_RADIUS + TURBOTUNNEL_TUNNEL_WIDTH / 2 && turbotunnelCar.vr > 0) ||
      (distanceFromCenter < TURBOTUNNEL_TUNNEL_RADIUS - TURBOTUNNEL_TUNNEL_WIDTH / 2 && turbotunnelCar.vr < 0)) {
    turbotunnelCar.vr *= -1;
    isSpawningObstacle = true;
  }
  color = BLUE;
  thickness = 4;
  arc(50, 50, TURBOTUNNEL_TUNNEL_RADIUS + TURBOTUNNEL_TUNNEL_WIDTH / 2, 0, CGLP_PI * 2, &scratch);
  thickness = 4;
  arc(50, 50, TURBOTUNNEL_TUNNEL_RADIUS - TURBOTUNNEL_TUNNEL_WIDTH / 2, 0, CGLP_PI * 2, &scratch);
  color = LIGHT_RED;
  thickness = 3;
  bar(turbotunnelCar.pos.x, turbotunnelCar.pos.y, 1, turbotunnelCar.angle + CGLP_PI / 2, &scratch);
  color = RED;
  box(turbotunnelCar.pos.x, turbotunnelCar.pos.y, 1, 1, &scratch);

  bool hasDestroyed = false;
  FOR_EACH(turbotunnelObstacles, oi) {
    ASSIGN_ARRAY_ITEM(turbotunnelObstacles, oi, TurbotunnelObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->angle += o->va / (o->radius * 0.03);
    turbotunnelPosOnRadius(o->radius, o->angle, &o->pos);
    if (o->destroyedTicks > 0) {
      hasDestroyed = true;
      o->destroyedTicks -= difficulty;
      if (o->destroyedTicks <= 0) {
        play(POWER_UP);
        addScore(turbotunnelMultiplier, o->pos.x, o->pos.y);
        turbotunnelMultiplier++;
        particle(o->pos.x, o->pos.y, 9, 3, 0, CGLP_PI * 2);
        o->isAlive = false;
        continue;
      }
    }
    if (o->destroyedTicks > 0) {
      color = PURPLE;
    } else {
      color = YELLOW;
    }
    float a = o->angle + CGLP_PI / 2 + (o->destroyedTicks + 1) * 0.5;
    thickness = 3;
    Collision oc;
    bar(o->pos.x, o->pos.y, 1, a, &oc);
    if (o->destroyedTicks < 0 && oc.isColliding.rect[YELLOW]) {
      o->destroyedTicks = 30;
    }
    if (o->destroyedTicks < 0 && oc.isColliding.rect[RED]) {
      play(EXPLOSION);
      gameOver();
    }
  }

  color = TRANSPARENT;
  FOR_EACH(turbotunnelObstacles, oi2) {
    ASSIGN_ARRAY_ITEM(turbotunnelObstacles, oi2, TurbotunnelObstacle, o2);
    SKIP_IS_NOT_ALIVE(o2);
    Collision tc;
    box(o2->pos.x, o2->pos.y, 5, 5, &tc);
    if (o2->destroyedTicks < 0 && tc.isColliding.rect[PURPLE]) {
      play(COIN);
      o2->destroyedTicks = 30;
      hasDestroyed = true;
    }
  }
  if (!hasDestroyed) {
    turbotunnelMultiplier = 1;
  }
  if (isSpawningObstacle) {
    float oAngle = turbotunnelCar.angle + (CGLP_PI * 2 - 3 / turbotunnelCar.radius);
    float oVa = -rnd(0.01, 0.03) * difficulty;
    float oRadius = turbotunnelCar.radius;
    Vector oPos;
    turbotunnelPosOnRadius(oRadius, oAngle, &oPos);
    color = TRANSPARENT;
    Collision spawnCheck;
    box(oPos.x, oPos.y, 9, 9, &spawnCheck);
    if (!spawnCheck.isColliding.rect[YELLOW] && !spawnCheck.isColliding.rect[PURPLE]) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(turbotunnelObstacles, turbotunnelObstacleIndex, TurbotunnelObstacle, no);
      no->angle = oAngle;
      no->va = oVa;
      no->radius = oRadius;
      no->destroyedTicks = -1;
      no->pos = oPos;
      no->isAlive = true;
      turbotunnelObstacleIndex = cgl_wrap(turbotunnelObstacleIndex + 1, 0, TURBOTUNNEL_MAX_OBSTACLE_COUNT);
    }
  }
}

void addGameTurbotunnel() {
  addGame(turbotunnelTitle, turbotunnelDescription, turbotunnelCharacters, turbotunnelCharactersCount,
          &turbotunnelOptions, false, &turbotunnelUpdate);
}
