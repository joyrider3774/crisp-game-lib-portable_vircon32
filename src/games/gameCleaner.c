#include "../cglp.h"

int* cleanerTitle = "CLEANER";
int* cleanerDescription = "[Hold]\n Sweep";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] cleanerCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int cleanerCharactersCount = 0;

Options cleanerOptions = {100, 150, 8, true};

Vector cleanerHeadPos;
Vector cleanerPos;
float cleanerLength;

struct CleanerDust {
  Vector pos;
  Vector size;
  float angle;
  bool isAlive;
};
// Spawn interval scales as ~15/difficulty (ticks) but lifetime only as
// ~190/sqrt(difficulty) -> concurrent count ~12.67*sqrt(difficulty) grows
// unbounded; 256 covers difficulty up to ~400 (~400 min continuous play).
#define CLEANER_MAX_DUST_COUNT 256
CleanerDust[CLEANER_MAX_DUST_COUNT] cleanerDusts;
int cleanerDustIndex;
float cleanerNextDustDist;

struct CleanerHole {
  Vector pos;
  float size;
  bool isAlive;
};
#define CLEANER_MAX_HOLE_COUNT 16
CleanerHole[CLEANER_MAX_HOLE_COUNT] cleanerHoles;
int cleanerHoleIndex;
float cleanerNextHoleDist;

int cleanerMultiplier;

void cleanerUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&cleanerHeadPos, 50, 130);
    vectorSet(&cleanerPos, 50, 130);
    cleanerLength = 0;
    INIT_UNALIVED_ARRAY_FAST(cleanerDusts);
    cleanerDustIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(cleanerHoles);
    cleanerHoleIndex = 0;
    cleanerNextHoleDist = 99;
    cleanerNextDustDist = 0;
    cleanerMultiplier = 1;
  }
  float sd = sqrt(difficulty);
  float scrollSpeed = sd;
  cleanerHeadPos = cleanerPos;
  addWithAngle(&cleanerHeadPos, sin(ticks * 0.1 * sd) - CGLP_PI_2, cleanerLength);
  if (input.isJustPressed) {
    play(LASER);
    cleanerMultiplier = 1;
  }
  if (input.isJustReleased) {
    play(CLICK);
  }
  if (input.isPressed) {
    cleanerLength += (150 - cleanerLength) * 0.005 * difficulty;
  } else {
    cleanerPos.x += (cleanerHeadPos.x - cleanerPos.x) * 0.005 * cleanerLength * sd;
    cleanerLength *= 1 - 0.1 * sd;
  }
  cleanerPos.x = clamp(cleanerPos.x, 0, 100);
  color = LIGHT_CYAN;
  thickness = 7;
  line(cleanerHeadPos.x, cleanerHeadPos.y, cleanerPos.x, cleanerPos.y, &scratch);
  color = CYAN;
  box(cleanerHeadPos.x, cleanerHeadPos.y, 15, 10, &scratch);
  cleanerNextDustDist -= scrollSpeed;
  if (cleanerNextDustDist < 0) {
    ASSIGN_ARRAY_ITEM(cleanerDusts, cleanerDustIndex, CleanerDust, nd);
    vectorSet(&nd->pos, rnd(10, 90), -20);
    vectorSet(&nd->size, rnd(3, 9), rnd(3, 9));
    nd->angle = rnd(0, CGLP_PI * 2);
    nd->isAlive = true;
    cleanerDustIndex = cgl_wrap(cleanerDustIndex + 1, 0, CLEANER_MAX_DUST_COUNT);
    cleanerNextDustDist = rnd(10, 20) / sd;
  }
  color = LIGHT_BLACK;
  FOR_EACH(cleanerDusts, i) {
    ASSIGN_ARRAY_ITEM(cleanerDusts, i, CleanerDust, d);
    SKIP_IS_NOT_ALIVE(d);
    d->pos.y += scrollSpeed;
    thickness = d->size.y;
    bar(d->pos.x, d->pos.y, d->size.x, d->angle, &scratch);
    if (scratch.isColliding.rect[CYAN] || scratch.isColliding.rect[LIGHT_CYAN]) {
      play(COIN);
      particle(d->pos.x, d->pos.y, 9, 1, 0, CGLP_PI * 2);
      addScore(cleanerMultiplier, d->pos.x, d->pos.y);
      cleanerMultiplier++;
      d->isAlive = false;
      continue;
    }
    if (d->pos.y > 170) {
      d->isAlive = false;
      continue;
    }
  }
  cleanerNextHoleDist -= scrollSpeed;
  if (cleanerNextHoleDist < 0) {
    ASSIGN_ARRAY_ITEM(cleanerHoles, cleanerHoleIndex, CleanerHole, nh);
    vectorSet(&nh->pos, rnd(0, 100), -20);
    nh->size = rnd(5, 12);
    nh->isAlive = true;
    cleanerHoleIndex = cgl_wrap(cleanerHoleIndex + 1, 0, CLEANER_MAX_HOLE_COUNT);
    cleanerNextHoleDist = rnd(99, 120);
  }
  FOR_EACH(cleanerHoles, i) {
    ASSIGN_ARRAY_ITEM(cleanerHoles, i, CleanerHole, h);
    SKIP_IS_NOT_ALIVE(h);
    h->pos.y += scrollSpeed;
    color = RED;
    thickness = 3;
    arc(h->pos.x, h->pos.y, h->size, 0, CGLP_PI * 2, &scratch);
    if (h->pos.y > 170) {
      h->isAlive = false;
      continue;
    }
  }
  color = BLUE;
  thickness = 5;
  arc(cleanerPos.x, cleanerPos.y + 9, 3, 0, CGLP_PI * 2, &scratch);
  if (scratch.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameCleaner() {
  addGame(cleanerTitle, cleanerDescription, cleanerCharacters,
          cleanerCharactersCount, &cleanerOptions, false, &cleanerUpdate);
}
