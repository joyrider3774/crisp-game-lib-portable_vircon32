#include "../cglp.h"

int* shinyTitle = "SHINY";
int* shinyDescription = "[Hold]\n Rainy";

int[6][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] shinyCharacters = {
    {
        "  ll  ",
        "  l   ",
        "lllll ",
        "  l   ",
        " l lll",
        "l     ",
    },
    {
        "   ll ",
        "   l  ",
        "lllll ",
        "  l   ",
        "ll ll ",
        "     l",
    },
    {
        "  lll ",
        " l l l",
        "  lll ",
        "   l  ",
        "llll  ",
        "    l ",
    },
    {
        " lll  ",
        "l l l ",
        " lll  ",
        "  l   ",
        " l lll",
        "l     ",
    },
    {
        "l l l ",
        "l l l ",
        " lll  ",
        "  l   ",
        " l l  ",
        "l   l ",
    },
    {
        "  ll  ",
        " llll ",
        "llllll",
        "llllll",
        " llll ",
        "  ll  ",
    },
};
int shinyCharactersCount = 6;

Options shinyOptions = {200, 100, 7, false};

#define SHINY_GROUND_COUNT 22
#define SHINY_RIGHT_EDGE_GROUND_INDEX 20
#define SHINY_HUMAN_Y 89

struct ShinyGround {
  Vector pos;
  float size;
};
ShinyGround[SHINY_GROUND_COUNT] shinyGrounds;

struct ShinyCloud {
  Vector pos;
  Vector rainyPos;
  Vector shinyPos;
  float radius;
  float weakDropTicks;
  float strongDropTicks;
};
ShinyCloud[30] shinyClouds;

#define SHINY_MAX_HUMAN_COUNT 16
struct ShinyHuman {
  Vector pos;
  float speed;
  float ticks;
  bool isRunning;
  bool isFalling;
  bool isAlive;
};
ShinyHuman[SHINY_MAX_HUMAN_COUNT] shinyHumans;
int shinyHumansCount;

#define SHINY_DROP_TYPE_WEAK 0
#define SHINY_DROP_TYPE_STRONG 1
struct ShinyDrop {
  Vector pos;
  Vector vel;
  int type;
  bool isAlive;
};
#define SHINY_MAX_DROP_COUNT 128
ShinyDrop[SHINY_MAX_DROP_COUNT] shinyDrops;
int shinyDropIndex;

float shinyRainyRatio;
float shinyWindY;

void shinyAddDrop(float x, float y, int type) {
  ASSIGN_ARRAY_ITEM(shinyDrops, shinyDropIndex, ShinyDrop, nd);
  vectorSet(&nd->pos, x, y);
  vectorSet(&nd->vel, (rnd(0, 0.2) * RNDPM() + shinyWindY) * difficulty, difficulty);
  nd->type = type;
  nd->isAlive = true;
  shinyDropIndex = cgl_wrap(shinyDropIndex + 1, 0, SHINY_MAX_DROP_COUNT);
}

void shinyUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(shinyHumans);
    INIT_UNALIVED_ARRAY_FAST(shinyDrops);
    shinyDropIndex = 0;
    TIMES(SHINY_GROUND_COUNT, gi) {
      vectorSet(&shinyGrounds[gi].pos, (gi - 1) * 10 + 5, 95);
      shinyGrounds[gi].size = 8;
    }
    TIMES(30, ci) {
      ShinyCloud* c = &shinyClouds[ci];
      vectorSet(&c->rainyPos, ((ci + 1) / 31.0) * 199 + rnd(0, 3) * RNDPM(), rnd(5, 15));
      float shinyX;
      if (c->rainyPos.x < 99) {
        shinyX = -20;
      } else {
        shinyX = 220;
      }
      vectorSet(&c->shinyPos, shinyX, c->rainyPos.y);
      c->pos = c->shinyPos;
      c->radius = rnd(5, 10);
      c->weakDropTicks = rnd(0, 99);
      c->strongDropTicks = rnd(100, 900);
    }
    shinyRainyRatio = 0;
    shinyHumansCount = 0;
    shinyWindY = 0;
  }
  COUNT_IS_ALIVE(shinyHumans, shinyEmptyCheckCount);
  if (shinyEmptyCheckCount == 0) {
    if (shinyHumansCount < 9) {
      shinyHumansCount++;
    }
    INIT_UNALIVED_ARRAY_FAST(shinyHumans);
    TIMES(shinyHumansCount, hi) {
      ShinyHuman* h = &shinyHumans[hi];
      vectorSet(&h->pos, -4, SHINY_HUMAN_Y);
      h->speed = rnd(0.1, 0.15) * difficulty;
      h->ticks = rndi(0, 999);
      h->isRunning = false;
      h->isFalling = false;
      h->isAlive = true;
    }
    shinyGrounds[SHINY_RIGHT_EDGE_GROUND_INDEX].size = 8;
  }
  bool isRainy = input.isPressed;
  if (input.isJustPressed) {
    play(HIT);
    shinyWindY = rnd(0, 0.5) * RNDPM() * sqrt(difficulty);
    shinyGrounds[SHINY_RIGHT_EDGE_GROUND_INDEX].size -= 0.1;
  }
  if (input.isJustReleased) {
    play(HIT);
  }
  if (!isRainy && shinyRainyRatio > 0) {
    shinyRainyRatio -= 1;
  } else if (isRainy && shinyRainyRatio < 10) {
    shinyRainyRatio += 1;
  }
  float rr = shinyRainyRatio / 10;
  color = YELLOW;
  Vector p1;
  vectorSet(&p1, 100, 10);
  character("f", p1.x, p1.y, &scratch);
  if (rr < 1) {
    Vector p2;
    TIMES(7, i) {
      float a = ticks * 0.05 + (i * CGLP_PI * 2) / 7.0;
      float l = fabs(sin(i + ticks * 0.05) * 5 * (1 - rr)) + 10;
      vectorSet(&p1, 100, 10);
      addWithAngle(&p1, a, 7);
      vectorSet(&p2, 100, 10);
      addWithAngle(&p2, a, l);
      thickness = 3;
      line(p1.x, p1.y, p2.x, p2.y, &scratch);
    }
  }
  color = LIGHT_BLACK;
  TIMES(30, ci2) {
    ShinyCloud* c = &shinyClouds[ci2];
    vectorSet(&c->pos, c->rainyPos.x * rr + c->shinyPos.x * (1 - rr),
              c->rainyPos.y * rr + c->shinyPos.y * (1 - rr));
    if (c->pos.y > -9 && c->pos.y < 209) {
      box(c->pos.x, c->pos.y, c->radius * 2, c->radius * 2, &scratch);
    }
    c->weakDropTicks--;
    if (c->weakDropTicks < 0) {
      if (isRainy) {
        shinyAddDrop(c->pos.x, c->pos.y + c->radius, SHINY_DROP_TYPE_WEAK);
      }
      c->weakDropTicks = rnd(100, 200);
    }
    c->strongDropTicks--;
    if (c->strongDropTicks < 0) {
      if (isRainy) {
        shinyAddDrop(c->pos.x, c->pos.y + c->radius, SHINY_DROP_TYPE_STRONG);
      }
      c->strongDropTicks = rnd(500, 999) / difficulty;
    }
  }
  TIMES(SHINY_GROUND_COUNT, gi2) {
    ShinyGround* g = &shinyGrounds[gi2];
    if (gi2 == SHINY_RIGHT_EDGE_GROUND_INDEX) {
      color = BLACK;
      if (g->size >= 1) {
        box(g->pos.x, g->pos.y - (4 - g->size / 2), 8, g->size, &scratch);
        g->size -= 0.005 * difficulty;
      }
    } else {
      if (g->size < 8) {
        color = LIGHT_BLACK;
        box(g->pos.x, g->pos.y, g->size, g->size, &scratch);
        g->size += 0.12 * difficulty;
      } else {
        color = BLACK;
        box(g->pos.x, g->pos.y, 8, 8, &scratch);
      }
    }
  }
  COUNT_IS_ALIVE(shinyHumans, shinyHumansAliveThisFrame);
  color = RED;
  FOR_EACH(shinyHumans, hi2) {
    ASSIGN_ARRAY_ITEM(shinyHumans, hi2, ShinyHuman, h);
    SKIP_IS_NOT_ALIVE(h);
    int ci3;
    if (h->isRunning) {
      ci3 = 2 + (int)floor(h->ticks / 15) % 2;
    } else {
      ci3 = 0 + (int)floor(h->ticks / 30) % 2;
    }
    if (!h->isFalling) {
      float mul;
      if (h->isRunning) {
        mul = 5;
      } else {
        mul = 1;
      }
      h->pos.x += h->speed * mul;
    } else {
      ci3 = 4;
    }
    int[2] hc;
    hc[0] = 'a' + ci3;
    hc[1] = 0;
    Collision hcoll;
    character(hc, h->pos.x, h->pos.y, &hcoll);
    if (!hcoll.isColliding.rect[BLACK]) {
      h->isFalling = true;
      h->pos.y += difficulty;
      if (h->pos.y > 103) {
        shinyHumansCount--;
        if (shinyHumansCount <= 0) {
          play(RANDOM);  // Equivalent to "lucky" in JS
          gameOver();
        } else {
          play(EXPLOSION);
        }
        h->isAlive = false;
        continue;
      }
    } else {
      h->pos.y = SHINY_HUMAN_Y;
      h->isFalling = false;
    }
    h->ticks++;
    int ti = (int)floor(30 / difficulty);
    if (ti > 0 && (int)h->ticks % ti == 0) {
      h->isRunning = isRainy;
    }
    if (h->pos.x > 203) {
      play(POWER_UP);
      int s = (int)floor(shinyGrounds[20].size * shinyHumansAliveThisFrame);
      if (s > 0) {
        addScore(s, 190, 80);
      }
      h->isAlive = false;
      continue;
    }
  }
  FOR_EACH(shinyDrops, di) {
    ASSIGN_ARRAY_ITEM(shinyDrops, di, ShinyDrop, d);
    SKIP_IS_NOT_ALIVE(d);
    vectorAdd(&d->pos, d->vel.x, d->vel.y);
    if (d->type == SHINY_DROP_TYPE_STRONG) {
      color = BLUE;
      Vector dEnd;
      dEnd = d->vel;
      vectorMul(&dEnd, 4.0 / vectorLength(&dEnd));
      vectorAdd(&dEnd, d->pos.x, d->pos.y);
      thickness = 3;
      Collision dc;
      line(d->pos.x, d->pos.y, dEnd.x, dEnd.y, &dc);
      if (dc.isColliding.character['a'] || dc.isColliding.character['b'] ||
          dc.isColliding.character['c'] || dc.isColliding.character['d']) {
        d->isAlive = false;
        continue;
      }
    } else {
      color = LIGHT_BLUE;
      Vector dEnd2;
      dEnd2 = d->vel;
      vectorMul(&dEnd2, 3.0 / vectorLength(&dEnd2));
      vectorAdd(&dEnd2, d->pos.x, d->pos.y);
      thickness = 2;
      line(d->pos.x, d->pos.y, dEnd2.x, dEnd2.y, &scratch);
    }
    if (d->pos.y > 90) {
      if (d->type == SHINY_DROP_TYPE_STRONG) {
        int gi3 = (int)floor(d->pos.x / 10) + 1;
        if (gi3 >= 2 && gi3 < 20) {
          play(SELECT);
          shinyGrounds[gi3].size = 0;
        }
      }
      d->isAlive = false;
      continue;
    }
  }
}

void addGameShiny() {
  addGame(shinyTitle, shinyDescription, shinyCharacters, shinyCharactersCount, &shinyOptions,
          false, &shinyUpdate);
}
