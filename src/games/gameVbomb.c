#include "../cglp.h"

int* vbombTitle = "V BOMB";
int* vbombDescription = "[Tap]\n Turn";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] vbombCharacters = {
    {
        "ll    ",
        "lllll ",
        "llllll",
    },
    {
        "lll   ",
        "lll   ",
        "lll   ",
        "lll   ",
        " l    ",
    },
    {
        "  ll  ",
        " l ll ",
        "l llll",
        "llllll",
        " llll ",
        "  ll  ",
    },
};
int vbombCharactersCount = 3;

Options vbombOptions = {100, 100, 200, false};

struct VbombShip {
  Vector pos;
  float vx;
  float targetVx;
};
VbombShip vbombShip;

struct VbombEnemy {
  Vector pos;
  Vector vel;
  float bombTicks;
  bool isAlive;
};
#define VBOMB_MAX_ENEMY_COUNT 32
VbombEnemy[VBOMB_MAX_ENEMY_COUNT] vbombEnemies;
int vbombEnemyIndex;
float vbombNextEnemyTicks;

struct VbombBomb {
  Vector pos;
  float count;
  float vy;
  bool isAlive;
};
#define VBOMB_MAX_BOMB_COUNT 32
VbombBomb[VBOMB_MAX_BOMB_COUNT] vbombBombs;
int vbombBombIndex;

struct VbombExplosion {
  Vector pos;
  float height;
  bool isAlive;
};
#define VBOMB_MAX_EXPLOSION_COUNT 32
VbombExplosion[VBOMB_MAX_EXPLOSION_COUNT] vbombExplosions;
int vbombExplosionIndex;

int vbombMultiplier;

void vbombUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&vbombShip.pos, 40, 10);
    vbombShip.vx = 1;
    vbombShip.targetVx = 1;
    INIT_UNALIVED_ARRAY_FAST(vbombEnemies);
    vbombEnemyIndex = 0;
    vbombNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(vbombBombs);
    vbombBombIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(vbombExplosions);
    vbombExplosionIndex = 0;
    vbombMultiplier = 1;
  }
  color = BLACK;
  rect(0, 90, 100, 10, &scratch);
  vbombNextEnemyTicks--;
  if (vbombNextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(vbombEnemies, vbombEnemyIndex, VbombEnemy, ne);
    float ex;
    if (rnd(0, 1) < 0.5) {
      ex = -3;
    } else {
      ex = 103;
    }
    vectorSet(&ne->pos, ex, rnd(70, 80));
    vectorSet(&ne->vel, 0, -sqrt(difficulty) * rnd(0.01, 0.03));
    ne->bombTicks = 0;
    ne->isAlive = true;
    vbombEnemyIndex = cgl_wrap(vbombEnemyIndex + 1, 0, VBOMB_MAX_ENEMY_COUNT);
    vbombNextEnemyTicks = rnd(120, 150) / difficulty;
  }
  COUNT_IS_ALIVE(vbombExplosions, aliveExplosionCount);
  if (aliveExplosionCount == 0) {
    vbombMultiplier = 1;
  }
  color = RED;
  FOR_EACH(vbombExplosions, i) {
    ASSIGN_ARRAY_ITEM(vbombExplosions, i, VbombExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    box(e->pos.x, e->pos.y, 6, e->height, &scratch);
    e->pos.y -= 7;
    e->height += 5;
    e->isAlive = e->pos.y + e->height >= 0;
  }
  FOR_EACH(vbombEnemies, i) {
    ASSIGN_ARRAY_ITEM(vbombEnemies, i, VbombEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->bombTicks--;
    if (e->bombTicks < 0) {
      if (e->pos.x > 1 && e->pos.x < 99) {
        play(LASER);
        ASSIGN_ARRAY_ITEM(vbombBombs, vbombBombIndex, VbombBomb, nb);
        nb->pos = e->pos;
        nb->count = rndi(3, 9) + 0.9;
        nb->vy = 0;
        nb->isAlive = true;
        vbombBombIndex = cgl_wrap(vbombBombIndex + 1, 0, VBOMB_MAX_BOMB_COUNT);
      }
      e->bombTicks = rnd(200, 300);
    }
    float ms = sqrt(difficulty) * 0.1;
    e->vel.x = clamp(e->vel.x + (vbombShip.pos.x - e->pos.x) * 0.0002 * sqrt(difficulty), -ms, ms);
    if (e->pos.y < 10) {
      e->vel.y = 0;
    }
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    int[2] ec;
    bool small = fabs(e->vel.x) < 0.1;
    if (small) {
      ec[0] = 'b';
    } else {
      ec[0] = 'a';
    }
    ec[1] = 0;
    characterOptions.isMirrorX = e->vel.x < 0;
    characterOptions.isMirrorY = small;
    Collision ecoll;
    character(ec, e->pos.x, e->pos.y, &ecoll);
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    if (ecoll.isColliding.rect[RED]) {
      play(COIN);
      addScore(vbombMultiplier, e->pos.x, e->pos.y);
      vbombMultiplier++;
      particle(e->pos.x, e->pos.y, 9, 3, 0, CGLP_PI * 2);
      e->isAlive = false;
      continue;
    }
  }
  FOR_EACH(vbombBombs, i) {
    ASSIGN_ARRAY_ITEM(vbombBombs, i, VbombBomb, b);
    SKIP_IS_NOT_ALIVE(b);
    b->vy += 0.1 * difficulty;
    b->pos.y += b->vy;
    if (b->pos.y > 87) {
      b->pos.y = 87;
    }
    color = PURPLE;
    character("c", b->pos.x, b->pos.y + 3 - (10 - b->count) / 2, &scratch);
    if (b->count <= 1) {
      color = RED;
    } else {
      color = BLACK;
    }
    float ty = b->pos.y - (10 - b->count) * 2;
    if (b->count < 1) {
      ty -= (1 - b->count) * 50;
    }
    Collision tc;
    text(intToChar((int)ceil(b->count)), b->pos.x, ty, &tc);
    if (tc.isColliding.rect[RED]) {
      b->count = 0;
    }
    color = PURPLE;
    particle(b->pos.x, b->pos.y - (10 - b->count) / 2, 0.3, (10 - b->count) * 0.1,
             -CGLP_PI / 2, CGLP_PI / 3);
    float pc = ceil(b->count);
    b->count -= 1.0 / 60;
    if (pc != ceil(b->count)) {
      play(HIT);
    }
    if (b->count <= 0) {
      play(EXPLOSION);
      particle(b->pos.x, b->pos.y, 20, 3, -CGLP_PI / 2, CGLP_PI / 8);
      ASSIGN_ARRAY_ITEM(vbombExplosions, vbombExplosionIndex, VbombExplosion, ne2);
      ne2->pos = b->pos;
      ne2->height = 6;
      ne2->isAlive = true;
      vbombExplosionIndex = cgl_wrap(vbombExplosionIndex + 1, 0, VBOMB_MAX_EXPLOSION_COUNT);
      b->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  if (input.isJustPressed || (vbombShip.pos.x > 99 && vbombShip.targetVx > 0) ||
      (vbombShip.pos.x < 1 && vbombShip.targetVx < 0)) {
    play(SELECT);
    vbombShip.targetVx *= -1;
  }
  vbombShip.vx += (vbombShip.targetVx - vbombShip.vx) * 0.1;
  vbombShip.pos.x += vbombShip.vx * sqrt(difficulty);
  int[2] sc2;
  bool shipSmall = fabs(vbombShip.vx) < 0.5;
  if (shipSmall) {
    sc2[0] = 'b';
  } else {
    sc2[0] = 'a';
  }
  sc2[1] = 0;
  characterOptions.isMirrorX = vbombShip.vx < 0;
  Collision sc;
  character(sc2, vbombShip.pos.x, vbombShip.pos.y, &sc);
  characterOptions.isMirrorX = false;
  if (sc.isColliding.rect[RED] || sc.isColliding.character['a'] ||
      sc.isColliding.character['b']) {
    play(RANDOM);
    gameOver();
  }
}

void addGameVbomb() {
  addGame(vbombTitle, vbombDescription, vbombCharacters, vbombCharactersCount,
          &vbombOptions, false, &vbombUpdate);
}
