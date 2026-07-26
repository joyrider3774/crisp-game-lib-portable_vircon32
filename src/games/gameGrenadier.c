#include "../cglp.h"

int* grenadierTitle = "GRENADIER";
int* grenadierDescription = "[Tap]  Climb out\n[Hold] Throw";

int[6][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] grenadierCharacters = {
    {
        "  ll  ",
        "  l   ",
        " llll ",
        " ll   ",
        "  llll",
        "ll   l",
    },
    {
        "ll    ",
        "l  ll ",
        " ll   ",
        "ll    ",
        "  llll",
        "ll    ",
    },
    {
        "  ll  ",
        "  l   ",
        " lllll",
        "l l   ",
        "  llll",
        "ll    ",
    },
    {
        "   ll ",
        "ll l  ",
        "  lll ",
        "   l l",
        "llll  ",
        "    l ",
    },
    {
        "  ll  ",
        "llll  ",
        "  lll ",
        " llll ",
        "l l ll",
        " llll ",
    },
    {
        " llll ",
        "llll  ",
        " llll ",
    },
};
int grenadierCharactersCount = 6;

Options grenadierOptions = {200, 80, 5, false};

#define GRENADIER_STATE_IN_HOLE 0
#define GRENADIER_STATE_THROWING 1
#define GRENADIER_STATE_RUNNING 2

struct GrenadierHole {
  float x;
  bool isAlive;
};
#define GRENADIER_MAX_HOLE_COUNT 32
GrenadierHole[GRENADIER_MAX_HOLE_COUNT] grenadierHoles;
int grenadierHoleIndex;

struct GrenadierTank {
  float x;
  float vx;
  float fireTicks;
  float fireInterval;
  float fireSpeed;
  bool isAlive;
};
#define GRENADIER_MAX_TANK_COUNT 32
GrenadierTank[GRENADIER_MAX_TANK_COUNT] grenadierTanks;
int grenadierTankIndex;
float grenadierTankAddTicks;

struct GrenadierBullet {
  float x;
  float vx;
  bool isAlive;
};
#define GRENADIER_MAX_BULLET_COUNT 32
GrenadierBullet[GRENADIER_MAX_BULLET_COUNT] grenadierBullets;
int grenadierBulletIndex;

