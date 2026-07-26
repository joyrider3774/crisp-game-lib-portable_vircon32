#include "../cglp.h"

int* liedownTitle = "LIE DOWN";
int* liedownDescription = "[Hold] Lie down";

int[6][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] liedownCharacters = {
    {
        "    ll",
        "   l  ",
        " llll ",
        "l l   ",
        "ll l  ",
        "    l ",
    },
    {
        "    ll",
        "   l  ",
        " lll  ",
        "  l   ",
        " ll   ",
        " ll   ",
    },
    {
        "l   ll",
        "llllll",
    },
    {
        " ll l ",
        "  l l ",
        "  ll  ",
        "  l   ",
        "ll l  ",
        "    l ",
    },
    {
        "ll    ",
        " l    ",
        " lll  ",
        "l l   ",
        " l l  ",
        " l  l ",
    },
    {
        " l    ",
        "lll   ",
        " l    ",
    },
};
int liedownCharactersCount = 6;

Options liedownOptions = {200, 50, 2, false};

struct LiedownHole {
  float x;
  float height;
  float speed;
  bool isAlive;
};
#define LIEDOWN_MAX_HOLE_COUNT 32
LiedownHole[LIEDOWN_MAX_HOLE_COUNT] liedownHoles;
int liedownHoleIndex;
float liedownNextHoleTicks;

struct LiedownEnemy {
  float x;
  float height;
  float speed;
  bool isAlive;
};
#define LIEDOWN_MAX_ENEMY_COUNT 32
LiedownEnemy[LIEDOWN_MAX_ENEMY_COUNT] liedownEnemies;
int liedownEnemyIndex;
float liedownNextEnemyTicks;

struct LiedownRock {
  float x;
  float vx;
  float targetVx;
  bool isAlive;
};
#define LIEDOWN_MAX_ROCK_COUNT 32
LiedownRock[LIEDOWN_MAX_ROCK_COUNT] liedownRocks;
int liedownRockIndex;

struct LiedownPlayer {
  float x;
  float vx;
  float y;
};
LiedownPlayer liedownPlayer;

float liedownSeparateLine;
int liedownMultiplier;

void liedownUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(liedownHoles);
    liedownHoleIndex = 0;
    liedownNextHoleTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(liedownEnemies);
    liedownEnemyIndex = 0;
    liedownNextEnemyTicks = 60;
    INIT_UNALIVED_ARRAY_FAST(liedownRocks);
    liedownRockIndex = 0;
    liedownPlayer.x = 20;
    liedownPlayer.vx = 0;
    liedownPlayer.y = 0;
    liedownSeparateLine = 100;
    liedownMultiplier = 1;
  }
  float scr = difficulty * 0.1;
  if (liedownPlayer.x > 30) {
    scr += (liedownPlayer.x - 30) * 0.1;
  }
  if (liedownPlayer.y > 0) {
    scr = 0;
  }
  color = BLACK;
  rect(0, 40, 200, 10, &scratch);
  color = LIGHT_BLACK;
  liedownSeparateLine = cgl_wrap(liedownSeparateLine - scr, 0, 200);
  rect(liedownSeparateLine, 40, 1, 10, &scratch);
  color = BLACK;
  liedownNextEnemyTicks--;
  if (liedownNextEnemyTicks < 0) {
    play(LASER);
    ASSIGN_ARRAY_ITEM(liedownEnemies, liedownEnemyIndex, LiedownEnemy, ne);
    ne->x = rnd(100, 250);
    ne->height = 0;
    ne->speed = (0.1 + rnd(0, 2) * rnd(0, 2) * 0.1) * difficulty;
    ne->isAlive = true;
    liedownEnemyIndex = cgl_wrap(liedownEnemyIndex + 1, 0, LIEDOWN_MAX_ENEMY_COUNT);
    liedownNextEnemyTicks = 50 / sqrt(difficulty);
  }
  color = RED;
  FOR_EACH(liedownRocks, i) {
    ASSIGN_ARRAY_ITEM(liedownRocks, i, LiedownRock, r);
    SKIP_IS_NOT_ALIVE(r);
    r->x -= scr + r->vx;
    r->vx += (r->targetVx - r->vx) * 0.1;
    if (r->vx > r->targetVx * 0.5) {
      character("f", r->x, 34, &scratch);
    }
    if (r->x < -3 || r->x > 203) {
      r->isAlive = false;
      continue;
    }
  }
  liedownNextHoleTicks--;
  if (liedownNextHoleTicks < 0) {
    ASSIGN_ARRAY_ITEM(liedownHoles, liedownHoleIndex, LiedownHole, nh);
    nh->x = rnd(0, 300);
    nh->height = 0;
    nh->speed = rnd(0.1, 0.2) * difficulty;
    nh->isAlive = true;
    liedownHoleIndex = cgl_wrap(liedownHoleIndex + 1, 0, LIEDOWN_MAX_HOLE_COUNT);
    liedownNextHoleTicks = 100 / sqrt(difficulty);
  }
  color = WHITE;
  FOR_EACH(liedownHoles, i) {
    ASSIGN_ARRAY_ITEM(liedownHoles, i, LiedownHole, h);
    SKIP_IS_NOT_ALIVE(h);
    h->x -= scr;
    h->height += h->speed;
    if (h->height > 11 && h->speed > 0) {
      h->speed *= -1;
    }
    float hg = clamp(h->height, 0, 10);
    rect(h->x, 50 - hg, 9, hg + 1, &scratch);
    if (h->height < 0) {
      h->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  if (liedownPlayer.y > 0) {
    liedownPlayer.y++;
    characterOptions.isMirrorY = true;
    character("a", liedownPlayer.x, 37 + liedownPlayer.y, &scratch);
    characterOptions.isMirrorY = false;
    if (37 + liedownPlayer.y > 45) {
      play(EXPLOSION);
      gameOver();
    }
  } else {
    if (input.isJustPressed) {
      play(SELECT);
      if (liedownMultiplier > 1) {
        liedownMultiplier--;
      }
    }
    if (!input.isPressed) {
      liedownPlayer.vx += difficulty * 0.02;
    }
    liedownPlayer.vx *= 0.99;
    liedownPlayer.x += liedownPlayer.vx - scr;
    if (liedownPlayer.x < 0) {
      play(EXPLOSION);
      gameOver();
    }
    int[2] pc;
    float py;
    if (input.isPressed) {
      pc[0] = 'c';
      py = 39;
    } else {
      pc[0] = 'a' + (int)floor(ticks / 20) % 2;
      py = 37;
    }
    pc[1] = 0;
    Collision pcColl;
    character(pc, liedownPlayer.x, py, &pcColl);
    if (pcColl.isColliding.character['f']) {
      play(EXPLOSION);
      gameOver();
    }
    color = TRANSPARENT;
    Collision c1;
    character("a", liedownPlayer.x - 5, 38, &c1);
    Collision c2;
    character("a", liedownPlayer.x + 5, 38, &c2);
    if (c1.isColliding.rect[WHITE] && c2.isColliding.rect[WHITE]) {
      liedownPlayer.vx = 0;
      if (input.isPressed) {
        liedownPlayer.x -= 5;
      } else {
        liedownPlayer.y = 1;
      }
    }
  }
  FOR_EACH(liedownEnemies, i) {
    ASSIGN_ARRAY_ITEM(liedownEnemies, i, LiedownEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x -= scr;
    if (e->speed > 0) {
      e->height += sqrt(e->speed * 10) / 10;
    } else {
      e->height += e->speed;
    }
    if (e->height > 15 && e->speed > 0) {
      play(POWER_UP);
      ASSIGN_ARRAY_ITEM(liedownRocks, liedownRockIndex, LiedownRock, nr);
      nr->x = e->x;
      nr->vx = 0;
      nr->targetVx = e->speed * 3;
      nr->isAlive = true;
      liedownRockIndex = cgl_wrap(liedownRockIndex + 1, 0, LIEDOWN_MAX_ROCK_COUNT);
      e->speed *= -1;
    }
    float hg = 10 - clamp(e->height, 0, 10);
    float y = 37 + (hg * hg) / 10;
    if (e->height > 12 && e->speed > 0) {
      color = RED;
    } else {
      color = BLACK;
    }
    int[2] ec;
    if (e->speed > 0) {
      ec[0] = 'd';
    } else {
      ec[0] = 'd' + 1;
    }
    ec[1] = 0;
    Collision ecColl;
    character(ec, e->x, y, &ecColl);
    if (ecColl.isColliding.character['a'] || ecColl.isColliding.character['b'] ||
        ecColl.isColliding.character['c']) {
      play(COIN);
      addScore(liedownMultiplier, e->x, y);
      liedownMultiplier++;
      particle(e->x, y, 16, 1, 0, CGLP_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (e->height < 0) {
      e->isAlive = false;
      continue;
    }
  }
}

void addGameLiedown() {
  addGame(liedownTitle, liedownDescription, liedownCharacters,
          liedownCharactersCount, &liedownOptions, false, &liedownUpdate);
}
