#include "../cglp.h"

int* mjammingTitle = "M JAMMING";
int* mjammingDescription = "[Hold]\n Expand range\n[Release]\n Jamming";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] mjammingCharacters = {{
    "bblb  ",
    "b l b ",
    "  b   ",
    "bb bb ",
}};
int mjammingCharactersCount = 1;

Options mjammingOptions = {100, 100, 6, true};

struct MjammingRobot {
  Vector pos;
  Vector vel;
  float radius;
};
MjammingRobot mjammingRobot;

struct MjammingMissile {
  Vector pos;
  float angle;
  float angleVel;
  float speed;
  float explosionRadius;
  bool isAlive;
};
// 64->1024: spawn burst is floor(rnd(6,9)*difficulty) every ~65 ticks (uncapped
// linear growth) while lifetime only shrinks like 1/sqrt(difficulty); simulation
// of the spawn/homing/bounds-exit logic shows concurrent count already exceeds
// 64 by ~difficulty 6 (under 5 min in) and reaches ~400+ by difficulty 100.
#define MJAMMING_MAX_MISSILE_COUNT 1024
MjammingMissile[MJAMMING_MAX_MISSILE_COUNT] mjammingMissiles;
int mjammingMissileIndex;
float mjammingNextMissileTicks;

struct MjammingExplosion {
  Vector pos;
  float radius;
  float targetRadius;
  float vr;
  bool isAlive;
};
#define MJAMMING_MAX_EXPLOSION_COUNT 16
MjammingExplosion[MJAMMING_MAX_EXPLOSION_COUNT] mjammingExplosions;
int mjammingExplosionIndex;

struct MjammingStar {
  Vector pos;
  float spRatio;
  int color;
};
#define MJAMMING_STAR_COUNT 30
MjammingStar[MJAMMING_STAR_COUNT] mjammingStars;

int mjammingMultiplier;

void mjammingUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&mjammingRobot.pos, 50, 50);
    vectorSet(&mjammingRobot.vel, 0, 0);
    mjammingRobot.radius = 0;
    INIT_UNALIVED_ARRAY_FAST(mjammingMissiles);
    mjammingMissileIndex = 0;
    mjammingNextMissileTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(mjammingExplosions);
    mjammingExplosionIndex = 0;
    int[3] starColors;
    starColors[0] = LIGHT_CYAN;
    starColors[1] = LIGHT_PURPLE;
    starColors[2] = LIGHT_YELLOW;
    TIMES(MJAMMING_STAR_COUNT, i) {
      vectorSet(&mjammingStars[i].pos, rnd(0, 99), rnd(0, 99));
      mjammingStars[i].spRatio = rnd(0.2, 0.3);
      mjammingStars[i].color = starColors[rndi(0, 3)];
    }
    mjammingMultiplier = 1;
  }
  Vector scr;
  vectorSet(&scr, 50 - mjammingRobot.pos.x, 50 - mjammingRobot.pos.y);
  vectorMul(&scr, 0.05 * sqrt(difficulty));
  TIMES(MJAMMING_STAR_COUNT, i) {
    MjammingStar* s = &mjammingStars[i];
    s->pos.x += scr.x * s->spRatio;
    s->pos.y += scr.y * s->spRatio;
    s->pos.x = cgl_wrap(s->pos.x, 0, 99);
    s->pos.y = cgl_wrap(s->pos.y, 0, 99);
    color = s->color;
    rect(s->pos.x, s->pos.y, 1, 1, &scratch);
  }
  color = RED;
  FOR_EACH(mjammingExplosions, i) {
    ASSIGN_ARRAY_ITEM(mjammingExplosions, i, MjammingExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    e->radius += sqrt(difficulty) * e->vr * 0.5;
    if (e->radius > e->targetRadius && e->vr > 0) {
      e->vr = -1;
    }
    thickness = 5;
    arc(e->pos.x, e->pos.y, e->radius, 0, CGLP_PI * 2, &scratch);
    if (e->radius < 1) {
      e->isAlive = false;
      continue;
    }
  }
  vectorMul(&mjammingRobot.vel, 0.98);
  vectorAdd(&mjammingRobot.pos, mjammingRobot.vel.x, mjammingRobot.vel.y);
  vectorAdd(&mjammingRobot.pos, scr.x, scr.y);
  color = BLACK;
  if (mjammingRobot.vel.x < 0) {
    characterOptions.isMirrorX = true;
  } else {
    characterOptions.isMirrorX = false;
  }
  character("a", mjammingRobot.pos.x, mjammingRobot.pos.y, &scratch);
  characterOptions.isMirrorX = false;
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed || input.isJustReleased) {
    mjammingRobot.radius *= 1 + 0.01 * sqrt(difficulty);
    vectorMul(&mjammingRobot.vel, 0.9);
    color = CYAN;
  } else {
    mjammingRobot.radius = clamp(mjammingRobot.radius + sqrt(difficulty) * 0.5, 0, 15);
    color = LIGHT_CYAN;
  }
  thickness = 2;
  arc(mjammingRobot.pos.x, mjammingRobot.pos.y, mjammingRobot.radius, 0, CGLP_PI * 2, &scratch);
  if (input.isJustReleased || mjammingRobot.radius > 50) {
    play(LASER);
    mjammingMultiplier = 1;
    int mc = 0;
    FOR_EACH(mjammingMissiles, i) {
      ASSIGN_ARRAY_ITEM(mjammingMissiles, i, MjammingMissile, m);
      SKIP_IS_NOT_ALIVE(m);
      if (distanceTo(&m->pos, mjammingRobot.pos.x, mjammingRobot.pos.y) < mjammingRobot.radius + 1 &&
          m->explosionRadius < 0) {
        mc++;
      }
    }
    FOR_EACH(mjammingMissiles, i) {
      ASSIGN_ARRAY_ITEM(mjammingMissiles, i, MjammingMissile, m);
      SKIP_IS_NOT_ALIVE(m);
      if (distanceTo(&m->pos, mjammingRobot.pos.x, mjammingRobot.pos.y) < mjammingRobot.radius + 1 &&
          m->explosionRadius < 0) {
        m->angle = angleTo(&mjammingRobot.pos, m->pos.x, m->pos.y);
        m->explosionRadius = clamp(mc * sqrt(mc), 1, 25);
        m->angleVel = 0;
        m->speed /= 2;
        addScore(mjammingMultiplier, m->pos.x, m->pos.y);
        mjammingMultiplier++;
      }
    }
    mjammingRobot.radius = mc;
  }
  mjammingNextMissileTicks--;
  if (mjammingNextMissileTicks < 0) {
    int c = (int)floor(rnd(6, 9) * difficulty);
    TIMES(c, k) {
      Vector pos;
      vectorSet(&pos, 50, 50);
      addWithAngle(&pos, rnd(0, CGLP_PI * 2), 99);
      float angle = angleTo(&pos, rnd(40, 60), rnd(40, 60));
      float angleVel = rnd(1, sqrt(difficulty)) * 0.005;
      float speed = sqrt(difficulty) * 0.4;
      ASSIGN_ARRAY_ITEM(mjammingMissiles, mjammingMissileIndex, MjammingMissile, nm);
      nm->pos = pos;
      nm->angle = angle;
      nm->angleVel = angleVel;
      nm->speed = speed;
      nm->explosionRadius = -1;
      nm->isAlive = true;
      mjammingMissileIndex = cgl_wrap(mjammingMissileIndex + 1, 0, MJAMMING_MAX_MISSILE_COUNT);
    }
    mjammingNextMissileTicks = rnd(60, 70);
  }
  bool hasNearest = false;
  Vector nearestPos;
  float nmDist = 50;
  FOR_EACH(mjammingMissiles, i) {
    ASSIGN_ARRAY_ITEM(mjammingMissiles, i, MjammingMissile, m);
    SKIP_IS_NOT_ALIVE(m);
    float ta = angleTo(&m->pos, mjammingRobot.pos.x, mjammingRobot.pos.y);
    float oa = cgl_wrap(m->angle - ta, -CGLP_PI, CGLP_PI);
    if (oa > m->angleVel) {
      m->angle -= m->angleVel;
    } else if (oa < -m->angleVel) {
      m->angle += m->angleVel;
    } else {
      m->angle = ta;
    }
    addWithAngle(&m->pos, m->angle, m->speed);
    vectorAdd(&m->pos, scr.x, scr.y);
    if (m->explosionRadius < 0) {
      color = PURPLE;
    } else {
      color = CYAN;
    }
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision mc2;
    bar(m->pos.x, m->pos.y, 3, m->angle, &mc2);
    color = RED;
    particle(m->pos.x, m->pos.y, 1, m->speed, m->angle + CGLP_PI, 1);
    if (mc2.isColliding.rect[RED]) {
      if (m->explosionRadius > 1) {
        play(POWER_UP);
        ASSIGN_ARRAY_ITEM(mjammingExplosions, mjammingExplosionIndex, MjammingExplosion, ne);
        ne->pos = m->pos;
        ne->radius = 1;
        ne->targetRadius = m->explosionRadius;
        ne->vr = 1;
        ne->isAlive = true;
        mjammingExplosionIndex = cgl_wrap(mjammingExplosionIndex + 1, 0, MJAMMING_MAX_EXPLOSION_COUNT);
      } else {
        play(COIN);
        particle(m->pos.x, m->pos.y, 9, 2, 0, CGLP_PI * 2);
      }
      addScore(mjammingMultiplier, m->pos.x, m->pos.y);
      mjammingMultiplier++;
      m->isAlive = false;
      continue;
    }
    if (mc2.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
    float d = distanceTo(&m->pos, mjammingRobot.pos.x, mjammingRobot.pos.y);
    if (d < nmDist) {
      nmDist = d;
      hasNearest = true;
      nearestPos = m->pos;
    }
    bool inRect = m->pos.x >= -100 && m->pos.x < 100 && m->pos.y >= -100 && m->pos.y < 100;
    if (!inRect) {
      m->isAlive = false;
      continue;
    }
  }
  FOR_EACH(mjammingMissiles, i) {
    ASSIGN_ARRAY_ITEM(mjammingMissiles, i, MjammingMissile, m);
    SKIP_IS_NOT_ALIVE(m);
    if (m->explosionRadius > 0) {
      color = TRANSPARENT;
      thickness = 3;
      barCenterPosRatio = 0.5;
      Collision mc3;
      bar(m->pos.x, m->pos.y, 3, m->angle, &mc3);
      if (mc3.isColliding.rect[PURPLE]) {
        play(POWER_UP);
        ASSIGN_ARRAY_ITEM(mjammingExplosions, mjammingExplosionIndex, MjammingExplosion, ne2);
        ne2->pos = m->pos;
        ne2->radius = 1;
        ne2->targetRadius = m->explosionRadius;
        ne2->vr = 1;
        ne2->isAlive = true;
        mjammingExplosionIndex = cgl_wrap(mjammingExplosionIndex + 1, 0, MJAMMING_MAX_EXPLOSION_COUNT);
        color = RED;
        particle(m->pos.x, m->pos.y, 9, 2, 0, CGLP_PI * 2);
        addScore(mjammingMultiplier, m->pos.x, m->pos.y);
        mjammingMultiplier++;
        m->isAlive = false;
        continue;
      }
    }
  }
  if (hasNearest) {
    addWithAngle(&mjammingRobot.vel,
                 angleTo(&nearestPos, mjammingRobot.pos.x, mjammingRobot.pos.y) + CGLP_PI / 3,
                 sqrt(difficulty) * 0.01);
  }
}

void addGameMjamming() {
  addGame(mjammingTitle, mjammingDescription, mjammingCharacters,
          mjammingCharactersCount, &mjammingOptions, false, &mjammingUpdate);
}
