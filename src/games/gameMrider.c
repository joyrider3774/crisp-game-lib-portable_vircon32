#include "../cglp.h"

int* mriderTitle = "M RIDER";
int* mriderDescription = "[Hold] Go up";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] mriderCharacters = {
    {
        "  ll  ",
        "  l  l",
        " llll ",
        "l l   ",
        " l l  ",
        "ll  l ",
    },
    {
        "  ll  ",
        " l    ",
        "llll  ",
        " l    ",
        "  l   ",
        "   ll ",
    },
    {
        "l     ",
        "ll    ",
        "llllll",
        " ll   ",
        " l    ",
    },
};
int mriderCharactersCount = 3;

Options mriderOptions = {200, 100, 2, false};

struct MriderMissile {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define MRIDER_MAX_MISSILE_COUNT 32
MriderMissile[MRIDER_MAX_MISSILE_COUNT] mriderMissiles;
int mriderMissileIndex;
float mriderNextMissileTicks;

struct MriderPlane {
  Vector pos;
  Vector vel;
  float removeTicks;
  float baseRemoveTicks;
  bool isAlive;
};
#define MRIDER_MAX_PLANE_COUNT 32
MriderPlane[MRIDER_MAX_PLANE_COUNT] mriderPlanes;
int mriderPlaneIndex;
float mriderNextPlaneTicks;

struct MriderRider {
  Vector pos;
  Vector vel;
  int missileIndex;
};
MriderRider mriderRider;
int mriderMultiplier;

void mriderUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(mriderMissiles);
    mriderMissileIndex = 0;
    ASSIGN_ARRAY_ITEM(mriderMissiles, mriderMissileIndex, MriderMissile, m0);
    vectorSet(&m0->pos, 30, 50);
    vectorSet(&m0->vel, 1, 0);
    m0->isAlive = true;
    mriderMissileIndex = cgl_wrap(mriderMissileIndex + 1, 0, MRIDER_MAX_MISSILE_COUNT);
    mriderNextMissileTicks = 9;
    INIT_UNALIVED_ARRAY_FAST(mriderPlanes);
    mriderPlaneIndex = 0;
    mriderNextPlaneTicks = 0;
    vectorSet(&mriderRider.pos, 0, 0);
    vectorSet(&mriderRider.vel, 0, 0);
    mriderRider.missileIndex = 0;
    mriderMultiplier = 1;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (mriderRider.missileIndex < 0) {
    float rvx;
    if (input.isPressed) {
      rvx = 0.04;
    } else {
      rvx = -0.02;
    }
    mriderRider.vel.x += rvx * sqrt(difficulty);
    float rvy;
    if (input.isPressed) {
      rvy = 0.01;
    } else {
      rvy = 0.05;
    }
    mriderRider.vel.y += rvy * sqrt(difficulty);
    vectorMul(&mriderRider.vel, 0.99);
    vectorAdd(&mriderRider.pos, mriderRider.vel.x, mriderRider.vel.y);
  } else {
    MriderMissile* m = &mriderMissiles[mriderRider.missileIndex];
    float dvy;
    if (input.isPressed) {
      dvy = -1;
    } else {
      dvy = 1;
    }
    m->vel.y += dvy * 0.05 * difficulty;
    m->vel.y *= 0.99;
    m->vel.x += (sqrt(difficulty) - m->vel.x) * 0.1;
    vectorSet(&mriderRider.pos, m->pos.x, m->pos.y - 4);
    mriderRider.vel = m->vel;
  }
  float scr;
  if (mriderRider.pos.x > 50) {
    scr = (mriderRider.pos.x - 50) * 0.1 * sqrt(difficulty);
  } else {
    scr = 0;
  }
  mriderRider.pos.x -= scr;
  color = BLACK;
  int[2] rc;
  if (mriderRider.missileIndex < 0) {
    rc[0] = 'b';
    characterOptions.rotation = (int)floor(ticks / 10) % 4;
  } else {
    rc[0] = 'a';
    characterOptions.rotation = 0;
  }
  rc[1] = 0;
  character(rc, mriderRider.pos.x, clamp(mriderRider.pos.y, -2, 99), &scratch);
  characterOptions.rotation = 0;
  if ((mriderRider.missileIndex >= 0 && mriderRider.pos.y < -1) || mriderRider.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  mriderNextMissileTicks--;
  if (mriderNextMissileTicks < 0) {
    Vector pos;
    vectorSet(&pos, 210, rnd(40, 70));
    ASSIGN_ARRAY_ITEM(mriderMissiles, mriderMissileIndex, MriderMissile, nm);
    nm->pos = pos;
    float a = angleTo(&pos, 0, rnd(50, 60)) + CGLP_PI;
    vectorSet(&nm->vel, rnd(0.4, 0.5) * sqrt(difficulty), 0);
    rotate(&nm->vel, a);
    nm->isAlive = true;
    mriderMissileIndex = cgl_wrap(mriderMissileIndex + 1, 0, MRIDER_MAX_MISSILE_COUNT);
    mriderNextMissileTicks = rnd(60, 300) / sqrt(difficulty);
  }
  FOR_EACH(mriderMissiles, i) {
    ASSIGN_ARRAY_ITEM(mriderMissiles, i, MriderMissile, m);
    SKIP_IS_NOT_ALIVE(m);
    vectorAdd(&m->pos, m->vel.x, m->vel.y);
    m->pos.x -= scr;
    color = LIGHT_RED;
    box(m->pos.x - 9, m->pos.y, 5, 5, &scratch);
    int pc;
    if (i == mriderRider.missileIndex) {
      pc = 3;
    } else {
      pc = 2;
    }
    particle(m->pos.x - 9, m->pos.y, pc, 1, CGLP_PI, 0.5);
    if (i == mriderRider.missileIndex) {
      color = RED;
    } else {
      color = BLACK;
    }
    Collision mc;
    box(m->pos.x, m->pos.y, 18, 3, &mc);
    if (mc.isColliding.character['b']) {
      play(POWER_UP);
      mriderMultiplier = (int)ceil(mriderMultiplier * 0.5);
      mriderRider.missileIndex = i;
    }
    bool inRect = m->pos.x >= -9 && m->pos.x < 230 && m->pos.y >= -3 && m->pos.y < 116;
    if (!inRect) {
      m->isAlive = false;
      continue;
    }
  }
  mriderNextPlaneTicks--;
  if (mriderNextPlaneTicks < 0) {
    Vector pos;
    vectorSet(&pos, 203, rnd(30, 90));
    ASSIGN_ARRAY_ITEM(mriderPlanes, mriderPlaneIndex, MriderPlane, np);
    np->pos = pos;
    float a = angleTo(&pos, 0, rnd(40, 90)) + CGLP_PI;
    vectorSet(&np->vel, rnd(0.05, 0.3) * sqrt(difficulty), 0);
    rotate(&np->vel, a);
    np->removeTicks = 0;
    np->baseRemoveTicks = 0;
    np->isAlive = true;
    mriderPlaneIndex = cgl_wrap(mriderPlaneIndex + 1, 0, MRIDER_MAX_PLANE_COUNT);
    mriderNextPlaneTicks = rnd(20, 24) / sqrt(difficulty);
  }
  FOR_EACH(mriderPlanes, i) {
    ASSIGN_ARRAY_ITEM(mriderPlanes, i, MriderPlane, p);
    SKIP_IS_NOT_ALIVE(p);
    vectorAdd(&p->pos, p->vel.x, p->vel.y);
    p->pos.x -= scr;
    if (p->removeTicks > 0) {
      color = LIGHT_RED;
      particle(p->pos.x, p->pos.y, 1, 1, 0, CGLP_PI * 2);
      p->removeTicks -= sqrt(difficulty);
      if (p->removeTicks <= 0) {
        play(COIN);
        particle(p->pos.x, p->pos.y, 9, 2, 0, CGLP_PI * 2);
        addScore(mriderMultiplier, p->pos.x, p->pos.y);
        mriderMultiplier++;
        float rt = p->baseRemoveTicks * 0.9;
        if (rt > 2) {
          FOR_EACH(mriderPlanes, j) {
            ASSIGN_ARRAY_ITEM(mriderPlanes, j, MriderPlane, ap);
            SKIP_IS_NOT_ALIVE(ap);
            if (i == j) {
              continue;
            }
            if (distanceTo(&ap->pos, p->pos.x, p->pos.y) < 36) {
              thickness = 3;
              line(ap->pos.x, ap->pos.y, p->pos.x, p->pos.y, &scratch);
              ap->removeTicks = rt;
              ap->baseRemoveTicks = rt;
            }
          }
        }
        p->isAlive = false;
        continue;
      }
    }
    color = BLACK;
    Collision pcoll;
    character("c", p->pos.x, p->pos.y, &pcoll);
    if (pcoll.isColliding.rect[RED]) {
      if (mriderRider.missileIndex >= 0) {
        play(JUMP);
        particle(p->pos.x, p->pos.y, 9, 3, 0, CGLP_PI * 2);
        mriderRider.vel.y = -2 * sqrt(difficulty);
        mriderMissiles[mriderRider.missileIndex].pos.x = -99;
        mriderRider.missileIndex = -1;
      }
      p->baseRemoveTicks = 9;
      p->removeTicks = 9;
    }
    bool inRect = p->pos.x >= -3 && p->pos.x < 203 && p->pos.y >= -3 && p->pos.y < 103;
    if (!inRect) {
      p->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(mriderMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameMrider() {
  addGame(mriderTitle, mriderDescription, mriderCharacters, mriderCharactersCount,
          &mriderOptions, false, &mriderUpdate);
}
