#include "../cglp.h"

int* up1wayTitle = "UP 1 WAY";
int* up1wayDescription = "[Tap]\n Go up";

int[8][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] up1wayCharacters = {
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
        "      ",
    },
    {
        "  yy  ",
        "  YY  ",
        " yyyy ",
        " YYYY ",
        "yyyyyy",
        "YYYYYY",
    },
    {
        "  rr  ",
        "  rr  ",
        "  rr  ",
        "  rr  ",
        "  rr  ",
        "  rr  ",
    },
    {
        "  rr  ",
        " rRRr ",
        " r  r ",
        " rRRr ",
        " rRRr ",
        "  rr  ",
    },
    {
        "  rr  ",
        " rRRr ",
        "r RR r",
        "rRRRRr",
        " rRRr ",
        "  rr  ",
    },
    {
        "  rr  ",
        " rRRr ",
        " r  r ",
        " rRRr ",
        " rRRr ",
        "  rr  ",
    },
    {
        "yyyy  ",
        "y  y  ",
        "yyyy  ",
        "y Y YY",
        "y Y YY",
        "y Y YY",
    },
};
int up1wayCharactersCount = 8;

Options up1wayOptions = {200, 100, 12, false};

#define UP1WAY_FLOOR_COUNT 6
#define UP1WAY_MAX_HOLE_COUNT 24
#define UP1WAY_MAX_BAMBOO_COUNT 48
#define UP1WAY_MAX_SKULL_COUNT 24
#define UP1WAY_MAX_POW_COUNT 8

int[4] up1waySkullFrameOffsets = {0, 1, 2, 1};

struct Up1wayHole {
  float x;
  bool isAlive;
};
struct Up1wayBamboo {
  float x;
  bool isAlive;
};
struct Up1waySkull {
  float x;
  bool isAlive;
};
struct Up1wayPow {
  float x;
  bool isAlive;
};

struct Up1wayFloor {
  float y;
  Up1wayHole[UP1WAY_MAX_HOLE_COUNT] holes;
  int holeIndex;
  float nextHoleDist;
  Up1wayBamboo[UP1WAY_MAX_BAMBOO_COUNT] bamboos;
  int bambooIndex;
  Up1waySkull[UP1WAY_MAX_SKULL_COUNT] skulls;
  int skullIndex;
  Up1wayPow[UP1WAY_MAX_POW_COUNT] pows;
  int powIndex;
};
Up1wayFloor[UP1WAY_FLOOR_COUNT] up1wayFloors;

float up1wayNextBambooDist;
int up1wayNextBambooFloorIndex;
float up1wayNextSkullDist;
float up1wayNextPowDist;

struct Up1wayPlayer {
  Vector pos;
  int floorIndex;
  int targetFi;
};
Up1wayPlayer up1wayPlayer;

float up1wayAnimTicks;

float up1wayFloorIndexToY(int i) {
  return 16 + i * 15;
}

bool up1wayCheckHole(int fi, float x) {
  Up1wayFloor* f = &up1wayFloors[fi];
  FOR_EACH(f->holes, i) {
    ASSIGN_ARRAY_ITEM(f->holes, i, Up1wayHole, h);
    SKIP_IS_NOT_ALIVE(h);
    if (x > h->x + 3 && x < h->x + 6) {
      return true;
    }
  }
  return false;
}

// Converts skulls on every floor, not just the current one - matches upstream.
void up1wayConvertSkullsToBamboo() {
  TIMES(UP1WAY_FLOOR_COUNT, fi) {
    Up1wayFloor* f = &up1wayFloors[fi];
    FOR_EACH(f->skulls, si) {
      ASSIGN_ARRAY_ITEM(f->skulls, si, Up1waySkull, sk);
      SKIP_IS_NOT_ALIVE(sk);
      ASSIGN_ARRAY_ITEM(f->bamboos, f->bambooIndex, Up1wayBamboo, nb);
      nb->x = sk->x;
      nb->isAlive = true;
      f->bambooIndex = cgl_wrap(f->bambooIndex + 1, 0, UP1WAY_MAX_BAMBOO_COUNT);
      sk->isAlive = false;
    }
  }
}

