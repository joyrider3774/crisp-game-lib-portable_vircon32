#include "../cglp.h"

int* jumponTitle = "JUMP ON";
int* jumponDescription = "[Tap] Jump on";

int[12][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] jumponCharacters = {
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
        "llllll",
        "l ll l",
        "l ll l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
    },
    {
        " llll ",
        "l ll l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        "      ",
        "      ",
        "  ll  ",
        " l ll ",
        " llll ",
        "  ll  ",
    },
    {
        "      ",
        "      ",
        "llllll",
    },
    {
        "      ",
        "      ",
        "ll  ll",
        "  ll  ",
    },
    {
        "      ",
        "      ",
        "l    l",
        " l  l ",
        "  ll  ",
    },
    {
        "      ",
        "  ll  ",
        "ll  ll",
    },
    {
        "  ll  ",
        " l  l ",
        "l    l",
    },
};
int jumponCharactersCount = 12;

Options jumponOptions = {100, 100, 1, false};

int[7] jumponHoleAnimPattern = {1, 2, 1, 0, 3, 4, 3};

#define JUMPON_STATE_WALK 0
#define JUMPON_STATE_JUMP_TO 1
#define JUMPON_STATE_DOWN 2
#define JUMPON_STATE_UP 3
#define JUMPON_STATE_JUMP_FROM 4

struct JumponEnemy {
  Vector pos;
  int holeIndex;
  float nextDotsDist;
  int state;
  bool isAlive;
};
#define JUMPON_MAX_ENEMY_COUNT 32
JumponEnemy[JUMPON_MAX_ENEMY_COUNT] jumponEnemies;
int jumponEnemyIndex;
int jumponGroupEnemyCount;
float jumponNextGroupTicks;
float jumponNextEnemyTicks;

struct JumponHole {
  float x;
  float animTicks;
  bool isAlive;
};
#define JUMPON_MAX_HOLE_COUNT 32
JumponHole[JUMPON_MAX_HOLE_COUNT] jumponHoles;
int jumponHoleIndex;
float jumponNextHoleDist;

struct JumponPlayer {
  Vector pos;
  int holeIndex;
  int state;
};
JumponPlayer jumponPlayer;

struct JumponDot {
  Vector pos;
  bool isAlive;
};
#define JUMPON_MAX_DOT_COUNT 64
JumponDot[JUMPON_MAX_DOT_COUNT] jumponDots;
int jumponDotIndex;

int jumponMultiplier;

void jumponUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(jumponEnemies);
    jumponEnemyIndex = 0;
    jumponGroupEnemyCount = 0;
    jumponNextGroupTicks = 0;
    jumponNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(jumponHoles);
    jumponHoleIndex = 0;
    ASSIGN_ARRAY_ITEM(jumponHoles, jumponHoleIndex, JumponHole, h0);
    h0->x = 30;
    h0->animTicks = 99;
    h0->isAlive = true;
    jumponHoleIndex = cgl_wrap(jumponHoleIndex + 1, 0, JUMPON_MAX_HOLE_COUNT);
    ASSIGN_ARRAY_ITEM(jumponHoles, jumponHoleIndex, JumponHole, h1);
    h1->x = 60;
    h1->animTicks = 99;
    h1->isAlive = true;
    jumponHoleIndex = cgl_wrap(jumponHoleIndex + 1, 0, JUMPON_MAX_HOLE_COUNT);
    jumponNextHoleDist = 0;
    vectorSet(&jumponPlayer.pos, 5, 87);
    jumponPlayer.holeIndex = -1;
    jumponPlayer.state = JUMPON_STATE_WALK;
    INIT_UNALIVED_ARRAY_FAST(jumponDots);
    jumponDotIndex = 0;
    jumponMultiplier = 1;
  }
  color = LIGHT_YELLOW;
  rect(0, 90, 100, 10, &scratch);
  TIMES(5, i) { rect(0, 90 - (i + 1) * 12, 100, 3, &scratch); }
  color = LIGHT_PURPLE;
  rect(0, 10, 100, 10, &scratch);
  float scr = difficulty * 0.03;
  if (jumponPlayer.pos.x > 15) {
    scr += (jumponPlayer.pos.x - 15) * 0.1;
  }
  jumponNextHoleDist -= scr;
  if (jumponNextHoleDist < 0) {
    ASSIGN_ARRAY_ITEM(jumponHoles, jumponHoleIndex, JumponHole, nh);
    nh->x = 104;
    nh->animTicks = 99;
    nh->isAlive = true;
    jumponHoleIndex = cgl_wrap(jumponHoleIndex + 1, 0, JUMPON_MAX_HOLE_COUNT);
    jumponNextHoleDist += rnd(30, 45);
  }
  FOR_EACH(jumponHoles, i) {
    ASSIGN_ARRAY_ITEM(jumponHoles, i, JumponHole, h);
    SKIP_IS_NOT_ALIVE(h);
    h->x -= scr;
    color = WHITE;
    box(h->x, 60, 8, 80, &scratch);
    color = BLACK;
    h->animTicks += difficulty;
    int ai;
    if (h->animTicks < 21) {
      ai = jumponHoleAnimPattern[(int)floor(h->animTicks / 3)];
    } else {
      ai = 0;
    }
    int[2] hc;
    hc[0] = 'h' + ai;
    hc[1] = 0;
    character(hc, h->x, 90, &scratch);
    if (h->x < -4) {
      h->isAlive = false;
      continue;
    }
  }
  jumponNextGroupTicks--;
  if (jumponNextGroupTicks < 0) {
    jumponNextEnemyTicks = 0;
    jumponGroupEnemyCount = rndi(1, 1 + (int)round(sqrt(difficulty) * 2));
    jumponNextGroupTicks = (jumponGroupEnemyCount * 8 + rnd(100, 110)) / difficulty;
  }
  jumponNextEnemyTicks--;
  if (jumponNextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(jumponEnemies, jumponEnemyIndex, JumponEnemy, ne);
    vectorSet(&ne->pos, 103, 87 - rndi(0, 6) * 12);
    ne->holeIndex = -1;
    ne->state = JUMPON_STATE_WALK;
    ne->nextDotsDist = rnd(0, 6);
    ne->isAlive = true;
    jumponEnemyIndex = cgl_wrap(jumponEnemyIndex + 1, 0, JUMPON_MAX_ENEMY_COUNT);
    jumponGroupEnemyCount--;
    if (jumponGroupEnemyCount == 0) {
      jumponNextEnemyTicks = 9999;
    } else {
      jumponNextEnemyTicks = rnd(8, 10) / difficulty;
    }
  }
  color = RED;
  FOR_EACH(jumponEnemies, i) {
    ASSIGN_ARRAY_ITEM(jumponEnemies, i, JumponEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (e->state == JUMPON_STATE_WALK) {
      e->pos.x += -difficulty - scr;
      FOR_EACH(jumponHoles, hi) {
        ASSIGN_ARRAY_ITEM(jumponHoles, hi, JumponHole, h);
        SKIP_IS_NOT_ALIVE(h);
        float o = e->pos.x - h->x;
        if (o < 8 && o > 0) {
          play(LASER);
          e->state = JUMPON_STATE_JUMP_TO;
          e->holeIndex = hi;
        }
      }
      characterOptions.isMirrorX = true;
      character("e", e->pos.x, e->pos.y, &scratch);
      characterOptions.isMirrorX = false;
    } else if (e->state == JUMPON_STATE_JUMP_TO) {
      e->pos.x += -difficulty - scr;
      float o = e->pos.x - jumponHoles[e->holeIndex].x;
      if (o < 4) {
        e->pos.y += 1;
      } else {
        e->pos.y -= 1;
      }
      if (o <= 0) {
        e->state = JUMPON_STATE_DOWN;
      }
      characterOptions.isMirrorX = true;
      character("d", e->pos.x, e->pos.y, &scratch);
      characterOptions.isMirrorX = false;
    } else if (e->state == JUMPON_STATE_DOWN) {
      e->pos.x = jumponHoles[e->holeIndex].x;
      e->pos.y += difficulty;
      if (e->pos.y > 90) {
        play(HIT);
        e->state = JUMPON_STATE_UP;
        jumponHoles[e->holeIndex].animTicks = 0;
      }
      character("f", e->pos.x, e->pos.y, &scratch);
    } else if (e->state == JUMPON_STATE_UP) {
      e->pos.x = jumponHoles[e->holeIndex].x;
      e->pos.y -= difficulty;
      if (e->pos.y < 23) {
        e->state = JUMPON_STATE_DOWN;
      }
      float o = e->pos.y - jumponPlayer.pos.y;
      if (o < 1 && o > -9) {
        e->state = JUMPON_STATE_JUMP_FROM;
        e->pos.y = ceil((e->pos.y - 90) / 12) * 12 + 87;
      }
      character("f", e->pos.x, e->pos.y, &scratch);
    } else if (e->state == JUMPON_STATE_JUMP_FROM) {
      e->pos.x += -difficulty - scr;
      float o = jumponHoles[e->holeIndex].x - e->pos.x;
      if (o < 4) {
        e->pos.y -= 1;
      } else {
        e->pos.y += 1;
      }
      if (o >= 8) {
        play(LASER);
        e->state = JUMPON_STATE_WALK;
        e->pos.y = round((e->pos.y - 90) / 12) * 12 + 87;
        jumponMultiplier = 1;
      }
      characterOptions.isMirrorX = true;
      character("d", e->pos.x, e->pos.y, &scratch);
      characterOptions.isMirrorX = false;
    }
    if (e->state == JUMPON_STATE_WALK) {
      e->nextDotsDist -= difficulty;
      if (e->nextDotsDist < 0) {
        ASSIGN_ARRAY_ITEM(jumponDots, jumponDotIndex, JumponDot, nd);
        nd->pos = e->pos;
        nd->isAlive = true;
        jumponDotIndex = cgl_wrap(jumponDotIndex + 1, 0, JUMPON_MAX_DOT_COUNT);
        e->nextDotsDist += 6;
      }
    }
    if (e->pos.x < -3) {
      e->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  if (jumponPlayer.state == JUMPON_STATE_WALK) {
    jumponPlayer.pos.x += difficulty - scr;
    FOR_EACH(jumponHoles, hi) {
      ASSIGN_ARRAY_ITEM(jumponHoles, hi, JumponHole, h);
      SKIP_IS_NOT_ALIVE(h);
      float o = h->x - jumponPlayer.pos.x;
      if (o < 8 && o > 0) {
        play(JUMP);
        jumponPlayer.state = JUMPON_STATE_JUMP_TO;
        jumponPlayer.holeIndex = hi;
      }
    }
    int[2] pc;
    pc[0] = 'a' + (int)floor(ticks / 10) % 2;
    pc[1] = 0;
    Collision pcColl;
    character(pc, jumponPlayer.pos.x, jumponPlayer.pos.y, &pcColl);
    if (pcColl.isColliding.character['e']) {
      play(EXPLOSION);
      gameOver();
    }
  } else if (jumponPlayer.state == JUMPON_STATE_JUMP_TO) {
    jumponPlayer.pos.x += difficulty - scr;
    float o = jumponHoles[jumponPlayer.holeIndex].x - jumponPlayer.pos.x;
    if (o < 4) {
      jumponPlayer.pos.y += 1;
    } else {
      jumponPlayer.pos.y -= 1;
    }
    if (o <= 0) {
      jumponPlayer.state = JUMPON_STATE_DOWN;
    }
    character("a", jumponPlayer.pos.x, jumponPlayer.pos.y, &scratch);
  } else if (jumponPlayer.state == JUMPON_STATE_DOWN) {
    jumponPlayer.pos.x = jumponHoles[jumponPlayer.holeIndex].x;
    jumponPlayer.pos.y += difficulty;
    if (jumponPlayer.pos.y > 90) {
      play(POWER_UP);
      jumponPlayer.state = JUMPON_STATE_UP;
      jumponHoles[jumponPlayer.holeIndex].animTicks = 0;
    }
    character("c", jumponPlayer.pos.x, jumponPlayer.pos.y, &scratch);
  } else if (jumponPlayer.state == JUMPON_STATE_UP) {
    jumponPlayer.pos.x = jumponHoles[jumponPlayer.holeIndex].x;
    jumponPlayer.pos.y -= difficulty;
    if (jumponPlayer.pos.y < 23) {
      jumponPlayer.state = JUMPON_STATE_DOWN;
    }
    if (input.isPressed) {
      play(JUMP);
      jumponPlayer.state = JUMPON_STATE_JUMP_FROM;
      jumponPlayer.pos.y = ceil((jumponPlayer.pos.y - 90) / 12) * 12 + 87;
    }
    character("c", jumponPlayer.pos.x, jumponPlayer.pos.y, &scratch);
  } else if (jumponPlayer.state == JUMPON_STATE_JUMP_FROM) {
    jumponPlayer.pos.x += difficulty - scr;
    float o = jumponPlayer.pos.x - jumponHoles[jumponPlayer.holeIndex].x;
    if (o < 4) {
      jumponPlayer.pos.y -= 1;
    } else {
      jumponPlayer.pos.y += 1;
    }
    if (o >= 8) {
      jumponPlayer.state = JUMPON_STATE_WALK;
      jumponPlayer.pos.y = round((jumponPlayer.pos.y - 90) / 12) * 12 + 87;
    }
    character("a", jumponPlayer.pos.x, jumponPlayer.pos.y, &scratch);
  }
  if (jumponPlayer.pos.x < 0) {
    play(EXPLOSION);
    gameOver();
  }
  color = YELLOW;
  FOR_EACH(jumponDots, i) {
    ASSIGN_ARRAY_ITEM(jumponDots, i, JumponDot, d);
    SKIP_IS_NOT_ALIVE(d);
    d->pos.x -= scr;
    Collision dc;
    character("g", d->pos.x, d->pos.y, &dc);
    if (dc.isColliding.character['a'] || dc.isColliding.character['b']) {
      play(COIN);
      addScore(jumponMultiplier, d->pos.x, d->pos.y);
      jumponMultiplier++;
      d->isAlive = false;
      continue;
    }
    if (d->pos.x < -3) {
      d->isAlive = false;
      continue;
    }
  }
}

void addGameJumpon() {
  addGame(jumponTitle, jumponDescription, jumponCharacters,
          jumponCharactersCount, &jumponOptions, false, &jumponUpdate);
}
