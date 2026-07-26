#include "../cglp.h"

int* rpsTitle = "RPS";
int* rpsDescription = "[Tap]\n Go right &\n Change rock\n        paper\n        scissors";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rpsCharacters = {
    {
        " lll  ",
        "lllll ",
        "llllll",
        "lllll ",
        " lll  ",
        " lll  ",
    },
    {
        "llll  ",
        "llll  ",
        "llll l",
        "llllll",
        "lllll ",
        " lll  ",
    },
    {
        " l l  ",
        " l l  ",
        "ll l  ",
        "llllll",
        " llll ",
        " lll  ",
    },
};
int rpsCharactersCount = 3;

Options rpsOptions = {100, 100, 60, false};

int[3] rpsHandColors = {CYAN, PURPLE, YELLOW};

struct RpsLane {
  float x;
  int handType;
  float nextTicks;
};
RpsLane[4] rpsLanes;

struct RpsHand {
  int laneIndex;
  float y;
  float my;
  float baseMy;
  int type;
  bool isDestroyed;
  bool isAlive;
};
#define RPS_MAX_HAND_COUNT 64
RpsHand[RPS_MAX_HAND_COUNT] rpsHands;
int rpsHandIndex;

struct RpsMyHand {
  int laneIndex;
  Vector pos;
  float ty;
  float vy;
  int type;
  float freezeTicks;
};
RpsMyHand rpsMyHand;

int rpsMultiplier;

void rpsUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - hand-vs-hand
  // and hand-vs-my-hand overlap are direct same-lane fabs(y-difference)
  // checks (see the "< 6" / "< 5" comparisons below), so the engine's own
  // O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure waste here.
  // Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(rpsHands);
    rpsHandIndex = 0;
    TIMES(4, li) {
      rpsLanes[li].x = li * 20 + 20;
      rpsLanes[li].handType = rndi(0, 3);
      rpsLanes[li].nextTicks = rnd(0, 99);
    }
    rpsMyHand.laneIndex = 0;
    vectorSet(&rpsMyHand.pos, 20, 97);
    rpsMyHand.ty = 97;
    rpsMyHand.vy = -1;
    rpsMyHand.type = 0;
    rpsMyHand.freezeTicks = 0;
    rpsMultiplier = 1;
  }
  color = LIGHT_BLACK;
  TIMES(4, li2) {
    RpsLane* l = &rpsLanes[li2];
    l->nextTicks--;
    if (l->nextTicks < 0) {
      float my = rnd(1, difficulty) * 0.1;
      ASSIGN_ARRAY_ITEM(rpsHands, rpsHandIndex, RpsHand, nh);
      nh->laneIndex = li2;
      nh->y = -9;
      nh->my = my;
      nh->baseMy = my;
      nh->type = l->handType;
      nh->isDestroyed = false;
      nh->isAlive = true;
      rpsHandIndex = cgl_wrap(rpsHandIndex + 1, 0, RPS_MAX_HAND_COUNT);
      l->nextTicks = rnd(200, 300) / sqrt(difficulty);
      if (rnd(0, 1) < 0.4) {
        l->handType = rndi(0, 3);
      }
    }
    rect(l->x - 5, 0, 1, 100, &scratch);
    rect(l->x + 4, 0, 1, 100, &scratch);
  }
  FOR_EACH(rpsHands, hi) {
    ASSIGN_ARRAY_ITEM(rpsHands, hi, RpsHand, h);
    SKIP_IS_NOT_ALIVE(h);
    color = rpsHandColors[h->type];
    if (h->isDestroyed) {
      particle(rpsLanes[h->laneIndex].x, h->y, 16, 1, 0, CGLP_PI * 2);
      h->isAlive = false;
      continue;
    }
    h->y += h->my;
    float factor;
    if (h->y > 90) {
      factor = 2;
    } else {
      factor = 1;
    }
    h->my += (h->baseMy * factor - h->my) * 0.1;
    FOR_EACH(rpsHands, ohi) {
      if (ohi == hi) {
        continue;
      }
      ASSIGN_ARRAY_ITEM(rpsHands, ohi, RpsHand, oh);
      SKIP_IS_NOT_ALIVE(oh);
      if (oh->laneIndex == h->laneIndex && fabs(oh->y - h->y) < 6) {
        float cy = (oh->y + h->y) / 2;
        float t = oh->my;
        oh->my = h->my * 0.4 - oh->my * 0.2;
        h->my = t * 0.4 - h->my * 0.2;
        if (oh->y < h->y) {
          oh->y = cy - 3;
          h->y = cy + 3;
        } else {
          oh->y = cy + 3;
          h->y = cy - 3;
        }
      }
    }
    int[2] hc;
    hc[0] = 'a' + h->type;
    hc[1] = 0;
    characterOptions.rotation = 2;
    character(hc, rpsLanes[h->laneIndex].x, h->y, &scratch);
    characterOptions.rotation = 0;
    if (h->y > 99) {
      play(EXPLOSION);
      gameOver();
    }
  }
  rpsMyHand.freezeTicks--;
  if (rpsMyHand.freezeTicks < 0 && input.isJustPressed) {
    play(SELECT);
    rpsMyHand.laneIndex = (int)cgl_wrap(rpsMyHand.laneIndex + 1, 0, 4);
    rpsMyHand.type = (int)cgl_wrap(rpsMyHand.type + 1, 0, 3);
    rpsMyHand.ty = 97;
    rpsMyHand.vy = -sqrt(difficulty);
  }
  float vyTarget;
  if (rpsMyHand.freezeTicks < 0) {
    vyTarget = -sqrt(difficulty);
  } else {
    vyTarget = 0;
  }
  rpsMyHand.vy += (vyTarget - rpsMyHand.vy) * 0.05;
  rpsMyHand.ty = clamp(rpsMyHand.ty + rpsMyHand.vy, 10, 97);
  float ox = rpsLanes[rpsMyHand.laneIndex].x - rpsMyHand.pos.x;
  rpsMyHand.pos.x += ox * 0.5;
  if (ox < 1) {
    rpsMyHand.pos.x = rpsLanes[rpsMyHand.laneIndex].x;
  }
  rpsMyHand.pos.y += (rpsMyHand.ty - rpsMyHand.pos.y) * 0.5;
  color = rpsHandColors[rpsMyHand.type];
  int[2] mhc;
  mhc[0] = 'a' + rpsMyHand.type;
  mhc[1] = 0;
  character(mhc, rpsMyHand.pos.x, rpsMyHand.pos.y, &scratch);
  FOR_EACH(rpsHands, hi2) {
    ASSIGN_ARRAY_ITEM(rpsHands, hi2, RpsHand, h2);
    SKIP_IS_NOT_ALIVE(h2);
    if (h2->laneIndex == rpsMyHand.laneIndex && fabs(rpsMyHand.ty - h2->y) < 5) {
      int o = (int)cgl_wrap(rpsMyHand.type - h2->type, 0, 3);
      if (o == 0) {
        play(LASER);
        rpsMyHand.vy = 0;
        h2->my -= sqrt(difficulty) * 4;
        h2->y = rpsMyHand.ty - 3;
      } else if (o == 1) {
        play(COIN);
        addScore(rpsMultiplier, rpsMyHand.pos.x, rpsMyHand.pos.y);
        rpsMultiplier++;
        h2->isDestroyed = true;
      } else if (o == 2) {
        play(HIT);
        rpsMultiplier = 1;
        rpsMyHand.vy = sqrt(difficulty) * 5;
        h2->my -= sqrt(difficulty) * 0.5;
        rpsMyHand.ty = h2->y + 3;
        rpsMyHand.freezeTicks = 60 / sqrt(difficulty);
      }
    }
  }
}

void addGameRps() {
  addGame(rpsTitle, rpsDescription, rpsCharacters, rpsCharactersCount, &rpsOptions, false,
          &rpsUpdate);
}
