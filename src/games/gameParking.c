#include "../cglp.h"

int* parkingTitle = "PARKING";
int* parkingDescription = "[Hold]\n Turn right";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] parkingCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int parkingCharactersCount = 1;

Options parkingOptions = {100, 100, 3, false};

int[4] parkingCarColors = {RED, BLUE, CYAN, PURPLE};
int* parkingWordChars = "PARKING";

struct ParkingCar {
  Vector pos;
  int color;
  bool isAlive;
};
#define PARKING_MAX_CAR_COUNT 32
ParkingCar[PARKING_MAX_CAR_COUNT] parkingCars;
int parkingCarIndex;

struct ParkingParkedCar {
  Vector pos;
  float angle;
  int color;
  bool isAlive;
};
#define PARKING_MAX_PARKED_CAR_COUNT 64
ParkingParkedCar[PARKING_MAX_PARKED_CAR_COUNT] parkingParkedCars;
int parkingParkedCarIndex;

float parkingCarAngle;
float parkingCarCount;
float parkingNextCarDist;
float parkingNextParkedCarDist;
bool parkingGoldActive;
Vector parkingGoldPos;
float parkingRoadY;
int parkingMultiplier;

void parkingDrawCar(float px, float py, float angle, int carColor, Collision* result) {
  Vector o;
  Collision scratch;
  color = BLACK;
  vectorSet(&o, px, py);
  addWithAngle(&o, angle + CGLP_PI / 4, 3);
  box(o.x, o.y, 3, 3, &scratch);
  vectorSet(&o, px, py);
  addWithAngle(&o, angle + (CGLP_PI / 4) * 3, 3);
  box(o.x, o.y, 3, 3, &scratch);
  vectorSet(&o, px, py);
  addWithAngle(&o, angle + (CGLP_PI / 4) * 5, 3);
  box(o.x, o.y, 3, 3, &scratch);
  vectorSet(&o, px, py);
  addWithAngle(&o, angle + (CGLP_PI / 4) * 7, 3);
  box(o.x, o.y, 3, 3, &scratch);
  color = carColor;
  thickness = 4;
  barCenterPosRatio = 0.5;
  bar(px, py, 4, angle, result);
}

void parkingUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(parkingCars);
    parkingCarIndex = 0;
    parkingCarAngle = -CGLP_PI / 2;
    parkingNextCarDist = 0;
    parkingCarCount = 1;
    INIT_UNALIVED_ARRAY_FAST(parkingParkedCars);
    parkingParkedCarIndex = 0;
    parkingNextParkedCarDist = 0;
    parkingGoldActive = false;
    parkingRoadY = 0;
    parkingMultiplier = 1;
  }
  float carSpeed = difficulty;
  float scr = carSpeed * 1.05;
  bool foundLeadCar = false;
  float leadCarY = 0;
  FOR_EACH(parkingCars, i) {
    ASSIGN_ARRAY_ITEM(parkingCars, i, ParkingCar, c);
    if (c->isAlive) {
      leadCarY = c->pos.y;
      foundLeadCar = true;
      break;
    }
  }
  if (foundLeadCar) {
    if (leadCarY < 50) {
      scr += (50 - leadCarY) * 0.1;
    }
  } else {
    parkingNextCarDist--;
  }
  parkingRoadY -= scr;
  color = LIGHT_BLACK;
  rect(0, 0, 11, 100, &scratch);
  rect(89, 0, 11, 100, &scratch);
  TIMES(3, x) {
    float lx = 30 + x * 20;
    TIMES(6, y) {
      float ly = y * 20 - fmod(-parkingRoadY, 20) - 10;
      box(lx, ly, 3, 10, &scratch);
    }
  }
  color = LIGHT_YELLOW;
  rect(82, fmod(-parkingRoadY, 93) + 6, 7, 90, &scratch);
  rect(82, fmod(-parkingRoadY, 93) - 88, 7, 90, &scratch);
  color = WHITE;
  box(93, fmod(-parkingRoadY, 200) - 50, 7, 44, &scratch);
  color = LIGHT_BLACK;
  float wordY = fmod(-parkingRoadY, 200) - 69;
  TIMES(7, i) {
    int[2] wc;
    wc[0] = parkingWordChars[i];
    wc[1] = 0;
    text(wc, 92, wordY, &scratch);
    wordY += 6;
  }
  if (!parkingGoldActive) {
    vectorSet(&parkingGoldPos, rnd(40, 70), rnd(-50, -20));
    parkingGoldActive = true;
  }
  color = YELLOW;
  box(parkingGoldPos.x, parkingGoldPos.y, 8, 8, &scratch);
  color = WHITE;
  text("$", parkingGoldPos.x, parkingGoldPos.y, &scratch);
  parkingGoldPos.y += scr;
  if (parkingGoldPos.y > 103) {
    if (parkingMultiplier > 1) {
      parkingMultiplier--;
    }
    parkingGoldActive = false;
  }
  parkingNextParkedCarDist -= scr;
  if (parkingNextParkedCarDist < 0) {
    play(LASER);
    ASSIGN_ARRAY_ITEM(parkingParkedCars, parkingParkedCarIndex, ParkingParkedCar, pc);
    vectorSet(&pc->pos, rnd(80, 84), -5);
    pc->angle = rnd((-CGLP_PI / 8) * 3, -CGLP_PI / 8);
    pc->color = parkingCarColors[rndi(0, 4)];
    pc->isAlive = true;
    parkingParkedCarIndex = cgl_wrap(parkingParkedCarIndex + 1, 0, PARKING_MAX_PARKED_CAR_COUNT);
    if (rnd(0, 1) < 0.7) {
      parkingNextParkedCarDist = rnd(8, 12);
    } else {
      parkingNextParkedCarDist = rnd(100, 200);
    }
  }
  FOR_EACH(parkingParkedCars, i) {
    ASSIGN_ARRAY_ITEM(parkingParkedCars, i, ParkingParkedCar, pc);
    SKIP_IS_NOT_ALIVE(pc);
    pc->pos.y += scr;
    Collision discard;
    parkingDrawCar(pc->pos.x, pc->pos.y, pc->angle, pc->color, &discard);
    pc->isAlive = pc->pos.y <= 105;
  }
  parkingNextCarDist -= scr - carSpeed;
  COUNT_IS_ALIVE(parkingCars, aliveCarCount);
  if (aliveCarCount < parkingCarCount && parkingNextCarDist < 0) {
    play(HIT);
    ASSIGN_ARRAY_ITEM(parkingCars, parkingCarIndex, ParkingCar, nc);
    vectorSet(&nc->pos, rnd(20, 60), -5);
    nc->color = parkingCarColors[rndi(0, 4)];
    nc->isAlive = true;
    parkingCarIndex = cgl_wrap(parkingCarIndex + 1, 0, PARKING_MAX_CAR_COUNT);
    parkingNextCarDist = 15 * sqrt(difficulty);
  }
  if (input.isPressed) {
    parkingCarAngle += sqrt(difficulty) * 0.1;
  } else {
    parkingCarAngle += -sqrt(difficulty) * 0.1;
  }
  parkingCarAngle = clamp(parkingCarAngle, -CGLP_PI / 2, 0);
  FOR_EACH(parkingCars, i) {
    ASSIGN_ARRAY_ITEM(parkingCars, i, ParkingCar, c);
    SKIP_IS_NOT_ALIVE(c);
    float ca;
    if (c->pos.y < 3) {
      ca = -CGLP_PI / 2;
    } else {
      ca = parkingCarAngle;
    }
    addWithAngle(&c->pos, ca, carSpeed);
    c->pos.y += scr;
    Collision cl;
    parkingDrawCar(c->pos.x, c->pos.y, ca, c->color, &cl);
    if (cl.isColliding.rect[RED] || cl.isColliding.rect[BLUE] ||
        cl.isColliding.rect[CYAN] || cl.isColliding.rect[PURPLE]) {
      play(EXPLOSION);
      color = LIGHT_RED;
      text("X", c->pos.x, c->pos.y, &scratch);
      gameOver();
    }
    if (cl.isColliding.rect[LIGHT_YELLOW]) {
      play(POWER_UP);
      addScore(parkingMultiplier * 10, c->pos.x, c->pos.y);
      ASSIGN_ARRAY_ITEM(parkingParkedCars, parkingParkedCarIndex, ParkingParkedCar, pc2);
      pc2->pos = c->pos;
      pc2->angle = parkingCarAngle;
      pc2->color = c->color;
      pc2->isAlive = true;
      parkingParkedCarIndex = cgl_wrap(parkingParkedCarIndex + 1, 0, PARKING_MAX_PARKED_CAR_COUNT);
      parkingCarCount += 1 / parkingCarCount;
      c->isAlive = false;
      continue;
    }
    if (cl.isColliding.rect[YELLOW]) {
      if (parkingGoldPos.y > -3) {
        play(COIN);
        addScore(parkingMultiplier, parkingGoldPos.x, parkingGoldPos.y);
        parkingMultiplier++;
      }
      parkingGoldActive = false;
    }
    if (c->pos.y > 103) {
      play(EXPLOSION);
      color = LIGHT_RED;
      text("X", c->pos.x, 96, &scratch);
      gameOver();
    }
  }
}

void addGameParking() {
  addGame(parkingTitle, parkingDescription, parkingCharacters,
          parkingCharactersCount, &parkingOptions, false, &parkingUpdate);
}
