#include "../cglp.h"

int* rringTitle = "R RING";
int* rringDescription = "[Slide]\n Move";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rringCharacters = {
    {
        " c c  ",
        "blblb ",
        "lblbl ",
        " lbl  ",
        " l l  ",
        " l l  ",
    },
    {
        " bb   ",
        "bccb  ",
        "cllc  ",
        "cllc  ",
        "bccb  ",
        " bb   ",
    },
    {
        " lrl  ",
        "r   r ",
        " lrl  ",
        "L L L ",
        " LLL  ",
    },
};
int rringCharactersCount = 3;

Options rringOptions = {100, 100, 7, true};

int[3] rringStarColors = {LIGHT_CYAN, LIGHT_PURPLE, LIGHT_BLACK};

struct RringShip {
  Vector pos;
  Vector targetPos;
  float angle;
  float stopTicks;
  float moveTicks;
  float fireTicks;
};
RringShip rringShip;

// Ship trail used to place "option" follower glyphs a fixed number of frames
// behind the ship - a ring buffer of the last 99 positions reproduces the
// JS array's unshift()/pop() (front=newest, capped length) exactly.
Vector[99] rringShipHistory;
int rringHistoryCount;
int rringHistoryHead;

int rringOptionCount;
int rringMaxOptionCount;

struct RringRing {
  Vector pos;
  float vy;
  float radius;
  bool isHit;
  bool isAlive;
};
// Fire interval is 60/difficulty (scales with difficulty directly) while each ring's lifetime only
// shrinks as 1/sqrt(difficulty) (vy=sqrt(difficulty)), and up to 5 rings fire per trigger once all 4
// options are active - concurrent count grows unboundedly (~7*sqrt(difficulty)) over a long session.
#define RRING_MAX_RING_COUNT 512
RringRing[RRING_MAX_RING_COUNT] rringRings;
int rringRingIndex;

struct RringStar {
  Vector pos;
  float vy;
  int color;
};
RringStar[20] rringStars;

int rringMultiplier;
float rringNextPowerUpTicks;

void rringFire(float x, float y) {
  ASSIGN_ARRAY_ITEM(rringRings, rringRingIndex, RringRing, nr);
  vectorSet(&nr->pos, x, y);
  nr->vy = sqrt(difficulty);
  nr->radius = 1;
  nr->isHit = false;
  nr->isAlive = true;
  rringRingIndex = cgl_wrap(rringRingIndex + 1, 0, RRING_MAX_RING_COUNT);
}

void rringUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&rringShip.pos, 20, 20);
    rringShip.angle = 0;
    vectorSet(&rringShip.targetPos, 20, 20);
    rringShip.stopTicks = 0;
    rringShip.moveTicks = 0;
    rringShip.fireTicks = 0;
    rringHistoryCount = 0;
    rringHistoryHead = 0;
    rringOptionCount = 0;
    INIT_UNALIVED_ARRAY_FAST(rringRings);
    rringRingIndex = 0;
    TIMES(20, si) {
      vectorSet(&rringStars[si].pos, rnd(0, 99), rnd(0, 99));
      rringStars[si].vy = rnd(0.5, 1);
      rringStars[si].color = rringStarColors[rndi(0, 3)];
    }
    rringMultiplier = 1;
    rringNextPowerUpTicks = 60;
    rringMaxOptionCount = 1;
  }
  float sd = sqrt(difficulty);
  TIMES(20, si2) {
    RringStar* s = &rringStars[si2];
    s->pos.y += s->vy;
    if (s->pos.y > 99) {
      s->pos.x = rnd(0, 99);
      s->pos.y = 0;
    }
    color = s->color;
    rect(s->pos.x, s->pos.y, 1, 1, &scratch);
  }
  rringShip.stopTicks--;
  if (rringShip.stopTicks < 0) {
    rringShip.moveTicks = rnd(120, 180) / sd;
    rringShip.stopTicks = rringShip.moveTicks + rnd(40, 60) / sd;
  }
  rringShip.moveTicks--;
  if (rringShip.moveTicks > 0) {
    if (distanceTo(&rringShip.pos, rringShip.targetPos.x, rringShip.targetPos.y) < 5) {
      vectorSet(&rringShip.targetPos, rnd(10, 90), rnd(3, 30));
    }
    float ta = angleTo(&rringShip.pos, rringShip.targetPos.x, rringShip.targetPos.y);
    float oa = cgl_wrap(ta - rringShip.angle, -CGLP_PI, CGLP_PI);
    float va = 0.1 * difficulty;
    if (fabs(oa) < va) {
      rringShip.angle = ta;
    } else {
      if (oa > 0) {
        rringShip.angle += va;
      } else {
        rringShip.angle -= va;
      }
    }
    addWithAngle(&rringShip.pos, rringShip.angle, 0.5 * difficulty);
    rringShip.pos.x = clamp(rringShip.pos.x, 0, 99);
    rringShip.pos.y = clamp(rringShip.pos.y, 0, 50);
    rringHistoryHead = (rringHistoryHead - 1 + 99) % 99;
    rringShipHistory[rringHistoryHead] = rringShip.pos;
    if (rringHistoryCount < 99) {
      rringHistoryCount++;
    }
  }
  color = BLACK;
  character("a", rringShip.pos.x, rringShip.pos.y, &scratch);
  rringShip.fireTicks--;
  if (rringShip.fireTicks < 0) {
    play(HIT);
    rringFire(rringShip.pos.x, rringShip.pos.y);
  }
  rringNextPowerUpTicks--;
  if (rringNextPowerUpTicks < 0) {
    play(POWER_UP);
    rringOptionCount++;
    if (rringOptionCount > rringMaxOptionCount) {
      rringOptionCount = 0;
      rringMaxOptionCount = clamp(rringMaxOptionCount + 1, 1, 4);
    }
    if (rringOptionCount == 0) {
      rringNextPowerUpTicks = 60 / sd;
    } else {
      rringNextPowerUpTicks = (300.0 / rringOptionCount) / sd;
    }
  }
  TIMES(rringOptionCount, oi) {
    int histIdx = (oi + 1) * 24;
    if (histIdx < rringHistoryCount) {
      Vector hp = rringShipHistory[(rringHistoryHead + histIdx) % 99];
      // Vircon32 port note: JS's per-call {scale:{x:s,y:s}} pulsing option
      // glyph size has no engine equivalent here - drawn at normal size.
      character("b", hp.x, hp.y, &scratch);
      if (rringShip.fireTicks < 0) {
        rringFire(hp.x, hp.y);
      }
    }
  }
  if (rringShip.fireTicks < 0) {
    rringShip.fireTicks = 60 / difficulty;
  }
  color = PURPLE;
  thickness = 3;
  FOR_EACH(rringRings, ri1) {
    ASSIGN_ARRAY_ITEM(rringRings, ri1, RringRing, r);
    SKIP_IS_NOT_ALIVE(r);
    r->pos.y += r->vy;
    r->radius += r->vy * 0.5;
    arc(r->pos.x, r->pos.y - r->radius * 0.3, r->radius * 0.6, CGLP_PI / 4, (CGLP_PI / 4) * 3,
        &scratch);
  }
  color = RED;
  thickness = 3;
  FOR_EACH(rringRings, ri2) {
    ASSIGN_ARRAY_ITEM(rringRings, ri2, RringRing, r2);
    SKIP_IS_NOT_ALIVE(r2);
    arc(r2->pos.x - r2->radius * 0.32, r2->pos.y, r2->radius * 0.2, (CGLP_PI / 6) * 5,
        (CGLP_PI / 6) * 7, &scratch);
    arc(r2->pos.x + r2->radius * 0.32, r2->pos.y, r2->radius * 0.2, -(CGLP_PI / 6), CGLP_PI / 6,
        &scratch);
  }
  color = BLACK;
  Collision cColl;
  character("c", clamp(input.pos.x, 0, 99), 95, &cColl);
  if (cColl.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(rringRings, ri3) {
    ASSIGN_ARRAY_ITEM(rringRings, ri3, RringRing, r3);
    SKIP_IS_NOT_ALIVE(r3);
    if (r3->isHit) {
      color = LIGHT_PURPLE;
    } else {
      color = PURPLE;
    }
    thickness = 3;
    Collision arcColl;
    arc(r3->pos.x, r3->pos.y + r3->radius * 0.3, r3->radius * 0.6, -CGLP_PI / 4,
        -(CGLP_PI / 4) * 3, &arcColl);
    if (arcColl.isColliding.character['c'] && !r3->isHit) {
      play(COIN);
      addScore(rringMultiplier, r3->pos.x, r3->pos.y);
      rringMultiplier++;
      r3->isHit = true;
    }
    if (r3->pos.y > 110) {
      if (!r3->isHit && rringMultiplier > 1) {
        rringMultiplier--;
      }
      r3->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "+");
  strcat(multText, intToChar(rringMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameRring() {
  addGame(rringTitle, rringDescription, rringCharacters, rringCharactersCount, &rringOptions,
          true, &rringUpdate);
}
