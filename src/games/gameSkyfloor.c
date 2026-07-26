#include "../cglp.h"

int* skyfloorTitle = "SKY FLOOR";
int* skyfloorDescription = "[Slide] Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] skyfloorCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int skyfloorCharactersCount = 0;

Options skyfloorOptions = {100, 100, 100, false};

#define SKYFLOOR_BALL_RADIUS 2

struct SkyfloorFloor {
  Vector pos;
  Vector size;
  bool isAlive;
};
#define SKYFLOOR_MAX_FLOOR_COUNT 32
SkyfloorFloor[SKYFLOOR_MAX_FLOOR_COUNT] skyfloorFloors;
int skyfloorFloorIndex;
float skyfloorLastFloorX;
float skyfloorLastFloorWidth;
float skyfloorNextFloorDist;

struct SkyfloorFan {
  Vector pos;
  float speed;
  float angle;
  float ticks;
  bool isAlive;
};
#define SKYFLOOR_MAX_FAN_COUNT 32
SkyfloorFan[SKYFLOOR_MAX_FAN_COUNT] skyfloorFans;
int skyfloorFanIndex;

struct SkyfloorCoin {
  Vector pos;
  bool isAlive;
};
#define SKYFLOOR_MAX_COIN_COUNT 64
SkyfloorCoin[SKYFLOOR_MAX_COIN_COUNT] skyfloorCoins;
int skyfloorCoinIndex;

struct SkyfloorLine {
  Vector pos;
  float width;
  float angle;
  bool isAlive;
};
#define SKYFLOOR_MAX_LINE_COUNT 32
SkyfloorLine[SKYFLOOR_MAX_LINE_COUNT] skyfloorLines;
int skyfloorLineIndex;
float skyfloorNextLineDist;

struct SkyfloorBall {
  Vector pos;
  Vector vel;
  float radius;
  bool isOut;
};
SkyfloorBall skyfloorBall;

Vector skyfloorScr;
int skyfloorMultiplier;

void skyfloorUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(skyfloorFloors);
    skyfloorFloorIndex = 0;
    ASSIGN_ARRAY_ITEM(skyfloorFloors, skyfloorFloorIndex, SkyfloorFloor, f0);
    vectorSet(&f0->pos, 50, 50);
    vectorSet(&f0->size, 80, 150);
    f0->isAlive = true;
    skyfloorFloorIndex = cgl_wrap(skyfloorFloorIndex + 1, 0, SKYFLOOR_MAX_FLOOR_COUNT);
    skyfloorLastFloorX = 50;
    skyfloorLastFloorWidth = 80;
    skyfloorNextFloorDist = 0;
    INIT_UNALIVED_ARRAY_FAST(skyfloorFans);
    skyfloorFanIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(skyfloorCoins);
    skyfloorCoinIndex = 0;
    vectorSet(&skyfloorBall.pos, 50, 90);
    vectorSet(&skyfloorBall.vel, 0, 0);
    skyfloorBall.radius = SKYFLOOR_BALL_RADIUS;
    skyfloorBall.isOut = false;
    INIT_UNALIVED_ARRAY_FAST(skyfloorLines);
    skyfloorLineIndex = 0;
    skyfloorNextLineDist = 0;
    vectorSet(&skyfloorScr, 0, 0);
    skyfloorMultiplier = 1;
  }
  vectorSet(&skyfloorScr, 0, 0);
  if (skyfloorBall.pos.x < 45) {
    skyfloorScr.x += (45 - skyfloorBall.pos.x) * 0.1;
  } else if (skyfloorBall.pos.x > 55) {
    skyfloorScr.x += (55 - skyfloorBall.pos.x) * 0.1;
  }
  if (skyfloorBall.pos.y < 80) {
    skyfloorScr.y += (80 - skyfloorBall.pos.y) * 0.1;
  }
  skyfloorNextLineDist -= skyfloorScr.y * 0.2;
  if (skyfloorNextLineDist < 0) {
    ASSIGN_ARRAY_ITEM(skyfloorLines, skyfloorLineIndex, SkyfloorLine, nl);
    vectorSet(&nl->pos, rnd(0, 99), -20);
    nl->width = rnd(5, 20);
    if (rnd(0, 1) < 0.1) {
      nl->angle = rnd(0, CGLP_PI * 2);
    } else {
      nl->angle = rndi(0, 2) * CGLP_PI / 2;
    }
    nl->isAlive = true;
    skyfloorLineIndex = cgl_wrap(skyfloorLineIndex + 1, 0, SKYFLOOR_MAX_LINE_COUNT);
    skyfloorNextLineDist += rnd(5, 10);
  }
  color = LIGHT_YELLOW;
  FOR_EACH(skyfloorLines, i) {
    ASSIGN_ARRAY_ITEM(skyfloorLines, i, SkyfloorLine, l);
    SKIP_IS_NOT_ALIVE(l);
    l->pos.x += skyfloorScr.x * 0.2;
    l->pos.y += skyfloorScr.y * 0.2;
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(l->pos.x, l->pos.y, l->width, l->angle, &scratch);
    if (l->pos.y > 110) {
      l->isAlive = false;
      continue;
    }
  }
  skyfloorNextFloorDist -= skyfloorScr.y;
  if (skyfloorNextFloorDist < 0) {
    float lfX = skyfloorLastFloorX;
    float lfW = skyfloorLastFloorWidth;
    float x = 0;
    float w = 0;
    TIMES(9, i) {
      x = rnd(0, 99);
      w = rnd(30, 80);
      if (fabs(x - lfX) < (w + lfW) / 2) {
        break;
      } else {
        x = lfX;
        w = lfW;
      }
    }
    float h = rnd(30, 99);
    ASSIGN_ARRAY_ITEM(skyfloorFloors, skyfloorFloorIndex, SkyfloorFloor, nf);
    vectorSet(&nf->pos, x, -h / 2 - 20);
    vectorSet(&nf->size, w, h);
    nf->isAlive = true;
    skyfloorFloorIndex = cgl_wrap(skyfloorFloorIndex + 1, 0, SKYFLOOR_MAX_FLOOR_COUNT);
    skyfloorLastFloorX = x;
    skyfloorLastFloorWidth = w;
    int fanCount = rndi(0, (int)ceil(h / 30));
    TIMES(fanCount, k) {
      float wy = rndi(0, 2) * 2 - 1;
      ASSIGN_ARRAY_ITEM(skyfloorFans, skyfloorFanIndex, SkyfloorFan, nfan);
      vectorSet(&nfan->pos, x + rnd(w * 0.6, w * 0.7) * wy,
                -h / 2 - 20 + rnd(0, h * 0.4) * RNDPM());
      nfan->speed = rnd(1, 2);
      float baseAngle;
      if (wy == -1) {
        baseAngle = 0;
      } else {
        baseAngle = CGLP_PI;
      }
      nfan->angle = baseAngle + rnd(0, CGLP_PI / 8) * RNDPM();
      nfan->ticks = 0;
      nfan->isAlive = true;
      skyfloorFanIndex = cgl_wrap(skyfloorFanIndex + 1, 0, SKYFLOOR_MAX_FAN_COUNT);
    }
    int cc = rndi(0, (int)ceil(h / 10));
    float cy = rnd(0, h * 0.5) * RNDPM() - (cc * 10) / 2.0;
    float cx = x + rnd(0, w * 0.2) * RNDPM();
    TIMES(cc, k) {
      if (cy > -h * 0.3 && cy < h * 0.3) {
        ASSIGN_ARRAY_ITEM(skyfloorCoins, skyfloorCoinIndex, SkyfloorCoin, nc);
        vectorSet(&nc->pos, cx, cy - h / 2 - 20);
        nc->isAlive = true;
        skyfloorCoinIndex = cgl_wrap(skyfloorCoinIndex + 1, 0, SKYFLOOR_MAX_COIN_COUNT);
      }
      cy += 10;
    }
    skyfloorNextFloorDist = rnd(h * 0.6, h * 0.8);
  }
  color = LIGHT_BLUE;
  FOR_EACH(skyfloorFloors, i) {
    ASSIGN_ARRAY_ITEM(skyfloorFloors, i, SkyfloorFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    vectorAdd(&f->pos, skyfloorScr.x, skyfloorScr.y);
    box(f->pos.x, f->pos.y, f->size.x, f->size.y, &scratch);
    if (f->pos.y - f->size.y / 2 > 99) {
      f->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(skyfloorFans, i) {
    ASSIGN_ARRAY_ITEM(skyfloorFans, i, SkyfloorFan, f);
    SKIP_IS_NOT_ALIVE(f);
    f->ticks += f->speed * f->speed;
    vectorAdd(&f->pos, skyfloorScr.x, skyfloorScr.y);
    float oa = fabs(cgl_wrap(angleTo(&f->pos, skyfloorBall.pos.x, skyfloorBall.pos.y) - f->angle,
                              -CGLP_PI, CGLP_PI));
    float d = distanceTo(&f->pos, skyfloorBall.pos.x, skyfloorBall.pos.y) + 1;
    float fr = (f->speed * clamp(CGLP_PI / 4 - oa, 0, CGLP_PI / 4) / d) * 3;
    addWithAngle(&skyfloorBall.vel, f->angle, fr);
    Vector fanBoxPos;
    vectorSet(&fanBoxPos, f->pos.x, f->pos.y);
    addWithAngle(&fanBoxPos, f->angle + CGLP_PI, 5);
    box(fanBoxPos.x, fanBoxPos.y, 3, 3, &scratch);
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision barColl;
    bar(f->pos.x, f->pos.y, cos(f->ticks * 0.1) * 9, f->angle + CGLP_PI / 2, &barColl);
    if (f->pos.y < -9 &&
        (barColl.isColliding.rect[LIGHT_BLUE] || barColl.isColliding.rect[BLACK])) {
      f->isAlive = false;
      continue;
    }
    particle(f->pos.x, f->pos.y, f->speed * 0.2, f->speed * f->speed, f->angle, CGLP_PI / 4);
    if (f->pos.y > 105) {
      f->isAlive = false;
      continue;
    }
  }
  float o = input.pos.x - skyfloorBall.pos.x;
  float oSign;
  if (o < 0) {
    oSign = -1;
  } else {
    oSign = 1;
  }
  skyfloorBall.vel.x += o * o * 0.0001 * oSign;
  skyfloorBall.vel.y -= 0.1;
  vectorMul(&skyfloorBall.vel, 0.95);
  skyfloorBall.vel.y *= 1 - clamp(0.03 * sqrt(fabs(o)), 0, 0.5);
  skyfloorBall.pos.x += skyfloorBall.vel.x * sqrt(difficulty);
  skyfloorBall.pos.y += skyfloorBall.vel.y * sqrt(difficulty);
  vectorAdd(&skyfloorBall.pos, skyfloorScr.x, skyfloorScr.y);
  skyfloorBall.pos.y -= difficulty * 0.1;
  color = RED;
  thickness = 3;
  Collision arcColl;
  arc(skyfloorBall.pos.x, skyfloorBall.pos.y, skyfloorBall.radius, 0, CGLP_PI * 2, &arcColl);
  if (arcColl.isColliding.rect[LIGHT_BLUE]) {
    skyfloorBall.isOut = false;
    skyfloorBall.radius += (SKYFLOOR_BALL_RADIUS - skyfloorBall.radius) * 0.1;
  } else {
    play(HIT);
    if (!skyfloorBall.isOut) {
      skyfloorBall.vel.x /= 2;
      skyfloorBall.isOut = true;
    }
    skyfloorBall.radius *= 0.925;
    if (skyfloorBall.radius < 0.1) {
      play(EXPLOSION);
      gameOver();
    }
  }
  color = BLUE;
  thickness = 3;
  arc(skyfloorBall.pos.x, skyfloorBall.pos.y, 0.1, 0, CGLP_PI * 2, &scratch);
  color = YELLOW;
  FOR_EACH(skyfloorCoins, i) {
    ASSIGN_ARRAY_ITEM(skyfloorCoins, i, SkyfloorCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    vectorAdd(&c->pos, skyfloorScr.x, skyfloorScr.y);
    Collision cc;
    box(c->pos.x, c->pos.y, 8, 8, &cc);
    if (cc.isColliding.rect[RED]) {
      play(COIN);
      addScore(skyfloorMultiplier, c->pos.x, c->pos.y);
      particle(c->pos.x, c->pos.y, 16, 1, 0, CGLP_PI * 2);
      skyfloorMultiplier++;
      c->isAlive = false;
      continue;
    }
    if (c->pos.y > 105) {
      if (skyfloorMultiplier > 1) {
        skyfloorMultiplier--;
      }
      c->isAlive = false;
      continue;
    }
  }
}

void addGameSkyfloor() {
  addGame(skyfloorTitle, skyfloorDescription, skyfloorCharacters,
          skyfloorCharactersCount, &skyfloorOptions, true, &skyfloorUpdate);
}