struct GrenadierGrenade {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define GRENADIER_MAX_GRENADE_COUNT 16
GrenadierGrenade[GRENADIER_MAX_GRENADE_COUNT] grenadierGrenades;
int grenadierGrenadeIndex;

float grenadierPx;
int grenadierPState;
float grenadierPAngle;
float grenadierSpeedRatio;

void grenadierUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(grenadierHoles);
    grenadierHoleIndex = 0;
    ASSIGN_ARRAY_ITEM(grenadierHoles, grenadierHoleIndex, GrenadierHole, h0);
    h0->x = 10;
    h0->isAlive = true;
    grenadierHoleIndex = cgl_wrap(grenadierHoleIndex + 1, 0, GRENADIER_MAX_HOLE_COUNT);
    INIT_UNALIVED_ARRAY_FAST(grenadierTanks);
    grenadierTankIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(grenadierBullets);
    grenadierBulletIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(grenadierGrenades);
    grenadierGrenadeIndex = 0;
    grenadierPx = 10;
    grenadierPState = GRENADIER_STATE_IN_HOLE;
    grenadierTankAddTicks = 0;
    grenadierSpeedRatio = 1;
  }
  float scr = (grenadierPx - 10) * 0.05;
  color = BLACK;
  rect(0, 70, 200, 9, &scratch);
  color = WHITE;
  FOR_EACH(grenadierHoles, i) {
    ASSIGN_ARRAY_ITEM(grenadierHoles, i, GrenadierHole, h);
    SKIP_IS_NOT_ALIVE(h);
    h->x -= scr;
    Collision hc;
    box(h->x, 70, 6, 10, &hc);
    if (hc.isColliding.character['e']) {
      h->isAlive = false;
      continue;
    }
    if (h->x <= -4) {
      h->isAlive = false;
      continue;
    }
  }
  color = RED;
  FOR_EACH(grenadierTanks, i) {
    ASSIGN_ARRAY_ITEM(grenadierTanks, i, GrenadierTank, t);
    SKIP_IS_NOT_ALIVE(t);
    character("e", t->x, 67, &scratch);
  }
  color = BLACK;
  FOR_EACH(grenadierGrenades, i) {
    ASSIGN_ARRAY_ITEM(grenadierGrenades, i, GrenadierGrenade, g);
    SKIP_IS_NOT_ALIVE(g);
    vectorAdd(&g->pos, g->vel.x, g->vel.y);
    g->vel.y += 0.1 * difficulty;
    Collision gc;
    text("o", g->pos.x, g->pos.y, &gc);
    if (gc.isColliding.character['e']) {
      g->isAlive = false;
      continue;
    }
    if (g->pos.y > 68) {
      play(HIT);
      particle(g->pos.x, g->pos.y, 10, 1, -CGLP_PI / 2, CGLP_PI * 0.7);
      ASSIGN_ARRAY_ITEM(grenadierHoles, grenadierHoleIndex, GrenadierHole, nh);
      nh->x = g->pos.x;
      nh->isAlive = true;
      grenadierHoleIndex = cgl_wrap(grenadierHoleIndex + 1, 0, GRENADIER_MAX_HOLE_COUNT);
      g->isAlive = false;
      continue;
    }
  }
  grenadierTankAddTicks--;
  if (grenadierTankAddTicks < 0) {
    float sd = rnd(0, sqrt(difficulty) - 1) + 1;
    float fi = 300 / (rnd(0, sqrt(difficulty)) + 1);
    float fs = rnd(0, sqrt(sqrt(difficulty)) - 1) + 1;
    ASSIGN_ARRAY_ITEM(grenadierTanks, grenadierTankIndex, GrenadierTank, nt);
    nt->x = 203;
    nt->vx = 0.08 * sd;
    nt->fireTicks = rnd(0, fi);
    nt->fireInterval = fi;
    nt->fireSpeed = 0.4 * fs;
    nt->isAlive = true;
    grenadierTankIndex = cgl_wrap(grenadierTankIndex + 1, 0, GRENADIER_MAX_TANK_COUNT);
    grenadierTankAddTicks = 200 / (rnd(0, sqrt(difficulty)) + 1);
  }
  FOR_EACH(grenadierTanks, i) {
    ASSIGN_ARRAY_ITEM(grenadierTanks, i, GrenadierTank, t);
    SKIP_IS_NOT_ALIVE(t);
    t->x -= t->vx * grenadierSpeedRatio + scr;
    color = TRANSPARENT;
    Collision tc;
    box(t->x, 67, 6, 6, &tc);
    if (tc.isColliding.text['o']) {
      play(EXPLOSION);
      color = RED;
      particle(t->x, 67, 20, 2, -CGLP_PI / 2, CGLP_PI * 1.2);
      addScore(t->x, t->x, 67);
      t->isAlive = false;
      continue;
    }
    t->fireTicks--;
    if (t->x > 150 && grenadierPState != GRENADIER_STATE_IN_HOLE && t->fireTicks < 0) {
      play(LASER);
      t->fireTicks = t->fireInterval;
      ASSIGN_ARRAY_ITEM(grenadierBullets, grenadierBulletIndex, GrenadierBullet, nb);
      nb->x = t->x - 5;
      nb->vx = t->fireSpeed;
      nb->isAlive = true;
      grenadierBulletIndex = cgl_wrap(grenadierBulletIndex + 1, 0, GRENADIER_MAX_BULLET_COUNT);
    }
    if (t->x <= -4) {
      t->isAlive = false;
      continue;
    }
  }
  color = RED;
  FOR_EACH(grenadierBullets, i) {
    ASSIGN_ARRAY_ITEM(grenadierBullets, i, GrenadierBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    b->x -= b->vx * grenadierSpeedRatio + scr;
    character("f", b->x, 65, &scratch);
    if (b->x <= -4) {
      b->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(grenadierTanks, aliveTankCount);
  if (aliveTankCount == 0) {
    grenadierTankAddTicks *= 0.7;
  }
  color = TRANSPARENT;
  FOR_EACH(grenadierHoles, i) {
    ASSIGN_ARRAY_ITEM(grenadierHoles, i, GrenadierHole, h);
    SKIP_IS_NOT_ALIVE(h);
    Collision hc2;
    box(h->x, 70, 7, 10, &hc2);
    if (hc2.isColliding.character['e']) {
      h->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  grenadierPx -= scr;
  if (grenadierPState == GRENADIER_STATE_IN_HOLE) {
    grenadierSpeedRatio += 0.05;
    if (input.isJustPressed) {
      grenadierPState = GRENADIER_STATE_RUNNING;
      grenadierPx += 6;
    } else {
      Collision ac;
      character("a", grenadierPx, 72, &ac);
      if (ac.isColliding.character['e']) {
        play(RANDOM);  // Equivalent to "lucky" in JS
        gameOver();
      }
      color = TRANSPARENT;
      Collision bc2;
      box(grenadierPx + 5, 72, 6, 6, &bc2);
      if (bc2.isColliding.rect[WHITE]) {
        grenadierPx++;
      }
    }
  } else if (grenadierPState == GRENADIER_STATE_RUNNING) {
    grenadierSpeedRatio += (1 - grenadierSpeedRatio) * 0.1;
    grenadierPx += 0.8 * sqrt(difficulty);
    int[2] rc;
    rc[0] = 'c' + (int)floor(ticks / 30) % 2;
    rc[1] = 0;
    Collision runColl;
    character(rc, grenadierPx, 67, &runColl);
    if (runColl.isColliding.character['e'] || runColl.isColliding.character['f']) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
    if (runColl.isColliding.rect[WHITE]) {
      grenadierPState = GRENADIER_STATE_IN_HOLE;
    } else if (input.isJustPressed) {
      grenadierPState = GRENADIER_STATE_THROWING;
      grenadierPAngle = 0;
    }
  } else if (grenadierPState == GRENADIER_STATE_THROWING) {
    grenadierSpeedRatio += (1 - grenadierSpeedRatio) * 0.1;
    Vector p;
    vectorSet(&p, grenadierPx, 67);
    if (input.isJustReleased || grenadierPAngle < -1) {
      play(POWER_UP);
      ASSIGN_ARRAY_ITEM(grenadierGrenades, grenadierGrenadeIndex, GrenadierGrenade, ng);
      ng->pos = p;
      vectorSet(&ng->vel, (4 - grenadierPAngle * 0.5) * sqrt(difficulty), 0);
      rotate(&ng->vel, grenadierPAngle);
      ng->isAlive = true;
      grenadierGrenadeIndex = cgl_wrap(grenadierGrenadeIndex + 1, 0, GRENADIER_MAX_GRENADE_COUNT);
      grenadierPState = GRENADIER_STATE_RUNNING;
    } else {
      Collision bcoll;
      character("b", p.x, p.y, &bcoll);
      if (bcoll.isColliding.character['e'] || bcoll.isColliding.character['f']) {
        play(RANDOM);  // Equivalent to "lucky" in JS
        gameOver();
      }
      Vector dir;
      vectorSet(&dir, 10, 0);
      rotate(&dir, grenadierPAngle);
      thickness = 2;
      line(p.x, p.y, p.x + dir.x, p.y + dir.y, &scratch);
      grenadierPAngle -= 0.02 * sqrt(difficulty);
    }
  }
}

void addGameGrenadier() {
  addGame(grenadierTitle, grenadierDescription, grenadierCharacters,
          grenadierCharactersCount, &grenadierOptions, false, &grenadierUpdate);
}
