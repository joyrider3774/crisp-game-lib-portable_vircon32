#include "../cglp.h"

int* teeterTitle = "TEETER";
int* teeterDescription = "[Tap]\n Change angle";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] teeterCharacters = {{
    " llll ",
    "l llll",
    "llllll",
    "llllll",
    "llllll",
    " llll ",
}};
int teeterCharactersCount = 1;

Options teeterOptions = {100, 100, 60, false};

struct TeeterBar {
  Vector pos;
  float angle;
  float width;
  bool isAlive;
};
// Bars never expire on their own - only "landed on a negative-barCount
// floor" removes them, while landing on a positive one adds up to 4 more;
// a lucky/skilled run can keep net-adding bars indefinitely across many
// rounds (game only ends when zero bars remain), so keep well above the
// ~30-40 that can physically fit without overlapping.
#define TEETER_MAX_BAR_COUNT 128
TeeterBar[TEETER_MAX_BAR_COUNT] teeterBars;
int teeterBarIndex;

int teeterBarAngleSign;
bool teeterHasBall;
Vector teeterBallTargetPos;
Vector teeterBallPos;

struct TeeterFloor {
  Vector pos;
  float width;
  float score;
  float barCount;
  bool isAlive;
};
#define TEETER_MAX_FLOOR_COUNT 16
TeeterFloor[TEETER_MAX_FLOOR_COUNT] teeterFloors;
int teeterFloorIndex;

bool teeterAddBar() {
  Collision scratch;
  color = WHITE;
  FOR_EACH(teeterBars, bi) {
    ASSIGN_ARRAY_ITEM(teeterBars, bi, TeeterBar, b);
    SKIP_IS_NOT_ALIVE(b);
    thickness = 6;
    bar(b->pos.x, b->pos.y, b->width + 5, b->angle, &scratch);
    bar(b->pos.x, b->pos.y, b->width + 5, -b->angle, &scratch);
  }
  color = TRANSPARENT;
  bool isPlaced = false;
  TIMES(99, ti) {
    float width = rnd(12, 18);
    float angle = rnd(0.2, 0.8) * RNDPM();
    float aw = width * cos(angle);
    float px = rnd(5 + aw / 2, 95 - aw / 2);
    float py = rnd(20, 70);
    thickness = 6;
    Collision c1;
    bar(px, py, width + 5, angle, &c1);
    Collision c2;
    bar(px, py, width + 5, -angle, &c2);
    if (!c1.isColliding.rect[WHITE] && !c2.isColliding.rect[WHITE]) {
      ASSIGN_ARRAY_ITEM(teeterBars, teeterBarIndex, TeeterBar, nb);
      vectorSet(&nb->pos, px, py);
      nb->angle = angle;
      nb->width = width;
      nb->isAlive = true;
      teeterBarIndex = cgl_wrap(teeterBarIndex + 1, 0, TEETER_MAX_BAR_COUNT);
      isPlaced = true;
      break;
    }
  }
  return isPlaced;
}

void teeterUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(teeterBars);
    teeterBarIndex = 0;
    TIMES(7, i) { teeterAddBar(); }
    teeterBarAngleSign = 1;
    teeterHasBall = false;
    INIT_UNALIVED_ARRAY_FAST(teeterFloors);
    teeterFloorIndex = 0;
  }
  if (!teeterHasBall) {
    play(LASER);
    float x = 50;
    TIMES(99, i) {
      x = rnd(20, 80);
      bool isOnBar = false;
      FOR_EACH(teeterBars, bi2) {
        ASSIGN_ARRAY_ITEM(teeterBars, bi2, TeeterBar, b2);
        SKIP_IS_NOT_ALIVE(b2);
        if (fabs(b2->pos.x - x) < (b2->width / 2) * cos(b2->angle)) {
          isOnBar = true;
        }
      }
      if (isOnBar) {
        break;
      }
    }
    vectorSet(&teeterBallTargetPos, x, -4);
    teeterBallPos = teeterBallTargetPos;
    teeterHasBall = true;
    INIT_UNALIVED_ARRAY_FAST(teeterFloors);
    teeterFloorIndex = 0;
    int waveStart = teeterFloorIndex;
    int waveCount = 0;
    float fw = 0;
    while (fw < 100) {
      float width = rnd(20, 30);
      ASSIGN_ARRAY_ITEM(teeterFloors, teeterFloorIndex, TeeterFloor, nf);
      vectorSet(&nf->pos, fw + width / 2, 95);
      nf->width = width;
      nf->score = floor(rnd(1, 3.1) * rnd(1, 3.1));
      nf->barCount = 0;
      nf->isAlive = true;
      teeterFloorIndex = cgl_wrap(teeterFloorIndex + 1, 0, TEETER_MAX_FLOOR_COUNT);
      waveCount++;
      fw += width;
    }
    int fi = rndi(0, waveCount);
    TeeterFloor* ff1 = &teeterFloors[(waveStart + fi) % TEETER_MAX_FLOOR_COUNT];
    ff1->score = 0;
    ff1->barCount = floor(rnd(1, 2.2) * rnd(1, 2.2));
    fi = rndi(0, waveCount);
    TeeterFloor* ff2 = &teeterFloors[(waveStart + fi) % TEETER_MAX_FLOOR_COUNT];
    ff2->score = 0;
    ff2->barCount = -floor(rnd(1, 2.2) * rnd(1, 2.2));
    TIMES(waveCount, wi) {
      TeeterFloor* wf = &teeterFloors[(waveStart + wi) % TEETER_MAX_FLOOR_COUNT];
      wf->pos.x -= (fw - 100) / 2;
    }
  }
  teeterBallTargetPos.y += difficulty * 0.5;
  vectorAdd(&teeterBallPos, (teeterBallTargetPos.x - teeterBallPos.x) * 0.5,
            (teeterBallTargetPos.y - teeterBallPos.y) * 0.5);
  color = BLACK;
  character("a", teeterBallPos.x, teeterBallPos.y, &scratch);
  if (input.isJustPressed) {
    play(SELECT);
    teeterBarAngleSign *= -1;
  }
  FOR_EACH(teeterBars, bi3) {
    ASSIGN_ARRAY_ITEM(teeterBars, bi3, TeeterBar, b3);
    SKIP_IS_NOT_ALIVE(b3);
    float a = b3->angle * teeterBarAngleSign;
    thickness = 3;
    Collision bc;
    bar(b3->pos.x, b3->pos.y, b3->width, a, &bc);
    if (bc.isColliding.character['a']) {
      play(HIT);
      float dir;
      if (a > 0) {
        dir = 1;
      } else {
        dir = -1;
      }
      vectorSet(&teeterBallTargetPos, b3->pos.x, b3->pos.y);
      addWithAngle(&teeterBallTargetPos, a, (b3->width / 2 + 7) * dir);
    }
  }
  float barCountDiff = 0;
  FOR_EACH(teeterFloors, fi2) {
    ASSIGN_ARRAY_ITEM(teeterFloors, fi2, TeeterFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    int[16] t;
    int tColor;
    if (f->score > 0) {
      strcpy(t, intToChar((int)f->score));
      if (f->score < 5) {
        tColor = LIGHT_BLACK;
      } else {
        tColor = BLACK;
      }
    } else if (f->barCount < 0) {
      strcpy(t, intToChar((int)f->barCount));
      tColor = RED;
    } else {
      strcpy(t, "+");
      strcat(t, intToChar((int)f->barCount));
      tColor = BLUE;
    }
    color = tColor;
    Collision fc;
    box(f->pos.x, f->pos.y, f->width - 1, 10, &fc);
    if (fc.isColliding.character['a']) {
      particle(f->pos.x, f->pos.y, 16, 1, 0, CGLP_PI * 2);
      if (f->score > 0) {
        play(COIN);
        addScore(f->score, f->pos.x, f->pos.y);
        f->score++;
      } else {
        barCountDiff += f->barCount;
      }
      f->isAlive = false;
      continue;
    }
    color = WHITE;
    int tLen = strlen(t);
    text(t, clamp(f->pos.x - (tLen - 1) * 3, 3, 97 - (tLen - 1) * 6), f->pos.y, &scratch);
    if (f->pos.x > 99 + f->width / 2) {
      f->isAlive = false;
      continue;
    }
  }
  if (barCountDiff > 0) {
    int sc = 0;
    TIMES((int)barCountDiff, bci) {
      play(POWER_UP);
      if (!teeterAddBar()) {
        sc++;
      }
    }
    if (sc > 0) {
      addScore(sc, teeterBallPos.x, teeterBallPos.y);
    }
  } else if (barCountDiff < 0) {
    play(EXPLOSION);
    color = BLACK;
    int removeCount = (int)(-barCountDiff);
    TIMES(removeCount, rci) {
      COUNT_IS_ALIVE(teeterBars, aliveCount);
      if (aliveCount == 0) {
        break;
      }
      int target = rndi(0, aliveCount);
      int seen = 0;
      FOR_EACH(teeterBars, bi4) {
        ASSIGN_ARRAY_ITEM(teeterBars, bi4, TeeterBar, b4);
        if (!b4->isAlive) {
          continue;
        }
        if (seen == target) {
          particle(b4->pos.x, b4->pos.y, 16, 1, 0, CGLP_PI * 2);
          b4->isAlive = false;
          break;
        }
        seen++;
      }
    }
  }
  if (teeterBallPos.y > 99) {
    COUNT_IS_ALIVE(teeterBars, finalAliveCount);
    if (finalAliveCount == 0) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
    teeterHasBall = false;
  }
}

void addGameTeeter() {
  addGame(teeterTitle, teeterDescription, teeterCharacters, teeterCharactersCount,
          &teeterOptions, false, &teeterUpdate);
}
