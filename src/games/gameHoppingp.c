#include "../cglp.h"

int* hoppingpTitle = "HOPPING P";
int* hoppingpDescription = "[Tap]\n Turn";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] hoppingpCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int hoppingpCharactersCount = 0;

Options hoppingpOptions = {200, 100, 1, false};

#define HOPPINGP_TYPE_PLAYER 0
#define HOPPINGP_TYPE_ENEMY 1
#define HOPPINGP_TYPE_POWER 2

#define HOPPINGP_BASE_HOP_X 0.5
#define HOPPINGP_BASE_HOP_Y -1.2
#define HOPPINGP_BASE_GRV 0.02

struct HoppingpHopping {
  Vector pos;
  Vector vel;
  float vx;
  Vector hop;
  float grv;
  float hopTicks;
  int type;
  bool isAlive;
};
// Enemies never expire on their own (only removed by being eaten while
// powered up); spawn interval shrinks as sqrt(difficulty) grows while count
// only self-throttles by sqrt(N+1), so N grows roughly ~9*(minutes+1)
// unbounded over a long session - 32 overflows within ~3 minutes of normal
// play, so raise the cap well past that.
#define HOPPINGP_MAX_HOPPING_COUNT 512
HoppingpHopping[HOPPINGP_MAX_HOPPING_COUNT] hoppingpHoppings;
int hoppingpHoppingIndex;

float hoppingpEnemyAppTicks;
float hoppingpPowerAppTicks;
float hoppingpPowerTicks;
int hoppingpMultiplier;
bool hoppingpIsFirstPower;

float[4][3] hoppingpEnemyPattern = {
    {1, 1, 0.65},
    {1, 0.8, 2},
    {1.6, 0.6, 2},
    {1, 2, 1.8},
};

// The JS version draws the "P" power glyph and the empowered player glyph at
// 2x scale via a text `scale` option this port doesn't support; both are
// drawn at normal size here instead (visual size only, not a gameplay change).
void hoppingpDrawHopping(HoppingpHopping* h, float r, Collision* result) {
  bool useBlue = hoppingpPowerTicks > 0 && h->type == HOPPINGP_TYPE_ENEMY &&
                 !(hoppingpPowerTicks < 60 && (int)hoppingpPowerTicks % 15 > 7);
  if (useBlue) {
    color = BLUE;
  } else if (h->type == HOPPINGP_TYPE_PLAYER) {
    color = GREEN;
  } else if (h->type == HOPPINGP_TYPE_ENEMY) {
    color = RED;
  } else {
    color = YELLOW;
  }
  int[2] txt;
  if (h->type == HOPPINGP_TYPE_PLAYER) {
    txt[0] = 'o';
  } else if (h->type == HOPPINGP_TYPE_ENEMY) {
    txt[0] = 'x';
  } else {
    txt[0] = 'P';
  }
  txt[1] = 0;
  if ((h->type == HOPPINGP_TYPE_PLAYER && hoppingpPowerTicks > 0) ||
      h->type == HOPPINGP_TYPE_POWER) {
    text(txt, h->pos.x, h->pos.y - 10 * r - 2, result);
  } else {
    text(txt, h->pos.x, h->pos.y - 10 * r, result);
  }
}

bool hoppingpCheckCollision(HoppingpHopping* h, Collision* c, int aliveHoppingCount) {
  if (h->type == HOPPINGP_TYPE_ENEMY) {
    if (c->isColliding.text['o']) {
      if (hoppingpPowerTicks > 0) {
        play(COIN);
        color = BLUE;
        particle(h->pos.x, h->pos.y, 19, 2, 0, CGLP_PI * 2);
        addScore(hoppingpMultiplier, h->pos.x, h->pos.y);
        if (hoppingpMultiplier < 64) {
          hoppingpMultiplier *= 2;
        }
        return false;
      }
      play(EXPLOSION);
      gameOver();
    }
  } else if (h->type == HOPPINGP_TYPE_POWER) {
    if (c->isColliding.text['o']) {
      play(POWER_UP);
      color = YELLOW;
      particle(h->pos.x, h->pos.y, 29, 3, 0, CGLP_PI * 2);
      hoppingpPowerTicks =
          (float)floor((300 / sqrt(difficulty)) * sqrt(sqrt(aliveHoppingCount)));
      FOR_EACH(hoppingpHoppings, k) {
        ASSIGN_ARRAY_ITEM(hoppingpHoppings, k, HoppingpHopping, hk);
        SKIP_IS_NOT_ALIVE(hk);
        if (hk->type != HOPPINGP_TYPE_PLAYER) {
          hk->vel.x *= -1;
          hk->vx *= -1;
        }
      }
      return false;
    }
  }
  return true;
}

void hoppingpUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(hoppingpHoppings);
    hoppingpHoppingIndex = 0;
    ASSIGN_ARRAY_ITEM(hoppingpHoppings, hoppingpHoppingIndex, HoppingpHopping, p0);
    vectorSet(&p0->pos, 99, 60);
    vectorSet(&p0->vel, HOPPINGP_BASE_HOP_X, 0);
    p0->vx = 1;
    vectorSet(&p0->hop, HOPPINGP_BASE_HOP_X, HOPPINGP_BASE_HOP_Y);
    p0->grv = HOPPINGP_BASE_GRV;
    p0->hopTicks = 0;
    p0->type = HOPPINGP_TYPE_PLAYER;
    p0->isAlive = true;
    hoppingpHoppingIndex = cgl_wrap(hoppingpHoppingIndex + 1, 0, HOPPINGP_MAX_HOPPING_COUNT);
    hoppingpEnemyAppTicks = 0;
    hoppingpPowerAppTicks = 200;
    hoppingpPowerTicks = 0;
    hoppingpIsFirstPower = true;
  }
  color = BLUE;
  rect(0, 90, 199, 9, &scratch);
  bool isWeakApp = hoppingpPowerTicks > 60;
  if (isWeakApp) {
    hoppingpEnemyAppTicks -= 3;
  } else {
    hoppingpEnemyAppTicks -= 1;
  }
  COUNT_IS_ALIVE(hoppingpHoppings, hopCountA);
  if (hoppingpEnemyAppTicks < 0) {
    float x;
    if (rnd(0, 1) < 0.5) {
      x = -5;
    } else {
      x = 205;
    }
    int patternIdx = rndi(0, 4);
    float t0 = hoppingpEnemyPattern[patternIdx][0];
    float t1 = hoppingpEnemyPattern[patternIdx][1];
    float t2 = hoppingpEnemyPattern[patternIdx][2];
    ASSIGN_ARRAY_ITEM(hoppingpHoppings, hoppingpHoppingIndex, HoppingpHopping, ne);
    vectorSet(&ne->pos, x, 89);
    vectorSet(&ne->vel, 0, 0);
    if (x < 99) {
      ne->vx = 1;
    } else {
      ne->vx = -1;
    }
    float hopXMul;
    if (isWeakApp) {
      hopXMul = 0.5;
    } else {
      hopXMul = 1;
    }
    vectorSet(&ne->hop, HOPPINGP_BASE_HOP_X * t0 * hopXMul, HOPPINGP_BASE_HOP_Y * t1);
    ne->grv = HOPPINGP_BASE_GRV * t2;
    ne->hopTicks = 0;
    ne->type = HOPPINGP_TYPE_ENEMY;
    ne->isAlive = true;
    hoppingpHoppingIndex = cgl_wrap(hoppingpHoppingIndex + 1, 0, HOPPINGP_MAX_HOPPING_COUNT);
    hoppingpEnemyAppTicks = (rnd(100, 150) / sqrt(difficulty)) * sqrt(hopCountA + 1);
  }
  COUNT_IS_ALIVE(hoppingpHoppings, hopCountB);
  if (hopCountB == 1) {
    hoppingpEnemyAppTicks = 0;
  }
  hoppingpPowerAppTicks--;
  if (hoppingpPowerAppTicks < 0) {
    float x;
    if (rnd(0, 1) < 0.5) {
      x = -5;
    } else {
      x = 205;
    }
    float t0, t1, t2;
    if (hoppingpIsFirstPower) {
      t0 = 0.88;
      t1 = 1.25;
      t2 = 1.25;
    } else {
      int patternIdx = rndi(0, 4);
      t0 = hoppingpEnemyPattern[patternIdx][0];
      t1 = hoppingpEnemyPattern[patternIdx][1];
      t2 = hoppingpEnemyPattern[patternIdx][2];
    }
    hoppingpIsFirstPower = false;
    ASSIGN_ARRAY_ITEM(hoppingpHoppings, hoppingpHoppingIndex, HoppingpHopping, np);
    vectorSet(&np->pos, x, 89);
    vectorSet(&np->vel, 0, 0);
    if (x < 99) {
      np->vx = 1;
    } else {
      np->vx = -1;
    }
    vectorSet(&np->hop, HOPPINGP_BASE_HOP_X * t0 * 0.8, HOPPINGP_BASE_HOP_Y * t1 * 0.8);
    np->grv = HOPPINGP_BASE_GRV * t2 * 0.8;
    np->hopTicks = 0;
    np->type = HOPPINGP_TYPE_POWER;
    np->isAlive = true;
    hoppingpHoppingIndex = cgl_wrap(hoppingpHoppingIndex + 1, 0, HOPPINGP_MAX_HOPPING_COUNT);
    hoppingpPowerAppTicks = rnd(700, 999) / sqrt(difficulty);
  }
  hoppingpPowerTicks--;
  if (hoppingpPowerTicks > 60) {
    if ((int)hoppingpPowerTicks % 30 == 0) {
      play(SELECT);
    }
  } else if (hoppingpPowerTicks > 0) {
    if ((int)hoppingpPowerTicks % 10 == 0) {
      play(SELECT);
    }
  }
  if (hoppingpPowerTicks <= 0) {
    hoppingpMultiplier = 1;
  }
  COUNT_IS_ALIVE(hoppingpHoppings, aliveHoppingCount);
  FOR_EACH(hoppingpHoppings, i) {
    ASSIGN_ARRAY_ITEM(hoppingpHoppings, i, HoppingpHopping, h);
    SKIP_IS_NOT_ALIVE(h);
    if (h->type == HOPPINGP_TYPE_PLAYER && input.isJustPressed) {
      play(HIT);
      h->vx *= -1;
      h->vel.x *= -1;
    }
    bool handledMidHop = false;
    if (h->hopTicks > 0) {
      h->hopTicks -= difficulty;
      if (h->hopTicks <= 0) {
        color = BLACK;
        if (h->type == HOPPINGP_TYPE_PLAYER) {
          play(JUMP);
          particle(h->pos.x, h->pos.y, 9, -h->hop.y, -CGLP_PI / 2, CGLP_PI / 2);
        } else {
          play(LASER);
          particle(h->pos.x, h->pos.y, 4, -h->hop.y, -CGLP_PI / 2, CGLP_PI / 2);
        }
        vectorSet(&h->vel, h->hop.x * h->vx * difficulty, h->hop.y * difficulty);
      } else {
        float r;
        if (h->hopTicks < 5) {
          r = 1 - h->hopTicks / 5;
        } else {
          r = (h->hopTicks - 5) / 5;
        }
        color = BLACK;
        box(h->pos.x, h->pos.y - 2 * r, 2, 4 * r, &scratch);
        box(h->pos.x, h->pos.y - 5 * r, 6, 2 * r, &scratch);
        Collision c;
        hoppingpDrawHopping(h, r, &c);
        if (!hoppingpCheckCollision(h, &c, aliveHoppingCount)) {
          h->isAlive = false;
        }
        handledMidHop = true;
      }
    }
    if (handledMidHop) {
      continue;
    }
    vectorAdd(&h->pos, h->vel.x, h->vel.y);
    h->vel.y += h->grv * difficulty * difficulty;
    if (h->type == HOPPINGP_TYPE_PLAYER && hoppingpPowerTicks > 0) {
      vectorAdd(&h->pos, h->vel.x, h->vel.y);
      h->vel.y += h->grv * difficulty * difficulty;
    }
    if (h->pos.y > 90) {
      h->pos.y = 90;
      h->hopTicks = 9;
    }
    if ((h->pos.x < 0 && h->vx < 0) || (h->pos.x > 199 && h->vx > 0)) {
      h->vx *= -1;
      h->vel.x *= -1;
    }
    color = BLACK;
    box(h->pos.x, h->pos.y - 2, 2, 4, &scratch);
    box(h->pos.x, h->pos.y - 5, 6, 2, &scratch);
    Collision c2;
    hoppingpDrawHopping(h, 1, &c2);
    if (!hoppingpCheckCollision(h, &c2, aliveHoppingCount)) {
      h->isAlive = false;
      continue;
    }
  }
}

void addGameHoppingp() {
  addGame(hoppingpTitle, hoppingpDescription, hoppingpCharacters,
          hoppingpCharactersCount, &hoppingpOptions, false, &hoppingpUpdate);
}
