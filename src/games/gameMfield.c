#include "../cglp.h"

int* mfieldTitle = "M FIELD";
int* mfieldDescription = "[Tap]\n Jump / Double jump\n[Hold]\n Speed up";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] mfieldCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
};
int mfieldCharactersCount = 4;

Options mfieldOptions = {200, 100, 2, false};

struct MfieldMine {
  float x;
  float ticks;
  bool isBlinking;
  bool isAlive;
};
#define MFIELD_MAX_MINE_COUNT 32
MfieldMine[MFIELD_MAX_MINE_COUNT] mfieldMines;
int mfieldMineIndex;
float mfieldNextMineDist;

struct MfieldExplosion {
  float x;
  float ticks;
  bool isAlive;
};
#define MFIELD_MAX_EXPLOSION_COUNT 16
MfieldExplosion[MFIELD_MAX_EXPLOSION_COUNT] mfieldExplosions;
int mfieldExplosionIndex;

struct MfieldEnemy {
  float x;
  float vx;
  bool isAlive;
};
#define MFIELD_MAX_ENEMY_COUNT 32
MfieldEnemy[MFIELD_MAX_ENEMY_COUNT] mfieldEnemies;
int mfieldEnemyIndex;
float mfieldNextEnemyDist;

struct MfieldPlayer {
  Vector pos;
  Vector vel;
  int jumpCount;
};
MfieldPlayer mfieldPlayer;
float mfieldGx;
int mfieldMultiplier;

void mfieldExplode(float x) {
  play(EXPLOSION);
  ASSIGN_ARRAY_ITEM(mfieldExplosions, mfieldExplosionIndex, MfieldExplosion, ne);
  ne->x = x;
  ne->ticks = 0;
  ne->isAlive = true;
  mfieldExplosionIndex = cgl_wrap(mfieldExplosionIndex + 1, 0, MFIELD_MAX_EXPLOSION_COUNT);
  color = RED;
  particle(x, 89, 9, 2, -CGLP_PI / 2, CGLP_PI / 2);
}

void mfieldUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(mfieldMines);
    mfieldMineIndex = 0;
    mfieldNextMineDist = 0;
    INIT_UNALIVED_ARRAY_FAST(mfieldExplosions);
    mfieldExplosionIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(mfieldEnemies);
    mfieldEnemyIndex = 0;
    mfieldNextEnemyDist = 0;
    vectorSet(&mfieldPlayer.pos, 80, 87);
    vectorSet(&mfieldPlayer.vel, 0, 0);
    mfieldPlayer.jumpCount = 0;
    mfieldGx = 0;
    mfieldMultiplier = 1;
  }
  float scr;
  if (mfieldPlayer.pos.x > 99) {
    scr = (mfieldPlayer.pos.x - 99) * 0.1 * sqrt(difficulty);
  } else {
    scr = 0;
  }
  color = LIGHT_BLACK;
  rect(0, 90, 200, 10, &scratch);
  mfieldGx = cgl_wrap(mfieldGx - scr, 0, 200);
  color = WHITE;
  rect(mfieldGx, 90, 2, 10, &scratch);
  color = RED;
  FOR_EACH(mfieldExplosions, i) {
    ASSIGN_ARRAY_ITEM(mfieldExplosions, i, MfieldExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x -= scr;
    e->ticks += sqrt(difficulty);
    box(e->x, 86, sin(e->ticks * 0.1) * 50, 8, &scratch);
    if (e->ticks > CGLP_PI / 0.1) {
      e->isAlive = false;
      continue;
    }
  }
  mfieldNextEnemyDist -= scr;
  if (mfieldNextEnemyDist < 0) {
    ASSIGN_ARRAY_ITEM(mfieldEnemies, mfieldEnemyIndex, MfieldEnemy, ne);
    if (rnd(0, 1) < 0.7) {
      ne->x = 203;
      ne->vx = -rnd(1, 2) * sqrt(difficulty) * 0.4;
    } else {
      ne->x = -3;
      ne->vx = rnd(1.5, 2) * sqrt(difficulty) * 0.4;
    }
    ne->isAlive = true;
    mfieldEnemyIndex = cgl_wrap(mfieldEnemyIndex + 1, 0, MFIELD_MAX_ENEMY_COUNT);
    mfieldNextEnemyDist += rnd(20, 30);
  }
  color = PURPLE;
  FOR_EACH(mfieldEnemies, i) {
    ASSIGN_ARRAY_ITEM(mfieldEnemies, i, MfieldEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x += e->vx - scr;
    if (mfieldPlayer.jumpCount == 0 &&
        ((e->x < mfieldPlayer.pos.x - 20 && e->vx < 0) ||
         (e->x > mfieldPlayer.pos.x + 20 && e->vx > 0))) {
      e->vx *= -1;
    }
    int[2] ec;
    ec[0] = 'c' + (int)floor(ticks / 20) % 2;
    ec[1] = 0;
    if (e->vx > 0) {
      characterOptions.isMirrorX = true;
    } else {
      characterOptions.isMirrorX = false;
    }
    Collision ecColl;
    character(ec, e->x, 87, &ecColl);
    characterOptions.isMirrorX = false;
    if (ecColl.isColliding.rect[RED]) {
      play(POWER_UP);
      addScore(mfieldMultiplier, e->x, 87);
      particle(e->x, 87, 16, 1, 0, CGLP_PI * 2);
      if (mfieldMultiplier < 64) {
        mfieldMultiplier *= 2;
      }
      e->isAlive = false;
      continue;
    }
    if (e->x < -9) {
      e->isAlive = false;
      continue;
    }
  }
  float pressMul;
  if (input.isPressed) {
    pressMul = 1;
  } else {
    pressMul = 0.3;
  }
  mfieldPlayer.vel.x += (pressMul * sqrt(difficulty) - mfieldPlayer.vel.x) * 0.1;
  if (mfieldPlayer.jumpCount > 0) {
    float gravMul;
    if (input.isPressed) {
      gravMul = 0.05;
    } else {
      gravMul = 0.1;
    }
    mfieldPlayer.vel.y += gravMul * difficulty;
    if (mfieldPlayer.pos.y > 87) {
      mfieldPlayer.pos.y = 87;
      mfieldPlayer.vel.y = 0;
      mfieldPlayer.jumpCount = 0;
    }
  }
  if (mfieldPlayer.jumpCount < 2 && input.isJustPressed) {
    play(JUMP);
    mfieldPlayer.vel.y = -2 * sqrt(difficulty);
    mfieldPlayer.pos.y -= 6;
    mfieldPlayer.jumpCount++;
  }
  vectorAdd(&mfieldPlayer.pos, mfieldPlayer.vel.x, mfieldPlayer.vel.y);
  mfieldPlayer.pos.x -= scr;
  color = BLACK;
  int[2] pc;
  if (mfieldPlayer.jumpCount > 0 || ticks % 30 > 15) {
    pc[0] = 'a';
  } else {
    pc[0] = 'b';
  }
  pc[1] = 0;
  Collision c;
  character(pc, mfieldPlayer.pos.x, mfieldPlayer.pos.y, &c);
  if (c.isColliding.rect[RED]) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  if (c.isColliding.character['c'] || c.isColliding.character['d']) {
    if (mfieldPlayer.vel.y > 0) {
      play(JUMP);
      mfieldPlayer.vel.y *= -0.8;
      mfieldPlayer.pos.y = 80;
    } else {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  mfieldNextMineDist -= scr;
  if (mfieldNextMineDist < 0) {
    ASSIGN_ARRAY_ITEM(mfieldMines, mfieldMineIndex, MfieldMine, nm);
    nm->x = 203;
    nm->ticks = 0;
    nm->isBlinking = false;
    nm->isAlive = true;
    mfieldMineIndex = cgl_wrap(mfieldMineIndex + 1, 0, MFIELD_MAX_MINE_COUNT);
    if (rnd(0, 1) < 0.6) {
      mfieldNextMineDist = rnd(9, 20);
    } else {
      mfieldNextMineDist = rnd(50, 80);
    }
  }
  FOR_EACH(mfieldMines, i) {
    ASSIGN_ARRAY_ITEM(mfieldMines, i, MfieldMine, m);
    SKIP_IS_NOT_ALIVE(m);
    m->x -= scr;
    float y;
    if (m->ticks > 0) {
      y = 89;
    } else {
      y = 91;
    }
    color = PURPLE;
    if (m->ticks > 0) {
      m->ticks += sqrt(difficulty);
      if (m->ticks > 59) {
        mfieldExplode(m->x);
        m->isAlive = false;
        continue;
      } else if (fmod(m->ticks, 30) > 15) {
        if (!m->isBlinking) {
          play(LASER);
        }
        m->isBlinking = true;
      } else {
        color = TRANSPARENT;
        m->isBlinking = false;
      }
    }
    Collision mc;
    box(m->x, y, 6, 3, &mc);
    if (mc.isColliding.rect[RED]) {
      mfieldExplode(m->x);
      m->isAlive = false;
      continue;
    } else if (m->ticks == 0 && (mc.isColliding.character['a'] || mc.isColliding.character['b'])) {
      play(HIT);
      mfieldMultiplier = 1;
      m->ticks = 1;
    }
    if (m->x < -3) {
      m->isAlive = false;
      continue;
    }
  }
}

void addGameMfield() {
  addGame(mfieldTitle, mfieldDescription, mfieldCharacters, mfieldCharactersCount,
          &mfieldOptions, false, &mfieldUpdate);
}
