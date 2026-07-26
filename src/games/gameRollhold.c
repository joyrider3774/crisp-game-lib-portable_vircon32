#include "../cglp.h"

int* rollholdTitle = "ROLL HOLD";
int* rollholdDescription = "[Hold]\n Hold an angle";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rollholdCharacters = {
    {
        " ll   ",
        "lll   ",
        "lllll ",
        " bb   ",
        "  cc  ",
        " bb   ",
    },
    {
        "lll   ",
        "rr    ",
        "llrrll",
        "LLRRll",
        "rr    ",
        "lll   ",
    },
    {
        "lrl   ",
        "llrrll",
        "LLRRll",
        "lrl   ",
        "      ",
        "      ",
    },
    {
        "lrlLll",
        "lrlLll",
        "      ",
        "      ",
        "      ",
        "      ",
    },
};
int rollholdCharactersCount = 4;

Options rollholdOptions = {100, 100, 9, true};

#define ROLLHOLD_TURRET_RADIUS 12

Vector rollholdPlayerPos;
float rollholdTurretAngle;
float rollholdTurretVa;
float rollholdFireTicks;
float rollholdAnimTicks;

int[4] rollholdEnemyFrameOffsets = {0, 1, 2, 1};

struct RollholdShot {
  Vector pos;
  float angle;
  bool isAlive;
};
#define ROLLHOLD_MAX_SHOT_COUNT 32
RollholdShot[ROLLHOLD_MAX_SHOT_COUNT] rollholdShots;
int rollholdShotIndex;

struct RollholdEnemy {
  Vector pos;
  float vx;
  float score;
  bool isFired;
  bool isAlive;
};
#define ROLLHOLD_MAX_ENEMY_COUNT 48
RollholdEnemy[ROLLHOLD_MAX_ENEMY_COUNT] rollholdEnemies;
int rollholdEnemyIndex;

struct RollholdNextEnemy {
  Vector pos;
  float vx;
};
RollholdNextEnemy rollholdNextEnemy;
float rollholdNextEnemyTicks;

