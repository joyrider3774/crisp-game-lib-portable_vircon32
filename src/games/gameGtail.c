#include "../cglp.h"

int* gtailTitle = "G TAIL";
int* gtailDescription = "[Slide]\n Move";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] gtailCharacters = {
    {
        " llll ",
        "llccll",
        "llccll",
        "llllll",
        "bbbbbb",
        "bb  bb",
    },
    {
        " rrrr ",
        "rRRrRr",
        "rrRRRr",
        "rRRrrr",
        "rrRrRr",
        " rrrr ",
    },
};
int gtailCharactersCount = 2;

Options gtailOptions = {100, 100, 9, true};

struct GtailStar {
  Vector pos;
  float vy;
  int color;
};
#define GTAIL_MAX_STAR_COUNT 30
GtailStar[GTAIL_MAX_STAR_COUNT] gtailStars;
int[3] gtailStarColors = {LIGHT_CYAN, LIGHT_RED, LIGHT_YELLOW};

// The original JS keeps up to 200 positions of history per meteor "just in
// case", but golds only ever reference an index up to
// (max gold count - 1 + 2) * 5 = 40 (see gtailAddMeteor's golds init below),
// so a much smaller cap already behaves identically while using far less
// RAM for MAX_METEOR_COUNT meteors at once.
#define GTAIL_MAX_POS_HISTORY 50
#define GTAIL_MAX_GOLD_COUNT 8
struct GtailMeteor {
  Vector pos;
  Vector vel;
  float accel;
  Vector[GTAIL_MAX_POS_HISTORY] posHistory;
  int posHistoryCount;
  int[GTAIL_MAX_GOLD_COUNT] golds;
  int goldCount;
  bool isAlive;
};
#define GTAIL_MAX_METEOR_COUNT 32
GtailMeteor[GTAIL_MAX_METEOR_COUNT] gtailMeteors;
int gtailMeteorIndex;
int gtailNextMeteorTicks;
int gtailMultiplier;
Vector gtailShipPos;

void gtailAddMeteor() {
  ASSIGN_ARRAY_ITEM(gtailMeteors, gtailMeteorIndex, GtailMeteor, m);
  vectorSet(&m->pos, rnd(10, 90), -5);
  vectorSet(&m->vel, rnd(0, 0.5) * RNDPM(), 0);
  m->accel = rnd(1, sqrt(difficulty));
  m->posHistoryCount = 0;
  m->goldCount = rndi(3, 8);
  TIMES(m->goldCount, i) { m->golds[i] = (i + 2) * 5; }
  m->isAlive = true;
  gtailMeteorIndex = cgl_wrap(gtailMeteorIndex + 1, 0, GTAIL_MAX_METEOR_COUNT);
}

void gtailShiftGoldsDown(GtailMeteor* m, int removedIndex) {
  memcpy(&m->golds[removedIndex], &m->golds[removedIndex + 1],
         (m->goldCount - 1 - removedIndex) * sizeof(m->golds[0]));
  m->goldCount--;
}

void gtailUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(gtailMeteors);
    gtailMeteorIndex = 0;
    gtailNextMeteorTicks = 0;
    gtailMultiplier = 1;
    vectorSet(&gtailShipPos, 50, 50);
    TIMES(GTAIL_MAX_STAR_COUNT, i) {
      ASSIGN_ARRAY_ITEM(gtailStars, i, GtailStar, s);
      vectorSet(&s->pos, rnd(0, 99), rnd(0, 99));
      s->vy = rnd(1, 2);
      s->color = gtailStarColors[rndi(0, 3)];
    }
  }
  FOR_EACH(gtailStars, i) {
    ASSIGN_ARRAY_ITEM(gtailStars, i, GtailStar, s);
    s->pos.y += s->vy;
    color = s->color;
    rect(s->pos.x, s->pos.y, 1, 1, &scratch);
    if (s->pos.y > 110) {
      vectorSet(&s->pos, rnd(0, 99), -rnd(0, 9));
    }
  }
  if (gtailShipPos.y > 99) {
    gtailShipPos.y = 99;
    gameOver();
  }
  gtailShipPos.x = clamp(input.pos.x, 0, 99);
  gtailShipPos.y += (50 - gtailShipPos.y) * 0.001;
  color = BLACK;
  float oy = clamp(gtailShipPos.y - 55, 0, 99);
  character("a", gtailShipPos.x + rnd(0, oy * 0.01) * RNDPM(),
            gtailShipPos.y + rnd(0, oy * 0.01) * RNDPM(), &scratch);
  color = RED;
  particle(gtailShipPos.x, gtailShipPos.y + 2, 1, 2, CGLP_PI / 2, CGLP_PI / 8);
  gtailNextMeteorTicks--;
  if (gtailNextMeteorTicks < 0) {
    gtailAddMeteor();
    COUNT_IS_ALIVE(gtailMeteors, aliveMeteorCount);
    gtailNextMeteorTicks =
        (rnd(99, 120) / sqrt(difficulty)) * sqrt(aliveMeteorCount);
  }
  FOR_EACH(gtailMeteors, i) {
    ASSIGN_ARRAY_ITEM(gtailMeteors, i, GtailMeteor, m);
    SKIP_IS_NOT_ALIVE(m);

    Vector delta;
    vectorSet(&delta, gtailShipPos.x, gtailShipPos.y);
    vectorAdd(&delta, -m->pos.x, -m->pos.y);
    vectorMul(&delta, 1.0 / 999);
    vectorMul(&delta, m->accel);
    vectorAdd(&m->vel, delta.x, delta.y);

    m->vel.x = clamp(m->vel.x, -2, 2);
    m->vel.y = clamp(m->vel.y, -2, 2);
    vectorAdd(&m->pos, m->vel.x, m->vel.y);

    int shiftCount = m->posHistoryCount;
    if (shiftCount > GTAIL_MAX_POS_HISTORY - 1) {
      shiftCount = GTAIL_MAX_POS_HISTORY - 1;
    }
    for (int k = shiftCount - 1; k >= 0; k--) {
      m->posHistory[k + 1] = m->posHistory[k];
    }
    m->posHistory[0] = m->pos;
    if (m->posHistoryCount < GTAIL_MAX_POS_HISTORY) {
      m->posHistoryCount++;
    }

    color = YELLOW;
    int gi = 0;
    while (gi < m->goldCount) {
      bool goldRemoved = false;
      if (m->posHistoryCount > m->golds[gi]) {
        Vector gp = m->posHistory[m->golds[gi]];
        Collision gc;
        text("$", gp.x, gp.y, &gc);
        if (gc.isColliding.character['a']) {
          play(COIN);
          particle(gp.x, gp.y, 16, 1, 0, CGLP_PI * 2);
          addScore(gtailMultiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
          goldRemoved = true;
        }
      }
      if (goldRemoved) {
        gtailShiftGoldsDown(m, gi);
      } else {
        gi++;
      }
    }

    if (m->goldCount == 0) {
      play(POWER_UP);
      m->pos.x = clamp(m->pos.x, 5, 95);
      m->pos.y = clamp(m->pos.y, 5, 95);
      addScore(gtailMultiplier, m->pos.x, m->pos.y);
      color = RED;
      particle(m->pos.x, m->pos.y, 20, 2, 0, CGLP_PI * 2);
      gtailMultiplier++;
      m->isAlive = false;
      continue;
    }

    color = BLACK;
    Collision bc;
    character("b", m->pos.x, m->pos.y, &bc);
    if (bc.isColliding.character['a']) {
      play(EXPLOSION);
      color = BLACK;
      particle(gtailShipPos.x, gtailShipPos.y, 30, 3, 0, CGLP_PI * 2);
      gtailShipPos.y += 10;
      m->isAlive = false;
      continue;
    }

    bool isInSmallRect = m->pos.x >= -3 && m->pos.x < 103 && m->pos.y >= -3 &&
                         m->pos.y < 103;
    if (!isInSmallRect) {
      color = RED;
      float bx = clamp(m->pos.x, -3, 103);
      float by = clamp(m->pos.y, -3, 103);
      Collision boxc;
      box(bx, by, 8, 8, &boxc);
    }
    bool isInBigRect = m->pos.x >= -100 && m->pos.x < 200 && m->pos.y >= -100 &&
                       m->pos.y < 200;
    m->isAlive = isInBigRect;
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(gtailMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameGtail() {
  addGame(gtailTitle, gtailDescription, gtailCharacters, gtailCharactersCount,
          &gtailOptions, true, &gtailUpdate);
}
