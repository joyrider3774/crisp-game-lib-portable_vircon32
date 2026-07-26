#include "../cglp.h"

int* udcaveTitle = "UD CAVE";
int* udcaveDescription = "[Hold]\n Go right";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] udcaveCharacters = {{
    " l    ",
    "lll   ",
    " l    ",
    "l l   ",
}};
int udcaveCharactersCount = 1;

Options udcaveOptions = {100, 100, 5, true};

#define UDCAVE_WALL_HEIGHT 10

struct UdcaveCave {
  float x;
  float vx;
  float w;
  float vw;
};
#define UDCAVE_CAVE_COUNT 3
UdcaveCave[UDCAVE_CAVE_COUNT] udcaveCaves;

struct UdcaveWall {
  Vector pos;
  float width;
  float vy;
  bool isAlive;
};
#define UDCAVE_MAX_WALL_COUNT 64
UdcaveWall[UDCAVE_MAX_WALL_COUNT] udcaveWalls;
int udcaveWallIndex;
float udcaveNextWallDist;

struct UdcaveGold {
  Vector pos;
  float vy;
  bool isAlive;
};
#define UDCAVE_MAX_GOLD_COUNT 32
UdcaveGold[UDCAVE_MAX_GOLD_COUNT] udcaveGolds;
int udcaveGoldIndex;
float udcaveNextGoldDist;

float udcavePlayerX;
int udcaveMultiplier;

