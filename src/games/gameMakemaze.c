#include "../cglp.h"

int* makemazeTitle = "MAKE MAZE";
int* makemazeDescription = "[Tap][Slide]\n Add/Remove wall";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] makemazeCharacters = {
    {
        "llll l",
        "llll l",
        "      ",
        "ll lll",
        "ll lll",
        "      ",
    },
    {
        "  ll  ",
        "  lll ",
        "llllll",
        "llllll",
        "  lll ",
        "  ll  ",
    },
    {
        "  ll  ",
        " l ll ",
        "l llll",
        "llllll",
        " llll ",
        "  ll  ",
    },
};
int makemazeCharactersCount = 3;

Options makemazeOptions = {100, 100, 30, false};

#define MAKEMAZE_WALL_SIZE_X 16
#define MAKEMAZE_WALL_SIZE_Y 18

bool[MAKEMAZE_WALL_SIZE_X][MAKEMAZE_WALL_SIZE_Y] makemazeWalls;
Vector makemazeWallOfs;
bool[MAKEMAZE_WALL_SIZE_X][MAKEMAZE_WALL_SIZE_Y] makemazeWallFills;
Vector makemazePwp;
Vector[8] makemazeAngleOfs;

struct MakemazeEnemy {
  Vector pos;
  int angle;
  int angleVel;
  Vector scPos;
  float moveInterval;
  float ticks;
  bool isAngry;
  bool isAlive;
};
#define MAKEMAZE_MAX_ENEMY_COUNT 32
MakemazeEnemy[MAKEMAZE_MAX_ENEMY_COUNT] makemazeEnemies;
int makemazeEnemyIndex;
float makemazeNextEnemyTicks;

struct MakemazeGold {
  Vector pos;
  bool isAlive;
};
#define MAKEMAZE_MAX_GOLD_COUNT 64
MakemazeGold[MAKEMAZE_MAX_GOLD_COUNT] makemazeGolds;
int makemazeGoldIndex;
float makemazeGoldMinY;
float makemazeMissScr;
int makemazeMultiplier;

bool makemazeGetWall(Vector* cp, int ta) {
  int wrappedTa = (int)cgl_wrap(ta, 0, 8);
  float px = cp->x + makemazeAngleOfs[wrappedTa].x;
  float py = cp->y + makemazeAngleOfs[wrappedTa].y;
  if (py < 0 || py >= MAKEMAZE_WALL_SIZE_Y) {
    return false;
  }
  return makemazeWalls[(int)px][(int)py];
}

void makemazeRemoveGold(Vector* p) {
  color = YELLOW;
  FOR_EACH(makemazeGolds, i) {
    ASSIGN_ARRAY_ITEM(makemazeGolds, i, MakemazeGold, g);
    SKIP_IS_NOT_ALIVE(g);
    if (g->pos.x == p->x && g->pos.y == p->y) {
      play(COIN);
      float gx = makemazeWallOfs.x + g->pos.x * 6;
      float gy = makemazeWallOfs.y + g->pos.y * 6;
      addScore(makemazeMultiplier, gx, gy);
      particle(gx, gy, 16, 1, 0, CGLP_PI * 2);
      makemazeMultiplier++;
      g->isAlive = false;
    }
  }
}

bool makemazeCheckGold(Vector* p) {
  bool exists = false;
  FOR_EACH(makemazeGolds, i) {
    ASSIGN_ARRAY_ITEM(makemazeGolds, i, MakemazeGold, g);
    SKIP_IS_NOT_ALIVE(g);
    if (g->pos.x == p->x && g->pos.y == p->y) {
      exists = true;
    }
  }
  return exists;
}