void up1wayUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(UP1WAY_FLOOR_COUNT, i) {
      Up1wayFloor* f = &up1wayFloors[i];
      f->y = up1wayFloorIndexToY(i);
      INIT_UNALIVED_ARRAY_FAST(f->holes);
      f->holeIndex = 0;
      if (i == 5) {
        f->nextHoleDist = 999999999;
      } else {
        ASSIGN_ARRAY_ITEM(f->holes, 0, Up1wayHole, h0);
        h0->x = rnd(99, 180);
        h0->isAlive = true;
        f->holeIndex = 1;
        f->nextHoleDist = rnd(0, 99);
      }
      INIT_UNALIVED_ARRAY_FAST(f->bamboos);
      f->bambooIndex = 0;
      INIT_UNALIVED_ARRAY_FAST(f->skulls);
      f->skullIndex = 0;
      INIT_UNALIVED_ARRAY_FAST(f->pows);
      f->powIndex = 0;
    }
    up1wayNextBambooDist = 0;
    up1wayNextBambooFloorIndex = rndi(0, UP1WAY_FLOOR_COUNT);
    up1wayNextSkullDist = rnd(49, 99);
    up1wayNextPowDist = 999;
    up1wayPlayer.floorIndex = 5;
    vectorSet(&up1wayPlayer.pos, 20, up1wayFloorIndexToY(5));
    up1wayPlayer.targetFi = -1;
    up1wayAnimTicks = 0;
  }

  up1wayAnimTicks += difficulty;
  if (up1wayPlayer.targetFi != -1) {
    float ty = up1wayFloorIndexToY(up1wayPlayer.targetFi);
    float vy = 1;
    if (ty <= up1wayPlayer.pos.y) {
      vy = -1;
    }
    up1wayPlayer.pos.y += vy * difficulty * 3;
    if ((up1wayPlayer.pos.y - ty) * vy > 0) {
      up1wayPlayer.pos.y = ty;
      up1wayPlayer.floorIndex = up1wayPlayer.targetFi;
      up1wayPlayer.targetFi = -1;
    }
  }
  if (up1wayPlayer.targetFi == -1) {
    if (input.isJustPressed && up1wayPlayer.floorIndex > 0) {
      play(JUMP);
      up1wayPlayer.targetFi = up1wayPlayer.floorIndex - 1;
    } else if (up1wayCheckHole(up1wayPlayer.floorIndex, up1wayPlayer.pos.x)) {
      play(CLICK);
      up1wayPlayer.targetFi = up1wayPlayer.floorIndex + 1;
    }
  }
  color = BLACK;
  int[2] up1wayPlayerChar;
  up1wayPlayerChar[0] = 'a' + ((int)(up1wayAnimTicks / 20) % 2);
  up1wayPlayerChar[1] = 0;
  character(up1wayPlayerChar, up1wayPlayer.pos.x, up1wayPlayer.pos.y - 5, &scratch);

  float scr = difficulty;

  up1wayNextBambooDist -= scr;
  if (up1wayNextBambooDist < 0) {
    Up1wayFloor* nbf = &up1wayFloors[up1wayNextBambooFloorIndex];
    if (nbf->nextHoleDist < 9) {
      up1wayNextBambooFloorIndex = rndi(0, UP1WAY_FLOOR_COUNT);
    } else {
      ASSIGN_ARRAY_ITEM(nbf->bamboos, nbf->bambooIndex, Up1wayBamboo, nb);
      nb->x = 209;
      nb->isAlive = true;
      nbf->bambooIndex = cgl_wrap(nbf->bambooIndex + 1, 0, UP1WAY_MAX_BAMBOO_COUNT);
      if (rnd(0, 1) < 0.3) {
        up1wayNextBambooDist = 6;
      } else {
        up1wayNextBambooDist = rnd(200, 300);
        up1wayNextBambooFloorIndex = rndi(0, UP1WAY_FLOOR_COUNT);
      }
    }
  }

  up1wayNextSkullDist -= scr;
  if (up1wayNextSkullDist < 0) {
    int fi = rndi(0, UP1WAY_FLOOR_COUNT);
    if (up1wayFloors[fi].nextHoleDist > 9 && up1wayNextBambooDist > 9) {
      Up1wayFloor* sf = &up1wayFloors[fi];
      ASSIGN_ARRAY_ITEM(sf->skulls, sf->skullIndex, Up1waySkull, nsk);
      nsk->x = 209;
      nsk->isAlive = true;
      sf->skullIndex = cgl_wrap(sf->skullIndex + 1, 0, UP1WAY_MAX_SKULL_COUNT);
    }
    up1wayNextSkullDist += rnd(30, 50);
  }

  up1wayNextPowDist -= scr;
  if (up1wayNextPowDist < 0) {
    int pfi = rndi(0, UP1WAY_FLOOR_COUNT);
    Up1wayFloor* pf = &up1wayFloors[pfi];
    ASSIGN_ARRAY_ITEM(pf->pows, pf->powIndex, Up1wayPow, np);
    np->x = 209;
    np->isAlive = true;
    pf->powIndex = cgl_wrap(pf->powIndex + 1, 0, UP1WAY_MAX_POW_COUNT);
    up1wayNextPowDist = 999;
  }

  color = LIGHT_BLUE;
  rect(0, 97, 200, 3, &scratch);

  TIMES(UP1WAY_FLOOR_COUNT, fidx) {
    Up1wayFloor* f = &up1wayFloors[fidx];

    f->nextHoleDist -= scr;
    if (f->nextHoleDist < 0) {
      ASSIGN_ARRAY_ITEM(f->holes, f->holeIndex, Up1wayHole, nh);
      nh->x = 200;
      nh->isAlive = true;
      f->holeIndex = cgl_wrap(f->holeIndex + 1, 0, UP1WAY_MAX_HOLE_COUNT);
      f->nextHoleDist = rnd(32, 99);
    }

    // See the file header port note: scan for the smallest remaining alive
    // hole x each step, rather than relying on ring-buffer index order.
    float fx = 0;
    bool[UP1WAY_MAX_HOLE_COUNT] holeUsed;
    memset(holeUsed, 0, sizeof(holeUsed));
    TIMES(UP1WAY_MAX_HOLE_COUNT, step) {
      int bestIdx = -1;
      float bestX = 100000;
      FOR_EACH(f->holes, hi) {
        if (f->holes[hi].isAlive && !holeUsed[hi] && f->holes[hi].x < bestX) {
          bestX = f->holes[hi].x;
          bestIdx = hi;
        }
      }
      if (bestIdx < 0) {
        break;
      }
      holeUsed[bestIdx] = true;
      if (bestX > fx) {
        color = GREEN;
        rect(fx, f->y - 2, bestX - fx, 2, &scratch);
        color = LIGHT_BLACK;
        rect(fx, f->y, bestX - fx, 3, &scratch);
      }
      fx = bestX + 9;
    }
    if (fx < 200) {
      color = GREEN;
      rect(fx, f->y - 2, 200 - fx, 2, &scratch);
      color = LIGHT_BLACK;
      rect(fx, f->y, 200 - fx, 3, &scratch);
    }

    color = BLACK;
    FOR_EACH(f->holes, hi2) {
      ASSIGN_ARRAY_ITEM(f->holes, hi2, Up1wayHole, h);
      SKIP_IS_NOT_ALIVE(h);
      h->x -= scr;
      if (h->x < -9) {
        h->isAlive = false;
        continue;
      }
    }

    FOR_EACH(f->bamboos, bi) {
      ASSIGN_ARRAY_ITEM(f->bamboos, bi, Up1wayBamboo, b);
      SKIP_IS_NOT_ALIVE(b);
      character("c", b->x, f->y - 5, &scratch);
      if (scratch.isColliding.character['a'] || scratch.isColliding.character['b']) {
        play(COIN);
        addScore(1, b->x, f->y - 5);
        b->x = -9;
      }
      b->x -= scr;
      if (b->x < -3) {
        b->isAlive = false;
        continue;
      }
    }

    FOR_EACH(f->pows, powi) {
      ASSIGN_ARRAY_ITEM(f->pows, powi, Up1wayPow, p);
      SKIP_IS_NOT_ALIVE(p);
      character("h", p->x, f->y - 5, &scratch);
      if (scratch.isColliding.character['a'] || scratch.isColliding.character['b']) {
        play(POWER_UP);
        up1wayConvertSkullsToBamboo();
        p->x = -9;
      }
      p->x -= scr;
      if (p->x < -3) {
        p->isAlive = false;
        continue;
      }
    }

    FOR_EACH(f->skulls, si) {
      ASSIGN_ARRAY_ITEM(f->skulls, si, Up1waySkull, sk);
      SKIP_IS_NOT_ALIVE(sk);
      int frame = up1waySkullFrameOffsets[(int)(up1wayAnimTicks / 15) % 4];
      int[2] skullChar;
      skullChar[0] = 'd' + frame;
      skullChar[1] = 0;
      character(skullChar, sk->x, f->y - 5, &scratch);
      if (scratch.isColliding.character['c'] || scratch.isColliding.character['h']) {
        sk->x = -9;
      }
      if (scratch.isColliding.character['a'] || scratch.isColliding.character['b']) {
        play(EXPLOSION);
        gameOver();
      }
      sk->x -= scr;
      if (sk->x < -3) {
        sk->isAlive = false;
        continue;
      }
    }
  }
}

void addGameUp1way() {
  addGame(up1wayTitle, up1wayDescription, up1wayCharacters, up1wayCharactersCount,
          &up1wayOptions, false, &up1wayUpdate);
}
