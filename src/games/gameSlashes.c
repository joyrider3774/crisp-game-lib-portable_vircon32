#include "../cglp.h"

int* slashesTitle = "SLASHES";
int* slashesDescription = "[Tap] Turn";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] slashesCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int slashesCharactersCount = 1;

Options slashesOptions = {100, 100, 3, true};

struct SlashesSlash {
  Vector pos;
  float angle;
  float width;
  bool isAppearing;
  bool isAlive;
};
#define SLASHES_MAX_SLASH_COUNT 32
SlashesSlash[SLASHES_MAX_SLASH_COUNT] slashesSlashes;
int slashesSlashIndex;
float slashesSlashAddTicks;
float slashesPlayerAngle;
float slashesSpeed;
float slashesAddingScore;

void slashesUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(slashesSlashes);
    slashesSlashIndex = 0;
    ASSIGN_ARRAY_ITEM(slashesSlashes, slashesSlashIndex, SlashesSlash, s0);
    vectorSet(&s0->pos, 22, 50);
    s0->angle = CGLP_PI / 4;
    s0->width = 30;
    s0->isAppearing = false;
    s0->isAlive = true;
    slashesSlashIndex = cgl_wrap(slashesSlashIndex + 1, 0, SLASHES_MAX_SLASH_COUNT);
    slashesSlashAddTicks = 0;
    slashesPlayerAngle = 1;
    slashesSpeed = 1;
    slashesAddingScore = 0;
  }
  if (input.isJustPressed) {
    play(SELECT);
    slashesPlayerAngle *= -1;
  }
  slashesSpeed += (difficulty - slashesSpeed) * 0.03;
  Vector scroll;
  vectorSet(&scroll, slashesSpeed * slashesPlayerAngle, slashesSpeed);
  slashesSlashAddTicks -= difficulty;
  if (slashesSlashAddTicks < 0) {
    float w = rndi(20, 50) + 20;
    ASSIGN_ARRAY_ITEM(slashesSlashes, slashesSlashIndex, SlashesSlash, ns);
    vectorSet(&ns->pos, rndi(-99, 199), -w);
    if (rnd(0, 1) < 0.5) {
      ns->angle = -CGLP_PI / 4;
    } else {
      ns->angle = CGLP_PI / 4;
    }
    ns->width = w;
    ns->isAppearing = true;
    ns->isAlive = true;
    slashesSlashIndex = cgl_wrap(slashesSlashIndex + 1, 0, SLASHES_MAX_SLASH_COUNT);
    slashesSlashAddTicks += rnd(10, 15);
  }
  FOR_EACH(slashesSlashes, i) {
    ASSIGN_ARRAY_ITEM(slashesSlashes, i, SlashesSlash, s);
    SKIP_IS_NOT_ALIVE(s);
    color = LIGHT_PURPLE;
    thickness = 22;
    barCenterPosRatio = 0.5;
    bar(s->pos.x, s->pos.y, s->width, s->angle, &scratch);
  }
  FOR_EACH(slashesSlashes, i) {
    ASSIGN_ARRAY_ITEM(slashesSlashes, i, SlashesSlash, s);
    SKIP_IS_NOT_ALIVE(s);
    color = PURPLE;
    if (s->isAppearing) {
      thickness = 20;
    } else {
      thickness = 4;
    }
    barCenterPosRatio = 0.5;
    Collision sc;
    bar(s->pos.x, s->pos.y, s->width, s->angle, &sc);
    if (s->isAppearing) {
      if (sc.isColliding.rect[PURPLE]) {
        s->isAlive = false;
        continue;
      }
      s->isAppearing = false;
      s->width -= 20;
    }
    vectorAdd(&s->pos, scroll.x, scroll.y);
    s->isAlive = s->pos.y < 99 + s->width;
  }
  color = BLACK;
  float pa = -(slashesPlayerAngle * CGLP_PI) / 4 + CGLP_PI / 2;
  thickness = 3;
  barCenterPosRatio = 0.9;
  Collision pc;
  bar(50, 90, 4, pa, &pc);
  if (pc.isColliding.rect[PURPLE]) {
    play(EXPLOSION);
    gameOver();
  } else if (pc.isColliding.rect[LIGHT_PURPLE]) {
    play(LASER);
    slashesSpeed += 0.03 * difficulty;
    slashesAddingScore += slashesSpeed;
    particle(50, 90, 3, slashesSpeed * 2, pa, 0.3);
  } else {
    particle(50, 90, 1, slashesSpeed, pa, 0);
    if (slashesAddingScore > 0) {
      play(POWER_UP);
      addScore(slashesAddingScore, 50, 90);
      slashesAddingScore = 0;
    }
  }
}

void addGameSlashes() {
  addGame(slashesTitle, slashesDescription, slashesCharacters,
          slashesCharactersCount, &slashesOptions, false, &slashesUpdate);
}
