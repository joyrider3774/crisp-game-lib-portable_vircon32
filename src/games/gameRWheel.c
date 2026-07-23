#include "../cglp.h"

char* rwheelTitle = "R WHEEL";
char* rwheelDescription = "[Tap]\n Multiple jumps";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rwheelCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "      ",
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
    },
    {
        " llll ",
        "  ll  ",
        "      ",
        " llll ",
        "      ",
        "llllll",
    }};
int rwheelCharactersCount = 3;

Options rwheelOptions = {100, 100, 4, false};

struct RwheelSpike {
  int height;
  bool isHit;
};
#define RWHEEL_SPIKE_COUNT 32
RwheelSpike[RWHEEL_SPIKE_COUNT] rwheelSpikes;
float rwheelAngleOfs;
struct RwheelBonus {
  float angle;
  float radius;
  bool isAlive;
};
#define RWHEEL_MAX_BONUS_COUNT 8
RwheelBonus[RWHEEL_MAX_BONUS_COUNT] rwheelBonuses;
int rwheelBonusIndex;
struct RwheelPlayer {
  float y;
  float vy;
};
RwheelPlayer rwheelPlayer;
struct RwheelBar {
  Vector pos;
  float width;
  bool isSpike;
  bool isAlive;
};
#define RWHEEL_MAX_BAR_COUNT 8
RwheelBar[RWHEEL_MAX_BAR_COUNT] rwheelBars;
int rwheelBarIndex;
int rwheelMultiplier;
int rwheelValidSpikeCount;
float rwheelWheelRadius = 40;

void rwheelAddBonus() {
  TIMES((int)ceil((float)rwheelValidSpikeCount / 9), i) {
    ASSIGN_ARRAY_ITEM(rwheelBonuses, rwheelBonusIndex, RwheelBonus, b);
    b->angle = -M_PI / 3 * 2 + rnd(0, M_PI / 4 * 3) * RNDPM();
    b->radius = rnd(10, 30);
    b->isAlive = true;
    rwheelBonusIndex = cgl_wrap(rwheelBonusIndex + 1, 0, RWHEEL_MAX_BONUS_COUNT);
  }
}

