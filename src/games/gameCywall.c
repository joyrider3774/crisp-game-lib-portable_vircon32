#include "../cglp.h"

int* cywallTitle = "CYWALL";
int* cywallDescription = "[Tap]\n Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] cywallCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int cywallCharactersCount = 0;

Options cywallOptions = {100, 100, 27, false};

struct CywallCircle {
  Vector pos;
  float r;
  int num;
  float a;
  float v;
  float l;
  int nextIndex;
  bool isAlive;
};
#define CYWALL_MAX_CIRCLE_COUNT 64
CywallCircle[CYWALL_MAX_CIRCLE_COUNT] cywallCircles;
int cywallCircleIndex;
float cywallCircleAddDist;
int cywallLastCircleIndex;
int cywallPlayerCircleIndex;

void cywallAddCircle() {
  float r = rnd(20, 30);
  int newIndex = cywallCircleIndex;
  ASSIGN_ARRAY_ITEM(cywallCircles, newIndex, CywallCircle, c);
  vectorSet(&c->pos, rnd(20, 80), -r);
  c->r = r;
  c->num = rndi(1, 4);
  c->a = rnd(0, CGLP_PI * 2);
  c->v = rnd(0.02, 0.05) * RNDPM() * difficulty;
  c->l = rnd(10, 20);
  c->nextIndex = -1;
  c->isAlive = true;
  if (cywallLastCircleIndex >= 0) {
    cywallCircles[cywallLastCircleIndex].nextIndex = newIndex;
  }
  if (cywallPlayerCircleIndex < 0) {
    cywallPlayerCircleIndex = newIndex;
  }
  cywallLastCircleIndex = newIndex;
  cywallCircleIndex = cgl_wrap(cywallCircleIndex + 1, 0, CYWALL_MAX_CIRCLE_COUNT);
}

void cywallUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(cywallCircles);
    cywallCircleIndex = 0;
    cywallCircleAddDist = 0;
    cywallLastCircleIndex = -1;
    cywallPlayerCircleIndex = -1;
  }
  if (cywallCircleAddDist <= 0) {
    cywallAddCircle();
    cywallCircleAddDist += rnd(20, 40);
  }
  float sc = difficulty * 0.1;
  CywallCircle* playerCircle = &cywallCircles[cywallPlayerCircleIndex];
  float py = playerCircle->pos.y;
  if (py < 50) {
    sc += (50 - py) * 0.05;
  }
  cywallCircleAddDist -= sc;
  addScore(sc, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  if (playerCircle->pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(cywallCircles, i) {
    ASSIGN_ARRAY_ITEM(cywallCircles, i, CywallCircle, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.y += sc;
    if (c->pos.y > 99 + c->r) {
      c->isAlive = false;
      continue;
    }
    bool isPlayerOrNext =
        (i == cywallPlayerCircleIndex) ||
        (playerCircle->nextIndex >= 0 && i == playerCircle->nextIndex);
    if (isPlayerOrNext) {
      color = CYAN;
    } else {
      color = RED;
    }
    box(c->pos.x, c->pos.y, 3, 3, &scratch);
    color = RED;
    c->a += c->v;
    TIMES(c->num, k) {
      float a = c->a + k * CGLP_PI * 2 / c->num;
      Vector barPos;
      barPos = c->pos;
      addWithAngle(&barPos, a, c->r);
      thickness = 3;
      barCenterPosRatio = 0.5;
      bar(barPos.x, barPos.y, c->l, a + CGLP_PI_2, &scratch);
    }
  }
  color = CYAN;
  if (playerCircle->nextIndex >= 0 && input.isJustPressed) {
    CywallCircle* nextCircle = &cywallCircles[playerCircle->nextIndex];
    thickness = 3;
    line(playerCircle->pos.x, playerCircle->pos.y, nextCircle->pos.x, nextCircle->pos.y,
         &scratch);
    if (scratch.isColliding.rect[RED]) {
      play(EXPLOSION);
      gameOver();
    } else {
      play(COIN);
      Vector p;
      p = playerCircle->pos;
      Vector o;
      o.x = (nextCircle->pos.x - playerCircle->pos.x) / 9;
      o.y = (nextCircle->pos.y - playerCircle->pos.y) / 9;
      float a = vectorAngle(&o);
      TIMES(9, i) {
        particle(p.x, p.y, 4, 2, a + CGLP_PI, 0.5);
        vectorAdd(&p, o.x, o.y);
      }
    }
    cywallPlayerCircleIndex = playerCircle->nextIndex;
  } else {
    box(playerCircle->pos.x, playerCircle->pos.y, 5, 5, &scratch);
  }
}

void addGameCywall() {
  addGame(cywallTitle, cywallDescription, cywallCharacters,
          cywallCharactersCount, &cywallOptions, false, &cywallUpdate);
}
