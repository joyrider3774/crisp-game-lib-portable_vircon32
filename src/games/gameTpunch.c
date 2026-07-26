#include "../cglp.h"

int* tpunchTitle = "T PUNCH";
int* tpunchDescription = "[Tap]\n Punch\n[Hold]\n Roll";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tpunchCharacters = {
    {
        " bbbb ",
        "ppbbbb",
        "ppprrr",
        " prrr ",
    },
    {
        "c   c ",
        " ccc  ",
        "ccbcc ",
        " ccc  ",
        "c   c ",
    },
    {
        " lll  ",
        "lyyyl ",
        "l   l ",
        "l b l ",
        " lll  ",
    },
};
int tpunchCharactersCount = 3;

Options tpunchOptions = {100, 100, 90, false};

#define TPUNCH_ARM_COUNT 6
struct TpunchArm {
  float angle;
  float av;
  float length;
  bool isAttacking;
  bool[TPUNCH_ARM_COUNT] isAlive;
};
TpunchArm tpunchArm;

struct TpunchEnemy {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define TPUNCH_MAX_ENEMY_COUNT 64
TpunchEnemy[TPUNCH_MAX_ENEMY_COUNT] tpunchEnemies;
int tpunchEnemyIndex;
float tpunchNextEnemyTicks;

struct TpunchBonus {
  Vector pos;
  Vector vel;
  float ticks;
  bool isAlive;
};
#define TPUNCH_MAX_BONUS_COUNT 256
TpunchBonus[TPUNCH_MAX_BONUS_COUNT] tpunchBonuses;
int tpunchBonusIndex;

int tpunchMultiplier;

void tpunchUpdate() {
  Collision scratch;
  if (!ticks) {
    tpunchArm.angle = 0;
    tpunchArm.av = 0;
    tpunchArm.length = 0;
    tpunchArm.isAttacking = false;
    TIMES(TPUNCH_ARM_COUNT, ai) { tpunchArm.isAlive[ai] = true; }
    INIT_UNALIVED_ARRAY_FAST(tpunchEnemies);
    tpunchEnemyIndex = 0;
    tpunchNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(tpunchBonuses);
    tpunchBonusIndex = 0;
    tpunchMultiplier = 1;
  }
  float sd = sqrt(difficulty);
  int an = TPUNCH_ARM_COUNT;
  color = BLACK;
  character("c", 49, 49, &scratch);
  if (input.isJustPressed) {
    if (tpunchArm.length < 5) {
      play(SELECT);
      tpunchArm.isAttacking = true;
    } else {
      play(LASER);
    }
    tpunchMultiplier = 1;
  }
  if (!input.isPressed || tpunchArm.length > 30) {
    tpunchArm.isAttacking = false;
  }
  float lenTarget;
  if (input.isPressed) {
    lenTarget = 36;
  } else {
    lenTarget = 0;
  }
  tpunchArm.length += (lenTarget - tpunchArm.length) * 0.1 * sqrt(difficulty);
  if (input.isPressed) {
    tpunchArm.av += sqrt(difficulty) * 0.001;
  } else {
    tpunchArm.av += (0.03 * sqrt(difficulty) - tpunchArm.av) * 0.2;
  }
  tpunchArm.angle += tpunchArm.av;
  float sz = sqrt(tpunchArm.length) * 0.5 + 3;
  Vector p;
  float a = tpunchArm.angle;
  TIMES(an, ii) {
    a += (CGLP_PI * 2) / an;
    if (!tpunchArm.isAlive[ii]) {
      continue;
    }
    float r = 5;
    float ri = tpunchArm.length / 4;
    TIMES(5, i) {
      float s = sz;
      if (i == 4 && tpunchArm.isAttacking) {
        color = RED;
        s *= 1.5;
      } else {
        color = CYAN;
      }
      vectorSet(&p, 50, 50);
      addWithAngle(&p, a, r);
      box(p.x, p.y, s, s, &scratch);
      r += ri;
    }
  }
  COUNT_IS_ALIVE(tpunchEnemies, aliveEnemyCheck);
  if (aliveEnemyCheck == 0) {
    tpunchNextEnemyTicks = 0;
  }
  tpunchNextEnemyTicks--;
  if (tpunchNextEnemyTicks < 0) {
    Vector pos;
    float py;
    if (rnd(0, 1) < 0.5) {
      py = -3;
    } else {
      py = 3;
    }
    vectorSet(&pos, rnd(0, 99), py);
    if (rnd(0, 1) < 0.5) {
      float tmp = pos.x;
      pos.x = pos.y;
      pos.y = tmp;
    }
    ASSIGN_ARRAY_ITEM(tpunchEnemies, tpunchEnemyIndex, TpunchEnemy, ne);
    ne->pos = pos;
    vectorSet(&ne->vel, rnd(0, sd) * RNDPM() * 0.3, rnd(0, sd) * RNDPM() * 0.3);
    ne->isAlive = true;
    tpunchEnemyIndex = cgl_wrap(tpunchEnemyIndex + 1, 0, TPUNCH_MAX_ENEMY_COUNT);
    tpunchNextEnemyTicks = rnd(60, 99) / difficulty;
  }
  Vector tpunchCp;
  vectorSet(&tpunchCp, 50, 50);
  FOR_EACH(tpunchBonuses, bi2) {
    ASSIGN_ARRAY_ITEM(tpunchBonuses, bi2, TpunchBonus, b);
    SKIP_IS_NOT_ALIVE(b);
    addWithAngle(&b->vel, angleTo(&tpunchCp, b->pos.x, b->pos.y), sd * 0.002);
    vectorMul(&b->vel, 0.98);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    float t = 60 / sd;
    if (b->ticks > t) {
      color = BLACK;
    } else {
      color = LIGHT_BLUE;
    }
    Collision bc;
    character("b", b->pos.x, b->pos.y, &bc);
    if (b->ticks > t && (bc.isColliding.rect[RED] || bc.isColliding.rect[CYAN])) {
      play(COIN);
      addScore(tpunchMultiplier, b->pos.x, b->pos.y);
      tpunchMultiplier++;
      b->isAlive = false;
      continue;
    }
    b->ticks++;
    bool inRect = b->pos.x >= -3 && b->pos.x < 103 && b->pos.y >= -3 && b->pos.y < 103;
    if (!inRect) {
      b->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(tpunchEnemies, ei) {
    ASSIGN_ARRAY_ITEM(tpunchEnemies, ei, TpunchEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (distanceTo(&e->pos, 50, 50) > 30) {
      addWithAngle(&e->vel, angleTo(&e->pos, 50, 50), sd * 0.005);
      vectorMul(&e->vel, 0.99);
    }
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    characterOptions.isMirrorX = e->vel.x <= 0;
    Collision ec;
    character("a", e->pos.x, e->pos.y, &ec);
    characterOptions.isMirrorX = false;
    if (ec.isColliding.rect[RED]) {
      play(POWER_UP);
      TIMES(16, bqi) {
        ASSIGN_ARRAY_ITEM(tpunchBonuses, tpunchBonusIndex, TpunchBonus, nb);
        nb->pos = e->pos;
        vectorSet(&nb->vel, bqi * 0.05, 0);
        rotate(&nb->vel, bqi);
        nb->ticks = 0;
        nb->isAlive = true;
        tpunchBonusIndex = cgl_wrap(tpunchBonusIndex + 1, 0, TPUNCH_MAX_BONUS_COUNT);
      }
      int aidx = rndi(0, an);
      TIMES(an, aloop) {
        if (!tpunchArm.isAlive[aidx]) {
          tpunchArm.isAlive[aidx] = true;
          break;
        }
        aidx = (int)cgl_wrap(aidx + 1, 0, an);
      }
      e->isAlive = false;
      continue;
    }
    if (ec.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      e->isAlive = false;
      continue;
    }
    if (ec.isColliding.character['c']) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  int ac = 0;
  a = tpunchArm.angle;
  TIMES(an, jj) {
    a += (CGLP_PI * 2) / an;
    if (!tpunchArm.isAlive[jj]) {
      continue;
    }
    ac++;
    float r = 5;
    float ri = tpunchArm.length / 4;
    TIMES(5, i2) {
      if (i2 == 4 && tpunchArm.isAttacking) {
        continue;
      }
      color = TRANSPARENT;
      vectorSet(&p, 50, 50);
      addWithAngle(&p, a, r);
      Collision pc;
      box(p.x, p.y, sz, sz, &pc);
      if (pc.isColliding.character['a']) {
        color = CYAN;
        particle(p.x, p.y, 9, 2, 0, CGLP_PI * 2);
        tpunchArm.isAlive[jj] = false;
      }
      r += ri;
    }
  }
  if (ac == 0) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
}

void addGameTpunch() {
  addGame(tpunchTitle, tpunchDescription, tpunchCharacters, tpunchCharactersCount,
          &tpunchOptions, false, &tpunchUpdate);
}
