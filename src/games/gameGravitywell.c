#include "../cglp.h"

int* gravitywellTitle = "GRAVITY WELL";
int* gravitywellDescription = "[Tap]\n Anti Gravity Pulse";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] gravitywellCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int gravitywellCharactersCount = 0;

Options gravitywellOptions = {100, 100, 5, true};

struct GravitywellPlanet {
  Vector pos;
  Vector velocity;
  float radius;
};
GravitywellPlanet gravitywellPlanet;
float gravitywellNextScoreAddingDist;

struct GravitywellBlackHole {
  Vector pos;
  float radius;
  float strength;
  bool isAlive;
};
#define GRAVITYWELL_MAX_BLACKHOLE_COUNT 16
GravitywellBlackHole[GRAVITYWELL_MAX_BLACKHOLE_COUNT] gravitywellBlackHoles;
int gravitywellBlackHoleIndex;
float gravitywellNextBlackHoleDist;

struct GravitywellPulse {
  Vector pos;
  float radius;
  float strength;
  bool isAlive;
};
#define GRAVITYWELL_MAX_PULSE_COUNT 32
GravitywellPulse[GRAVITYWELL_MAX_PULSE_COUNT] gravitywellPulses;
int gravitywellPulseIndex;

struct GravitywellStar {
  Vector pos;
  float vx;
};
#define GRAVITYWELL_STAR_COUNT 20
GravitywellStar[GRAVITYWELL_STAR_COUNT] gravitywellStars;

Vector gravitywellScrollingSpeed;

void gravitywellUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&gravitywellPlanet.pos, 50, 50);
    vectorSet(&gravitywellPlanet.velocity, 0, 0);
    gravitywellPlanet.radius = 9;
    gravitywellNextScoreAddingDist = 0;
    INIT_UNALIVED_ARRAY_FAST(gravitywellBlackHoles);
    gravitywellBlackHoleIndex = 0;
    gravitywellNextBlackHoleDist = 0;
    INIT_UNALIVED_ARRAY_FAST(gravitywellPulses);
    gravitywellPulseIndex = 0;
    TIMES(GRAVITYWELL_STAR_COUNT, i) {
      vectorSet(&gravitywellStars[i].pos, rnd(0, 100), rnd(0, 100));
      gravitywellStars[i].vx = -rnd(0.1, 0.2);
    }
    vectorSet(&gravitywellScrollingSpeed, 0, 0);
  }
  gravitywellScrollingSpeed.x = -0.5 * sqrt(difficulty);
  gravitywellNextScoreAddingDist += gravitywellScrollingSpeed.x;
  if (gravitywellNextScoreAddingDist < 0) {
    addScore(floor(gravitywellPlanet.radius), gravitywellPlanet.pos.x, gravitywellPlanet.pos.y);
    gravitywellNextScoreAddingDist += 30;
  }
  color = BLACK;
  TIMES(GRAVITYWELL_STAR_COUNT, i) {
    GravitywellStar* star = &gravitywellStars[i];
    star->pos.x += star->vx;
    if (star->pos.x < 0) {
      star->pos.x += 100;
    }
    box(star->pos.x, star->pos.y, 1, 1, &scratch);
  }
  FOR_EACH(gravitywellBlackHoles, i) {
    ASSIGN_ARRAY_ITEM(gravitywellBlackHoles, i, GravitywellBlackHole, bh);
    SKIP_IS_NOT_ALIVE(bh);
    vectorAdd(&bh->pos, gravitywellScrollingSpeed.x, gravitywellScrollingSpeed.y);
  }
  gravitywellNextBlackHoleDist += gravitywellScrollingSpeed.x;
  if (gravitywellNextBlackHoleDist < 0) {
    float radius = rnd(5, 9);
    ASSIGN_ARRAY_ITEM(gravitywellBlackHoles, gravitywellBlackHoleIndex, GravitywellBlackHole, nbh);
    vectorSet(&nbh->pos, 100 + radius, rnd(10, 90));
    nbh->radius = radius;
    nbh->strength = 0.1;
    nbh->isAlive = true;
    gravitywellBlackHoleIndex =
        cgl_wrap(gravitywellBlackHoleIndex + 1, 0, GRAVITYWELL_MAX_BLACKHOLE_COUNT);
    gravitywellNextBlackHoleDist += rnd(30, 40);
  }
  FOR_EACH(gravitywellBlackHoles, i) {
    ASSIGN_ARRAY_ITEM(gravitywellBlackHoles, i, GravitywellBlackHole, bh);
    SKIP_IS_NOT_ALIVE(bh);
    if (bh->pos.x < -10) {
      bh->isAlive = false;
    }
  }
  vectorAdd(&gravitywellPlanet.pos, gravitywellPlanet.velocity.x, gravitywellPlanet.velocity.y);
  float o = gravitywellPlanet.pos.x + 10;
  gravitywellPlanet.velocity.x += 1 / o / o;
  o = 110 - gravitywellPlanet.pos.x;
  gravitywellPlanet.velocity.x -= 1 / o / o;
  o = gravitywellPlanet.pos.y + 10;
  gravitywellPlanet.velocity.y += 1 / o / o;
  o = 105 - gravitywellPlanet.pos.y;
  gravitywellPlanet.velocity.y -= 1 / o / o;
  vectorMul(&gravitywellPlanet.velocity, 0.99);
  FOR_EACH(gravitywellBlackHoles, i) {
    ASSIGN_ARRAY_ITEM(gravitywellBlackHoles, i, GravitywellBlackHole, bh);
    SKIP_IS_NOT_ALIVE(bh);
    float dx = bh->pos.x - gravitywellPlanet.pos.x;
    float dy = bh->pos.y - gravitywellPlanet.pos.y;
    float distance = sqrt(dx * dx + dy * dy);
    if (distance > 0) {
      float force = bh->strength / distance;
      vectorAdd(&gravitywellPlanet.velocity, dx / distance * force, dy / distance * force);
    }
  }
  if (input.isJustPressed && gravitywellPlanet.radius > 2) {
    play(LASER);
    gravitywellPlanet.radius -= 1;
    ASSIGN_ARRAY_ITEM(gravitywellPulses, gravitywellPulseIndex, GravitywellPulse, np);
    np->pos = gravitywellPlanet.pos;
    np->radius = 0;
    np->strength = 0.5;
    np->isAlive = true;
    gravitywellPulseIndex = cgl_wrap(gravitywellPulseIndex + 1, 0, GRAVITYWELL_MAX_PULSE_COUNT);
  }
  FOR_EACH(gravitywellPulses, i) {
    ASSIGN_ARRAY_ITEM(gravitywellPulses, i, GravitywellPulse, pulse);
    SKIP_IS_NOT_ALIVE(pulse);
    pulse->pos = gravitywellPlanet.pos;
    pulse->radius += 1;
    FOR_EACH(gravitywellBlackHoles, j) {
      ASSIGN_ARRAY_ITEM(gravitywellBlackHoles, j, GravitywellBlackHole, bh);
      SKIP_IS_NOT_ALIVE(bh);
      float dx = bh->pos.x - pulse->pos.x;
      float dy = bh->pos.y - pulse->pos.y;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance > 0 && distance < pulse->radius + bh->radius) {
        float force = pulse->strength / sqrt(distance);
        float ux = dx / distance;
        float uy = dy / distance;
        vectorAdd(&gravitywellPlanet.velocity, -ux * force, -uy * force);
        vectorAdd(&bh->pos, ux * force, uy * force);
      }
    }
  }
  FOR_EACH(gravitywellPulses, i) {
    ASSIGN_ARRAY_ITEM(gravitywellPulses, i, GravitywellPulse, pulse);
    SKIP_IS_NOT_ALIVE(pulse);
    if (pulse->radius > 20) {
      pulse->isAlive = false;
    }
  }
  FOR_EACH(gravitywellBlackHoles, i) {
    ASSIGN_ARRAY_ITEM(gravitywellBlackHoles, i, GravitywellBlackHole, bh);
    SKIP_IS_NOT_ALIVE(bh);
    color = WHITE;
    box(bh->pos.x, bh->pos.y, bh->radius * 2, bh->radius * 2, &scratch);
    color = PURPLE;
    thickness = 3;
    arc(bh->pos.x, bh->pos.y, bh->radius, 0, CGLP_PI * 2, &scratch);
  }
  color = CYAN;
  FOR_EACH(gravitywellPulses, i) {
    ASSIGN_ARRAY_ITEM(gravitywellPulses, i, GravitywellPulse, pulse);
    SKIP_IS_NOT_ALIVE(pulse);
    thickness = 3;
    arc(pulse->pos.x, pulse->pos.y, pulse->radius, 0, CGLP_PI * 2, &scratch);
  }
  color = YELLOW;
  thickness = 3;
  arc(gravitywellPlanet.pos.x, gravitywellPlanet.pos.y, gravitywellPlanet.radius, 0, CGLP_PI * 2,
      &scratch);
  if (scratch.isColliding.rect[PURPLE]) {
    play(HIT);
    gravitywellPlanet.radius -= 0.2;
  } else {
    gravitywellPlanet.radius = clamp(gravitywellPlanet.radius + 0.05, 1, 9);
  }
  if (gravitywellPlanet.radius < 1) {
    play(EXPLOSION);
    gameOver();
  }
  if ((gravitywellPlanet.pos.x < 0 && gravitywellPlanet.velocity.x < 0) ||
      (gravitywellPlanet.pos.x > 100 && gravitywellPlanet.velocity.x > 0)) {
    gravitywellPlanet.velocity.x *= -1;
  }
  if ((gravitywellPlanet.pos.y < 0 && gravitywellPlanet.velocity.y < 0) ||
      (gravitywellPlanet.pos.y > 100 && gravitywellPlanet.velocity.y > 0)) {
    gravitywellPlanet.velocity.y *= -1;
  }
}

void addGameGravitywell() {
  addGame(gravitywellTitle, gravitywellDescription, gravitywellCharacters,
          gravitywellCharactersCount, &gravitywellOptions, false,
          &gravitywellUpdate);
}
