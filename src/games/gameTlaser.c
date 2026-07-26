#include "../cglp.h"

int* tlaserTitle = "T LASER";
int* tlaserDescription = "[D-Pad]\n Move sight";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tlaserCharacters = {
    {
        "  gG  ",
        "  gG  ",
        "L gG l",
        "LLgGll",
        " LgGl ",
        " LgGl ",
    },
    {
        "     r",
        " l  l ",
        "l    l",
        "l    l",
        " l  l ",
        "r     ",
    },
};
int tlaserCharactersCount = 2;

Options tlaserOptions = {100, 100, 1, true};

#define TLASER_SIGHT_SPEED 1.5

struct TlaserStar {
  Vector pos;
  float z;
};
#define TLASER_STAR_COUNT 50
TlaserStar[TLASER_STAR_COUNT] tlaserStars;

struct TlaserEnemy {
  Vector pos;
  float z;
  Vector vel;
  int lockIndex; // -1 = not locked onto
  int lockTicks;
  bool isAlive;
};
// Once locked, an enemy never expires on its own - it only dies when the
// single laser eventually reaches it (processed one at a time in lock
// order), while any UNLOCKED enemy reaching the screen edge ends the game.
// So normal play forces the player to lock everything, and spawn batches
// (3-6 every rnd(60,99)/difficulty ticks) arrive faster than the one laser
// can clear them as difficulty climbs - the locked backlog grows without
// bound over a long session, so sized with real headroom.
#define TLASER_MAX_ENEMY_COUNT 512
TlaserEnemy[TLASER_MAX_ENEMY_COUNT] tlaserEnemies;
int tlaserEnemyIndex;

#define TLASER_LASER_HISTORY_MAX 19
struct TlaserLaser {
  bool isAlive;
  Vector pos;
  float z;
  Vector vel;
  int lockIndex;
  Vector[TLASER_LASER_HISTORY_MAX] posHistory;
  int posHistoryCount;
  int posHistoryStart;
};
TlaserLaser tlaserLaser;

Vector tlaserSightPos;
int tlaserNextLockIndex;
float tlaserNextEnemyTicks;
int tlaserMultiplier;

// Push a new trailing position onto the laser's history, keeping at
// most the last TLASER_LASER_HISTORY_MAX positions (mirrors upstream's
// posHistory.push(...); if (length > 19) posHistory.shift();).
void tlaserPushLaserHistory(Vector* p) {
  int idx = cgl_wrap(tlaserLaser.posHistoryStart + tlaserLaser.posHistoryCount, 0, TLASER_LASER_HISTORY_MAX);
  tlaserLaser.posHistory[idx] = *p;
  if (tlaserLaser.posHistoryCount < TLASER_LASER_HISTORY_MAX) {
    tlaserLaser.posHistoryCount++;
  } else {
    tlaserLaser.posHistoryStart = cgl_wrap(tlaserLaser.posHistoryStart + 1, 0, TLASER_LASER_HISTORY_MAX);
  }
}

void tlaserUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&tlaserSightPos, 50, 50);
    INIT_UNALIVED_ARRAY_FAST(tlaserEnemies);
    tlaserEnemyIndex = 0;
    tlaserNextEnemyTicks = 0;
    tlaserNextLockIndex = 0;
    tlaserLaser.isAlive = false;
    TIMES(TLASER_STAR_COUNT, sti) {
      vectorSet(&tlaserStars[sti].pos, rnd(0, 99), rnd(-20, 120));
      tlaserStars[sti].z = rnd(0.5, 3);
    }
    tlaserMultiplier = 1;
  }

  TIMES(TLASER_STAR_COUNT, i) {
    TlaserStar* s = &tlaserStars[i];
    s->pos.y += 0.3 / sqrt(s->z);
    if (s->pos.y > 120) {
      s->pos.y -= 140;
    }
    int m = i % 3;
    if (m == 0) {
      color = LIGHT_CYAN;
    } else if (m == 1) {
      color = LIGHT_YELLOW;
    } else {
      color = LIGHT_RED;
    }
    rect(s->pos.x, s->pos.y, 1, 1, &scratch);
  }

  color = BLACK;
  // Upstream drags the sight via mouse/touch; redesigned to direct d-pad movement.
  if (input.left.isPressed) {
    tlaserSightPos.x -= TLASER_SIGHT_SPEED;
  }
  if (input.right.isPressed) {
    tlaserSightPos.x += TLASER_SIGHT_SPEED;
  }
  if (input.up.isPressed) {
    tlaserSightPos.y -= TLASER_SIGHT_SPEED;
  }
  if (input.down.isPressed) {
    tlaserSightPos.y += TLASER_SIGHT_SPEED;
  }
  tlaserSightPos.x = clamp(tlaserSightPos.x, 0, 99);
  tlaserSightPos.y = clamp(tlaserSightPos.y, 0, 99);
  character("b", tlaserSightPos.x, tlaserSightPos.y, &scratch);

  int ti = 9999999;
  int teIndex = -1;
  FOR_EACH(tlaserEnemies, ei) {
    ASSIGN_ARRAY_ITEM(tlaserEnemies, ei, TlaserEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (e->lockIndex >= 0 && e->lockIndex < ti) {
      ti = e->lockIndex;
      teIndex = ei;
    }
  }

  if (!tlaserLaser.isAlive && teIndex >= 0) {
    play(EXPLOSION);
    tlaserLaser.isAlive = true;
    vectorSet(&tlaserLaser.pos, 50, 99);
    tlaserLaser.z = 1;
    vectorSet(&tlaserLaser.vel, 0, -1);
    tlaserLaser.lockIndex = 0;
    vectorSet(&tlaserLaser.posHistory[0], 50, 99);
    tlaserLaser.posHistoryCount = 1;
    tlaserLaser.posHistoryStart = 0;
    color = BLACK;
    particle(50, 99, 30, 3, -CGLP_PI / 2, CGLP_PI / 4);
    tlaserMultiplier = 1;
  }
  if (tlaserLaser.isAlive) {
    float vz = 0;
    if (teIndex >= 0) {
      TlaserEnemy* te = &tlaserEnemies[teIndex];
      Vector diff;
      vectorSet(&diff, te->pos.x, te->pos.y);
      vectorAdd(&diff, -tlaserLaser.pos.x, -tlaserLaser.pos.y);
      vectorMul(&diff, 0.5);
      vectorAdd(&tlaserLaser.vel, diff.x, diff.y);
      vectorMul(&tlaserLaser.vel, 0.5);
      vz = (te->z - tlaserLaser.z) * 0.1;
      tlaserLaser.z += vz;
    } else {
      float len = vectorLength(&tlaserLaser.vel);
      // Vircon32 port note: real division by zero hard-traps the CPU on
      // this platform (unlike JS, which would silently produce
      // Infinity/NaN) - guard the normalize step's length divide.
      if (len > 0) {
        vectorMul(&tlaserLaser.vel, 1.0 / len);
      }
      vectorMul(&tlaserLaser.vel, 5);
    }
    Vector step;
    step = tlaserLaser.vel;
    vectorMul(&step, 1.0 / tlaserLaser.z);
    vectorMul(&step, 1.0 / (1 + fabs(vz) * 9));
    vectorAdd(&tlaserLaser.pos, step.x, step.y);
    Vector newHistPos;
    newHistPos = tlaserLaser.pos;
    tlaserPushLaserHistory(&newHistPos);

    bool hasPrev = false;
    Vector pp;
    for (int k = 0; k < tlaserLaser.posHistoryCount; k++) {
      int idx = cgl_wrap(tlaserLaser.posHistoryStart + k, 0, TLASER_LASER_HISTORY_MAX);
      Vector p = tlaserLaser.posHistory[idx];
      if (hasPrev) {
        if (rnd(0, 1) < (float)k / 9) {
          color = PURPLE;
        } else {
          color = BLACK;
        }
        float rs = 5 / tlaserLaser.z;
        thickness = 3 / tlaserLaser.z;
        float x1 = pp.x + rnd(0, rs) * RNDPM();
        float y1 = pp.y + rnd(0, rs) * RNDPM();
        float x2 = p.x + rnd(0, rs) * RNDPM();
        float y2 = p.y + rnd(0, rs) * RNDPM();
        line(x1, y1, x2, y2, &scratch);
      }
      pp = p;
      hasPrev = true;
    }
    float o = 99 / tlaserLaser.z;
    if (!(tlaserLaser.pos.x >= -o && tlaserLaser.pos.x <= 100 + o && tlaserLaser.pos.y >= -o && tlaserLaser.pos.y <= 100 + o)) {
      tlaserLaser.isAlive = false;
      tlaserMultiplier = 1;
    }
  }

  tlaserNextEnemyTicks--;
  if (tlaserNextEnemyTicks < 0) {
    int c = rndi(3, 6);
    Vector vel;
    vectorSet(&vel, 0, 0.2 + rnd(0, 0.3) * RNDPM());
    vectorMul(&vel, sqrt(difficulty));
    float startY;
    if (vel.y > 0) {
      startY = -5;
    } else {
      startY = 105;
    }
    Vector spos;
    vectorSet(&spos, rnd(10, 90), startY);
    float z = rnd(0.5, 3);
    vectorMul(&vel, 1.0 / sqrt(z));
    TIMES(c, ci) {
      ASSIGN_ARRAY_ITEM(tlaserEnemies, tlaserEnemyIndex, TlaserEnemy, ne);
      ne->pos = spos;
      ne->z = z;
      ne->vel = vel;
      ne->lockIndex = -1;
      ne->lockTicks = 9;
      ne->isAlive = true;
      tlaserEnemyIndex = cgl_wrap(tlaserEnemyIndex + 1, 0, TLASER_MAX_ENEMY_COUNT);
      spos.y -= vel.y * 30;
    }
    tlaserNextEnemyTicks += rnd(60, 99) / difficulty;
  }

  FOR_EACH(tlaserEnemies, ei2) {
    ASSIGN_ARRAY_ITEM(tlaserEnemies, ei2, TlaserEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    color = BLACK;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = e->vel.y > 0;
    characterOptions.rotation = 0;
    Collision ec;
    character("a", e->pos.x, e->pos.y, &ec);
    if (ei2 == teIndex && ec.isColliding.rect[PURPLE]) {
      play(HIT);
      particle(e->pos.x, e->pos.y, 9, 2 / sqrt(e->z), 0, CGLP_PI * 2);
      addScore(tlaserMultiplier, e->pos.x, e->pos.y);
      tlaserMultiplier++;
      e->isAlive = false;
      continue;
    }
    if (e->lockIndex < 0 && ec.isColliding.character['b']) {
      play(SELECT);
      e->lockIndex = tlaserNextLockIndex;
      tlaserNextLockIndex++;
    }
    if (e->lockIndex >= 0) {
      if (e->lockTicks > 0) {
        e->lockTicks--;
      }
      float a = -CGLP_PI / 2 - e->lockTicks * 0.3;
      float r = 5 + e->lockTicks * 5;
      thickness = 1;
      TIMES(4, li) {
        float pa = a;
        a += CGLP_PI * 2 / (3 + (1.0 / 8) * e->lockTicks);
        Vector p1;
        vectorSet(&p1, r, 0);
        rotate(&p1, pa);
        vectorAdd(&p1, e->pos.x, e->pos.y);
        Vector p2;
        vectorSet(&p2, r, 0);
        rotate(&p2, a);
        vectorAdd(&p2, e->pos.x, e->pos.y);
        line(p1.x, p1.y, p2.x, p2.y, &scratch);
      }
    }
    if (e->lockIndex < 0) {
      bool offscreen;
      if (e->vel.y < 0) {
        offscreen = e->pos.y < 0;
      } else {
        offscreen = e->pos.y > 99;
      }
      if (offscreen) {
        color = RED;
        characterOptions.isMirrorX = false;
        characterOptions.isMirrorY = false;
        characterOptions.rotation = 0;
        text("X", e->pos.x, clamp(e->pos.y, 5, 95), &scratch);
        play(RANDOM);
        gameOver();
      }
    }
  }
  color = BLACK;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(tlaserMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameTlaser() {
  addGame(tlaserTitle, tlaserDescription, tlaserCharacters, tlaserCharactersCount,
          &tlaserOptions, false, &tlaserUpdate);
}
