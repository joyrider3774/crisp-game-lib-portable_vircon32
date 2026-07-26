#include "../cglp.h"

int* fallbounceTitle = "FALL BOUNCE";
int* fallbounceDescription = "[Hold]\n Fall & Bounce";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] fallbounceCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int fallbounceCharactersCount = 0;

Options fallbounceOptions = {100, 100, 1, false};

#define FALLBOUNCE_PLATFORM_WIDTH 15

struct FallbouncePlatform {
  Vector pos;
  float width;
  bool isAlive;
};
#define FALLBOUNCE_MAX_PLATFORM_COUNT 16
FallbouncePlatform[FALLBOUNCE_MAX_PLATFORM_COUNT] fallbouncePlatforms;
int fallbouncePlatformIndex;
float fallbounceNextPlatformTicks;

struct FallbounceWhale {
  Vector pos;
  float vx;
  float width;
  bool hasTargetY;
  float targetY;
};
FallbounceWhale fallbounceWhale;

void fallbounceSpawnPlatform() {
  ASSIGN_ARRAY_ITEM(fallbouncePlatforms, fallbouncePlatformIndex, FallbouncePlatform, p);
  vectorSet(&p->pos, rnd(0, 100 - FALLBOUNCE_PLATFORM_WIDTH), -5);
  p->width = rnd(FALLBOUNCE_PLATFORM_WIDTH, 35);
  p->isAlive = true;
  fallbouncePlatformIndex =
      cgl_wrap(fallbouncePlatformIndex + 1, 0, FALLBOUNCE_MAX_PLATFORM_COUNT);
}

void fallbounceUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(fallbouncePlatforms);
    fallbouncePlatformIndex = 0;
    TIMES(2, i) { fallbounceSpawnPlatform(); }
    vectorSet(&fallbounceWhale.pos, 50, 10);
    fallbounceWhale.vx = 1;
    fallbounceWhale.width = 10;
    fallbounceWhale.hasTargetY = false;
    fallbounceNextPlatformTicks = 0;
  }
  if (fallbounceWhale.hasTargetY) {
    fallbounceWhale.pos.y += (fallbounceWhale.targetY - fallbounceWhale.pos.y) * 0.2;
    if (fabs(fallbounceWhale.targetY - fallbounceWhale.pos.y) < 1) {
      fallbounceWhale.hasTargetY = false;
    }
  } else {
    float fallSpeed;
    if (input.isPressed) {
      fallSpeed = difficulty * 1.5;
    } else {
      fallSpeed = difficulty * 0.25;
    }
    fallbounceWhale.pos.y += fallSpeed;
  }
  fallbounceWhale.pos.x = cgl_wrap(fallbounceWhale.pos.x + fallbounceWhale.vx, 0, 100);
  if (input.isPressed) {
    color = CYAN;
  } else {
    color = LIGHT_BLACK;
  }
  box(fallbounceWhale.pos.x, fallbounceWhale.pos.y, fallbounceWhale.width, 3, &scratch);
  if (fallbounceWhale.pos.y > 103) {
    play(HIT);
    gameOver();
  }
  fallbounceNextPlatformTicks--;
  if (fallbounceNextPlatformTicks < 0) {
    TIMES(2, i) { fallbounceSpawnPlatform(); }
    fallbounceNextPlatformTicks = ceil(rnd(40, 60) / difficulty);
  }
  FOR_EACH(fallbouncePlatforms, i) {
    ASSIGN_ARRAY_ITEM(fallbouncePlatforms, i, FallbouncePlatform, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.y += difficulty * 1;
    color = BLUE;
    box(p->pos.x, p->pos.y, p->width, 5, &scratch);
    bool isLanded = scratch.isColliding.rect[CYAN] && !fallbounceWhale.hasTargetY;
    if (isLanded) {
      play(POWER_UP);
      addScore(fallbounceWhale.pos.y - 10, p->pos.x, p->pos.y);
      fallbounceWhale.targetY = 10;
      fallbounceWhale.hasTargetY = true;
    }
    if (p->pos.y >= 103) {
      p->isAlive = false;
      continue;
    }
  }
}

void addGameFallbounce() {
  addGame(fallbounceTitle, fallbounceDescription, fallbounceCharacters,
          fallbounceCharactersCount, &fallbounceOptions, false,
          &fallbounceUpdate);
}
