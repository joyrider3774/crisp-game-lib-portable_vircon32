#include "../cglp.h"

int* rotatingtunnelTitle = "ROTATING TUNNEL";
int* rotatingtunnelDescription = "[Hold] Move outward\n[Release] Move inward";
int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rotatingtunnelCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int rotatingtunnelCharactersCount = 1;

// "shape" theme has no equivalent in this port's Options (only "dark"/
// "shapeDark" map to isDarkColor) - dropped per the porting convention.
Options rotatingtunnelOptions = {100, 100, 70, false};

#define ROTATINGTUNNEL_CENTER_X 50
#define ROTATINGTUNNEL_CENTER_Y 50
#define ROTATINGTUNNEL_INNER_RADIUS 10
#define ROTATINGTUNNEL_OUTER_RADIUS 50
#define ROTATINGTUNNEL_SHIP_MOVE_SPEED 0.65
#define ROTATINGTUNNEL_OBSTACLE_MOVE_SPEED 0.25
#define ROTATINGTUNNEL_OBSTACLE_SPAWN_INTERVAL 80

struct RotatingtunnelShip {
  Vector pos;
  float angle;
  float radius;
};
RotatingtunnelShip rotatingtunnelShip;

struct RotatingtunnelObstacle {
  float radius;
  float gapStart;
  float gapSize;
  bool isGold;
  bool isAlive;
};
#define ROTATINGTUNNEL_MAX_OBSTACLE_COUNT 32
RotatingtunnelObstacle[ROTATINGTUNNEL_MAX_OBSTACLE_COUNT] rotatingtunnelObstacles;
int rotatingtunnelObstacleIndex;

float rotatingtunnelScoreAddingTicks;
float rotatingtunnelMultiplier;
float rotatingtunnelNextObstacleTicks;

void rotatingtunnelUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&rotatingtunnelShip.pos, ROTATINGTUNNEL_CENTER_X, ROTATINGTUNNEL_CENTER_Y);
    rotatingtunnelShip.angle = 0;
    rotatingtunnelShip.radius = 20;
    rotatingtunnelScoreAddingTicks = 0;
    rotatingtunnelMultiplier = 0;
    INIT_UNALIVED_ARRAY_FAST(rotatingtunnelObstacles);
    rotatingtunnelObstacleIndex = 0;
    rotatingtunnelNextObstacleTicks = 0;
  }
  float sd = sqrt(difficulty);
  rotatingtunnelShip.angle += 0.025 * sd;
  if (input.isJustPressed) {
    play(CLICK);
  }
  if (input.isPressed) {
    rotatingtunnelShip.radius = clamp(rotatingtunnelShip.radius + ROTATINGTUNNEL_SHIP_MOVE_SPEED,
                                       ROTATINGTUNNEL_INNER_RADIUS, ROTATINGTUNNEL_OUTER_RADIUS);
  } else {
    rotatingtunnelShip.radius = clamp(rotatingtunnelShip.radius - ROTATINGTUNNEL_SHIP_MOVE_SPEED,
                                       ROTATINGTUNNEL_INNER_RADIUS, ROTATINGTUNNEL_OUTER_RADIUS);
  }
  vectorSet(&rotatingtunnelShip.pos, ROTATINGTUNNEL_CENTER_X + rotatingtunnelShip.radius * cos(rotatingtunnelShip.angle),
            ROTATINGTUNNEL_CENTER_Y + rotatingtunnelShip.radius * sin(rotatingtunnelShip.angle));

  rotatingtunnelNextObstacleTicks -= sd;
  if (rotatingtunnelNextObstacleTicks < 0) {
    bool isGold = rnd(0, 1) < 0.3;
    if (isGold) {
      play(COIN);
    } else {
      play(LASER);
    }
    ASSIGN_ARRAY_ITEM(rotatingtunnelObstacles, rotatingtunnelObstacleIndex, RotatingtunnelObstacle, o);
    o->radius = ROTATINGTUNNEL_OUTER_RADIUS;
    o->gapStart = rnd(0, 2 * CGLP_PI);
    o->gapSize = rnd(CGLP_PI / 4, CGLP_PI / 3);
    o->isGold = isGold;
    o->isAlive = true;
    rotatingtunnelObstacleIndex = cgl_wrap(rotatingtunnelObstacleIndex + 1, 0, ROTATINGTUNNEL_MAX_OBSTACLE_COUNT);
    rotatingtunnelNextObstacleTicks += ROTATINGTUNNEL_OBSTACLE_SPAWN_INTERVAL;
  }

  FOR_EACH(rotatingtunnelObstacles, oi) {
    ASSIGN_ARRAY_ITEM(rotatingtunnelObstacles, oi, RotatingtunnelObstacle, o2);
    SKIP_IS_NOT_ALIVE(o2);
    o2->radius -= ROTATINGTUNNEL_OBSTACLE_MOVE_SPEED * sd;
    if (o2->radius <= ROTATINGTUNNEL_INNER_RADIUS) {
      o2->isAlive = false;
      continue;
    }
  }

  color = BLUE;
  arc(ROTATINGTUNNEL_CENTER_X, ROTATINGTUNNEL_CENTER_Y, ROTATINGTUNNEL_OUTER_RADIUS, 0, CGLP_PI * 2, &scratch);
  color = LIGHT_BLUE;
  arc(ROTATINGTUNNEL_CENTER_X, ROTATINGTUNNEL_CENTER_Y, ROTATINGTUNNEL_INNER_RADIUS, 0, CGLP_PI * 2, &scratch);
  color = CYAN;
  float shipSize = 1;
  Vector sp0;
  Vector sp1;
  Vector sp2;
  vectorSet(&sp0, rotatingtunnelShip.pos.x - shipSize * cos(rotatingtunnelShip.angle),
            rotatingtunnelShip.pos.y - shipSize * sin(rotatingtunnelShip.angle));
  vectorSet(&sp1, rotatingtunnelShip.pos.x + shipSize * cos(rotatingtunnelShip.angle),
            rotatingtunnelShip.pos.y + shipSize * sin(rotatingtunnelShip.angle));
  vectorSet(&sp2, rotatingtunnelShip.pos.x - shipSize * sin(rotatingtunnelShip.angle),
            rotatingtunnelShip.pos.y + shipSize * cos(rotatingtunnelShip.angle));
  line(sp0.x, sp0.y, sp1.x, sp1.y, &scratch);
  line(sp1.x, sp1.y, sp2.x, sp2.y, &scratch);
  line(sp2.x, sp2.y, sp0.x, sp0.y, &scratch);

  FOR_EACH(rotatingtunnelObstacles, oi2) {
    ASSIGN_ARRAY_ITEM(rotatingtunnelObstacles, oi2, RotatingtunnelObstacle, o3);
    SKIP_IS_NOT_ALIVE(o3);
    float startAngle = o3->gapStart;
    float endAngle = o3->gapStart + o3->gapSize;
    if (o3->isGold) {
      color = YELLOW;
    } else {
      color = PURPLE;
    }
    thickness = 3;
    arc(ROTATINGTUNNEL_CENTER_X, ROTATINGTUNNEL_CENTER_Y, o3->radius, startAngle, endAngle, &scratch);
    if (scratch.isColliding.rect[CYAN]) {
      if (o3->isGold) {
        rotatingtunnelScoreAddingTicks -= sd;
        if (rotatingtunnelScoreAddingTicks < 0) {
          play(POWER_UP);
          rotatingtunnelMultiplier++;
          addScore(ceil(rotatingtunnelMultiplier), rotatingtunnelShip.pos.x, rotatingtunnelShip.pos.y);
          rotatingtunnelScoreAddingTicks += 5;
        }
      } else {
        play(EXPLOSION);
        gameOver();
      }
    }
  }
  rotatingtunnelMultiplier *= 0.99;
}

void addGameRotatingtunnel() {
  addGame(rotatingtunnelTitle, rotatingtunnelDescription, rotatingtunnelCharacters,
          rotatingtunnelCharactersCount, &rotatingtunnelOptions, false, &rotatingtunnelUpdate);
}
