#include "../cglp.h"

int* lrainTitle = "L RAIN";
int* lrainDescription = "[Slide]\n Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] lrainCharacters = {{
    "  ll  ",
    "  ll  ",
    "b ll b",
    "b ll b",
    "bbllbb",
    "b ll b",
}};
int lrainCharactersCount = 1;

Options lrainOptions = {100, 100, 7, true};

struct LrainLaser {
  Vector pos;
  float vy;
  float width;
  float ticks;
  bool isAlive;
};
// 32->1024: at high difficulty l->ticks (used as sin(l->ticks*0.1) phase)
// steps by a large amount per tick, and once the step nears the ~62.8-tick
// sine period the phase can stall near a positive value for thousands of
// ticks before finally crossing zero - simulation shows laser count spiking
// past 30 (normally ~4-21) after roughly an hour of continuous play.
#define LRAIN_MAX_LASER_COUNT 1024
LrainLaser[LRAIN_MAX_LASER_COUNT] lrainLasers;
int lrainLaserIndex;
float lrainNextLaserTicks;
float lrainLaserDurationAngle;

struct LrainLaserBox {
  Vector pos;
  float vy;
  Vector size;
  int color;
  bool isAlive;
};
// 128->4096: each traveling laser that reaches the top/bottom edge spawns
// floor(w) boxes (w up to width*2, so up to ~18) EVERY tick for its whole
// box-emitting phase; simulation of the real spawn/lifetime formulas shows
// the box count blows past 128 within the first 1-2 seconds of play in
// every trial (peaking 600-1000+ within 25 minutes), so 128 was never enough.
#define LRAIN_MAX_LASERBOX_COUNT 4096
LrainLaserBox[LRAIN_MAX_LASERBOX_COUNT] lrainLaserBoxes;
int lrainLaserBoxIndex;

struct LrainShip {
  Vector pos;
  float px;
  float energy;
  float invincible;
};
LrainShip lrainShip;
float lrainSafeX;
float lrainSafeVx;
int lrainMultiplier;

void lrainUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(lrainLasers);
    lrainLaserIndex = 0;
    lrainNextLaserTicks = 0;
    lrainLaserDurationAngle = 0;
    INIT_UNALIVED_ARRAY_FAST(lrainLaserBoxes);
    lrainLaserBoxIndex = 0;
    vectorSet(&lrainShip.pos, 50, 50);
    lrainShip.px = 50;
    lrainShip.energy = 0;
    lrainShip.invincible = 0;
    lrainSafeX = 50;
    lrainSafeVx = 0;
    lrainMultiplier = 1;
  }
  lrainShip.px = lrainShip.pos.x;
  lrainShip.pos.x = clamp(input.pos.x, 0, 99);
  color = WHITE;
  rect(lrainShip.pos.x + 2, lrainShip.pos.y - 3,
       lrainShip.px + 6 - (lrainShip.pos.x + 2), 6, &scratch);
  rect(lrainShip.pos.x - 2, lrainShip.pos.y - 3,
       lrainShip.px - 6 - (lrainShip.pos.x - 2), 6, &scratch);
  color = BLACK;
  rect(lrainShip.pos.x, lrainShip.pos.y - 2, 1, 4, &scratch);
  COUNT_IS_ALIVE(lrainLasers, aliveLaserCount);
  lrainSafeX += lrainSafeVx / (aliveLaserCount + 1);
  lrainSafeVx += cgl_wrap(lrainSafeVx + rnd(0, 0.01) * RNDPM(), -0.1, 0.1);
  lrainSafeVx *= 0.99;
  if ((lrainSafeX < 9 && lrainSafeVx < 0) || (lrainSafeX > 90 && lrainSafeVx > 0)) {
    lrainSafeVx *= -1;
  }
  lrainLaserDurationAngle += 0.001 * difficulty;
  lrainNextLaserTicks--;
  if (lrainNextLaserTicks < 0) {
    play(LASER);
    float x = rnd(0, 99);
    float vy = sqrt(difficulty) * 3 * (rndi(0, 2) * 2 - 1);
    float width = rnd(3, 9);
    if (fabs(x - lrainSafeX) > width + 2) {
      ASSIGN_ARRAY_ITEM(lrainLasers, lrainLaserIndex, LrainLaser, nl);
      float ly;
      if (vy > 0) {
        ly = 0;
      } else {
        ly = 99;
      }
      vectorSet(&nl->pos, x, ly);
      nl->vy = vy;
      nl->width = width;
      nl->ticks = 0;
      nl->isAlive = true;
      lrainLaserIndex = cgl_wrap(lrainLaserIndex + 1, 0, LRAIN_MAX_LASER_COUNT);
    }
    float tr = fabs(cos(lrainLaserDurationAngle));
    if (tr < 0.15) {
      tr = 5;
    }
    lrainNextLaserTicks = rnd(0, 30 / tr) / difficulty;
  }
  color = LIGHT_RED;
  FOR_EACH(lrainLasers, i) {
    ASSIGN_ARRAY_ITEM(lrainLasers, i, LrainLaser, l);
    SKIP_IS_NOT_ALIVE(l);
    if (l->pos.y < 0 || l->pos.y > 99) {
      l->ticks += difficulty;
      float w = sin(l->ticks * 0.1) * l->width * 2;
      if (w <= 0) {
        l->isAlive = false;
        continue;
      }
      w = clamp(w, 0, l->width - 1);
      float sy = fabs(l->vy) * rnd(3, 5);
      int boxCount = (int)floor(w);
      TIMES(boxCount, k) {
        ASSIGN_ARRAY_ITEM(lrainLaserBoxes, lrainLaserBoxIndex, LrainLaserBox, nb);
        float ySign;
        if (l->vy > 0) {
          ySign = -1;
        } else {
          ySign = 1;
        }
        float by = l->pos.y + (50 - l->pos.y) * 2 + (rnd(0, 9) + sy / 2) * ySign;
        vectorSet(&nb->pos, l->pos.x + rnd(0, w) * RNDPM(), by);
        nb->vy = l->vy * 3;
        vectorSet(&nb->size, 3, sy);
        if (rnd(0, 1) < 0.5) {
          nb->color = RED;
        } else {
          nb->color = PURPLE;
        }
        nb->isAlive = true;
        lrainLaserBoxIndex = cgl_wrap(lrainLaserBoxIndex + 1, 0, LRAIN_MAX_LASERBOX_COUNT);
      }
    } else {
      l->pos.y += l->vy;
      float y;
      if (l->vy > 0) {
        y = 0;
      } else {
        y = 99;
      }
      rect(l->pos.x - l->width, y, l->width * 2, l->pos.y - y, &scratch);
      if (l->pos.y < 0 || l->pos.y > 99) {
        play(COIN);
      }
    }
  }
  bool isHit = false;
  FOR_EACH(lrainLaserBoxes, i) {
    ASSIGN_ARRAY_ITEM(lrainLaserBoxes, i, LrainLaserBox, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y += b->vy;
    color = b->color;
    Collision bc;
    box(b->pos.x, b->pos.y, b->size.x, b->size.y, &bc);
    if (bc.isColliding.rect[WHITE]) {
      play(HIT);
      addScore(lrainMultiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      color = BLACK;
      particle(lrainShip.pos.x, lrainShip.pos.y, 1, 1, 0, CGLP_PI * 2);
      lrainShip.energy += 0.1;
      if (lrainShip.energy > 15) {
        play(EXPLOSION);
        play(POWER_UP);
        lrainMultiplier++;
        particle(lrainShip.pos.x, lrainShip.pos.y, 30, 3, 0, CGLP_PI * 2);
        lrainShip.energy = 0;
        lrainShip.invincible = 30;
      }
    }
    if (bc.isColliding.rect[BLACK]) {
      isHit = true;
    }
    b->isAlive = !(b->pos.y < -b->size.y / 2 || b->pos.y > 99 + b->size.y / 2);
  }
  if (lrainShip.invincible < 3 && isHit) {
    play(RANDOM);
    gameOver();
  }
  color = LIGHT_CYAN;
  thickness = 2;
  arc(lrainShip.pos.x, lrainShip.pos.y, 15, 0, CGLP_PI * 2, &scratch);
  color = CYAN;
  thickness = 2;
  arc(lrainShip.pos.x, lrainShip.pos.y, lrainShip.energy, 0, CGLP_PI * 2, &scratch);
  color = BLACK;
  if (lrainShip.invincible >= 3) {
    lrainShip.invincible *= 1 - 0.02 * sqrt(difficulty);
    thickness = 3;
    arc(lrainShip.pos.x, lrainShip.pos.y, lrainShip.invincible, 0, CGLP_PI * 2, &scratch);
    if (ticks % 4 < 2) {
      color = CYAN;
    }
  }
  character("a", lrainShip.pos.x, lrainShip.pos.y, &scratch);
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(lrainMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameLrain() {
  addGame(lrainTitle, lrainDescription, lrainCharacters, lrainCharactersCount,
          &lrainOptions, true, &lrainUpdate);
}
