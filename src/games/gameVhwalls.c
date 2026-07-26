#include "../cglp.h"

int* vhwallsTitle = "VH WALLS";
int* vhwallsDescription = "[Tap] Place wall";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] vhwallsCharacters = {
    {
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
    },
    {
        "llllll",
        "llllll",
    },
};
int vhwallsCharactersCount = 2;

Options vhwallsOptions = {100, 100, 6, false};

struct VhwallsWall {
  Vector pos;
  int angle;
  bool isFixed;
  bool isAlive;
};
#define VHWALLS_MAX_WALL_COUNT 64
VhwallsWall[VHWALLS_MAX_WALL_COUNT] vhwallsWalls;
int vhwallsWallIndex;
int vhwallsNextWallAngle;

struct VhwallsBall {
  Vector pos;
  Vector vel;
};
VhwallsBall vhwallsBall;

struct VhwallsTarget {
  Vector pos;
  float size;
  int initTicks;
};
VhwallsTarget vhwallsTarget;

void vhwallsSetTarget() {
  vhwallsTarget.size = 20;
  vhwallsTarget.initTicks = 1;
  Vector p;
  vectorSet(&p, vhwallsTarget.pos.x, vhwallsTarget.pos.y);
  TIMES(9, i) {
    vhwallsTarget.pos.x = rnd(20, 80);
    vhwallsTarget.pos.y = rnd(20, 80);
    if (distanceTo(&vhwallsTarget.pos, p.x, p.y) > 40) {
      break;
    }
  }
}

void vhwallsAddWall(float x, float y, int angle, bool isFixed) {
  ASSIGN_ARRAY_ITEM(vhwallsWalls, vhwallsWallIndex, VhwallsWall, w);
  vectorSet(&w->pos, x, y);
  w->angle = angle;
  w->isFixed = isFixed;
  w->isAlive = true;
  vhwallsWallIndex = cgl_wrap(vhwallsWallIndex + 1, 0, VHWALLS_MAX_WALL_COUNT);
}

void vhwallsUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(vhwallsWalls);
    vhwallsWallIndex = 0;
    vhwallsAddWall(1, 50, 0, true);
    vhwallsAddWall(99, 50, 0, true);
    vhwallsAddWall(50, 1, 1, true);
    vhwallsAddWall(50, 99, 1, true);
    vhwallsNextWallAngle = 0;
    vectorSet(&vhwallsBall.pos, 50, 50);
    vectorSet(&vhwallsBall.vel, 1, 1);
    vectorSet(&vhwallsTarget.pos, 50, 50);
    vhwallsTarget.size = 0;
    vhwallsTarget.initTicks = 0;
    vhwallsSetTarget();
  }
  vhwallsBall.pos.x += vhwallsBall.vel.x * sqrt(difficulty);
  vhwallsBall.pos.y += vhwallsBall.vel.y * sqrt(difficulty);
  vhwallsBall.pos.x = clamp(vhwallsBall.pos.x, 1, 99);
  vhwallsBall.pos.y = clamp(vhwallsBall.pos.y, 1, 99);
  color = GREEN;
  box(vhwallsBall.pos.x, vhwallsBall.pos.y, 5, 5, &scratch);
  if (vhwallsNextWallAngle == 0) {
    color = LIGHT_BLUE;
  } else {
    color = LIGHT_CYAN;
  }
  int[2] wc;
  wc[0] = 'a' + vhwallsNextWallAngle;
  wc[1] = 0;
  character(wc, 45, 6, &scratch);
  bool posInRect = input.pos.x >= 1 && input.pos.x < 98 && input.pos.y >= 1 &&
                   input.pos.y < 98;
  if (input.isJustPressed && posInRect) {
    play(SELECT);
    vhwallsAddWall(input.pos.x, input.pos.y, vhwallsNextWallAngle, false);
    vhwallsNextWallAngle = cgl_wrap(vhwallsNextWallAngle + 1, 0, 2);
  }
  if (vhwallsTarget.initTicks > 0) {
    color = PURPLE;
    arc(vhwallsTarget.pos.x, vhwallsTarget.pos.y, vhwallsTarget.size, 0,
        CGLP_PI * 2, &scratch);
    vhwallsTarget.initTicks++;
    vhwallsTarget.size += (5 - vhwallsTarget.size) * 0.2;
    if (vhwallsTarget.initTicks > 16) {
      vhwallsTarget.initTicks = 0;
    }
  } else {
    color = RED;
    Collision tc;
    arc(vhwallsTarget.pos.x, vhwallsTarget.pos.y, vhwallsTarget.size, 0,
        CGLP_PI * 2, &tc);
    if (tc.isColliding.rect[GREEN]) {
      play(COIN);
      particle(vhwallsTarget.pos.x, vhwallsTarget.pos.y, 20, 3, 0, CGLP_PI * 2);
      COUNT_IS_ALIVE(vhwallsWalls, aliveWallCount);
      addScore(aliveWallCount - 4, vhwallsTarget.pos.x, vhwallsTarget.pos.y);
      vhwallsSetTarget();
    }
    vhwallsTarget.size += 0.02 * difficulty;
  }
  FOR_EACH(vhwallsWalls, i) {
    ASSIGN_ARRAY_ITEM(vhwallsWalls, i, VhwallsWall, w);
    SKIP_IS_NOT_ALIVE(w);
    if (w->isFixed) {
      if (w->angle == 0) {
        color = LIGHT_BLUE;
      } else {
        color = LIGHT_CYAN;
      }
    } else {
      if (w->angle == 0) {
        color = BLUE;
      } else {
        color = CYAN;
      }
    }
    float wd;
    if (w->isFixed) {
      wd = 100;
    } else {
      wd = 20;
    }
    float bw, bh;
    if (w->angle == 0) {
      bw = 2;
      bh = wd;
    } else {
      bw = wd;
      bh = 2;
    }
    Collision wc2;
    box(w->pos.x, w->pos.y, bw, bh, &wc2);
    if (!w->isFixed && (wc2.isColliding.rect[PURPLE] || wc2.isColliding.rect[BLUE] ||
                        wc2.isColliding.rect[CYAN])) {
      particle(w->pos.x, w->pos.y, 16, 1, 0, CGLP_PI * 2);
      w->isAlive = false;
      continue;
    }
    if (wc2.isColliding.rect[RED]) {
      play(EXPLOSION);
      color = PURPLE;
      text("X", w->pos.x, w->pos.y, &scratch);
      if (w->angle == 0) {
        text("X", w->pos.x, vhwallsTarget.pos.y, &scratch);
      } else {
        text("X", vhwallsTarget.pos.x, w->pos.y, &scratch);
      }
      gameOver();
    }
    if (wc2.isColliding.rect[GREEN]) {
      play(HIT);
      if (w->angle == 0) {
        vhwallsBall.vel.x *= -1;
        vhwallsBall.pos.x = w->pos.x + vhwallsBall.vel.x * 4;
      } else {
        vhwallsBall.vel.y *= -1;
        vhwallsBall.pos.y = w->pos.y + vhwallsBall.vel.y * 4;
      }
    }
  }
}

void addGameVhwalls() {
  addGame(vhwallsTitle, vhwallsDescription, vhwallsCharacters,
          vhwallsCharactersCount, &vhwallsOptions, true, &vhwallsUpdate);
}
