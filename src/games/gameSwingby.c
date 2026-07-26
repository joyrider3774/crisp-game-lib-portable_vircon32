#include "../cglp.h"

int* swingbyTitle = "SWINGBY";
int* swingbyDescription = "[Hold]\n Turn right";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] swingbyCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int swingbyCharactersCount = 1;

Options swingbyOptions = {100, 100, 70, true};

struct SwingbyStar {
  Vector pos;
  Vector screenPos;
  float radius;
  bool isAlive;
};
#define SWINGBY_MAX_STAR_COUNT 64
SwingbyStar[SWINGBY_MAX_STAR_COUNT] swingbyStars;
int swingbyStarIndex;

Vector swingbyStarAddPos;

struct SwingbyShip {
  Vector pos;
  Vector vel;
};
SwingbyShip swingbyShip;
float swingbyHitCount;
Vector swingbyShipScreenPos;

struct SwingbyBackStar {
  Vector pos;
  float velRatio;
};
#define SWINGBY_BACK_STAR_COUNT 30
SwingbyBackStar[SWINGBY_BACK_STAR_COUNT] swingbyBackStars;

void swingbyAddStar(float cx, float cy, float r, float af, float at) {
  TIMES(99, i) {
    Vector pos;
    vectorSet(&pos, cx, cy);
    addWithAngle(&pos, rnd(af, at), rnd(0, r));
    float radius = rnd(5, 15);
    bool hasSpace = true;
    FOR_EACH(swingbyStars, j) {
      ASSIGN_ARRAY_ITEM(swingbyStars, j, SwingbyStar, s);
      SKIP_IS_NOT_ALIVE(s);
      if (hasSpace && distanceTo(&s->pos, pos.x, pos.y) < s->radius + radius + 30) {
        hasSpace = false;
      }
    }
    if (hasSpace) {
      ASSIGN_ARRAY_ITEM(swingbyStars, swingbyStarIndex, SwingbyStar, ns);
      ns->pos = pos;
      vectorSet(&ns->screenPos, 0, 0);
      ns->radius = radius;
      ns->isAlive = true;
      swingbyStarIndex = cgl_wrap(swingbyStarIndex + 1, 0, SWINGBY_MAX_STAR_COUNT);
      break;
    }
  }
}

void swingbyUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(swingbyStars);
    swingbyStarIndex = 0;
    ASSIGN_ARRAY_ITEM(swingbyStars, swingbyStarIndex, SwingbyStar, placeholder);
    vectorSet(&placeholder->pos, 0, 0);
    vectorSet(&placeholder->screenPos, 0, 0);
    placeholder->radius = 20;
    placeholder->isAlive = true;
    int placeholderIndex = swingbyStarIndex;
    swingbyStarIndex = cgl_wrap(swingbyStarIndex + 1, 0, SWINGBY_MAX_STAR_COUNT);
    vectorSet(&swingbyStarAddPos, 0, 0);
    TIMES(20, i) {
      swingbyAddStar(swingbyStarAddPos.x, swingbyStarAddPos.y, 150, -CGLP_PI, CGLP_PI);
    }
    swingbyStars[placeholderIndex].isAlive = false;
    vectorSet(&swingbyShip.pos, 0, 0);
    vectorSet(&swingbyShip.vel, 0, 0);
    swingbyHitCount = 0;
    vectorSet(&swingbyShipScreenPos, 50, 50);
    TIMES(SWINGBY_BACK_STAR_COUNT, i) {
      vectorSet(&swingbyBackStars[i].pos, rnd(0, 99), rnd(0, 99));
      swingbyBackStars[i].velRatio = -rnd(0.05, 0.2);
    }
  }
  color = LIGHT_BLACK;
  TIMES(SWINGBY_BACK_STAR_COUNT, i) {
    SwingbyBackStar* s = &swingbyBackStars[i];
    vectorAdd(&s->pos, swingbyShip.vel.x * s->velRatio, swingbyShip.vel.y * s->velRatio);
    if (s->pos.x < 0 || s->pos.x > 99) {
      s->pos.y = rnd(0, 99);
    }
    if (s->pos.y < 0 || s->pos.y > 99) {
      s->pos.x = rnd(0, 99);
    }
    s->pos.x = cgl_wrap(s->pos.x, 0, 100);
    s->pos.y = cgl_wrap(s->pos.y, 0, 100);
    rect(s->pos.x, s->pos.y, 1, 1, &scratch);
  }
  Vector shipBoxPos;
  vectorSet(&shipBoxPos, swingbyShip.pos.x, swingbyShip.pos.y);
  vectorMul(&shipBoxPos, -1);
  vectorAdd(&shipBoxPos, swingbyShipScreenPos.x, swingbyShipScreenPos.y);
  shipBoxPos.x = clamp(shipBoxPos.x, -3, 103);
  shipBoxPos.y = clamp(shipBoxPos.y, -3, 103);
  box(shipBoxPos.x, shipBoxPos.y, 10, 10, &scratch);
  color = BLACK;
  FOR_EACH(swingbyStars, i) {
    ASSIGN_ARRAY_ITEM(swingbyStars, i, SwingbyStar, s);
    SKIP_IS_NOT_ALIVE(s);
    float d = distanceTo(&s->pos, swingbyShip.pos.x, swingbyShip.pos.y);
    float r = s->radius;
    float ang = angleTo(&swingbyShip.pos, s->pos.x, s->pos.y);
    addWithAngle(&swingbyShip.vel, ang, (difficulty * r * 0.01) / clamp(d - r, 2, 99));
    if (d < 99 + r) {
      vectorSet(&s->screenPos, s->pos.x, s->pos.y);
      vectorAdd(&s->screenPos, -swingbyShip.pos.x, -swingbyShip.pos.y);
      vectorAdd(&s->screenPos, swingbyShipScreenPos.x, swingbyShipScreenPos.y);
      thickness = 5;
      arc(s->screenPos.x, s->screenPos.y, r - 2, 0, CGLP_PI * 2, &scratch);
    } else {
      vectorSet(&s->screenPos, 999, 999);
    }
    s->isAlive = d <= 150;
  }
  if (distanceTo(&swingbyStarAddPos, swingbyShip.pos.x, swingbyShip.pos.y) > 50) {
    float a = angleTo(&swingbyStarAddPos, swingbyShip.pos.x, swingbyShip.pos.y);
    swingbyStarAddPos = swingbyShip.pos;
    TIMES(5, i) {
      Vector spawnCenter;
      vectorSet(&spawnCenter, swingbyStarAddPos.x, swingbyStarAddPos.y);
      addWithAngle(&spawnCenter, a, 100);
      swingbyAddStar(spawnCenter.x, spawnCenter.y, 70, a - CGLP_PI / 2, a + CGLP_PI / 2);
    }
  }
  vectorMul(&swingbyShip.vel, 1 - 0.01 / difficulty);
  vectorSet(&swingbyShipScreenPos, 50, 50);
  addWithAngle(&swingbyShipScreenPos, vectorAngle(&swingbyShip.vel) + CGLP_PI,
               clamp(sqrt(vectorLength(&swingbyShip.vel)) * 19, 1, 30));
  if (input.isPressed) {
    play(LASER);
    float a = vectorAngle(&swingbyShip.vel) + CGLP_PI / 2;
    addWithAngle(&swingbyShip.vel, a, difficulty * 0.05);
    particle(swingbyShipScreenPos.x, swingbyShipScreenPos.y, 1, 2, a + CGLP_PI, CGLP_PI / 8);
  }
  float a2 = vectorAngle(&swingbyShip.vel);
  addWithAngle(&swingbyShip.vel, a2, difficulty * 0.01);
  particle(swingbyShipScreenPos.x, swingbyShipScreenPos.y, 0.5, 1, a2 + CGLP_PI, CGLP_PI / 8);
  vectorAdd(&swingbyShip.pos, swingbyShip.vel.x, swingbyShip.vel.y);
  thickness = 3;
  barCenterPosRatio = 0.5;
  Collision bc;
  bar(swingbyShipScreenPos.x, swingbyShipScreenPos.y, 3, vectorAngle(&swingbyShip.vel), &bc);
  if (bc.isColliding.rect[BLACK]) {
    play(HIT);
    vectorMul(&swingbyShip.vel, 1 - 0.01 * difficulty);
    swingbyHitCount += 4 * difficulty;
    particle(swingbyShipScreenPos.x, swingbyShipScreenPos.y, 3, swingbyHitCount * 0.1, 0, CGLP_PI * 2);
    if (swingbyHitCount > 99) {
      play(EXPLOSION);
      gameOver();
    }
  } else {
    if (swingbyHitCount > 0) {
      swingbyHitCount -= difficulty;
    }
  }
  score = floor(vectorLength(&swingbyShip.pos));
}

void addGameSwingby() {
  addGame(swingbyTitle, swingbyDescription, swingbyCharacters,
          swingbyCharactersCount, &swingbyOptions, false, &swingbyUpdate);
}
