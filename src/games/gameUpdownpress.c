#include "../cglp.h"

int* updownpressTitle = "UP DOWN PRESS";
int* updownpressDescription = "[Tap]\n Jump\n[Hold]\n Speed up";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] updownpressCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int updownpressCharactersCount = 0;

Options updownpressOptions = {200, 100, 600, false};

#define UPDOWNPRESS_MY_CAR_SIZE 5
#define UPDOWNPRESS_MY_CAR_SPEED 1

struct UpdownpressRoad {
  Vector from;
  Vector to;
  float angle;
  bool isAlive;
};
#define UPDOWNPRESS_MAX_ROAD_COUNT 32
UpdownpressRoad[UPDOWNPRESS_MAX_ROAD_COUNT] updownpressRoads;
int updownpressRoadIndex;
int updownpressLastRoadIndex;
float updownpressNextRoadDist;

int[4] updownpressCarColors = {RED, YELLOW, GREEN, PURPLE};

struct UpdownpressCar {
  float x;
  float vx;
  float angle;
  float size;
  int color;
  float speed;
  float currentSpeed;
  bool isAlive;
};
#define UPDOWNPRESS_MAX_CAR_COUNT 64
UpdownpressCar[UPDOWNPRESS_MAX_CAR_COUNT] updownpressCars;
int updownpressCarIndex;
float updownpressNextCarDist;

Vector updownpressScr;

#define UPDOWNPRESS_STATE_GROUND 0
#define UPDOWNPRESS_STATE_JUMP 1
struct UpdownpressMyCar {
  Vector pos;
  float vy;
  float vx;
  float angle;
  float speed;
  int state;
};
UpdownpressMyCar updownpressMyCar;

int updownpressMultiplier;

bool updownpressCalcRoad(float x, float* yOut, float* angleOut) {
  bool found = false;
  FOR_EACH(updownpressRoads, ri) {
    ASSIGN_ARRAY_ITEM(updownpressRoads, ri, UpdownpressRoad, r);
    SKIP_IS_NOT_ALIVE(r);
    if (r->from.x <= x && x < r->to.x) {
      *yOut = ((r->from.y - r->to.y) * (x - r->to.x)) / (r->from.x - r->to.x) + r->to.y;
      *angleOut = r->angle;
      found = true;
    }
  }
  return found;
}

void updownpressUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(updownpressRoads);
    updownpressRoadIndex = 0;
    ASSIGN_ARRAY_ITEM(updownpressRoads, updownpressRoadIndex, UpdownpressRoad, r0);
    vectorSet(&r0->from, 0, 50);
    vectorSet(&r0->to, 100, 50);
    r0->angle = 0;
    r0->isAlive = true;
    updownpressLastRoadIndex = updownpressRoadIndex;
    updownpressRoadIndex = cgl_wrap(updownpressRoadIndex + 1, 0, UPDOWNPRESS_MAX_ROAD_COUNT);
    updownpressNextRoadDist = -250;
    INIT_UNALIVED_ARRAY_FAST(updownpressCars);
    updownpressCarIndex = 0;
    updownpressNextCarDist = 0;
    vectorSet(&updownpressScr, 0, 0);
    vectorSet(&updownpressMyCar.pos, 20, 50);
    updownpressMyCar.vy = 0;
    updownpressMyCar.vx = 0;
    updownpressMyCar.angle = 0;
    updownpressMyCar.speed = 1;
    updownpressMyCar.state = UPDOWNPRESS_STATE_GROUND;
    updownpressMultiplier = 1;
  }
  vectorSet(&updownpressScr, difficulty * 0.1, difficulty * 0.1);
  if (updownpressMyCar.pos.x > 50) {
    updownpressScr.x += (updownpressMyCar.pos.x - 50) * 0.1;
  }
  float ry, dummyAngle;
  bool foundRy = updownpressCalcRoad(updownpressMyCar.pos.x + 50, &ry, &dummyAngle);
  if (foundRy) {
    if (ry < 60) {
      updownpressScr.y += (ry - 60) * 0.1;
    } else if (ry > 90) {
      updownpressScr.y += (ry - 90) * 0.1;
    }
  }
  updownpressNextRoadDist -= updownpressScr.x;
  while (updownpressNextRoadDist < 0) {
    UpdownpressRoad* lr = &updownpressRoads[updownpressLastRoadIndex];
    Vector from = lr->to;
    Vector to = lr->to;
    float w = rnd(20, 60);
    to.x += w;
    if (lr->from.y - lr->to.y == 0) {
      to.y += rnd(0.4, 1.1) * RNDPM() * w;
    }
    ASSIGN_ARRAY_ITEM(updownpressRoads, updownpressRoadIndex, UpdownpressRoad, nr);
    nr->from = from;
    nr->to = to;
    nr->angle = angleTo(&from, to.x, to.y);
    nr->isAlive = true;
    updownpressLastRoadIndex = updownpressRoadIndex;
    updownpressRoadIndex = cgl_wrap(updownpressRoadIndex + 1, 0, UPDOWNPRESS_MAX_ROAD_COUNT);
    updownpressNextRoadDist += w;
  }
  color = LIGHT_BLACK;
  FOR_EACH(updownpressRoads, ri2) {
    ASSIGN_ARRAY_ITEM(updownpressRoads, ri2, UpdownpressRoad, r2);
    SKIP_IS_NOT_ALIVE(r2);
    r2->from.x -= updownpressScr.x;
    r2->from.y -= updownpressScr.y;
    r2->to.x -= updownpressScr.x;
    r2->to.y -= updownpressScr.y;
    line(r2->from.x, r2->from.y, r2->to.x, r2->to.y, &scratch);
    if (r2->to.x < -50) {
      r2->isAlive = false;
      continue;
    }
  }
  updownpressMyCar.pos.x +=
      updownpressMyCar.speed * sqrt(difficulty) - updownpressScr.x + updownpressMyCar.vx;
  updownpressMyCar.vx *= 0.9;
  if (updownpressMyCar.pos.x < 0) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  float y, a;
  bool foundY = updownpressCalcRoad(updownpressMyCar.pos.x, &y, &a);
  if (foundY) {
    if (updownpressMyCar.state == UPDOWNPRESS_STATE_JUMP && updownpressMyCar.pos.y > y) {
      updownpressMyCar.state = UPDOWNPRESS_STATE_GROUND;
    }
    if (updownpressMyCar.state == UPDOWNPRESS_STATE_GROUND) {
      updownpressMyCar.pos.y = y;
      updownpressMyCar.speed += UPDOWNPRESS_MY_CAR_SIZE * updownpressMyCar.angle * 0.02;
      updownpressMyCar.angle += (a - updownpressMyCar.angle) * 0.025;
      if (input.isJustPressed) {
        play(JUMP);
        updownpressMyCar.state = UPDOWNPRESS_STATE_JUMP;
        updownpressMyCar.vy = -2;
        updownpressMultiplier = 1;
      }
    }
  }
  if (updownpressMyCar.state == UPDOWNPRESS_STATE_JUMP) {
    updownpressMyCar.pos.y += updownpressMyCar.vy * sqrt(difficulty);
    float dvy2;
    if (input.isPressed) {
      dvy2 = 0.05;
    } else {
      dvy2 = 0.2;
    }
    updownpressMyCar.vy += dvy2;
    updownpressMyCar.angle +=
        (cgl_atan2(updownpressMyCar.vy, updownpressMyCar.speed) - updownpressMyCar.angle) * 0.05;
  }
  float speedMul;
  if (input.isPressed) {
    speedMul = 2.5;
  } else {
    speedMul = 0.5;
  }
  updownpressMyCar.speed += (UPDOWNPRESS_MY_CAR_SPEED * speedMul - updownpressMyCar.speed) * 0.1;
  Vector p;
  vectorSet(&p, updownpressMyCar.pos.x, updownpressMyCar.pos.y);
  addWithAngle(&p, updownpressMyCar.angle, UPDOWNPRESS_MY_CAR_SIZE * 0.6);
  float ts = UPDOWNPRESS_MY_CAR_SIZE;
  color = BLUE;
  box(p.x, p.y - ts / 2, ts, ts, &scratch);
  addWithAngle(&p, updownpressMyCar.angle, UPDOWNPRESS_MY_CAR_SIZE * -1.2);
  box(p.x, p.y - ts / 2, ts, ts, &scratch);
  color = CYAN;
  vectorSet(&p, updownpressMyCar.pos.x, updownpressMyCar.pos.y);
  addWithAngle(&p, updownpressMyCar.angle - CGLP_PI / 2, ts);
  thickness = ts;
  barCenterPosRatio = 0.5;
  bar(p.x, p.y, ts, updownpressMyCar.angle, &scratch);
  addWithAngle(&p, updownpressMyCar.angle - (CGLP_PI / 4) * 3, ts * 0.5);
  thickness = ts;
  bar(p.x, p.y, ts / 2, updownpressMyCar.angle, &scratch);
  updownpressNextCarDist -= updownpressScr.x;
  if (updownpressNextCarDist < 0) {
    float carSpeed = rnd(0.3, 1 + sqrt(difficulty));
    float cx;
    if (carSpeed > 2.5) {
      cx = -5;
    } else {
      cx = 205;
    }
    ASSIGN_ARRAY_ITEM(updownpressCars, updownpressCarIndex, UpdownpressCar, nc);
    nc->x = cx;
    nc->vx = 0;
    nc->size = rnd(5, 8);
    nc->speed = carSpeed;
    nc->currentSpeed = 0;
    nc->angle = 0;
    nc->color = updownpressCarColors[rndi(0, 4)];
    nc->isAlive = true;
    updownpressCarIndex = cgl_wrap(updownpressCarIndex + 1, 0, UPDOWNPRESS_MAX_CAR_COUNT);
    updownpressNextCarDist += rnd(100, 120) / sqrt(difficulty);
  }
  FOR_EACH(updownpressCars, ci) {
    ASSIGN_ARRAY_ITEM(updownpressCars, ci, UpdownpressCar, c);
    SKIP_IS_NOT_ALIVE(c);
    c->x += c->currentSpeed * sqrt(difficulty) - updownpressScr.x;
    float cy, ca;
    bool foundCy = updownpressCalcRoad(c->x, &cy, &ca);
    c->currentSpeed += c->size * c->angle * 0.02;
    c->currentSpeed += (c->speed - c->currentSpeed) * 0.1;
    if (!foundCy) {
      c->isAlive = false;
      continue;
    }
    c->angle += (ca - c->angle) * 0.025;
    Vector cp;
    vectorSet(&cp, c->x, cy);
    addWithAngle(&cp, c->angle, c->size * 0.6);
    float cts = c->size;
    color = BLACK;
    box(cp.x, cp.y - cts / 2, cts, cts, &scratch);
    addWithAngle(&cp, c->angle, c->size * -1.2);
    box(cp.x, cp.y - cts / 2, cts, cts, &scratch);
    color = c->color;
    vectorSet(&cp, c->x, cy);
    addWithAngle(&cp, c->angle - CGLP_PI / 2, cts);
    thickness = cts;
    barCenterPosRatio = 0.5;
    Collision cl1;
    bar(cp.x, cp.y, cts, c->angle, &cl1);
    addWithAngle(&cp, c->angle - (CGLP_PI / 4) * 3, cts * 0.5);
    thickness = cts;
    Collision cl2;
    bar(cp.x, cp.y, cts / 2, c->angle, &cl2);
    bool clCyan = cl1.isColliding.rect[CYAN] || cl2.isColliding.rect[CYAN];
    bool clBlue = cl1.isColliding.rect[BLUE] || cl2.isColliding.rect[BLUE];
    bool isPressing = updownpressMyCar.state == UPDOWNPRESS_STATE_JUMP && updownpressMyCar.vy >= 0;
    if (isPressing && (clCyan || clBlue)) {
      play(POWER_UP);
      addScore(updownpressMultiplier, cp.x, cp.y);
      if (updownpressMultiplier < 16) {
        updownpressMultiplier *= 2;
      }
      particle(cp.x, cp.y, 16, 1, 0, CGLP_PI * 2);
      updownpressMyCar.vy = -2;
      c->isAlive = false;
      continue;
    }
    if (!isPressing && clCyan) {
      updownpressMyCar.vx = -c->size - updownpressMyCar.speed * 1.5;
      play(EXPLOSION);
      color = CYAN;
      particle(updownpressMyCar.pos.x, updownpressMyCar.pos.y, 9, 2, 0, 1);
    }
  }
}

void addGameUpdownpress() {
  addGame(updownpressTitle, updownpressDescription, updownpressCharacters,
          updownpressCharactersCount, &updownpressOptions, false, &updownpressUpdate);
}