void udcaveUpdate() {
  Collision scratch;
  if (!ticks) {
    udcavePlayerX = 0;
    INIT_UNALIVED_ARRAY_FAST(udcaveWalls);
    udcaveWallIndex = 0;
    udcaveNextWallDist = -50;
    TIMES(UDCAVE_CAVE_COUNT, i) {
      udcaveCaves[i].x = 0;
      udcaveCaves[i].vx = 0;
      if (i > 0) {
        udcaveCaves[i].w = 15;
      } else {
        udcaveCaves[i].w = 20;
      }
      udcaveCaves[i].vw = 0;
    }
    INIT_UNALIVED_ARRAY_FAST(udcaveGolds);
    udcaveGoldIndex = 0;
    udcaveNextGoldDist = 5;
    udcaveMultiplier = 1;
  }
  float vy = difficulty;
  udcaveNextWallDist -= vy;
  if (udcaveNextWallDist < 0) {
    addScore(udcaveMultiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    TIMES(UDCAVE_CAVE_COUNT, i) {
      UdcaveCave* c = &udcaveCaves[i];
      c->vx += rnd(0, 2) * RNDPM() * sqrt(difficulty);
      c->vw += rnd(0, 1) * RNDPM() * sqrt(difficulty);
      c->x += c->vx;
      c->w += c->vw;
      float minX, maxX;
      if (i == 0) {
        minX = -(17 - 7 / sqrt(difficulty));
        maxX = 17 - 7 / sqrt(difficulty);
      } else {
        minX = udcaveCaves[0].x - udcaveCaves[0].w;
        maxX = udcaveCaves[0].x + udcaveCaves[0].w;
      }
      if ((c->x - c->w < minX && c->vx < 0) || (c->x + c->w > maxX && c->vx > 0)) {
        c->vx *= -0.5;
        c->x += c->vx;
      }
      float minW, maxW;
      if (i == 0) {
        minW = 5 + 5 / sqrt(difficulty);
        maxW = 7 + 7 / sqrt(difficulty);
      } else {
        minW = udcaveCaves[0].w;
        maxW = 9 + 9 / sqrt(difficulty);
      }
      if ((c->w < minW && c->vw < 0) || (c->w > maxW && c->vw > 0)) {
        c->vw *= -0.5;
        c->w += c->vw;
      }
    }
    UdcaveCave* c1 = &udcaveCaves[1];
    float x11 = c1->x - c1->w + 25;
    float x12 = c1->x + c1->w + 25;
    if (x11 > 0) {
      ASSIGN_ARRAY_ITEM(udcaveWalls, udcaveWallIndex, UdcaveWall, w1);
      vectorSet(&w1->pos, x11, -udcaveNextWallDist);
      w1->width = -x11;
      w1->vy = 1;
      w1->isAlive = true;
      udcaveWallIndex = cgl_wrap(udcaveWallIndex + 1, 0, UDCAVE_MAX_WALL_COUNT);
    }
    if (x12 < 50) {
      ASSIGN_ARRAY_ITEM(udcaveWalls, udcaveWallIndex, UdcaveWall, w2);
      vectorSet(&w2->pos, x12, -udcaveNextWallDist);
      w2->width = 50 - x12;
      w2->vy = 1;
      w2->isAlive = true;
      udcaveWallIndex = cgl_wrap(udcaveWallIndex + 1, 0, UDCAVE_MAX_WALL_COUNT);
    }
    UdcaveCave* c2 = &udcaveCaves[2];
    float x21 = 75 - c2->x - c2->w;
    float x22 = 75 - c2->x + c2->w;
    if (x21 > 50) {
      ASSIGN_ARRAY_ITEM(udcaveWalls, udcaveWallIndex, UdcaveWall, w3);
      vectorSet(&w3->pos, x21, 100 + udcaveNextWallDist);
      w3->width = 50 - x21;
      w3->vy = -1;
      w3->isAlive = true;
      udcaveWallIndex = cgl_wrap(udcaveWallIndex + 1, 0, UDCAVE_MAX_WALL_COUNT);
    }
    if (x22 < 100) {
      ASSIGN_ARRAY_ITEM(udcaveWalls, udcaveWallIndex, UdcaveWall, w4);
      vectorSet(&w4->pos, x22, 100 + udcaveNextWallDist);
      w4->width = 100 - x22;
      w4->vy = -1;
      w4->isAlive = true;
      udcaveWallIndex = cgl_wrap(udcaveWallIndex + 1, 0, UDCAVE_MAX_WALL_COUNT);
    }
    udcaveNextGoldDist--;
    if (udcaveNextGoldDist < 0) {
      if (rnd(0, 1) < 0.5) {
        ASSIGN_ARRAY_ITEM(udcaveGolds, udcaveGoldIndex, UdcaveGold, ng);
        vectorSet(&ng->pos, udcaveCaves[1].x + rnd(0, udcaveCaves[1].w * 0.8) * RNDPM() + 25,
                  -udcaveNextWallDist - UDCAVE_WALL_HEIGHT / 2);
        ng->vy = 1;
        ng->isAlive = true;
        udcaveGoldIndex = cgl_wrap(udcaveGoldIndex + 1, 0, UDCAVE_MAX_GOLD_COUNT);
      } else {
        ASSIGN_ARRAY_ITEM(udcaveGolds, udcaveGoldIndex, UdcaveGold, ng2);
        vectorSet(&ng2->pos, 75 - udcaveCaves[2].x + rnd(0, udcaveCaves[2].w * 0.8) * RNDPM(),
                  100 + udcaveNextWallDist + UDCAVE_WALL_HEIGHT / 2);
        ng2->vy = -1;
        ng2->isAlive = true;
        udcaveGoldIndex = cgl_wrap(udcaveGoldIndex + 1, 0, UDCAVE_MAX_GOLD_COUNT);
      }
      udcaveNextGoldDist = rnd(3, 9);
    }
    udcaveNextWallDist += UDCAVE_WALL_HEIGHT;
  }
  color = RED;
  FOR_EACH(udcaveWalls, i) {
    ASSIGN_ARRAY_ITEM(udcaveWalls, i, UdcaveWall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.y += w->vy * vy;
    rect(w->pos.x, w->pos.y, w->width, (UDCAVE_WALL_HEIGHT - 1) * -w->vy, &scratch);
    bool removed;
    if (w->vy > 0) {
      removed = w->pos.y > 100 + UDCAVE_WALL_HEIGHT;
    } else {
      removed = w->pos.y < -UDCAVE_WALL_HEIGHT;
    }
    w->isAlive = !removed;
  }
  float pdir;
  if (input.isPressed) {
    pdir = 1;
  } else {
    pdir = -1;
  }
  udcavePlayerX = clamp(udcavePlayerX + pdir * difficulty * 0.5, -25, 25);
  if (input.isJustPressed) {
    play(SELECT);
  } else if (input.isJustReleased) {
    play(LASER);
  }
  color = BLACK;
  Collision c1coll;
  character("a", udcavePlayerX + 25, 90, &c1coll);
  Collision c2coll;
  character("a", 75 - udcavePlayerX, 10, &c2coll);
  if (c1coll.isColliding.rect[RED] || c2coll.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  color = YELLOW;
  FOR_EACH(udcaveGolds, i) {
    ASSIGN_ARRAY_ITEM(udcaveGolds, i, UdcaveGold, g);
    SKIP_IS_NOT_ALIVE(g);
    g->pos.y += g->vy * vy;
    Collision gc;
    text("$", g->pos.x, g->pos.y, &gc);
    if (gc.isColliding.rect[RED]) {
      g->isAlive = false;
      continue;
    }
    if (gc.isColliding.character['a']) {
      play(POWER_UP);
      udcaveMultiplier++;
      g->isAlive = false;
      continue;
    }
    bool removed;
    if (g->vy > 0) {
      removed = g->pos.y > 103;
    } else {
      removed = g->pos.y < -3;
    }
    g->isAlive = !removed;
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(udcaveMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameUdcave() {
  addGame(udcaveTitle, udcaveDescription, udcaveCharacters,
          udcaveCharactersCount, &udcaveOptions, false, &udcaveUpdate);
}
