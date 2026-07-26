#include "../cglp.h"

int* scrambirdTitle = "SCRAMBIRD";
int* scrambirdDescription = "[Tap]\n Fly & Shoot";

int[5][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] scrambirdCharacters = {
    {
        " rr   ",
        " rr   ",
        "yrry  ",
        "yrry  ",
        "y  y  ",
        "y  y  ",
    },
    {
        " bbbb ",
        "bbbbbb",
        " bbbb ",
        " r  r ",
        "r    r",
    },
    {
        "pp    ",
        " l ll ",
        " rlrrl",
        " rllll",
        "   lr ",
    },
    {
        "      ",
        " l ll ",
        " rlrrl",
        " rllll",
        " l lr ",
        "pp    ",
    },
    {
        "l l   ",
    },
};
int scrambirdCharactersCount = 5;

Options scrambirdOptions = {100, 100, 3000, false};

int[4] scrambirdWallColors = {PURPLE, BLUE, GREEN, RED};

struct ScrambirdWall {
  float x;
  float height;
};
ScrambirdWall[11] scrambirdWalls;
float scrambirdWallHeight;
float scrambirdWallHeightVel;

struct ScrambirdMissile {
  Vector pos;
  float launchTicks;
  bool isAlive;
};
#define SCRAMBIRD_MAX_MISSILE_COUNT 64
ScrambirdMissile[SCRAMBIRD_MAX_MISSILE_COUNT] scrambirdMissiles;
int scrambirdMissileIndex;

struct ScrambirdTank {
  Vector pos;
  bool isAlive;
};
#define SCRAMBIRD_MAX_TANK_COUNT 64
ScrambirdTank[SCRAMBIRD_MAX_TANK_COUNT] scrambirdTanks;
int scrambirdTankIndex;
float scrambirdNextTankDist;

struct ScrambirdShip {
  Vector pos;
  float vy;
};
ScrambirdShip scrambirdShip;

struct ScrambirdShot {
  Vector pos;
  bool isAlive;
};
#define SCRAMBIRD_MAX_SHOT_COUNT 64
ScrambirdShot[SCRAMBIRD_MAX_SHOT_COUNT] scrambirdShots;
int scrambirdShotIndex;

