#include "../cglp.h"

int* kiteTitle = "KITE";
int* kiteDescription = "[Hold]\n Blow wind";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] kiteCharacters = {
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
        "     l",
        "lllll ",
        "      ",
        "llll  ",
        "      ",
        "llllll",
    },
    {
        " yyyy ",
        "yY YYy",
        "yY YYy",
        "yY YYy",
        "yY YYy",
        " yyyy ",
    },
};
int kiteCharactersCount = 4;

Options kiteOptions = {200, 100, 5, false};

#define KITE_STRING_DIST 50

struct KiteEntity {
  Vector pos;
  Vector vel;
};
KiteEntity kiteKite;

struct KitePlayer {
  Vector pos;
  Vector vel;
  float ticks;
};
KitePlayer kitePlayer;

struct KiteWind {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define KITE_MAX_WIND_COUNT 32
KiteWind[KITE_MAX_WIND_COUNT] kiteWinds;
int kiteWindIndex;
float kiteNextWindTicks;

struct KiteCoin {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define KITE_MAX_COIN_COUNT 32
KiteCoin[KITE_MAX_COIN_COUNT] kiteCoins;
int kiteCoinIndex;
float kiteNextCoinDist;

float kiteGroundX;

struct KiteSpike {
  Vector pos;
  float height;
  bool isAlive;
};
#define KITE_MAX_SPIKE_COUNT 64
KiteSpike[KITE_MAX_SPIKE_COUNT] kiteSpikes;
int kiteSpikeIndex;

struct KiteSpikeAngle {
  float yAngle;
  float hAngle;
};
#define KITE_SPIKE_ANGLE_COUNT 2
KiteSpikeAngle[KITE_SPIKE_ANGLE_COUNT] kiteSpikeAngles;
float kiteNextSpikeDist;

void kiteUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&kiteKite.pos, 40, 50);
    vectorSet(&kiteKite.vel, 0, 0);
    vectorSet(&kitePlayer.pos, 20, 87);
    vectorSet(&kitePlayer.vel, 0, 0);
    kitePlayer.ticks = 0;
    kiteGroundX = 0;
    INIT_UNALIVED_ARRAY_FAST(kiteWinds);
    kiteWindIndex = 0;
    kiteNextWindTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(kiteCoins);
    kiteCoinIndex = 0;
    kiteNextCoinDist = 0;
    INIT_UNALIVED_ARRAY_FAST(kiteSpikes);
    kiteSpikeIndex = 0;
    TIMES(2, i) {
      kiteSpikeAngles[i].yAngle = i * CGLP_PI / 2;
      kiteSpikeAngles[i].hAngle = i * CGLP_PI;
    }
    kiteNextSpikeDist = 0;
  }
  float sd = sqrt(difficulty);
  float scr;
  if (kiteKite.pos.x > 60) {
    scr = (kiteKite.pos.x - 60) * 0.1;
  } else {
    scr = 0;
  }
  kiteNextSpikeDist -= scr;
  if (kiteNextSpikeDist < 0) {
    addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    TIMES(2, i) {
      KiteSpikeAngle* a = &kiteSpikeAngles[i];
      float height = sin(a->yAngle) * 9 + sin(a->hAngle) * 20 - 10;
      a->yAngle += rnd(0, sd) * 0.2;
      a->hAngle += rnd(0, sd) * 0.4;
      if (height > 0) {
        ASSIGN_ARRAY_ITEM(kiteSpikes, kiteSpikeIndex, KiteSpike, ns);
        float spikeYSign;
        if (i == 0) {
          spikeYSign = 1;
        } else {
          spikeYSign = -1;
        }
        float py;
        if (i == 0) {
          py = 0;
        } else {
          py = 90;
        }
        vectorSet(&ns->pos, 205, py);
        ns->height = height * spikeYSign;
        ns->isAlive = true;
        kiteSpikeIndex = cgl_wrap(kiteSpikeIndex + 1, 0, KITE_MAX_SPIKE_COUNT);
      }
    }
    kiteNextSpikeDist += 10;
  }
  color = RED;
  FOR_EACH(kiteSpikes, i) {
    ASSIGN_ARRAY_ITEM(kiteSpikes, i, KiteSpike, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x -= scr;
    line(s->pos.x - 5, s->pos.y, s->pos.x, s->pos.y + s->height, &scratch);
    line(s->pos.x + 5, s->pos.y, s->pos.x, s->pos.y + s->height, &scratch);
    if (s->pos.x < -5) {
      s->isAlive = false;
      continue;
    }
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    vectorAdd(&kiteKite.vel, difficulty * 0.2, -difficulty * 0.2);
  }
  kiteKite.vel.y += difficulty * 0.01;
  vectorMul(&kiteKite.vel, 0.95);
  vectorAdd(&kiteKite.pos, kiteKite.vel.x, kiteKite.vel.y);
  kiteKite.pos.x -= scr;
  if (kiteKite.pos.y < 0) {
    kiteKite.pos.y = 0;
    kiteKite.vel.y = 0;
  } else if (kiteKite.pos.y > 87) {
    kiteKite.pos.y = 87;
    kiteKite.vel.y = 0;
  }
  color = BLUE;
  Collision kc;
  box(kiteKite.pos.x, kiteKite.pos.y, 6, 6, &kc);
  if (kc.isColliding.rect[RED]) {
    float s;
    if (kiteKite.pos.y < 50) {
      s = 1;
    } else {
      s = -1;
    }
    play(HIT);
    color = TRANSPARENT;
    int c = 0;
    Collision kc2;
    box(kiteKite.pos.x, kiteKite.pos.y, 6, 6, &kc2);
    while (kc2.isColliding.rect[RED] && c < 9) {
      kiteKite.pos.y += 3 * s;
      c++;
      box(kiteKite.pos.x, kiteKite.pos.y, 6, 6, &kc2);
    }
    kiteKite.vel.y = sqrt(c) * s * 2 * sd;
  }
  color = LIGHT_BLACK;
  Vector p;
  vectorSet(&p, kiteKite.pos.x - 3, kiteKite.pos.y + 3);
  thickness = 2;
  line(p.x, p.y, p.x - kiteKite.vel.x * 3, p.y + kiteKite.vel.y + 9, &scratch);
  vectorSet(&p, kiteKite.pos.x + 3, kiteKite.pos.y + 3);
  thickness = 2;
  line(p.x, p.y, p.x - kiteKite.vel.x * 3, p.y + kiteKite.vel.y + 9, &scratch);
  thickness = 2;
  line(kiteKite.pos.x, kiteKite.pos.y, kitePlayer.pos.x, kitePlayer.pos.y, &scratch);
  vectorAdd(&kitePlayer.pos, kitePlayer.vel.x, kitePlayer.vel.y);
  kitePlayer.pos.x -= scr;
  if (kitePlayer.pos.y < 87) {
    kitePlayer.vel.y += sd * sqrt(99 - kitePlayer.pos.y) * 0.01;
    kitePlayer.vel.x *= 0.95;
    kitePlayer.ticks = 0;
  } else {
    kitePlayer.vel.y = 0;
    kitePlayer.pos.y = 87;
    kitePlayer.vel.x *= 0.9;
    kitePlayer.ticks++;
  }
  color = BLACK;
  int[2] pc;
  pc[0] = 'a' + (int)floor(kitePlayer.ticks / 15) % 2;
  pc[1] = 0;
  Collision pcColl;
  character(pc, kitePlayer.pos.x, kitePlayer.pos.y, &pcColl);
  if (pcColl.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  float d = distanceTo(&kitePlayer.pos, kiteKite.pos.x, kiteKite.pos.y);
  if (d > KITE_STRING_DIST) {
    float a = angleTo(&kitePlayer.pos, kiteKite.pos.x, kiteKite.pos.y);
    addWithAngle(&kitePlayer.vel, a, (d - KITE_STRING_DIST) * 0.05);
    addWithAngle(&kiteKite.vel, a + CGLP_PI, (d - KITE_STRING_DIST) * 0.01);
    addWithAngle(&kiteKite.pos, a + CGLP_PI, d - KITE_STRING_DIST);
  }
  if (input.isPressed) {
    kiteNextWindTicks -= 3;
  } else {
    kiteNextWindTicks -= 1;
  }
  while (kiteNextWindTicks < 0) {
    ASSIGN_ARRAY_ITEM(kiteWinds, kiteWindIndex, KiteWind, nw);
    vectorSet(&nw->pos, -3, rnd(0, 87));
    float windSpeedMul;
    if (input.isPressed) {
      windSpeedMul = 2;
    } else {
      windSpeedMul = 1;
    }
    vectorSet(&nw->vel, rnd(1, 2) * sd * windSpeedMul, 0);
    nw->isAlive = true;
    kiteWindIndex = cgl_wrap(kiteWindIndex + 1, 0, KITE_MAX_WIND_COUNT);
    kiteNextWindTicks += 30 / sd;
  }
  color = LIGHT_CYAN;
  FOR_EACH(kiteWinds, i) {
    ASSIGN_ARRAY_ITEM(kiteWinds, i, KiteWind, w);
    SKIP_IS_NOT_ALIVE(w);
    vectorAdd(&w->pos, w->vel.x, w->vel.y);
    Collision wc;
    character("c", w->pos.x, w->pos.y, &wc);
    if (wc.isColliding.rect[BLUE]) {
      play(HIT);
      vectorAdd(&kiteKite.vel, w->vel.x, w->vel.y);
      w->isAlive = false;
      continue;
    }
    if (w->pos.x > 203) {
      w->isAlive = false;
      continue;
    }
  }
  kiteNextCoinDist -= scr;
  if (kiteNextCoinDist < 0) {
    ASSIGN_ARRAY_ITEM(kiteCoins, kiteCoinIndex, KiteCoin, nc);
    vectorSet(&nc->pos, 203, rnd(30, 80));
    vectorSet(&nc->vel, rnd(1, sd) * -0.5, 0);
    nc->isAlive = true;
    kiteCoinIndex = cgl_wrap(kiteCoinIndex + 1, 0, KITE_MAX_COIN_COUNT);
    kiteNextCoinDist += rnd(199, 299);
  }
  color = BLACK;
  FOR_EACH(kiteCoins, i) {
    ASSIGN_ARRAY_ITEM(kiteCoins, i, KiteCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    vectorAdd(&c->pos, c->vel.x, c->vel.y);
    c->pos.x -= scr;
    Collision cl;
    character("d", c->pos.x, c->pos.y, &cl);
    if (cl.isColliding.rect[RED]) {
      c->isAlive = false;
      continue;
    }
    if (cl.isColliding.character['a'] || cl.isColliding.character['b']) {
      play(COIN);
      addScore(clamp(ceil(kitePlayer.vel.x * 5), 1, 99) * 50, c->pos.x, c->pos.y);
      c->isAlive = false;
      continue;
    }
    if (cl.isColliding.character['c']) {
      c->pos.x++;
    }
    if (distanceTo(&c->pos, kitePlayer.pos.x, kitePlayer.pos.y) < 24) {
      addWithAngle(&c->vel, angleTo(&c->pos, kitePlayer.pos.x, kitePlayer.pos.y), sd);
      vectorMul(&c->vel, 0.9);
    }
    bool inRect = c->pos.x >= -3 && c->pos.x < 207 && c->pos.y >= -3 && c->pos.y < 90;
    if (!inRect) {
      c->isAlive = false;
      continue;
    }
  }
  kiteGroundX = cgl_wrap(kiteGroundX - scr, -9, 209);
  color = LIGHT_BLACK;
  rect(0, 90, 200, 10, &scratch);
  color = WHITE;
  rect(kiteGroundX, 90, 2, 10, &scratch);
}

void addGameKite() {
  addGame(kiteTitle, kiteDescription, kiteCharacters, kiteCharactersCount,
          &kiteOptions, false, &kiteUpdate);
}