bool makemazeCheckDownExit(Vector* p) {
  for (int x = 1; x < MAKEMAZE_WALL_SIZE_X - 1; x++) {
    for (int y = 0; y < MAKEMAZE_WALL_SIZE_Y; y++) {
      makemazeWallFills[x][y] = false;
    }
  }
  makemazeWallFills[(int)p->x][(int)p->y] = true;
  TIMES(9, iter) {
    for (int x = 1; x < MAKEMAZE_WALL_SIZE_X - 1; x++) {
      for (int y = 1; y < MAKEMAZE_WALL_SIZE_Y; y++) {
        if (!makemazeWalls[x][y] && !makemazeWallFills[x][y] &&
            (makemazeWallFills[x - 1][y] || makemazeWallFills[x][y - 1])) {
          if (y == MAKEMAZE_WALL_SIZE_Y - 1) {
            return true;
          }
          makemazeWallFills[x][y] = true;
        }
      }
    }
    for (int x = MAKEMAZE_WALL_SIZE_X - 2; x > 0; x--) {
      for (int y = MAKEMAZE_WALL_SIZE_Y - 2; y >= 0; y--) {
        if (!makemazeWalls[x][y] && !makemazeWallFills[x][y] &&
            (makemazeWallFills[x + 1][y] || makemazeWallFills[x][y + 1])) {
          makemazeWallFills[x][y] = true;
        }
      }
    }
  }
  return false;
}

void makemazeUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file (including the
  // helpers above) - every wall/gold/enemy check is direct grid indexing
  // via makemazeWalls[][]/makemazeGolds, so the engine's own O(n^2) hitbox
  // scan (see checkHitBox() in cglp.c) is pure waste here. Restored
  // automatically when the next real game starts, via resetDrawState() in
  // initInGame().
  hasCollision = false;
  if (!ticks) {
    vectorSet(&makemazeAngleOfs[0], 1, 0);
    vectorSet(&makemazeAngleOfs[1], 1, 1);
    vectorSet(&makemazeAngleOfs[2], 0, 1);
    vectorSet(&makemazeAngleOfs[3], -1, 1);
    vectorSet(&makemazeAngleOfs[4], -1, 0);
    vectorSet(&makemazeAngleOfs[5], -1, -1);
    vectorSet(&makemazeAngleOfs[6], 0, -1);
    vectorSet(&makemazeAngleOfs[7], 1, -1);
    TIMES(MAKEMAZE_WALL_SIZE_X, x) {
      TIMES(MAKEMAZE_WALL_SIZE_Y, y) {
        makemazeWalls[x][y] = x == 0 || x == 15 || (y == 10 && (x == 1 || x == 14)) ||
                              (y == 12 && x > 1 && x < 14) || (y == 14 && (x < 7 || x > 8));
      }
    }
    vectorSet(&makemazeWallOfs, 5, -2);
    TIMES(MAKEMAZE_WALL_SIZE_X, x) {
      TIMES(MAKEMAZE_WALL_SIZE_Y, y) { makemazeWallFills[x][y] = false; }
    }
    vectorSet(&makemazePwp, 0, 0);
    INIT_UNALIVED_ARRAY_FAST(makemazeEnemies);
    makemazeEnemyIndex = 0;
    makemazeNextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(makemazeGolds);
    makemazeGoldIndex = 0;
    makemazeGoldMinY = 99;
    makemazeMissScr = 0;
    makemazeMultiplier = 1;
  }
  float scr = 0.01 * sqrt(difficulty) + makemazeMissScr;
  makemazeMissScr *= 0.9;
  float gy = makemazeGoldMinY * 6 + makemazeWallOfs.y;
  if (gy > 50) {
    scr += (gy - 50) * 0.02 * sqrt(difficulty);
  }
  makemazeWallOfs.y -= scr;
  FOR_EACH(makemazeEnemies, i) {
    ASSIGN_ARRAY_ITEM(makemazeEnemies, i, MakemazeEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->scPos.y -= scr;
  }
  if (makemazeWallOfs.y < -2) {
    for (int y = 1; y < MAKEMAZE_WALL_SIZE_Y; y++) {
      for (int x = 0; x < MAKEMAZE_WALL_SIZE_X; x++) {
        makemazeWalls[x][y - 1] = makemazeWalls[x][y];
      }
    }
    for (int x = 0; x < MAKEMAZE_WALL_SIZE_X; x++) {
      makemazeWalls[x][MAKEMAZE_WALL_SIZE_Y - 1] = x == 0 || x == 15;
    }
    ASSIGN_ARRAY_ITEM(makemazeGolds, makemazeGoldIndex, MakemazeGold, ng);
    vectorSet(&ng->pos, rndi(1, MAKEMAZE_WALL_SIZE_X - 1), MAKEMAZE_WALL_SIZE_Y - 1);
    ng->isAlive = true;
    makemazeGoldIndex = cgl_wrap(makemazeGoldIndex + 1, 0, MAKEMAZE_MAX_GOLD_COUNT);
    makemazeWallOfs.y += 6;
    FOR_EACH(makemazeEnemies, i) {
      ASSIGN_ARRAY_ITEM(makemazeEnemies, i, MakemazeEnemy, e);
      SKIP_IS_NOT_ALIVE(e);
      e->pos.y--;
    }
    FOR_EACH(makemazeGolds, i) {
      ASSIGN_ARRAY_ITEM(makemazeGolds, i, MakemazeGold, g);
      SKIP_IS_NOT_ALIVE(g);
      g->pos.y--;
    }
    makemazePwp.y--;
  }
  if (input.isPressed) {
    Vector wp;
    vectorSet(&wp, floor((input.pos.x - (makemazeWallOfs.x - 3)) / 6),
              floor((input.pos.y - (makemazeWallOfs.y - 3)) / 6));
    if (wp.x > 0 && wp.x < MAKEMAZE_WALL_SIZE_X - 1 && wp.y >= 0 && wp.y < MAKEMAZE_WALL_SIZE_Y &&
        !(wp.x == makemazePwp.x && wp.y == makemazePwp.y) && !makemazeCheckGold(&wp)) {
      play(SELECT);
      makemazeWalls[(int)wp.x][(int)wp.y] = !makemazeWalls[(int)wp.x][(int)wp.y];
      makemazePwp = wp;
    }
  } else {
    vectorSet(&makemazePwp, 0, 0);
  }
  color = LIGHT_PURPLE;
  for (int y = 0; y < MAKEMAZE_WALL_SIZE_Y; y++) {
    for (int x = 0; x < MAKEMAZE_WALL_SIZE_X; x++) {
      if (makemazeWalls[x][y]) {
        character("a", makemazeWallOfs.x + x * 6, makemazeWallOfs.y + y * 6, &scratch);
      }
    }
  }
  makemazeNextEnemyTicks--;
  if (makemazeNextEnemyTicks < 0) {
    int x = 1;
    TIMES(99, i) {
      x = rndi(1, MAKEMAZE_WALL_SIZE_X - 1);
      if (!makemazeWalls[x][0]) {
        break;
      }
    }
    Vector pos;
    vectorSet(&pos, x, 0);
    ASSIGN_ARRAY_ITEM(makemazeEnemies, makemazeEnemyIndex, MakemazeEnemy, ne);
    ne->pos = pos;
    ne->angle = 2;
    ne->angleVel = rndi(0, 2) * 2 - 1;
    vectorSet(&ne->scPos, makemazeWallOfs.x + pos.x * 6, makemazeWallOfs.y + pos.y * 6);
    ne->moveInterval = ceil(60 / sqrt(difficulty));
    ne->ticks = 0;
    ne->isAngry = false;
    ne->isAlive = true;
    makemazeEnemyIndex = cgl_wrap(makemazeEnemyIndex + 1, 0, MAKEMAZE_MAX_ENEMY_COUNT);
    makemazeNextEnemyTicks = 150 / sqrt(difficulty);
  }
  FOR_EACH(makemazeEnemies, i) {
    ASSIGN_ARRAY_ITEM(makemazeEnemies, i, MakemazeEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->ticks--;
    if (e->ticks < 0) {
      Vector pp;
      vectorSet(&pp, e->pos.x, e->pos.y);
      makemazeRemoveGold(&e->pos);
      bool isMoving = false;
      if (!makemazeCheckDownExit(&e->pos)) {
        e->isAngry = true;
        e->angle = 2;
        e->pos.y++;
        makemazeRemoveGold(&e->pos);
        if (makemazeWalls[(int)e->pos.x][(int)e->pos.y]) {
          play(POWER_UP);
          makemazeWalls[(int)e->pos.x][(int)e->pos.y] = false;
        }
      } else {
        e->isAngry = false;
        TIMES(99, k) {
          if (makemazeGetWall(&e->pos, e->angle) ||
              makemazeGetWall(&e->pos, e->angle + e->angleVel * 2) ||
              makemazeGetWall(&e->pos, e->angle + e->angleVel * 3) ||
              makemazeGetWall(&e->pos, e->angle + e->angleVel * 4) || e->pos.y < 0 ||
              e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
            break;
          }
          vectorAdd(&e->pos, makemazeAngleOfs[e->angle].x, makemazeAngleOfs[e->angle].y);
          makemazeRemoveGold(&e->pos);
          isMoving = true;
        }
        if (e->pos.y < 0 || e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
          isMoving = true;
        }
        if (!isMoving) {
          TIMES(99, k) {
            int a = (int)cgl_wrap(e->angle + e->angleVel * 2, 0, 8);
            TIMES(4, j) {
              if (!makemazeGetWall(&e->pos, a)) {
                if (k > 0 && a != e->angle) {
                  break;
                }
                vectorAdd(&e->pos, makemazeAngleOfs[a].x, makemazeAngleOfs[a].y);
                makemazeRemoveGold(&e->pos);
                e->angle = a;
                break;
              }
              a = (int)cgl_wrap(a - e->angleVel * 2, 0, 8);
            }
            if (a != e->angle || e->pos.y < 0 || e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
              break;
            }
          }
        }
      }
      if (distanceTo(&pp, e->pos.x, e->pos.y) > 1) {
        play(HIT);
      }
      e->ticks = e->moveInterval;
    }
    Vector target;
    vectorSet(&target, makemazeWallOfs.x + e->pos.x * 6, makemazeWallOfs.y + e->pos.y * 6);
    e->scPos.x += (target.x - e->scPos.x) * 0.1;
    e->scPos.y += (target.y - e->scPos.y) * 0.1;
    if (e->isAngry) {
      color = RED;
    } else if (e->angleVel < 0) {
      color = BLUE;
    } else {
      color = PURPLE;
    }
    characterOptions.rotation = (int)cgl_wrap(e->angle / 2, 0, 4);
    character("b", e->scPos.x, e->scPos.y, &scratch);
    characterOptions.rotation = 0;
    if (e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
      play(EXPLOSION);
      makemazeMissScr++;
      if (makemazeMultiplier > 1) {
        makemazeMultiplier--;
      }
      addScore(-makemazeMultiplier, e->scPos.x, 99);
      particle(e->scPos.x, 110, 19, 2, -CGLP_PI / 2, -CGLP_PI / 8);
    }
    if (e->pos.y < 0 || e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
      e->isAlive = false;
      continue;
    }
  }
  makemazeGoldMinY = 99;
  FOR_EACH(makemazeGolds, i) {
    ASSIGN_ARRAY_ITEM(makemazeGolds, i, MakemazeGold, g);
    SKIP_IS_NOT_ALIVE(g);
    makemazeWalls[(int)g->pos.x][(int)g->pos.y] = false;
    float x = makemazeWallOfs.x + g->pos.x * 6;
    float y = makemazeWallOfs.y + g->pos.y * 6;
    if (y < 30) {
      color = RED;
    } else {
      color = YELLOW;
    }
    character("c", x, y, &scratch);
    if (g->pos.y < makemazeGoldMinY) {
      makemazeGoldMinY = g->pos.y;
    }
    if (y < 0) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      text("X", x, 3, &scratch);
      gameOver();
    }
  }
}

void addGameMakemaze() {
  addGame(makemazeTitle, makemazeDescription, makemazeCharacters,
          makemazeCharactersCount, &makemazeOptions, true, &makemazeUpdate);
}
