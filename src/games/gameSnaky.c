#include "../cglp.h"

int* snakyTitle = "SNAKY";
int* snakyDescription = "[Hold]\n Up";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] snakyCharacters = {{
    " llll ",
    "ll lll",
    "l llll",
    "llllll",
    "llllll",
    " llll ",
}};
int snakyCharactersCount = 1;

Options snakyOptions = {100, 100, 7, true};

struct SnakyNode {
  float angle;
  float va;
};
#define SNAKY_MAX_NODE_COUNT 10
SnakyNode[SNAKY_MAX_NODE_COUNT] snakyNodes;
int snakyNodeCount;

struct SnakyItem {
  Vector pos;
  float vx;
  bool isRed;
  bool isAlive;
};
#define SNAKY_MAX_ITEM_COUNT 32
SnakyItem[SNAKY_MAX_ITEM_COUNT] snakyItems;
int snakyItemIndex;
float snakyNextItemTicks;
int snakyNextRedItemCount;
float snakyNextItemY;

void snakyAddNode() {
  if (snakyNodeCount > 9) {
    return;
  }
  snakyNodes[snakyNodeCount].angle = 0;
  snakyNodes[snakyNodeCount].va = 0;
  snakyNodeCount++;
}

void snakyRemoveNode() {
  snakyNodeCount--;
}

void snakyUpdate() {
  Collision scratch;
  if (!ticks) {
    snakyNodeCount = 1;
    snakyNodes[0].angle = 0;
    snakyNodes[0].va = 0;
    snakyAddNode();
    INIT_UNALIVED_ARRAY_FAST(snakyItems);
    snakyItemIndex = 0;
    snakyNextItemTicks = 0;
    snakyNextRedItemCount = 2;
    snakyNextItemY = 50;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  float nl = (50 + snakyNodeCount * 3) / (float)snakyNodeCount;
  float na = CGLP_PI / (sqrt(snakyNodeCount - 1) + 3);
  Vector p;
  vectorSet(&p, 0, 50);
  TIMES(snakyNodeCount, i) {
    SnakyNode* n = &snakyNodes[i];
    if (i == 0) {
      float pa = n->angle;
      float dir;
      if (input.isPressed) {
        dir = -1;
      } else {
        dir = 1;
      }
      n->angle = clamp(n->angle + dir * 0.03 * sqrt(difficulty), -na, na);
      n->va = n->angle - pa;
    } else {
      SnakyNode* prevN = &snakyNodes[i - 1];
      n->angle += n->va;
      if ((n->angle > prevN->angle + na && n->va > 0) ||
          (n->angle < prevN->angle - na && n->va < 0)) {
        n->va *= -0.2;
      }
    }
    n->va *= 0.95;
    if (i < snakyNodeCount - 1) {
      SnakyNode* nextN = &snakyNodes[i + 1];
      nextN->va += n->va * 0.07 * sqrt(sqrt(difficulty));
    }
    if (i > 0) {
      SnakyNode* prevN2 = &snakyNodes[i - 1];
      prevN2->va += n->va * 0.01 * sqrt(sqrt(difficulty));
    }
    if (i == snakyNodeCount - 1) {
      color = RED;
    } else {
      color = BLACK;
    }
    thickness = 3;
    barCenterPosRatio = 0;
    bar(p.x, p.y, nl, n->angle, &scratch);
    addWithAngle(&p, n->angle, nl);
  }
  snakyNextItemTicks--;
  if (snakyNextItemTicks < 0) {
    bool isRed = false;
    snakyNextRedItemCount--;
    if (snakyNextRedItemCount < 0) {
      isRed = true;
      snakyNextRedItemCount = 7;
    }
    ASSIGN_ARRAY_ITEM(snakyItems, snakyItemIndex, SnakyItem, it);
    vectorSet(&it->pos, 103, snakyNextItemY);
    it->vx = -rnd(1, difficulty) * 0.1;
    it->isRed = isRed;
    it->isAlive = true;
    snakyItemIndex = cgl_wrap(snakyItemIndex + 1, 0, SNAKY_MAX_ITEM_COUNT);
    float ny = rnd(20, 80);
    snakyNextItemTicks = (fabs(snakyNextItemY - ny + 10) * 4) / sqrt(difficulty);
    snakyNextItemY = ny;
  }
  FOR_EACH(snakyItems, idx) {
    ASSIGN_ARRAY_ITEM(snakyItems, idx, SnakyItem, it);
    SKIP_IS_NOT_ALIVE(it);
    it->vx *= 1 + 0.005 * sqrt(difficulty);
    it->pos.x += it->vx;
    if (it->isRed) {
      color = RED;
    } else {
      color = BLACK;
    }
    Collision cc;
    character("a", it->pos.x, it->pos.y, &cc);
    float sc = 0;
    if (cc.isColliding.rect[RED]) {
      if (it->isRed) {
        play(POWER_UP);
        sc = snakyNodeCount * snakyNodeCount;
        snakyAddNode();
      } else {
        play(COIN);
        sc = snakyNodeCount;
      }
    } else if (cc.isColliding.rect[BLACK]) {
      if (it->isRed) {
        play(HIT);
        sc = -snakyNodeCount;
        snakyRemoveNode();
      } else {
        play(LASER);
        sc = 1;
      }
    }
    if (sc != 0) {
      addScore(sc, it->pos.x, it->pos.y);
      it->isAlive = false;
      continue;
    }
    if (it->pos.x < 0) {
      play(EXPLOSION);
      color = RED;
      text("X", 3, it->pos.y, &scratch);
      gameOver();
    }
  }
}

void addGameSnaky() {
  addGame(snakyTitle, snakyDescription, snakyCharacters, snakyCharactersCount,
          &snakyOptions, false, &snakyUpdate);
}
