#include "../cglp.h"

int* pulsepiperTitle = "PULSE PIPER";
int* pulsepiperDescription = "[Hold] Charge";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pulsepiperCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int pulsepiperCharactersCount = 0;

Options pulsepiperOptions = {100, 100, 1, false};

#define PULSEPIPER_PULSE_SPEED 0.7
#define PULSEPIPER_CHARGE_RATE 1
#define PULSEPIPER_OBSTACLE_SPEED 0.5
#define PULSEPIPER_SPAWN_INTERVAL 60
#define PULSEPIPER_WIRE_Y 50

struct PulsepiperPulse {
  Vector pos;
  float charge;
};
PulsepiperPulse pulsepiperPulse;

struct PulsepiperObstacle {
  Vector pos;
  Vector size;
  float gap;
  bool isDot;
  bool isAlive;
};
#define PULSEPIPER_MAX_OBSTACLE_COUNT 32
PulsepiperObstacle[PULSEPIPER_MAX_OBSTACLE_COUNT] pulsepiperObstacles;
int pulsepiperObstacleIndex;
float pulsepiperNextObstacleDist;
float pulsepiperObstacleScore;

void pulsepiperUpdatePulse() {
  Collision scratch;
  pulsepiperPulse.pos.x += PULSEPIPER_PULSE_SPEED * difficulty;
  if (pulsepiperPulse.pos.x > 100) {
    pulsepiperPulse.pos.x = 0;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    pulsepiperPulse.charge =
        clamp(pulsepiperPulse.charge + PULSEPIPER_CHARGE_RATE * difficulty, 0, 99);
  }
  if (input.isJustReleased) {
    play(LASER);
    color = YELLOW;
    particle(pulsepiperPulse.pos.x, pulsepiperPulse.pos.y, 10, 2, 0, CGLP_PI * 2);
    color = RED;
    pulsepiperObstacleScore = floor(pulsepiperPulse.charge);
    pulsepiperPulse.charge = 0;
  } else {
    color = YELLOW;
  }
  float pulseGap = 4 + pulsepiperPulse.charge;
  box(pulsepiperPulse.pos.x, pulsepiperPulse.pos.y - pulseGap / 2, 8, 4, &scratch);
  box(pulsepiperPulse.pos.x, pulsepiperPulse.pos.y + pulseGap / 2, 8, 4, &scratch);
}

void pulsepiperUpdateObstacles() {
  FOR_EACH(pulsepiperObstacles, i) {
    ASSIGN_ARRAY_ITEM(pulsepiperObstacles, i, PulsepiperObstacle, obstacle);
    SKIP_IS_NOT_ALIVE(obstacle);
    obstacle->pos.x -= PULSEPIPER_OBSTACLE_SPEED * difficulty;
    if (!obstacle->isDot) {
      color = PURPLE;
    } else {
      color = RED;
    }
    float obstacleGap = obstacle->gap + obstacle->size.y / 2;
    Collision c1, c2;
    box(obstacle->pos.x, obstacle->pos.y - obstacleGap, obstacle->size.x, obstacle->size.y, &c1);
    box(obstacle->pos.x, obstacle->pos.y + obstacleGap, obstacle->size.x, obstacle->size.y, &c2);
    if (obstacle->isDot && (c1.isColliding.rect[RED] || c2.isColliding.rect[RED])) {
      play(POWER_UP);
      color = RED;
      particle(obstacle->pos.x, obstacle->pos.y, 15, 3, 0, CGLP_PI * 2);
      addScore(pulsepiperObstacleScore, obstacle->pos.x, obstacle->pos.y);
      obstacle->isAlive = false;
      continue;
    }
    if (!obstacle->isDot && (c1.isColliding.rect[YELLOW] || c2.isColliding.rect[YELLOW])) {
      play(EXPLOSION);
      gameOver();
    }
    if (obstacle->pos.x + obstacle->size.x / 3 < 0) {
      obstacle->isAlive = false;
      continue;
    }
  }
}

void pulsepiperSpawnObstacle() {
  play(CLICK);
  bool isDot = rnd(0, 1) < 0.7;
  bool isWall = rnd(0, 1) < 0.3;
  Vector size;
  if (isDot) {
    vectorSet(&size, 4, 2);
  } else {
    float h;
    if (isWall) {
      h = 50;
    } else {
      h = 4;
    }
    vectorSet(&size, 9, h);
  }
  float gap;
  if (isDot) {
    gap = 0;
  } else {
    if (!isWall && pulsepiperPulse.pos.x < 40 && rnd(0, 1) < 0.65) {
      gap = 0;
    } else {
      gap = rnd(10, 20);
    }
  }
  ASSIGN_ARRAY_ITEM(pulsepiperObstacles, pulsepiperObstacleIndex, PulsepiperObstacle, no);
  vectorSet(&no->pos, 100 + size.x / 2, PULSEPIPER_WIRE_Y);
  no->size = size;
  no->gap = gap;
  no->isDot = isDot;
  no->isAlive = true;
  pulsepiperObstacleIndex = cgl_wrap(pulsepiperObstacleIndex + 1, 0, PULSEPIPER_MAX_OBSTACLE_COUNT);
}

void pulsepiperUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&pulsepiperPulse.pos, 10, PULSEPIPER_WIRE_Y);
    pulsepiperPulse.charge = 0;
    INIT_UNALIVED_ARRAY_FAST(pulsepiperObstacles);
    pulsepiperObstacleIndex = 0;
    pulsepiperNextObstacleDist = 0;
    pulsepiperObstacleScore = 0;
  }
  color = CYAN;
  thickness = 4;
  line(0, PULSEPIPER_WIRE_Y, 100, PULSEPIPER_WIRE_Y, &scratch);
  pulsepiperUpdatePulse();
  pulsepiperUpdateObstacles();
  pulsepiperNextObstacleDist -= difficulty;
  if (pulsepiperNextObstacleDist < 0) {
    pulsepiperSpawnObstacle();
    pulsepiperNextObstacleDist += PULSEPIPER_SPAWN_INTERVAL;
  }
}

void addGamePulsepiper() {
  addGame(pulsepiperTitle, pulsepiperDescription, pulsepiperCharacters,
          pulsepiperCharactersCount, &pulsepiperOptions, false, &pulsepiperUpdate);
}
