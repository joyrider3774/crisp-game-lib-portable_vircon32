#include "../cglp.h"

int* orbitmanTitle = "ORBIT MAN";
int* orbitmanDescription = "[Tap]\n Launch";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] orbitmanCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int orbitmanCharactersCount = 1;

Options orbitmanOptions = {100, 100, 600, true};

struct OrbitmanPlanet {
  Vector pos;
  float radius;
  bool isDestroyed;
  bool isAlive;
};
#define ORBITMAN_MAX_PLANET_COUNT 64
OrbitmanPlanet[ORBITMAN_MAX_PLANET_COUNT] orbitmanPlanets;
int orbitmanPlanetIndex;
float orbitmanNextPlanetDist;

struct OrbitmanMan {
  int planetIndex;
  float angle;
  float av;
  Vector pos;
  Vector target;
};
OrbitmanMan orbitmanMan;
float orbitmanFlyingTicks;
float orbitmanMultiplier;

struct OrbitmanStar {
  Vector pos;
  float vy;
};
#define ORBITMAN_STAR_COUNT 20
OrbitmanStar[ORBITMAN_STAR_COUNT] orbitmanStars;

#define ORBITMAN_MAX_PIERCED_COUNT 16

void orbitmanUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(orbitmanPlanets);
    orbitmanPlanetIndex = 0;
    ASSIGN_ARRAY_ITEM(orbitmanPlanets, orbitmanPlanetIndex, OrbitmanPlanet, p0);
    vectorSet(&p0->pos, 50, 0);
    p0->radius = 5;
    p0->isDestroyed = false;
    p0->isAlive = true;
    int planet0Index = orbitmanPlanetIndex;
    orbitmanPlanetIndex = cgl_wrap(orbitmanPlanetIndex + 1, 0, ORBITMAN_MAX_PLANET_COUNT);
    orbitmanNextPlanetDist = 20;
    orbitmanMan.planetIndex = planet0Index;
    orbitmanMan.angle = CGLP_PI / 2;
    orbitmanMan.av = 1;
    vectorSet(&orbitmanMan.pos, 50, 0);
    vectorSet(&orbitmanMan.target, 50, 0);
    orbitmanFlyingTicks = 0;
    orbitmanMultiplier = 1;
    TIMES(ORBITMAN_STAR_COUNT, i) {
      vectorSet(&orbitmanStars[i].pos, rnd(0, 99), rnd(0, 99));
      orbitmanStars[i].vy = rnd(3, 6);
    }
  }
  float scr = sqrt(difficulty) * 0.05;
  orbitmanFlyingTicks = clamp(orbitmanFlyingTicks - difficulty, 1, 99);
  OrbitmanPlanet* manPlanet = &orbitmanPlanets[orbitmanMan.planetIndex];
  if (manPlanet->pos.y < 80) {
    scr += (80 - manPlanet->pos.y) * (0.1 / orbitmanFlyingTicks);
  }
  color = LIGHT_BLACK;
  TIMES(ORBITMAN_STAR_COUNT, i) {
    OrbitmanStar* s = &orbitmanStars[i];
    s->pos.y += scr / s->vy;
    if (s->pos.y > 99) {
      vectorSet(&s->pos, rnd(0, 99), 0);
      s->vy = rnd(3, 6);
    }
    rect(s->pos.x, s->pos.y, 1, 1, &scratch);
  }
  orbitmanNextPlanetDist -= scr;
  while (orbitmanNextPlanetDist < 0) {
    float radius = rnd(4, 9);
    ASSIGN_ARRAY_ITEM(orbitmanPlanets, orbitmanPlanetIndex, OrbitmanPlanet, np);
    vectorSet(&np->pos, rnd(10 + radius, 90 - radius), orbitmanNextPlanetDist - 30);
    np->radius = radius;
    np->isDestroyed = false;
    np->isAlive = true;
    orbitmanPlanetIndex = cgl_wrap(orbitmanPlanetIndex + 1, 0, ORBITMAN_MAX_PLANET_COUNT);
    orbitmanNextPlanetDist += radius * rnd(1, 2);
  }
  orbitmanMan.angle += difficulty * 0.03 * orbitmanMan.av;
  color = LIGHT_BLUE;
  thickness = 4;
  barCenterPosRatio = -manPlanet->radius * 0.015;
  bar(manPlanet->pos.x, manPlanet->pos.y, 99, orbitmanMan.angle, &scratch);
  color = BLACK;
  int nextPlanetIndex = -1;
  float maxDist = 0;
  int[ORBITMAN_MAX_PIERCED_COUNT] piercedPlanetIndices;
  int piercedCount = 0;
  piercedPlanetIndices[piercedCount] = orbitmanMan.planetIndex;
  piercedCount++;
  FOR_EACH(orbitmanPlanets, i) {
    ASSIGN_ARRAY_ITEM(orbitmanPlanets, i, OrbitmanPlanet, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->isDestroyed) {
      particle(p->pos.x, p->pos.y, ceil(p->radius * 4), sqrt(p->radius) * 0.5, 0, CGLP_PI * 2);
      p->isAlive = false;
      continue;
    }
    p->pos.y += scr;
    thickness = 3;
    Collision pc;
    arc(p->pos.x, p->pos.y, p->radius, 0, CGLP_PI * 2, &pc);
    if (i != orbitmanMan.planetIndex && pc.isColliding.rect[BLACK]) {
      p->isAlive = false;
      continue;
    }
    if (i != orbitmanMan.planetIndex && p->pos.y > -p->radius - 4 &&
        pc.isColliding.rect[LIGHT_BLUE]) {
      if (piercedCount < ORBITMAN_MAX_PIERCED_COUNT) {
        piercedPlanetIndices[piercedCount] = i;
        piercedCount++;
      }
      float d = distanceTo(&p->pos, manPlanet->pos.x, manPlanet->pos.y);
      if (d > maxDist) {
        nextPlanetIndex = i;
        maxDist = d;
      }
    }
    p->isAlive = p->pos.y <= 100 + p->radius * 2;
  }
  if (input.isJustPressed) {
    if (nextPlanetIndex == -1) {
      play(EXPLOSION);
      TIMES(99, i) {
        addWithAngle(&orbitmanMan.pos, orbitmanMan.angle, 3);
        bool posInRect = orbitmanMan.pos.x >= 5 && orbitmanMan.pos.x < 100 &&
                          orbitmanMan.pos.y >= 5 && orbitmanMan.pos.y < 100;
        if (!posInRect) {
          break;
        }
      }
      gameOver();
    } else {
      play(POWER_UP);
      if (orbitmanMultiplier > 1) {
        orbitmanMultiplier--;
      }
      if (piercedCount > 2) {
        play(HIT);
      }
      TIMES(piercedCount, i) {
        if (piercedPlanetIndices[i] != nextPlanetIndex) {
          OrbitmanPlanet* pp = &orbitmanPlanets[piercedPlanetIndices[i]];
          pp->isDestroyed = true;
          addScore(orbitmanMultiplier, pp->pos.x, pp->pos.y);
          orbitmanMultiplier++;
        }
      }
      orbitmanMan.planetIndex = nextPlanetIndex;
      orbitmanMan.angle += CGLP_PI;
      orbitmanMan.av *= -1;
      orbitmanFlyingTicks = 20;
    }
  }
  manPlanet = &orbitmanPlanets[orbitmanMan.planetIndex];
  float a = orbitmanMan.angle;
  vectorSet(&orbitmanMan.target, manPlanet->pos.x, manPlanet->pos.y);
  addWithAngle(&orbitmanMan.target, a, manPlanet->radius);
  Vector delta;
  vectorSet(&delta, orbitmanMan.target.x, orbitmanMan.target.y);
  vectorAdd(&delta, -orbitmanMan.pos.x, -orbitmanMan.pos.y);
  vectorMul(&delta, 0.1);
  vectorAdd(&orbitmanMan.pos, delta.x, delta.y);
  color = CYAN;
  thickness = 2;
  Vector p1;
  vectorSet(&p1, orbitmanMan.pos.x, orbitmanMan.pos.y);
  addWithAngle(&p1, a, 4);
  Vector p2;
  vectorSet(&p2, p1.x, p1.y);
  addWithAngle(&p2, a + CGLP_PI * 0.75, 3);
  line(p1.x, p1.y, p2.x, p2.y, &scratch);
  vectorSet(&p2, p1.x, p1.y);
  addWithAngle(&p2, a - CGLP_PI * 0.75, 3);
  thickness = 2;
  line(p1.x, p1.y, p2.x, p2.y, &scratch);
  vectorSet(&p2, p1.x, p1.y);
  addWithAngle(&p2, a, 4);
  thickness = 2;
  line(p1.x, p1.y, p2.x, p2.y, &scratch);
  vectorSet(&p2, p1.x, p1.y);
  addWithAngle(&p2, a - CGLP_PI * 0.3, 3);
  addWithAngle(&p1, a + CGLP_PI * 0.3, 3);
  thickness = 2;
  line(p1.x, p1.y, p2.x, p2.y, &scratch);
  if (manPlanet->pos.y - manPlanet->radius > 99) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameOrbitman() {
  addGame(orbitmanTitle, orbitmanDescription, orbitmanCharacters,
          orbitmanCharactersCount, &orbitmanOptions, false, &orbitmanUpdate);
}
