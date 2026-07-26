#include "../cglp.h"

int* arcfireTitle = "ARCFIRE";
int* arcfireDescription = "[Hold]\n  Set arc\n[Release]\n  Fire";

int[5][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] arcfireCharacters = {
    {
        "  ll  ",
        "  l  l",
        " llll ",
        "l l   ",
        "  lll ",
        " l    ",
    },
    {
        "  ll  ",
        "l l   ",
        " llll ",
        "  l  l",
        "llll  ",
        "    l ",
    },
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    },
    {
        " llll ",
        "  l   ",
        " lllll",
        "l l   ",
        "  lll ",
        " l    ",
    },
    {
        " llll ",
        "  l   ",
        "lllll ",
        "  l  l",
        "llll  ",
        "    l ",
    },
};
int arcfireCharactersCount = 5;

Options arcfireOptions = {100, 100, 16, false};

struct ArcfireEnemy {
  Vector p;
  Vector v;
  bool isAlive;
};
#define ARCFIRE_MAX_ENEMY_COUNT 64
ArcfireEnemy[ARCFIRE_MAX_ENEMY_COUNT] arcfireEnemies;
int arcfireEnemyIndex;

Vector arcfirePos;
float arcfireMoveAngle;
float arcfireMoveDist;
float arcfireAngle;
float arcfireArcFrom;
float arcfireArcTo;
bool arcfireIsPressing;
float arcfireEnemyAddAngle;
float arcfireEnemyAddTicks;
int arcfireMultiplier;

bool arcfireShotIsAlive;
float arcfireShotD;
float arcfireShotRange;
float arcfireShotArcFrom;
float arcfireShotArcTo;

void arcfireUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&arcfirePos, 50, 50);
    arcfireAngle = 0;
    arcfireShotIsAlive = false;
    arcfireIsPressing = false;
    arcfireMoveAngle = 0;
    arcfireMoveDist = 0;
    INIT_UNALIVED_ARRAY_FAST(arcfireEnemies);
    arcfireEnemyIndex = 0;
    arcfireEnemyAddAngle = rnd(0, CGLP_PI * 2);
    arcfireEnemyAddTicks = 0;
    arcfireMultiplier = 1;
  }
  if (arcfireMoveDist > 1) {
    Vector d;
    vectorSet(&d, arcfireMoveDist * 0.2, 0);
    rotate(&d, arcfireMoveAngle);
    vectorAdd(&arcfirePos, d.x, d.y);
    arcfireMoveDist *= 0.2;
    if (!(arcfirePos.x >= 10 && arcfirePos.x <= 100 && arcfirePos.y >= 10 &&
          arcfirePos.y <= 100)) {
      arcfireMoveAngle += CGLP_PI;
    }
    arcfirePos.x = clamp(arcfirePos.x, 10, 90);
    arcfirePos.y = clamp(arcfirePos.y, 10, 90);
  }
  arcfireAngle += 0.07 * difficulty;
  color = LIGHT_BLUE;
  thickness = 4;
  arc(50, 50, 7, 0, CGLP_PI * 2, &scratch);
  color = LIGHT_BLACK;
  thickness = 2;
  Vector tip;
  vectorSet(&tip, 9, 0);
  rotate(&tip, arcfireAngle);
  vectorAdd(&tip, arcfirePos.x, arcfirePos.y);
  line(arcfirePos.x, arcfirePos.y, tip.x, tip.y, &scratch);
  color = BLACK;
  characterOptions.isMirrorX = cos(arcfireMoveAngle) < 0;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  int[2] playerChar;
  playerChar[0] = 'a' + ((int)(ticks / 30) % 2);
  playerChar[1] = 0;
  character(playerChar, arcfirePos.x, arcfirePos.y, &scratch);

  float range = 0;
  if (arcfireIsPressing) {
    arcfireArcTo = arcfireAngle;
    range = 300 / sqrt((arcfireArcTo - arcfireArcFrom) * 30);
    color = GREEN;
    thickness = 3;
    Vector l1;
    vectorSet(&l1, range, 0);
    rotate(&l1, arcfireArcFrom);
    vectorAdd(&l1, arcfirePos.x, arcfirePos.y);
    line(arcfirePos.x, arcfirePos.y, l1.x, l1.y, &scratch);
    Vector l2;
    vectorSet(&l2, range, 0);
    rotate(&l2, arcfireArcTo);
    vectorAdd(&l2, arcfirePos.x, arcfirePos.y);
    line(arcfirePos.x, arcfirePos.y, l2.x, l2.y, &scratch);
    thickness = 3;
    arc(arcfirePos.x, arcfirePos.y, range, arcfireArcFrom, arcfireArcTo, &scratch);
  }
  if (arcfireIsPressing && arcfireArcTo - arcfireArcFrom > CGLP_PI) {
    arcfireIsPressing = false;
  }
  if (arcfireIsPressing && input.isJustReleased) {
    arcfireIsPressing = false;
    if (!arcfireShotIsAlive) {
      play(SELECT);
      arcfireShotIsAlive = true;
      arcfireShotD = 0;
      arcfireShotRange = range;
      arcfireShotArcFrom = arcfireArcFrom;
      arcfireShotArcTo = arcfireArcTo;
    }
    arcfireMoveAngle = (arcfireArcTo + arcfireArcFrom) / 2;
    arcfireMoveDist = range / 2;
  }
  if (input.isJustPressed) {
    play(LASER);
    arcfireArcFrom = arcfireAngle;
    arcfireIsPressing = true;
    arcfireMultiplier = 1;
  }
  color = CYAN;
  if (arcfireShotIsAlive) {
    arcfireShotD += 2;
    thickness = 5;
    arc(arcfirePos.x, arcfirePos.y, arcfireShotD, arcfireShotArcFrom, arcfireShotArcTo, &scratch);
    arcfireShotIsAlive = arcfireShotD < arcfireShotRange;
  }

  arcfireEnemyAddTicks -= difficulty;
  if (arcfireEnemyAddTicks < 0) {
    Vector p;
    vectorSet(&p, 70, 0);
    rotate(&p, arcfireEnemyAddAngle);
    vectorAdd(&p, 50, 50);
    Vector v;
    vectorSet(&v, rnd(0, 10), 0);
    rotate(&v, rnd(0, CGLP_PI * 2));
    vectorAdd(&v, 50, 50);
    vectorAdd(&v, -p.x, -p.y);
    vectorMul(&v, 1.0 / (500 / rnd(1, difficulty)));
    ASSIGN_ARRAY_ITEM(arcfireEnemies, arcfireEnemyIndex, ArcfireEnemy, e);
    e->p = p;
    e->v = v;
    e->isAlive = true;
    arcfireEnemyIndex = cgl_wrap(arcfireEnemyIndex + 1, 0, ARCFIRE_MAX_ENEMY_COUNT);
    arcfireEnemyAddTicks += rnd(40, 60);
    if (rnd(0, 1) < 0.1) {
      arcfireEnemyAddAngle = rnd(0, CGLP_PI * 2);
    } else {
      arcfireEnemyAddAngle += rnd(0, 0.05) * RNDPM();
    }
  }
  color = RED;
  FOR_EACH(arcfireEnemies, i) {
    ASSIGN_ARRAY_ITEM(arcfireEnemies, i, ArcfireEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    vectorAdd(&e->p, e->v.x, e->v.y);
    characterOptions.isMirrorX = cos(vectorAngle(&e->v)) < 0;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    int[2] enemyChar;
    enemyChar[0] = 'd' + ((int)(ticks / 30) % 2);
    enemyChar[1] = 0;
    character(enemyChar, e->p.x, e->p.y, &scratch);
    if (scratch.isColliding.rect[CYAN]) {
      play(POWER_UP);
      particle(e->p.x, e->p.y, 9, 1, 0, CGLP_PI * 2);
      addScore(arcfireMultiplier, e->p.x, e->p.y);
      arcfireMultiplier++;
      e->isAlive = false;
      continue;
    }
    if (scratch.isColliding.character['a'] || scratch.isColliding.character['b'] ||
        scratch.isColliding.rect[LIGHT_BLUE]) {
      color = RED;
      if (scratch.isColliding.rect[LIGHT_BLUE]) {
        float tx = (e->p.x - 50) / 2 + 50;
        float ty = (e->p.y - 50) / 2 + 50;
        text("X", tx, ty, &scratch);
      } else {
        text("X", arcfirePos.x, arcfirePos.y, &scratch);
      }
      play(RANDOM);
      gameOver();
    }
  }
}

void addGameArcfire() {
  addGame(arcfireTitle, arcfireDescription, arcfireCharacters,
          arcfireCharactersCount, &arcfireOptions, false, &arcfireUpdate);
}
