#include "../cglp.h"

int* invinciblemanTitle = "INVINCIBLE MAN";
int* invinciblemanDescription = "[Tap]\n Turn\n[Hold]\n Walk outward";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] invinciblemanCharacters = {
    {
        "  ll  ",
        "  l   ",
        " llll ",
        "l l   ",
        " l ll ",
        "l     ",
    },
    {
        "   ll ",
        "  l   ",
        "llll  ",
        "  l   ",
        "ll l  ",
        "    l ",
    },
    {
        "ll ll ",
        "l  l  ",
        " llll ",
        "lllll ",
        "lllll ",
        "l l l ",
    },
    {
        " ll ll",
        " l  l ",
        " llll ",
        "lllll ",
        "lllll ",
        " l l  ",
    },
};
int invinciblemanCharactersCount = 4;

Options invinciblemanOptions = {100, 100, 2, true};

struct InvinciblemanPlayer {
  Vector pos;
  float va;
  float ticksVal;
};
InvinciblemanPlayer invinciblemanPlayer;

struct InvinciblemanHuman {
  Vector pos;
  Vector targetPos;
  Vector vel;
  float ticksVal;
  bool isAlive;
};
#define INVINCIBLEMAN_MAX_HUMAN_COUNT 16
InvinciblemanHuman[INVINCIBLEMAN_MAX_HUMAN_COUNT] invinciblemanHumans;
int invinciblemanHumanIndex;
float invinciblemanNextHumanTicks;

struct InvinciblemanEnemy {
  Vector pos;
  Vector vel;
  float ticksVal;
  bool isAlive;
};
#define INVINCIBLEMAN_MAX_ENEMY_COUNT 64
InvinciblemanEnemy[INVINCIBLEMAN_MAX_ENEMY_COUNT] invinciblemanEnemies;
int invinciblemanEnemyIndex;
float invinciblemanNextEnemyTicks;

struct InvinciblemanExplosion {
  Vector pos;
  float radius;
  float rv;
  bool isAlive;
};
#define INVINCIBLEMAN_MAX_EXPLOSION_COUNT 16
InvinciblemanExplosion[INVINCIBLEMAN_MAX_EXPLOSION_COUNT] invinciblemanExplosions;
int invinciblemanExplosionIndex;

int invinciblemanMultiplier;

int invinciblemanCountHumans() {
  COUNT_IS_ALIVE(invinciblemanHumans, c);
  return c;
}

