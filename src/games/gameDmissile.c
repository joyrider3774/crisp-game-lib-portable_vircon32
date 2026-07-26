#include "../cglp.h"

int* dmissileTitle = "D MISSILE";
int* dmissileDescription = "[Tap]  Launch\n[Hold] Turn";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] dmissileCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int dmissileCharactersCount = 1;

Options dmissileOptions = {100, 100, 9, false};

#define DMISSILE_INIT_ANGLE ((-CGLP_PI / 9) * 8)

struct DmissileEnemy {
  Vector pos;
  Vector vel;
  Vector from;
  int ticksLeft;
  int id;
  bool isRemoving;
  bool isAlive;
};
#define DMISSILE_MAX_ENEMY_COUNT 64
DmissileEnemy[DMISSILE_MAX_ENEMY_COUNT] dmissileEnemies;
int dmissileEnemyIndex;
float dmissileNextEnemyTicks;
int dmissileEnemyId;

struct DmissilePlayer {
  Vector pos;
  float angle;
  float speed;
  bool isAlive;
};
#define DMISSILE_MAX_PLAYER_COUNT 8
DmissilePlayer[DMISSILE_MAX_PLAYER_COUNT] dmissilePlayers;
int dmissilePlayerIndex;
float dmissileTapTicks;
float dmissileMultiplier;

void dmissileUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(dmissileEnemies);
    dmissileEnemyIndex = 0;
    dmissileNextEnemyTicks = 0;
    dmissileEnemyId = 0;
    INIT_UNALIVED_ARRAY_FAST(dmissilePlayers);
    dmissilePlayerIndex = 0;
    dmissileTapTicks = 0;
    dmissileMultiplier = 1;
  }
  if (input.isJustPressed) {
    play(SELECT);
    COUNT_IS_ALIVE(dmissilePlayers, alivePlayerCount);
    if (alivePlayerCount == 0) {
      dmissileTapTicks = 99;
    } else if (alivePlayerCount >= 3) {
      dmissileTapTicks = 0;
    } else {
      dmissileTapTicks = 9;
    }
  }
  dmissileTapTicks--;
  if (input.isJustReleased && dmissileTapTicks > 0) {
    play(COIN);
    ASSIGN_ARRAY_ITEM(dmissilePlayers, dmissilePlayerIndex, DmissilePlayer, np);
    vectorSet(&np->pos, 60, 90);
    np->angle = DMISSILE_INIT_ANGLE;
    np->speed = 0.5 * sqrt(difficulty);
    np->isAlive = true;
    dmissilePlayerIndex = cgl_wrap(dmissilePlayerIndex + 1, 0, DMISSILE_MAX_PLAYER_COUNT);
    dmissileMultiplier *= 0.9;
    if (dmissileMultiplier < 1) {
      dmissileMultiplier = 1;
    }
  }
  FOR_EACH(dmissilePlayers, i) {
    ASSIGN_ARRAY_ITEM(dmissilePlayers, i, DmissilePlayer, p);
    SKIP_IS_NOT_ALIVE(p);
    addWithAngle(&p->pos, p->angle, p->speed);
    p->speed *= 1.002;
    if (input.isPressed) {
      float rate;
      if (p->speed < 3 * sqrt(difficulty)) {
        rate = 0.1;
      } else {
        rate = 0.01;
      }
      p->angle += p->speed * rate;
    }
    color = PURPLE;
    Vector bp;
    vectorSet(&bp, p->pos.x, p->pos.y);
    addWithAngle(&bp, p->angle, -3);
    box(bp.x, bp.y, 5, 5, &scratch);
    color = BLUE;
    thickness = 3;
    barCenterPosRatio = 0.5;
    bar(p->pos.x, p->pos.y, 3, p->angle, &scratch);
    bool pInRect = p->pos.x >= -3 && p->pos.x < 103 && p->pos.y >= -3 && p->pos.y < 90;
    p->isAlive = pInRect;
  }
  COUNT_IS_ALIVE(dmissilePlayers, alivePlayerCount2);
  if (alivePlayerCount2 < 3) {
    color = BLUE;
    thickness = 3;
    barCenterPosRatio = 0.5;
    bar(60, 90, 3, DMISSILE_INIT_ANGLE, &scratch);
  }
  color = LIGHT_BLACK;
  rect(0, 90, 100, 10, &scratch);
  COUNT_IS_ALIVE(dmissileEnemies, aliveEnemyCount);
  if (aliveEnemyCount == 0) {
    dmissileNextEnemyTicks = 0;
  }
  dmissileNextEnemyTicks--;
  if (dmissileNextEnemyTicks < 0) {
    int c = rndi(3, 6);
    TIMES(c, i) {
      Vector pos;
      vectorSet(&pos, rnd(10, 90), -rnd(5, 25));
      float velAngle = angleTo(&pos, rnd(10, 90), 90);
      Vector vel;
      vectorSet(&vel, rnd(1, sqrt(difficulty)) * 0.1, 0);
      rotate(&vel, velAngle);
      int ticksVal;
      if (rnd(0, 1) < sqrt(difficulty)) {
        ticksVal = ceil(rnd(300, 400) / sqrt(difficulty));
      } else {
        ticksVal = 999;
      }
      ASSIGN_ARRAY_ITEM(dmissileEnemies, dmissileEnemyIndex, DmissileEnemy, ne);
      ne->pos = pos;
      ne->vel = vel;
      ne->from = pos;
      ne->ticksLeft = ticksVal;
      ne->id = dmissileEnemyId;
      ne->isRemoving = false;
      ne->isAlive = true;
      dmissileEnemyIndex = cgl_wrap(dmissileEnemyIndex + 1, 0, DMISSILE_MAX_ENEMY_COUNT);
      dmissileEnemyId++;
    }
    dmissileNextEnemyTicks = (150 / sqrt(difficulty)) * c;
  }
  FOR_EACH(dmissileEnemies, i) {
    ASSIGN_ARRAY_ITEM(dmissileEnemies, i, DmissileEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (e->isRemoving) {
      e->isAlive = false;
      continue;
    }
    if (e->ticksLeft > 0) {
      vectorAdd(&e->pos, e->vel.x, e->vel.y);
      e->ticksLeft--;
      if (e->ticksLeft == 0) {
        play(HIT);
        int c2 = rndi(2, 5);
        TIMES(c2, k) {
          Vector pos2;
          vectorSet(&pos2, e->pos.x, e->pos.y);
          float velLen = vectorLength(&e->vel);
          float velAngle2 = angleTo(&pos2, rnd(10, 90), 90);
          Vector vel2;
          vectorSet(&vel2, velLen, 0);
          rotate(&vel2, velAngle2);
          ASSIGN_ARRAY_ITEM(dmissileEnemies, dmissileEnemyIndex, DmissileEnemy, fe);
          fe->pos = pos2;
          fe->vel = vel2;
          fe->from = pos2;
          fe->ticksLeft = 999;
          if (k == 0) {
            fe->id = e->id;
          } else {
            fe->id = dmissileEnemyId;
          }
          fe->isRemoving = false;
          fe->isAlive = true;
          dmissileEnemyIndex = cgl_wrap(dmissileEnemyIndex + 1, 0, DMISSILE_MAX_ENEMY_COUNT);
          dmissileEnemyId++;
        }
      }
    }
    color = LIGHT_BLACK;
    thickness = 2;
    line(e->from.x, e->from.y, e->pos.x, e->pos.y, &scratch);
    if (e->ticksLeft > 0) {
      color = RED;
      Collision ec;
      box(e->pos.x, e->pos.y, 4, 4, &ec);
      if (ec.isColliding.rect[BLUE]) {
        play(POWER_UP);
        addScore(floor(dmissileMultiplier), e->pos.x, e->pos.y);
        particle(e->pos.x, e->pos.y, 16, 1, 0, CGLP_PI * 2);
        COUNT_IS_ALIVE(dmissilePlayers, alivePC);
        dmissileMultiplier += 1.0 / clamp(alivePC, 1, 99);
        int hitId = e->id;
        FOR_EACH(dmissileEnemies, j) {
          ASSIGN_ARRAY_ITEM(dmissileEnemies, j, DmissileEnemy, ae);
          if (!ae->isAlive) {
            continue;
          }
          if (ae->id == hitId) {
            ae->isRemoving = true;
          }
        }
        e->isAlive = false;
        continue;
      }
    }
    if (e->pos.y > 90) {
      play(EXPLOSION);
      gameOver();
    }
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(dmissileMultiplier)));
  text(multText, 3, 9, &scratch);
}

void addGameDmissile() {
  addGame(dmissileTitle, dmissileDescription, dmissileCharacters,
          dmissileCharactersCount, &dmissileOptions, false, &dmissileUpdate);
}