struct ScrambirdBomb {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define SCRAMBIRD_MAX_BOMB_COUNT 64
ScrambirdBomb[SCRAMBIRD_MAX_BOMB_COUNT] scrambirdBombs;
int scrambirdBombIndex;

float scrambirdFuel;
int scrambirdMultiplier;

void scrambirdUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(11, wi) {
      scrambirdWalls[wi].x = wi * 10;
      scrambirdWalls[wi].height = 10;
    }
    scrambirdWallHeight = 10;
    scrambirdWallHeightVel = 0;
    INIT_UNALIVED_ARRAY_FAST(scrambirdMissiles);
    scrambirdMissileIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(scrambirdTanks);
    scrambirdTankIndex = 0;
    scrambirdNextTankDist = 10;
    vectorSet(&scrambirdShip.pos, 10, 50);
    scrambirdShip.vy = 0;
    INIT_UNALIVED_ARRAY_FAST(scrambirdShots);
    scrambirdShotIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(scrambirdBombs);
    scrambirdBombIndex = 0;
    scrambirdFuel = 50;
    scrambirdMultiplier = 1;
  }
  float scr = difficulty * 0.3;
  int wallColor = scrambirdWallColors[(ticks / 420) % 4];
  color = wallColor;
  TIMES(11, wi2) {
    ScrambirdWall* w = &scrambirdWalls[wi2];
    w->x -= scr;
    if (w->x < -10) {
      w->x += 110;
      scrambirdWallHeight += scrambirdWallHeightVel;
      if ((scrambirdWallHeight < 10 && scrambirdWallHeightVel < 0) ||
          (scrambirdWallHeight > 50 && scrambirdWallHeightVel > 0)) {
        scrambirdWallHeightVel *= -1;
        scrambirdWallHeight += scrambirdWallHeightVel;
      } else if (rnd(0, 1) < 0.2) {
        scrambirdWallHeightVel = 0;
      } else if (rnd(0, 1) < 0.3) {
        if (rnd(0, 1) < 0.5) {
          scrambirdWallHeightVel = -10;
        } else {
          scrambirdWallHeightVel = 10;
        }
      }
      w->height = scrambirdWallHeight;
      scrambirdNextTankDist--;
      if (scrambirdNextTankDist < 0) {
        ASSIGN_ARRAY_ITEM(scrambirdTanks, scrambirdTankIndex, ScrambirdTank, nt);
        vectorSet(&nt->pos, w->x + 5, 90 - w->height - 3);
        nt->isAlive = true;
        scrambirdTankIndex = cgl_wrap(scrambirdTankIndex + 1, 0, SCRAMBIRD_MAX_TANK_COUNT);
        scrambirdNextTankDist = rnd(1, 16);
      } else if (rnd(0, 1) < 0.5) {
        ASSIGN_ARRAY_ITEM(scrambirdMissiles, scrambirdMissileIndex, ScrambirdMissile, nm);
        vectorSet(&nm->pos, w->x + 5, 90 - w->height - 3);
        if (rnd(0, 1) < 0.5 / sqrt(difficulty)) {
          nm->launchTicks = 9999;
        } else {
          nm->launchTicks = rnd(200, 300) / difficulty;
        }
        nm->isAlive = true;
        scrambirdMissileIndex =
            cgl_wrap(scrambirdMissileIndex + 1, 0, SCRAMBIRD_MAX_MISSILE_COUNT);
      }
    }
    rect(w->x, 90 - w->height, 9, w->height, &scratch);
    rect(w->x, 0, 9, 5, &scratch);
  }
  color = BLACK;
  if (input.isJustPressed) {
    if (scrambirdFuel > 0) {
      play(LASER);
    } else {
      play(HIT);
    }
    float vyDec;
    if (scrambirdFuel > 0) {
      vyDec = 0.5;
    } else {
      vyDec = 0.1;
    }
    scrambirdShip.vy -= difficulty * vyDec;
    ASSIGN_ARRAY_ITEM(scrambirdShots, scrambirdShotIndex, ScrambirdShot, ns);
    ns->pos = scrambirdShip.pos;
    ns->isAlive = true;
    scrambirdShotIndex = cgl_wrap(scrambirdShotIndex + 1, 0, SCRAMBIRD_MAX_SHOT_COUNT);
    ASSIGN_ARRAY_ITEM(scrambirdBombs, scrambirdBombIndex, ScrambirdBomb, nb);
    nb->pos = scrambirdShip.pos;
    vectorSet(&nb->vel, 2 * sqrt(difficulty), 0);
    nb->isAlive = true;
    scrambirdBombIndex = cgl_wrap(scrambirdBombIndex + 1, 0, SCRAMBIRD_MAX_BOMB_COUNT);
  }
  scrambirdShip.vy += 0.015 * difficulty;
  scrambirdShip.vy *= 0.98;
  scrambirdShip.pos.y += scrambirdShip.vy;
  int[2] shipChar;
  if (scrambirdShip.vy < 0) {
    shipChar[0] = 'c';
  } else {
    shipChar[0] = 'd';
  }
  shipChar[1] = 0;
  Collision shipColl;
  character(shipChar, scrambirdShip.pos.x, scrambirdShip.pos.y, &shipColl);
  if (shipColl.isColliding.rect[wallColor]) {
    play(EXPLOSION);
    gameOver();
  }
  color = RED;
  particle(scrambirdShip.pos.x - 2, scrambirdShip.pos.y, 0.5, 0.5, CGLP_PI, CGLP_PI / 5);
  FOR_EACH(scrambirdShots, si) {
    ASSIGN_ARRAY_ITEM(scrambirdShots, si, ScrambirdShot, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x += 2 * sqrt(difficulty);
    Collision sc;
    character("e", s->pos.x, s->pos.y, &sc);
    if (sc.isColliding.rect[wallColor]) {
      s->isAlive = false;
      continue;
    }
    if (s->pos.x > 103) {
      s->isAlive = false;
      continue;
    }
  }
  color = CYAN;
  FOR_EACH(scrambirdBombs, bi) {
    ASSIGN_ARRAY_ITEM(scrambirdBombs, bi, ScrambirdBomb, b);
    SKIP_IS_NOT_ALIVE(b);
    b->vel.y += 0.1 * difficulty;
    vectorMul(&b->vel, 0.9);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    thickness = 2;
    Collision bc;
    bar(b->pos.x, b->pos.y, 2, vectorAngle(&b->vel), &bc);
    if (bc.isColliding.rect[wallColor]) {
      b->isAlive = false;
      continue;
    }
  }
  FOR_EACH(scrambirdMissiles, mi) {
    ASSIGN_ARRAY_ITEM(scrambirdMissiles, mi, ScrambirdMissile, m);
    SKIP_IS_NOT_ALIVE(m);
    m->pos.x -= scr;
    m->launchTicks--;
    if (m->launchTicks < 0) {
      m->pos.y -= difficulty * 0.5;
    }
    color = BLACK;
    Collision mc;
    character("a", m->pos.x, m->pos.y, &mc);
    if (mc.isColliding.character['e'] || mc.isColliding.rect[CYAN]) {
      play(HIT);
      color = RED;
      particle(m->pos.x, m->pos.y, 16, 1, 0, CGLP_PI * 2);
      addScore(scrambirdMultiplier, m->pos.x, m->pos.y);
      scrambirdMultiplier++;
      m->isAlive = false;
      continue;
    }
    if (mc.isColliding.character['c'] || mc.isColliding.character['d']) {
      play(EXPLOSION);
      gameOver();
    }
    if (m->pos.x < -3 || m->pos.y < -3) {
      if (scrambirdMultiplier > 1) {
        scrambirdMultiplier--;
      }
      m->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(scrambirdTanks, ti2) {
    ASSIGN_ARRAY_ITEM(scrambirdTanks, ti2, ScrambirdTank, t);
    SKIP_IS_NOT_ALIVE(t);
    t->pos.x -= scr;
    Collision tc;
    character("b", t->pos.x, t->pos.y, &tc);
    if (tc.isColliding.character['e'] || tc.isColliding.rect[CYAN]) {
      play(POWER_UP);
      color = BLUE;
      particle(t->pos.x, t->pos.y, 16, 1, 0, CGLP_PI * 2);
      scrambirdFuel = clamp(scrambirdFuel + 10, 0, 50);
      t->isAlive = false;
      continue;
    }
    if (tc.isColliding.character['c'] || tc.isColliding.character['d']) {
      play(EXPLOSION);
      gameOver();
    }
    if (t->pos.x < -3) {
      t->isAlive = false;
      continue;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(scrambirdShots, si2) {
    ASSIGN_ARRAY_ITEM(scrambirdShots, si2, ScrambirdShot, s2);
    SKIP_IS_NOT_ALIVE(s2);
    Collision sc2;
    character("e", s2->pos.x, s2->pos.y, &sc2);
    if (sc2.isColliding.character['a'] || sc2.isColliding.character['b']) {
      s2->isAlive = false;
      continue;
    }
  }
  FOR_EACH(scrambirdBombs, bi2) {
    ASSIGN_ARRAY_ITEM(scrambirdBombs, bi2, ScrambirdBomb, b2);
    SKIP_IS_NOT_ALIVE(b2);
    thickness = 2;
    Collision bc2;
    bar(b2->pos.x, b2->pos.y, 2, vectorAngle(&b2->vel), &bc2);
    if (bc2.isColliding.character['a'] || bc2.isColliding.character['b']) {
      b2->isAlive = false;
      continue;
    }
  }
  scrambirdFuel = clamp(scrambirdFuel - difficulty * 0.025, 0, 50);
  color = YELLOW;
  text("FUEL", 10, 93, &scratch);
  rect(40, 90, scrambirdFuel, 6, &scratch);
  color = BLUE;
  rect(40 + scrambirdFuel, 90, 50 - scrambirdFuel, 6, &scratch);
}

void addGameScrambird() {
  addGame(scrambirdTitle, scrambirdDescription, scrambirdCharacters, scrambirdCharactersCount,
          &scrambirdOptions, false, &scrambirdUpdate);
}