int invinciblemanNearestEnemyIndex(float px, float py) {
  int best = -1;
  float bestDist = 0;
  FOR_EACH(invinciblemanEnemies, i) {
    ASSIGN_ARRAY_ITEM(invinciblemanEnemies, i, InvinciblemanEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float d = distanceTo(&e->pos, px, py);
    if (best < 0 || d < bestDist) {
      best = i;
      bestDist = d;
    }
  }
  return best;
}

int invinciblemanNearestHumanIndex(float px, float py) {
  int best = -1;
  float bestDist = 0;
  FOR_EACH(invinciblemanHumans, i) {
    ASSIGN_ARRAY_ITEM(invinciblemanHumans, i, InvinciblemanHuman, h);
    SKIP_IS_NOT_ALIVE(h);
    float d = distanceTo(&h->pos, px, py);
    if (best < 0 || d < bestDist) {
      best = i;
      bestDist = d;
    }
  }
  return best;
}

void invinciblemanUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&invinciblemanPlayer.pos, 50, 40);
    invinciblemanPlayer.va = -1;
    invinciblemanPlayer.ticksVal = 0;
    INIT_UNALIVED_ARRAY_FAST(invinciblemanHumans);
    TIMES(10, i) {
      vectorSet(&invinciblemanHumans[i].pos, rnd(40, 60), rnd(40, 60));
      vectorSet(&invinciblemanHumans[i].targetPos, rnd(40, 60), rnd(40, 60));
      vectorSet(&invinciblemanHumans[i].vel, 0, 0);
      invinciblemanHumans[i].ticksVal = rnd(0, 60);
      invinciblemanHumans[i].isAlive = true;
    }
    invinciblemanHumanIndex = 10 % INVINCIBLEMAN_MAX_HUMAN_COUNT;
    invinciblemanNextHumanTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(invinciblemanEnemies);
    invinciblemanEnemyIndex = 0;
    invinciblemanNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(invinciblemanExplosions);
    invinciblemanExplosionIndex = 0;
    invinciblemanMultiplier = 1;
  }
  float sd = difficulty;
  float tc = angleTo(&invinciblemanPlayer.pos, 50, 50);
  if (input.isJustPressed) {
    play(LASER);
    invinciblemanPlayer.va *= -1;
  }
  float px = invinciblemanPlayer.pos.x;
  if (input.isPressed) {
    addWithAngle(&invinciblemanPlayer.pos, tc, -sd);
  } else {
    addWithAngle(&invinciblemanPlayer.pos, tc + CGLP_PI_2 * invinciblemanPlayer.va, 0.7 * sd);
  }
  addWithAngle(&invinciblemanPlayer.pos, tc,
               (distanceTo(&invinciblemanPlayer.pos, 50, 50) + 9) * 0.005 * sd);
  invinciblemanPlayer.pos.x = clamp(invinciblemanPlayer.pos.x, 0, 100);
  invinciblemanPlayer.pos.y = clamp(invinciblemanPlayer.pos.y, 0, 100);
  invinciblemanPlayer.ticksVal += sd;
  color = CYAN;
  if (invinciblemanPlayer.pos.x > px) {
    characterOptions.isMirrorX = false;
  } else {
    characterOptions.isMirrorX = true;
  }
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  int[2] plc;
  plc[0] = 'a' + ((int)(invinciblemanPlayer.ticksVal / 30) % 2);
  plc[1] = 0;
  character(plc, invinciblemanPlayer.pos.x, invinciblemanPlayer.pos.y, &scratch);
  color = RED;
  FOR_EACH(invinciblemanExplosions, i) {
    ASSIGN_ARRAY_ITEM(invinciblemanExplosions, i, InvinciblemanExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    e->radius += e->rv * 0.6 * sd;
    if (e->rv > 0) {
      if (e->radius > 12) {
        e->rv = -1;
      }
    } else {
      if (e->radius < 1) {
        e->isAlive = false;
        continue;
      }
    }
    thickness = 3;
    arc(e->pos.x, e->pos.y, e->radius, 0, CGLP_PI * 2, &scratch);
  }
  invinciblemanNextHumanTicks -= sd;
  if (invinciblemanNextHumanTicks < 0) {
    if (invinciblemanCountHumans() < 9) {
      Vector p;
      vectorSet(&p, 50, 50);
      int hc = 1;
      FOR_EACH(invinciblemanHumans, i) {
        ASSIGN_ARRAY_ITEM(invinciblemanHumans, i, InvinciblemanHuman, h);
        SKIP_IS_NOT_ALIVE(h);
        vectorAdd(&p, h->pos.x, h->pos.y);
        hc++;
      }
      vectorMul(&p, 1.0 / hc);
      ASSIGN_ARRAY_ITEM(invinciblemanHumans, invinciblemanHumanIndex, InvinciblemanHuman, nh);
      nh->pos = p;
      vectorSet(&nh->targetPos, rnd(40, 60), rnd(40, 60));
      vectorSet(&nh->vel, 0, 0);
      nh->ticksVal = rnd(0, 60);
      nh->isAlive = true;
      invinciblemanHumanIndex = cgl_wrap(invinciblemanHumanIndex + 1, 0, INVINCIBLEMAN_MAX_HUMAN_COUNT);
    }
    invinciblemanNextHumanTicks = 600;
  }
  color = BLACK;
  FOR_EACH(invinciblemanHumans, i) {
    ASSIGN_ARRAY_ITEM(invinciblemanHumans, i, InvinciblemanHuman, h);
    SKIP_IS_NOT_ALIVE(h);
    bool hasTa = false;
    float ta = 0;
    int nei = invinciblemanNearestEnemyIndex(h->pos.x, h->pos.y);
    if (nei >= 0) {
      InvinciblemanEnemy* ne = &invinciblemanEnemies[nei];
      if (distanceTo(&ne->pos, h->pos.x, h->pos.y) < 25) {
        ta = angleTo(&ne->pos, h->pos.x, h->pos.y);
        hasTa = true;
      }
    }
    if (!hasTa) {
      if (distanceTo(&h->pos, h->targetPos.x, h->targetPos.y) < 1) {
        vectorSet(&h->targetPos, rnd(40, 60), rnd(40, 60));
      }
      ta = angleTo(&h->pos, h->targetPos.x, h->targetPos.y);
    }
    addWithAngle(&h->vel, ta, 0.01);
    vectorMul(&h->vel, 0.9);
    float hpx = h->pos.x;
    vectorAdd(&h->pos, h->vel.x * sd, h->vel.y * sd);
    h->pos.x = clamp(h->pos.x, 10, 90);
    h->pos.y = clamp(h->pos.y, 10, 90);
    h->ticksVal += sd;
    if (h->pos.x > hpx) {
      characterOptions.isMirrorX = false;
    } else {
      characterOptions.isMirrorX = true;
    }
    int[2] hc2;
    hc2[0] = 'a' + ((int)(h->ticksVal / 30) % 2);
    hc2[1] = 0;
    character(hc2, h->pos.x, h->pos.y, &scratch);
    if (scratch.isColliding.rect[RED]) {
      play(EXPLOSION);
      particle(h->pos.x, h->pos.y, 9, 2, 0, CGLP_PI * 2);
      h->isAlive = false;
      continue;
    }
  }
  invinciblemanNextEnemyTicks -= sd;
  if (invinciblemanNextEnemyTicks < 0) {
    Vector ep;
    vectorSet(&ep, 50, 50);
    addWithAngle(&ep, rnd(0, CGLP_PI * 2), 80);
    int ec = rndi(3, 9);
    TIMES(ec, i) {
      ASSIGN_ARRAY_ITEM(invinciblemanEnemies, invinciblemanEnemyIndex, InvinciblemanEnemy, ne);
      vectorSet(&ne->pos, ep.x + rnd(0, 9) * RNDPM(), ep.y + rnd(0, 9) * RNDPM());
      vectorSet(&ne->vel, rnd(0, 1 / sd) * RNDPM(), rnd(0, 1 / sd) * RNDPM());
      ne->ticksVal = rnd(0, 60);
      ne->isAlive = true;
      invinciblemanEnemyIndex = cgl_wrap(invinciblemanEnemyIndex + 1, 0, INVINCIBLEMAN_MAX_ENEMY_COUNT);
    }
    invinciblemanNextEnemyTicks = rnd(70, 99) * sqrt(ec);
  }
  color = RED;
  FOR_EACH(invinciblemanEnemies, i) {
    ASSIGN_ARRAY_ITEM(invinciblemanEnemies, i, InvinciblemanEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float epx = e->pos.x;
    if (invinciblemanCountHumans() > 0) {
      int nhi = invinciblemanNearestHumanIndex(e->pos.x, e->pos.y);
      InvinciblemanHuman* nh = &invinciblemanHumans[nhi];
      addWithAngle(&e->vel, angleTo(&e->pos, nh->pos.x, nh->pos.y), 0.005);
      vectorMul(&e->vel, 0.95);
      vectorAdd(&e->pos, e->vel.x * sd, e->vel.y * sd);
    }
    e->ticksVal += sd;
    if (e->pos.x > epx) {
      characterOptions.isMirrorX = false;
    } else {
      characterOptions.isMirrorX = true;
    }
    int[2] ec2;
    ec2[0] = 'c' + ((int)(e->ticksVal / 30) % 2);
    ec2[1] = 0;
    character(ec2, e->pos.x, e->pos.y, &scratch);
    if (scratch.isColliding.rect[RED] || scratch.isColliding.character['a'] ||
        scratch.isColliding.character['b']) {
      play(POWER_UP);
      ASSIGN_ARRAY_ITEM(invinciblemanExplosions, invinciblemanExplosionIndex, InvinciblemanExplosion, nex);
      nex->pos = e->pos;
      nex->radius = 1;
      nex->rv = 1;
      nex->isAlive = true;
      invinciblemanExplosionIndex = cgl_wrap(invinciblemanExplosionIndex + 1, 0, INVINCIBLEMAN_MAX_EXPLOSION_COUNT);
      addScore(invinciblemanMultiplier, e->pos.x, e->pos.y);
      invinciblemanMultiplier++;
      e->isAlive = false;
      continue;
    }
  }
  characterOptions.isMirrorX = false;
  COUNT_IS_ALIVE(invinciblemanExplosions, aliveExplosionCount);
  if (aliveExplosionCount == 0) {
    invinciblemanMultiplier = 1;
  }
  if (invinciblemanCountHumans() == 0) {
    play(RANDOM);
    gameOver();
  }
}

void addGameInvincibleman() {
  addGame(invinciblemanTitle, invinciblemanDescription, invinciblemanCharacters,
          invinciblemanCharactersCount, &invinciblemanOptions, false,
          &invinciblemanUpdate);
}