struct RollholdBullet {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define ROLLHOLD_MAX_BULLET_COUNT 32
RollholdBullet[ROLLHOLD_MAX_BULLET_COUNT] rollholdBullets;
int rollholdBulletIndex;

struct RollholdBuilding {
  Vector pos;
  Vector size;
  bool isAlive;
};
#define ROLLHOLD_MAX_BUILDING_COUNT 32
RollholdBuilding[ROLLHOLD_MAX_BUILDING_COUNT] rollholdBuildings;
int rollholdBuildingIndex;
float rollholdNextBuildingTicks;

void rollholdSetNextEnemy() {
  float vx = sqrt(difficulty) * RNDPM() * 0.4;
  float ex;
  if (vx > 0) {
    ex = -3;
  } else {
    ex = 103;
  }
  vectorSet(&rollholdNextEnemy.pos, ex, rnd(9, 90));
  rollholdNextEnemy.vx = vx;
}

void rollholdUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&rollholdPlayerPos, 50, 50);
    rollholdTurretAngle = 0;
    rollholdTurretVa = 1;
    INIT_UNALIVED_ARRAY_FAST(rollholdShots);
    rollholdShotIndex = 0;
    rollholdFireTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(rollholdEnemies);
    rollholdEnemyIndex = 0;
    vectorSet(&rollholdNextEnemy.pos, 0, 0);
    rollholdNextEnemy.vx = 0;
    rollholdSetNextEnemy();
    rollholdNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(rollholdBullets);
    rollholdBulletIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(rollholdBuildings);
    rollholdBuildingIndex = 0;
    rollholdNextBuildingTicks = 0;
    rollholdAnimTicks = 0;
  }

  float df = sqrt(difficulty);
  rollholdNextBuildingTicks--;
  if (rollholdNextBuildingTicks <= 0) {
    ASSIGN_ARRAY_ITEM(rollholdBuildings, rollholdBuildingIndex, RollholdBuilding, nb);
    vectorSet(&nb->pos, 100, 85);
    vectorSet(&nb->size, rndi(10, 20), -rndi(30, 60));
    nb->isAlive = true;
    rollholdBuildingIndex = cgl_wrap(rollholdBuildingIndex + 1, 0, ROLLHOLD_MAX_BUILDING_COUNT);
    rollholdNextBuildingTicks = rndi(5, 50) * 10;
  }

  FOR_EACH(rollholdBuildings, bi) {
    ASSIGN_ARRAY_ITEM(rollholdBuildings, bi, RollholdBuilding, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x -= 0.1;
    color = LIGHT_BLACK;
    rect(b->pos.x, b->pos.y, b->size.x, b->size.y, &scratch);
    color = WHITE;
    rect(b->pos.x + 1, b->pos.y, b->size.x - 2, b->size.y + 1, &scratch);
    if (b->pos.x + b->size.x < 0) {
      b->isAlive = false;
      continue;
    }
  }

  color = LIGHT_CYAN;
  rect(0, 85, 100, 20, &scratch);
  color = LIGHT_PURPLE;
  rect(0, 70, 100, 9, &scratch);
  rect(0, 92, 100, 3, &scratch);

  rollholdAnimTicks += df;
  if (input.isJustPressed) {
    play(SELECT);
    rollholdTurretVa *= -1;
    rollholdFireTicks = 0;
  }

  Vector tp;
  vectorSet(&tp, ROLLHOLD_TURRET_RADIUS, 0);
  rotate(&tp, rollholdTurretAngle);
  vectorAdd(&tp, rollholdPlayerPos.x, rollholdPlayerPos.y);

  color = LIGHT_CYAN;
  if (!input.isPressed) {
    rollholdTurretAngle = cgl_wrap(rollholdTurretAngle + rollholdTurretVa * 0.07 * df, -CGLP_PI, CGLP_PI);
  } else {
    rollholdFireTicks -= df;
    if (rollholdFireTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(rollholdShots, rollholdShotIndex, RollholdShot, ns);
      vectorSet(&ns->pos, tp.x, tp.y);
      ns->angle = rollholdTurretAngle;
      ns->isAlive = true;
      rollholdShotIndex = cgl_wrap(rollholdShotIndex + 1, 0, ROLLHOLD_MAX_SHOT_COUNT);
      rollholdFireTicks = 9;
      particle(tp.x, tp.y, 3, 1, rollholdTurretAngle, 0.5);
    }
    thickness = 1;
    barCenterPosRatio = -0.5;
    float gaugeAngle;
    if (rollholdTurretVa > 0) {
      gaugeAngle = rollholdTurretAngle + CGLP_PI / 2;
    } else {
      gaugeAngle = rollholdTurretAngle - CGLP_PI / 2;
    }
    bar(tp.x, tp.y, 4, gaugeAngle, &scratch);
  }

  color = CYAN;
  FOR_EACH(rollholdShots, si) {
    ASSIGN_ARRAY_ITEM(rollholdShots, si, RollholdShot, s);
    SKIP_IS_NOT_ALIVE(s);
    addWithAngle(&s->pos, s->angle, df * 2);
    thickness = 2;
    barCenterPosRatio = 0.5;
    bar(s->pos.x, s->pos.y, 3, s->angle, &scratch);
    if (!(s->pos.x >= -9 && s->pos.x <= 111 && s->pos.y >= -9 && s->pos.y <= 111)) {
      s->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  characterOptions.isMirrorX = (rollholdTurretAngle < -CGLP_PI / 2 || rollholdTurretAngle > CGLP_PI / 2);
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  character("a", rollholdPlayerPos.x, rollholdPlayerPos.y, &scratch);

  color = BLUE;
  thickness = 3;
  barCenterPosRatio = 0.5;
  bar(tp.x, tp.y, 2, rollholdTurretAngle, &scratch);

  rollholdNextEnemyTicks -= df;
  if (rollholdNextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(rollholdEnemies, rollholdEnemyIndex, RollholdEnemy, ne);
    vectorSet(&ne->pos, rollholdNextEnemy.pos.x, rollholdNextEnemy.pos.y);
    ne->vx = rollholdNextEnemy.vx;
    ne->score = 9;
    ne->isFired = false;
    ne->isAlive = true;
    rollholdEnemyIndex = cgl_wrap(rollholdEnemyIndex + 1, 0, ROLLHOLD_MAX_ENEMY_COUNT);
    if (rnd(0, 1) < 0.25) {
      rollholdSetNextEnemy();
      rollholdNextEnemyTicks = 120 / df;
    } else {
      rollholdNextEnemyTicks = 25 / df;
    }
  }

  color = BLACK;
  FOR_EACH(rollholdEnemies, ei) {
    ASSIGN_ARRAY_ITEM(rollholdEnemies, ei, RollholdEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.x += e->vx;
    if (!e->isFired && ((e->vx > 0 && e->pos.x > 90) || (e->vx < 0 && e->pos.x < 9))) {
      play(CLICK);
      float a = angleTo(&e->pos, rollholdPlayerPos.x, rollholdPlayerPos.y);
      ASSIGN_ARRAY_ITEM(rollholdBullets, rollholdBulletIndex, RollholdBullet, nbul);
      vectorSet(&nbul->pos, e->pos.x, e->pos.y);
      vectorSet(&nbul->vel, df * 0.3, 0);
      rotate(&nbul->vel, a);
      nbul->isAlive = true;
      rollholdBulletIndex = cgl_wrap(rollholdBulletIndex + 1, 0, ROLLHOLD_MAX_BULLET_COUNT);
      particle(e->pos.x, e->pos.y, 3, 2, a, 0.2);
      e->isFired = true;
    }
    int frame = rollholdEnemyFrameOffsets[(int)(rollholdAnimTicks / 20) % 4];
    int[2] enemyChar;
    enemyChar[0] = 'b' + frame;
    enemyChar[1] = 0;
    characterOptions.isMirrorX = !(e->vx > 0);
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    character(enemyChar, e->pos.x, e->pos.y, &scratch);
    if (scratch.isColliding.rect[CYAN] || scratch.isColliding.rect[BLUE]) {
      play(POWER_UP);
      float scoreValue;
      if (scratch.isColliding.rect[BLUE]) {
        scoreValue = 10;
      } else {
        scoreValue = ceil(e->score);
      }
      addScore(scoreValue, e->pos.x, e->pos.y);
      particle(e->pos.x, e->pos.y, 16, 1, 0, CGLP_PI * 2);
      e->isAlive = false;
      continue;
    }
    e->score -= 0.033 * df;
    if (e->pos.x < -5 || e->pos.x > 105) {
      e->isAlive = false;
      continue;
    }
  }

  color = RED;
  FOR_EACH(rollholdBullets, bui) {
    ASSIGN_ARRAY_ITEM(rollholdBullets, bui, RollholdBullet, bul);
    SKIP_IS_NOT_ALIVE(bul);
    vectorAdd(&bul->pos, bul->vel.x, bul->vel.y);
    box(bul->pos.x, bul->pos.y, 4, 4, &scratch);
    if (scratch.isColliding.rect[BLUE]) {
      play(POWER_UP);
      addScore(10, bul->pos.x, bul->pos.y);
      particle(bul->pos.x, bul->pos.y, 16, 1, 0, CGLP_PI * 2);
      bul->isAlive = false;
      continue;
    } else if (scratch.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void addGameRollhold() {
  addGame(rollholdTitle, rollholdDescription, rollholdCharacters, rollholdCharactersCount,
          &rollholdOptions, false, &rollholdUpdate);
}