void rwheelUpdate() {
  Collision scratch;
  if (!ticks) {
    FOR_EACH(rwheelSpikes, i) {
      ASSIGN_ARRAY_ITEM(rwheelSpikes, i, RwheelSpike, s);
      s->height = 0;
      s->isHit = false;
    }
    rwheelAngleOfs = 0;
    INIT_UNALIVED_ARRAY_FAST(rwheelBonuses);
    rwheelBonusIndex = 0;
    rwheelPlayer.y = 0;
    rwheelPlayer.vy = 0;
    INIT_UNALIVED_ARRAY_FAST(rwheelBars);
    rwheelBarIndex = 0;
    rwheelMultiplier = 1;
    rwheelValidSpikeCount = 0;
  }
  float sd = sqrt(difficulty);
  if (input.isJustPressed) {
    play(JUMP);
    rwheelPlayer.vy = -2 * sd;
    FOR_EACH(rwheelSpikes, i) {
      ASSIGN_ARRAY_ITEM(rwheelSpikes, i, RwheelSpike, s);
      s->isHit = false;
    }
    if (rwheelPlayer.y == 0) {
      rwheelPlayer.y += rwheelPlayer.vy;
      rwheelMultiplier = 0;
      rwheelAddBonus();
    }
  }
  if (rwheelPlayer.y < 0) {
    float pvy = rwheelPlayer.vy;
    float vyStep;
    if (input.isPressed) {
      vyStep = 1;
    } else {
      vyStep = 3;
    }
    rwheelPlayer.vy += vyStep * 0.03 * difficulty;
    rwheelPlayer.vy *= 0.98;
    if (rwheelPlayer.y < -rwheelWheelRadius * 2 + 6 && rwheelPlayer.vy < 0) {
      rwheelPlayer.vy *= -0.5;
    }
    if (pvy * rwheelPlayer.vy <= 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(rwheelBars, rwheelBarIndex, RwheelBar, b);
      vectorSet(&b->pos, 50, 50 + rwheelWheelRadius + rwheelPlayer.y);
      b->width = 0;
      b->isSpike = true;
      b->isAlive = true;
      rwheelBarIndex = cgl_wrap(rwheelBarIndex + 1, 0, RWHEEL_MAX_BAR_COUNT);
    }
    rwheelPlayer.y += rwheelPlayer.vy;
    if (rwheelPlayer.y > 0) {
      rwheelPlayer.y = rwheelPlayer.vy = 0;
    }
  }
  color = BLACK;
  int[2] pc;
  if (rwheelPlayer.y > 0) {
    pc[0] = 'a' + 0;
  } else {
    pc[0] = 'a' + (int)(ticks / 10) % 2;
  }
  pc[1] = 0;
  character(pc, 50, 50 + rwheelWheelRadius + rwheelPlayer.y - 3, &scratch);
  float va = 0.03 * sd;
  color = YELLOW;
  FOR_EACH(rwheelBonuses, i) {
    ASSIGN_ARRAY_ITEM(rwheelBonuses, i, RwheelBonus, b);
    SKIP_IS_NOT_ALIVE(b);
    Vector p;
    addWithAngle(vectorSet(&p, 50, 50), b->angle, b->radius);
    b->angle += va;
    Collision cl;
    character("c", p.x, p.y, &cl);
    bool* c = cl.isColliding.character;
    if (c['a'] || c['b']) {
      play(COIN);
      ASSIGN_ARRAY_ITEM(rwheelBars, rwheelBarIndex, RwheelBar, ba);
      vectorSet(&ba->pos, 50, 50 + rwheelWheelRadius + rwheelPlayer.y);
      ba->width = 0;
      ba->isSpike = false;
      ba->isAlive = true;
      rwheelBarIndex = cgl_wrap(rwheelBarIndex + 1, 0, RWHEEL_MAX_BAR_COUNT);
      b->isAlive = false;
    }
  }
  FOR_EACH(rwheelBars, i) {
    ASSIGN_ARRAY_ITEM(rwheelBars, i, RwheelBar, b);
    SKIP_IS_NOT_ALIVE(b);
    if (b->isSpike) {
      b->width += sd;
      b->pos.y += sd * 3;
      color = PURPLE;
    } else {
      b->width += sd * 2;
      b->pos.y += sd * 2;
      color = YELLOW;
    }
    box(b->pos.x, b->pos.y, b->width, 3, &scratch);
    b->isAlive = b->pos.y < 103;
  }
  rwheelAngleOfs += va;
  color = BLACK;
  arc(50, 50, rwheelWheelRadius + 3, rwheelAngleOfs, rwheelAngleOfs + M_PI * 2, &scratch);
  float a = rwheelAngleOfs;
  rwheelValidSpikeCount = 0;
  FOR_EACH(rwheelSpikes, i) {
    ASSIGN_ARRAY_ITEM(rwheelSpikes, i, RwheelSpike, s);
    if (s->height > 0) {
      color = RED;
    } else {
      color = TRANSPARENT;
    }
    Vector p;
    addWithAngle(vectorSet(&p, 50, 50), a, rwheelWheelRadius * (1 - s->height * 0.1));
    Vector bp;
    addWithAngle(vectorSet(&bp, 50, 50), a, rwheelWheelRadius);
    Vector bp2;
    addWithAngle(vectorSet(&bp2, bp.x, bp.y), a - M_PI / 2,
                 50 / RWHEEL_SPIKE_COUNT);
    Collision lc1;
    line(p.x, p.y, bp2.x, bp2.y, &lc1);
    Collisions c1 = lc1.isColliding;
    addWithAngle(vectorSet(&bp2, bp.x, bp.y), a + M_PI / 2,
                 50 / RWHEEL_SPIKE_COUNT);
    Collision lc2;
    line(p.x, p.y, bp2.x, bp2.y, &lc2);
    Collisions c2 = lc2.isColliding;
    if (!s->isHit && (c1.rect[PURPLE] || c2.rect[PURPLE])) {
      play(HIT);
      s->height++;
      s->isHit = true;
    }
    if (s->height > 0) {
      if (c1.rect[YELLOW] || c2.rect[YELLOW]) {
        play(SELECT);
        rwheelMultiplier += s->height;
        addScore(rwheelMultiplier, p.x, p.y);
        s->height = 0;
      } else if (c1.character['a'] || c2.character['a'] || c1.character['b'] ||
                 c2.character['b']) {
        play(EXPLOSION);
        gameOver();
      }
    }
    a += M_PI * 2 / RWHEEL_SPIKE_COUNT;
    if (s->height > 0) {
      rwheelValidSpikeCount++;
    }
  }
  if (rwheelValidSpikeCount == 0) {
    rwheelSpikes[(int)cgl_wrap(
               (int)(-rwheelAngleOfs - M_PI / 4) / (M_PI * 2 / RWHEEL_SPIKE_COUNT), 0,
               RWHEEL_SPIKE_COUNT)]
        .height = 1;
    rwheelSpikes[(int)cgl_wrap((int)(-rwheelAngleOfs - M_PI / 4 * 3) /
                         (M_PI * 2 / RWHEEL_SPIKE_COUNT),
                     0, RWHEEL_SPIKE_COUNT)]
        .height = 1;
    rwheelAddBonus();
  }
}

void addGameRWheel() {
  addGame(rwheelTitle, rwheelDescription, rwheelCharacters,
          rwheelCharactersCount, &rwheelOptions, false, &rwheelUpdate);
}
