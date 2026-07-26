#include "../cglp.h"

int* rollsTitle = "ROLL S";
int* rollsDescription = "[Tap]\n Change angle\n[Hold]\n Fire";

int[6][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rollsCharacters = {
    {
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
    },
    {
        "  ll  ",
        "   ll ",
        "    ll",
        "    ll",
        "   ll ",
        "  ll  ",
    },
    {
        "  lll ",
        "   ll ",
        "  lll ",
        " ll ll",
        "ll  ll",
        "ll  ll",
    },
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "llllll",
        "llllll",
    },
    {
        "      ",
        "      ",
        "      ",
        "ll    ",
        "llllll",
        "llllll",
    },
    {
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
    },
};
int rollsCharactersCount = 6;

Options rollsOptions = {100, 100, 200, false};

#define ROLLS_PLAYER_X 20

struct RollsPlayer {
  Vector pos;
  float angle;
  float va;
  float ticks;
  float fireTicks;
};
RollsPlayer rollsPlayer;

struct RollsShot {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define ROLLS_MAX_SHOT_COUNT 64
RollsShot[ROLLS_MAX_SHOT_COUNT] rollsShots;
int rollsShotIndex;

struct RollsEnemy {
  Vector pos;
  Vector vel;
  float angle;
  float ticks;
  float fireTicks;
  bool isAlive;
};
#define ROLLS_MAX_ENEMY_COUNT 64
RollsEnemy[ROLLS_MAX_ENEMY_COUNT] rollsEnemies;
int rollsEnemyIndex;
float rollsNextEnemyTicks;

struct RollsBullet {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define ROLLS_MAX_BULLET_COUNT 64
RollsBullet[ROLLS_MAX_BULLET_COUNT] rollsBullets;
int rollsBulletIndex;

float rollsScrOfs;
int rollsMultiplier;

void rollsUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&rollsPlayer.pos, ROLLS_PLAYER_X, 50);
    rollsPlayer.angle = 0;
    rollsPlayer.va = 1;
    rollsPlayer.ticks = 0;
    rollsPlayer.fireTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(rollsShots);
    rollsShotIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(rollsEnemies);
    rollsEnemyIndex = 0;
    rollsNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(rollsBullets);
    rollsBulletIndex = 0;
    rollsScrOfs = 0;
    rollsMultiplier = 1;
  }
  float scr = 0;
  float pa = floor(rollsPlayer.angle) * CGLP_PI / 4;
  Vector pc;
  vectorSet(&pc, rollsPlayer.pos.x, rollsPlayer.pos.y - 9);
  bool pd = false;
  if (input.isJustReleased) {
    play(SELECT);
    rollsPlayer.angle += rollsPlayer.va;
    if (rollsPlayer.angle < -1 || rollsPlayer.angle > 1) {
      rollsPlayer.va *= -1;
      rollsPlayer.angle += rollsPlayer.va * 2;
    }
    rollsPlayer.fireTicks = 9 / sqrt(difficulty);
  }
  if (input.isPressed) {
    if (rollsPlayer.angle == 0 || rollsPlayer.angle == 4) {
      float pcxofs;
      if (rollsPlayer.angle == 0) {
        pcxofs = 6;
      } else {
        pcxofs = -6;
      }
      vectorSet(&pc, rollsPlayer.pos.x + pcxofs, rollsPlayer.pos.y - 3);
      pd = true;
    }
    rollsPlayer.angle = floor(rollsPlayer.angle);
    rollsPlayer.fireTicks--;
    if (rollsPlayer.fireTicks < 0) {
      play(HIT);
      TIMES(5, i) {
        ASSIGN_ARRAY_ITEM(rollsShots, rollsShotIndex, RollsShot, ns);
        ns->pos = pc;
        vectorSet(&ns->vel, 3 * sqrt(difficulty), 0);
        rotate(&ns->vel, pa + i * 0.12 - 0.24);
        ns->isAlive = true;
        rollsShotIndex = cgl_wrap(rollsShotIndex + 1, 0, ROLLS_MAX_SHOT_COUNT);
      }
      rollsPlayer.fireTicks = 9 / sqrt(difficulty);
    }
  } else {
    scr = sqrt(difficulty) * 0.5;
    rollsPlayer.ticks += sqrt(difficulty);
  }
  rollsScrOfs += scr;
  if (rollsScrOfs > rollsMultiplier * 100) {
    play(COIN);
    rollsMultiplier++;
  }
  color = GREEN;
  rect(0, 20, 100, 5, &scratch);
  rect(0, 50, 100, 5, &scratch);
  rect(0, 80, 100, 5, &scratch);
  rect(0, 80, 100, 5, &scratch);
  color = LIGHT_BLACK;
  rect(0, 25, 100, 25, &scratch);
  rect(0, 55, 100, 25, &scratch);
  color = LIGHT_BLUE;
  rect(0, 85, 100, 15, &scratch);
  color = LIGHT_GREEN;
  rect(cgl_wrap(-rollsScrOfs + ROLLS_PLAYER_X, 0, 100), 25, 2, 25, &scratch);
  rect(cgl_wrap(-rollsScrOfs + 67, -10, 110), 55, 2, 25, &scratch);
  color = BLACK;
  if (pd) {
    character("d", pc.x, pc.y, &scratch);
    color = BLUE;
    character("e", rollsPlayer.pos.x, rollsPlayer.pos.y - 3, &scratch);
  } else {
    character("a", pc.x, pc.y, &scratch);
    color = BLUE;
    int[2] bc;
    bc[0] = 'b' + (int)floor(rollsPlayer.ticks / 15) % 2;
    bc[1] = 0;
    character(bc, rollsPlayer.pos.x, rollsPlayer.pos.y - 3, &scratch);
  }
  color = BLACK;
  thickness = 3;
  barCenterPosRatio = 0;
  bar(pc.x, pc.y, 6, pa, &scratch);
  barCenterPosRatio = 0.5;
  color = BLUE;
  FOR_EACH(rollsShots, si) {
    ASSIGN_ARRAY_ITEM(rollsShots, si, RollsShot, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    s->pos.x -= scr;
    thickness = 3;
    bar(s->pos.x, s->pos.y, 3, vectorAngle(&s->vel), &scratch);
    bool inRect = s->pos.x >= -3 && s->pos.x < 103 && s->pos.y >= -3 && s->pos.y < 103;
    if (!inRect) {
      s->isAlive = false;
      continue;
    }
  }
  float fireInterval = ceil(300 / sqrt(difficulty));
  float fireRepeatInterval = ceil(36 / sqrt(difficulty));
  rollsNextEnemyTicks--;
  if (rollsNextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(rollsEnemies, rollsEnemyIndex, RollsEnemy, ne);
    vectorSet(&ne->pos, 105, rndi(0, 3) * 30 + 20);
    vectorSet(&ne->vel, rnd(1, difficulty) * -0.2, 0);
    ne->angle = 0;
    ne->ticks = 0;
    ne->fireTicks = rndi(0, (int)fireInterval);
    ne->isAlive = true;
    rollsEnemyIndex = cgl_wrap(rollsEnemyIndex + 1, 0, ROLLS_MAX_ENEMY_COUNT);
    rollsNextEnemyTicks = rnd(50, 60) / difficulty;
  }
  FOR_EACH(rollsEnemies, ei) {
    ASSIGN_ARRAY_ITEM(rollsEnemies, ei, RollsEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    Vector ec;
    vectorSet(&ec, e->pos.x, e->pos.y - 9);
    e->fireTicks--;
    if (e->fireTicks < 0) {
      if (fmod(-e->fireTicks, fireRepeatInterval) == 0) {
        play(JUMP);
        ASSIGN_ARRAY_ITEM(rollsBullets, rollsBulletIndex, RollsBullet, nb);
        nb->pos = ec;
        vectorSet(&nb->vel, sqrt(difficulty), 0);
        rotate(&nb->vel, e->angle);
        nb->isAlive = true;
        rollsBulletIndex = cgl_wrap(rollsBulletIndex + 1, 0, ROLLS_MAX_BULLET_COUNT);
      }
      if (-e->fireTicks >= fireRepeatInterval * 3) {
        e->fireTicks = fireInterval;
      }
    } else {
      e->angle = floor((angleTo(&e->pos, rollsPlayer.pos.x, rollsPlayer.pos.y) + CGLP_PI / 8) /
                        (CGLP_PI / 4)) *
                 (CGLP_PI / 4);
      vectorAdd(&e->pos, e->vel.x, e->vel.y);
      e->ticks -= e->vel.x * 5;
    }
    e->pos.x -= scr;
    color = RED;
    characterOptions.isMirrorX = true;
    Collision c1;
    character("f", ec.x, ec.y, &c1);
    int[2] ebc;
    ebc[0] = 'b' + (int)floor(e->ticks / 15) % 2;
    ebc[1] = 0;
    Collision c2;
    character(ebc, e->pos.x, e->pos.y - 3, &c2);
    characterOptions.isMirrorX = false;
    if (e->fireTicks < 0) {
      color = RED;
    } else {
      color = BLACK;
    }
    thickness = 3;
    barCenterPosRatio = 0;
    bar(ec.x, ec.y, 6, e->angle, &scratch);
    barCenterPosRatio = 0.5;
    if (c1.isColliding.rect[BLUE] || c2.isColliding.rect[BLUE]) {
      play(POWER_UP);
      color = RED;
      addScore(rollsMultiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      particle(e->pos.x, e->pos.y, 16, 1, 0, CGLP_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (c1.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
    bool inRect2 = e->pos.x >= -5 && e->pos.x < 105 && e->pos.y >= -5 && e->pos.y < 105;
    if (!inRect2) {
      e->isAlive = false;
      continue;
    }
  }
  color = RED;
  FOR_EACH(rollsBullets, bi) {
    ASSIGN_ARRAY_ITEM(rollsBullets, bi, RollsBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    b->pos.x -= scr;
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision bc;
    bar(b->pos.x, b->pos.y, 3, vectorAngle(&b->vel), &bc);
    if (bc.isColliding.character['a'] || bc.isColliding.character['d']) {
      play(EXPLOSION);
      gameOver();
    }
    bool inRect3 = b->pos.x >= -5 && b->pos.x < 105 && b->pos.y >= -5 && b->pos.y < 105;
    if (!inRect3) {
      b->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(rollsMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameRolls() {
  addGame(rollsTitle, rollsDescription, rollsCharacters, rollsCharactersCount, &rollsOptions,
          false, &rollsUpdate);
}
