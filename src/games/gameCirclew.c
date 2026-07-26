#include "../cglp.h"

int* circlewTitle = "CIRCLE W";
int* circlewDescription = "[Hold]\n Expand";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] circlewCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int circlewCharactersCount = 0;

Options circlewOptions = {200, 100, 5, false};

#define CIRCLEW_TYPE_NORMAL 0
#define CIRCLEW_TYPE_PLAYER 1
#define CIRCLEW_TYPE_DANGER 2

struct CirclewCircle {
  Vector pos;
  float radius;
  int type;
  bool isAlive;
};
#define CIRCLEW_MAX_CIRCLE_COUNT 64
CirclewCircle[CIRCLEW_MAX_CIRCLE_COUNT] circlewCircles;
int circlewCircleIndex;

float circlewNextCircleDist;
int circlewNextDangerCount;
float circlewPlayerRadius;
Vector circlewScr;
int circlewMultiplier;

void circlewUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(circlewCircles);
    circlewCircleIndex = 0;
    ASSIGN_ARRAY_ITEM(circlewCircles, circlewCircleIndex, CirclewCircle, pc);
    vectorSet(&pc->pos, 200, 50);
    pc->radius = 10;
    pc->type = CIRCLEW_TYPE_PLAYER;
    pc->isAlive = true;
    circlewCircleIndex = cgl_wrap(circlewCircleIndex + 1, 0, CIRCLEW_MAX_CIRCLE_COUNT);
    circlewNextCircleDist = 0;
    circlewNextDangerCount = 30;
    circlewPlayerRadius = 1;
    circlewMultiplier = 1;
    vectorSet(&circlewScr, 0, 0);
  }

  if (input.isJustPressed) {
    play(LASER);
  }
  if (circlewPlayerRadius < 9 && circlewMultiplier > 1) {
    circlewMultiplier--;
  }
  float pressDir;
  if (input.isPressed) {
    pressDir = 1;
  } else {
    pressDir = -1;
  }
  circlewPlayerRadius += sqrt(difficulty) * pressDir * 0.5;
  if (circlewPlayerRadius < 1) {
    circlewPlayerRadius = 1;
  }

  // Vircon32 port note: upstream's remove() callback both scrolls/wraps
  // every circle AND (via closure side effect) captures a reference to
  // whichever circle currently has type "player" - there is exactly one
  // at a time. Recording its index here and re-reading through a pointer
  // below reproduces that same live-reference behavior.
  int circlewPlayerIndex = -1;
  FOR_EACH(circlewCircles, ci) {
    ASSIGN_ARRAY_ITEM(circlewCircles, ci, CirclewCircle, c);
    SKIP_IS_NOT_ALIVE(c);
    vectorAdd(&c->pos, circlewScr.x, circlewScr.y);
    c->pos.y = cgl_wrap(c->pos.y, -15, 115);
    if (c->type == CIRCLEW_TYPE_PLAYER) {
      circlewPlayerIndex = ci;
      continue;
    }
    if (c->pos.x < -c->radius) {
      c->isAlive = false;
    }
  }

  CirclewCircle* playerCircle = &circlewCircles[circlewPlayerIndex];
  Vector* pp = &playerCircle->pos;
  color = GREEN;
  arc(pp->x, pp->y, circlewPlayerRadius, 0, CGLP_PI * 2, &scratch);
  if (pp->x < 20) {
    pp->x = 20;
  }

  bool circlewIsSetPlayer = false;
  FOR_EACH(circlewCircles, ci2) {
    ASSIGN_ARRAY_ITEM(circlewCircles, ci2, CirclewCircle, c2);
    SKIP_IS_NOT_ALIVE(c2);
    if (c2->type == CIRCLEW_TYPE_PLAYER) {
      continue;
    }
    if (c2->type == CIRCLEW_TYPE_DANGER) {
      color = RED;
    } else {
      color = BLUE;
    }
    Collision cc;
    arc(c2->pos.x, c2->pos.y, c2->radius, 0, CGLP_PI * 2, &cc);
    if (cc.isColliding.rect[GREEN]) {
      if (c2->type == CIRCLEW_TYPE_DANGER) {
        play(EXPLOSION);
        gameOver();
      } else if (!circlewIsSetPlayer) {
        play(COIN);
        circlewMultiplier += ceil(circlewPlayerRadius);
        addScore(circlewMultiplier, c2->pos.x, c2->pos.y);
        c2->type = CIRCLEW_TYPE_PLAYER;
        circlewPlayerRadius = c2->radius;
        circlewIsSetPlayer = true;
        playerCircle->pos.x = -99;
        playerCircle->type = CIRCLEW_TYPE_NORMAL;
      }
    }
  }

  circlewNextCircleDist += circlewScr.x;
  if (circlewNextCircleDist < 0) {
    int spawnType;
    if (circlewNextDangerCount == 0) {
      spawnType = CIRCLEW_TYPE_DANGER;
    } else {
      spawnType = CIRCLEW_TYPE_NORMAL;
    }
    color = TRANSPARENT;
    TIMES(9, spawnI) {
      float radius = rnd(8, 15);
      float boxW;
      if (spawnType == CIRCLEW_TYPE_DANGER) {
        boxW = radius * 8;
      } else {
        boxW = radius * 5;
      }
      Vector spawnPos;
      vectorSet(&spawnPos, 200 + radius, rndi(0, 99));
      Collision spawnC;
      box(spawnPos.x, spawnPos.y, boxW, radius * 2.5, &spawnC);
      if (!(spawnC.isColliding.rect[BLUE] || spawnC.isColliding.rect[RED])) {
        ASSIGN_ARRAY_ITEM(circlewCircles, circlewCircleIndex, CirclewCircle, nc);
        nc->pos = spawnPos;
        nc->radius = radius;
        nc->type = spawnType;
        nc->isAlive = true;
        circlewCircleIndex = cgl_wrap(circlewCircleIndex + 1, 0, CIRCLEW_MAX_CIRCLE_COUNT);
        circlewNextDangerCount--;
        if (circlewNextDangerCount < 0) {
          play(HIT);
          circlewNextDangerCount = rndi(24, 30);
        }
        circlewNextCircleDist = 5;
        break;
      }
    }
  }

  vectorSet(&circlewScr, -sqrt(difficulty), (50 - pp->y) * 0.1);
  if (pp->x > 20) {
    circlewScr.x -= (pp->x - 20) * 0.1;
  }
  color = BLACK;
  int[16] circlewMultText;
  strcpy(circlewMultText, "x");
  strcat(circlewMultText, intToChar(circlewMultiplier));
  text(circlewMultText, 3, 9, &scratch);
}

void addGameCirclew() {
  addGame(circlewTitle, circlewDescription, circlewCharacters, circlewCharactersCount,
          &circlewOptions, false, &circlewUpdate);
}
