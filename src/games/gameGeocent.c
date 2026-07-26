#include "../cglp.h"

int* geocentTitle = "GEOCENT";
int* geocentDescription = "[Hold]\n Speed up & Turn";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] geocentCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int geocentCharactersCount = 1;

Options geocentOptions = {100, 100, 1, true};

struct GeocentStar {
  Vector cPos;
  float radius;
  float angle;
  float av;
  float size;
  int color;
  Vector pos;
};
#define GEOCENT_STAR_COUNT 9
GeocentStar[GEOCENT_STAR_COUNT] geocentStars;
float[GEOCENT_STAR_COUNT] geocentStarSizes = {3, 1, 1, 2, 1, 1, 1, 1, 1};
int[GEOCENT_STAR_COUNT] geocentStarColors = {YELLOW, RED, RED,  CYAN, RED,
                                              RED,    RED, RED, RED};

#define GEOCENT_BG_STAR_COUNT 20
Vector[GEOCENT_BG_STAR_COUNT] geocentBgStars;

bool geocentRocketDistActive;
float geocentRocketAngle;
float geocentRocketVa;
float geocentRocketDist;
float geocentRocketSpeed;

// The original JS keeps posHistory unbounded for the entire game and never
// pops it - crates only ever reference an index up to crateCount*9, and
// crateCount only grows (by at most +1) each time a full cycle completes
// without fully clearing its own crates, so this cap gives enormous
// headroom for any realistic play session while keeping the array small.
#define GEOCENT_MAX_POS_HISTORY 300
Vector[GEOCENT_MAX_POS_HISTORY] geocentPosHistory;
int geocentPosHistoryCount;

#define GEOCENT_MAX_CRATE_COUNT 32
int[GEOCENT_MAX_CRATE_COUNT] geocentCrates;
int geocentCrateCount;

void geocentUpdate() {
  Collision scratch;
  if (!ticks) {
    float av = 0.03;
    TIMES(GEOCENT_STAR_COUNT, i) {
      av *= 0.7;
      vectorSet(&geocentStars[i].cPos, 0, 0);
      geocentStars[i].radius = i * 4;
      geocentStars[i].angle = rnd(0, CGLP_PI * 2);
      geocentStars[i].av = av;
      geocentStars[i].size = geocentStarSizes[i];
      geocentStars[i].color = geocentStarColors[i];
      vectorSet(&geocentStars[i].pos, 0, 0);
    }
    geocentRocketAngle = 0;
    geocentRocketVa = 0;
    geocentRocketDistActive = false;
    geocentRocketSpeed = 0;
    geocentPosHistoryCount = 0;
    geocentCrateCount = 0;
    TIMES(GEOCENT_BG_STAR_COUNT, i) {
      vectorSet(&geocentBgStars[i], rnd(0, 99), rnd(0, 99));
    }
  }
  color = LIGHT_BLACK;
  TIMES(GEOCENT_BG_STAR_COUNT, i) {
    box(geocentBgStars[i].x, geocentBgStars[i].y, 1, 1, &scratch);
  }
  TIMES(GEOCENT_STAR_COUNT, i) {
    geocentStars[i].angle += geocentStars[i].av * difficulty;
    vectorSet(&geocentStars[i].cPos, 50, 50);
    addWithAngle(&geocentStars[i].cPos, geocentStars[i].angle, geocentStars[i].radius);
  }
  Vector ep;
  vectorSet(&ep, geocentStars[3].cPos.x, geocentStars[3].cPos.y);
  TIMES(GEOCENT_STAR_COUNT, i) {
    vectorSet(&geocentStars[i].pos, geocentStars[i].cPos.x, geocentStars[i].cPos.y);
    vectorAdd(&geocentStars[i].pos, -ep.x, -ep.y);
    vectorAdd(&geocentStars[i].pos, 50, 50);
    color = geocentStars[i].color;
    arc(geocentStars[i].pos.x, geocentStars[i].pos.y, geocentStars[i].size, 0,
        CGLP_PI * 2, &scratch);
  }
  if (!geocentRocketDistActive) {
    geocentRocketAngle = ((rndi(0, 4) + 0.3 + rnd(0, 0.4)) * CGLP_PI) / 2;
    geocentRocketDist = fabs(sin(geocentRocketAngle * 2)) * 25 + 36;
    geocentRocketSpeed = 0;
    geocentPosHistoryCount = 0;
    int cn = geocentCrateCount + 1;
    if (cn > GEOCENT_MAX_CRATE_COUNT) {
      cn = GEOCENT_MAX_CRATE_COUNT;
    }
    TIMES(cn, i) { geocentCrates[i] = (i + 1) * 9; }
    geocentCrateCount = cn;
    geocentRocketDistActive = true;
  }
  if (input.isJustPressed) {
    play(SELECT);
    geocentRocketSpeed += difficulty;
  }
  float targetSpeed;
  if (input.isPressed) {
    targetSpeed = 5;
  } else {
    targetSpeed = 1;
  }
  geocentRocketSpeed += (targetSpeed * difficulty * 0.1 - geocentRocketSpeed) * 0.5;
  geocentRocketDist -= geocentRocketSpeed;
  float targetVa;
  if (input.isPressed) {
    targetVa = 0.03;
  } else {
    targetVa = -0.003;
  }
  geocentRocketVa += (targetVa * difficulty - geocentRocketVa) * 0.05;
  geocentRocketAngle += geocentRocketVa;
  Vector p;
  vectorSet(&p, 50, 50);
  addWithAngle(&p, geocentRocketAngle, geocentRocketDist);
  int shiftCount = geocentPosHistoryCount;
  if (shiftCount > GEOCENT_MAX_POS_HISTORY - 1) {
    shiftCount = GEOCENT_MAX_POS_HISTORY - 1;
  }
  for (int k = shiftCount - 1; k >= 0; k--) {
    geocentPosHistory[k + 1] = geocentPosHistory[k];
  }
  geocentPosHistory[0] = p;
  if (geocentPosHistoryCount < GEOCENT_MAX_POS_HISTORY) {
    geocentPosHistoryCount++;
  }
  color = BLACK;
  int ci = 0;
  while (ci < geocentCrateCount) {
    float cpx, cpy;
    if (geocentCrates[ci] >= geocentPosHistoryCount) {
      cpx = p.x;
      cpy = p.y;
    } else {
      cpx = geocentPosHistory[geocentCrates[ci]].x;
      cpy = geocentPosHistory[geocentCrates[ci]].y;
    }
    Collision cc;
    box(cpx, cpy, 2, 2, &cc);
    if (cc.isColliding.rect[RED] || cc.isColliding.rect[YELLOW]) {
      play(HIT);
      particle(cpx, cpy, 16, 1, 0, CGLP_PI * 2);
      memcpy(&geocentCrates[ci], &geocentCrates[ci + 1],
             (geocentCrateCount - 1 - ci) * sizeof(geocentCrates[0]));
      geocentCrateCount--;
    } else {
      ci++;
    }
  }
  color = RED;
  particle(p.x, p.y, 1, geocentRocketSpeed * 3, geocentRocketAngle, 0.5);
  color = BLUE;
  Vector bp;
  vectorSet(&bp, p.x, p.y);
  addWithAngle(&bp, geocentRocketAngle, 2);
  thickness = 3;
  barCenterPosRatio = 0.5;
  bar(bp.x, bp.y, 2, geocentRocketAngle + CGLP_PI / 2, &scratch);
  color = BLACK;
  thickness = 3;
  barCenterPosRatio = 0.5;
  Collision finalC;
  bar(p.x, p.y, 1, geocentRocketAngle, &finalC);
  if (finalC.isColliding.rect[CYAN]) {
    play(COIN);
    addScore(geocentCrateCount, p.x, p.y);
    geocentRocketDistActive = false;
  } else if (finalC.isColliding.rect[RED] || finalC.isColliding.rect[YELLOW]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameGeocent() {
  addGame(geocentTitle, geocentDescription, geocentCharacters,
          geocentCharactersCount, &geocentOptions, false, &geocentUpdate);
}
