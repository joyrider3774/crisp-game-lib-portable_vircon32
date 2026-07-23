#include "../cglp.h"

char* castnTitle = "CAST N";
char* castnDescription =
    "[Hold]    Select power\n[Release] Cast\n[Tap]     Pull";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] castnCharacters = {
    {
        "      ",
        "  YYY ",
        " YyyyY",
        " YyyyY",
        " YyyyY",
        "  YYY ",
    },
    {
        "      ",
        "  lll ",
        "l ll l",
        "llllll",
        "l llll",
        "  ll  ",
    },
    {
        "      ",
        "llll  ",
        "llllll",
        "l l   ",
        "   lll",
        "      ",
    },
};
int castnCharactersCount = 2;

Options castnOptions = {150, 100, 2, false};

struct CastnNode {
  Vector pos;
  Vector vel;
  CastnNode* nextNode;
};
#define CAST_N_NODES_COUNT 20
CastnNode[CAST_N_NODES_COUNT] castnNodes;
// 0:ready, 1:angle, 2:throw, 3:pull
int castnNodeState;
float castnThrowPower;
struct CastnFish {
  Vector pos;
  Vector vel;
  int type;
  bool isAlive;
};
#define CAST_N_MAX_FISH_COUNT 80
CastnFish[CAST_N_MAX_FISH_COUNT] castnFishes;
int castnFishIndex;
int castnNextFishTicks;
int castnNextRedFishCount;
float castnWaterY;
int castnMultiplier;
Vector castnStartPos;
float castnNodeDist = 9;

void castnUpdate() {
  Collision scratch;
  // difficulty doesn't change during a single castnUpdate() call, so
  // sqrt(difficulty) is computed once here and reused everywhere below
  // instead of being recomputed on every use - most importantly, it was
  // being recomputed for every single live fish, every frame, in the
  // per-fish loop further down (up to CAST_N_MAX_FISH_COUNT=80 times),
  // when the exact same value works for all of them.
  float sd = sqrt(difficulty);
  if (!ticks) {
    vectorSet(&castnStartPos, 5, 20);
    TIMES(CAST_N_NODES_COUNT, i) {
      CastnNode* n = &castnNodes[i];
      vectorSet(&n->pos, castnStartPos.x, castnStartPos.y);
      vectorSet(&n->vel, 0, 0);
    }
    TIMES(CAST_N_NODES_COUNT - 1, i) { castnNodes[i + 1].nextNode = &castnNodes[i]; }
    castnNodeState = 0;
    castnThrowPower = 0;
    INIT_UNALIVED_ARRAY_FAST(castnFishes);
    castnFishIndex = 0;
    castnNextFishTicks = 0;
    castnNextRedFishCount = 1;
    castnWaterY = 40;
    castnMultiplier = 1;
  }
  color = LIGHT_YELLOW;
  thickness = 3;
  line(0, castnStartPos.y + 3, 150, 120, &scratch);
  color = LIGHT_BLUE;
  rect(0, castnWaterY, 150, 3, &scratch);
  color = BLACK;
  character("a", castnNodes[0].pos.x, castnNodes[0].pos.y, &scratch);
  if (castnNodeState != 3 && castnWaterY < castnStartPos.y - 4) {
    play(EXPLOSION);
    gameOver();
  }
  if (castnNodeState == 0) {
    if (input.isJustPressed) {
      play(SELECT);
      castnThrowPower = 1;
      castnNodeState = 1;
      castnMultiplier = 1;
    }
  }
  thickness = 2;
  if (castnNodeState == 1) {
    castnThrowPower += 0.05 * sd;
    float a = 0.1 - castnThrowPower * 0.2;
    Vector p;
    addWithAngle(vectorSet(&p, castnStartPos.x, castnStartPos.y), a, castnThrowPower * 5 + 3);
    line(castnStartPos.x, castnStartPos.y, p.x, p.y, &scratch);
    if (input.isJustReleased || castnThrowPower > 3) {
      play(JUMP);
      castnThrowPower = clamp(castnThrowPower, 1, 3);
      castnNodeState = 2;
      rotate(vectorSet(&castnNodes[0].vel, sd * castnThrowPower, 0), a);
    }
  }
  if (castnNodeState == 2) {
    FOR_EACH(castnNodes, i) {
      ASSIGN_ARRAY_ITEM(castnNodes, i, CastnNode, n);
      n->pos.x = clamp(n->pos.x, 0, 147);
      if (i == 0) {
        Collision ncl;
        character("a", n->pos.x, n->pos.y, &ncl);
        if (!ncl.isColliding.rect[LIGHT_YELLOW]) {
          float py = n->pos.y;
          vectorAdd(&n->pos, n->vel.x, n->vel.y);
          if (py < castnWaterY && n->pos.y >= castnWaterY) {
            n->vel.x = 0;
            n->vel.y *= 0.1;
          }
        }
        float vyStep;
        if (n->pos.y < castnWaterY) {
          vyStep = 0.05;
        } else {
          vyStep = 0.01;
        }
        n->vel.y += vyStep * difficulty;
        vectorMul(&n->vel, 0.99);
      } else {
        Collision lcl;
        line(n->nextNode->pos.x, n->nextNode->pos.y, n->pos.x, n->pos.y, &lcl);
        if (!lcl.isColliding.rect[LIGHT_YELLOW]) {
          float d = distanceTo(&n->pos, n->nextNode->pos.x, n->nextNode->pos.y);
          if (d > castnNodeDist) {
            float a2 = angleTo(&n->nextNode->pos, n->pos.x, n->pos.y);
            addWithAngle(vectorSet(&n->pos, n->nextNode->pos.x, n->nextNode->pos.y), a2,
                         castnNodeDist);
          }
          vectorAdd(&n->pos, n->vel.x, n->vel.y);
          float vyStep2;
          if (n->pos.y < castnWaterY) {
            vyStep2 = 0.005;
          } else {
            vyStep2 = 0.001;
          }
          n->vel.y += vyStep2 * difficulty;
          vectorMul(&n->vel, 0.99);
        }
      }
    }
    if (input.isJustPressed) {
      play(POWER_UP);
      castnNodeState = 3;
    }
  }
  if (castnNodeState == 3) {
    FOR_EACH(castnNodes, i) {
      ASSIGN_ARRAY_ITEM(castnNodes, i, CastnNode, n);
      vectorSet(&n->vel, 0, 0);
      if (distanceTo(&n->pos, castnStartPos.x, castnStartPos.y) > 1 && n->pos.x > castnStartPos.x) {
        float a = angleTo(&n->pos, castnStartPos.x, castnStartPos.y);
        addWithAngle(&n->pos, a, sd * 2);
      } else {
        vectorSet(&n->pos, castnStartPos.x, castnStartPos.y);
        if (i == 0) {
          castnNodeState = 0;
        }
      }
      if (i > 0) {
        line(n->nextNode->pos.x, n->nextNode->pos.y, n->pos.x, n->pos.y, &scratch);
      }
    }
  }
  COUNT_IS_ALIVE(castnFishes, fc);
  if (fc == 0) {
    castnNextFishTicks = 0;
  }
  castnNextFishTicks--;
  if (castnNextFishTicks < 0) {
    castnNextRedFishCount--;
    int type = 1;
    if (castnNextRedFishCount < 0) {
      type = 0;
      castnNextRedFishCount = rndi(1, 7);
    }
    int c = rndi(3, 8);
    Vector cp;
    float cpYOffset;
    if (type == 0) {
      cpYOffset = 19;
    } else {
      cpYOffset = 9;
    }
    vectorSet(&cp, 153, rnd(castnWaterY + cpYOffset, 90));
    float vxFactor;
    if (type == 0) {
      vxFactor = 0.3;
    } else {
      vxFactor = 0.2;
    }
    float vx = -rnd(1, sd) * vxFactor;
    TIMES(c, i) {
      ASSIGN_ARRAY_ITEM(castnFishes, castnFishIndex, CastnFish, f);
      Vector p;
      vectorAdd(vectorSet(&p, cp.x, cp.y), rnd(0, 20), rnd(0, 9) * RNDPM());
      p.x = clamp(p.x, 153, 180);
      p.y = clamp(p.y, castnWaterY + 4, 96);
      vectorSet(&f->pos, p.x, p.y);
      vectorSet(&f->vel, vx, 0);
      f->type = type;
      f->isAlive = true;
      castnFishIndex = cgl_wrap(castnFishIndex + 1, 0, CAST_N_MAX_FISH_COUNT);
    }
    castnNextFishTicks = rnd(120, 150) / sd;
  }
  FOR_EACH(castnFishes, i) {
    ASSIGN_ARRAY_ITEM(castnFishes, i, CastnFish, f);
    SKIP_IS_NOT_ALIVE(f);
    if (f->type == 0) {
      color = RED;
    } else {
      color = BLUE;
    }
    characterOptions.isMirrorX = f->vel.x < 0;
    int[2] fishChar;
    fishChar[0] = 'c' - f->type;
    fishChar[1] = 0;
    Collision fcl;
    character(fishChar, f->pos.x, f->pos.y, &fcl);
    Collisions c = fcl.isColliding;
    if (c.rect[BLACK] || c.character['a']) {
      if (castnNodeState == 3) {
        float a = angleTo(&f->pos, castnStartPos.x, castnStartPos.y);
        addWithAngle(&f->pos, a, sd * 2);
        f->pos.y -= difficulty * 0.3;
        if (f->pos.x < castnStartPos.x + 9) {
          int scoreDelta;
          if (f->type == 0) {
            scoreDelta = -1;
          } else {
            scoreDelta = castnMultiplier;
          }
          float scoreYOffset;
          if (f->type == 0) {
            scoreYOffset = 9;
          } else {
            scoreYOffset = 0;
          }
          addScore(scoreDelta,
                   castnStartPos.x + clamp(castnMultiplier * 6, 0, 140),
                   castnStartPos.y + scoreYOffset);
          castnMultiplier++;
          float waterDelta;
          if (f->type == 0) {
            waterDelta = -5;
          } else {
            waterDelta = 1;
          }
          castnWaterY =
              clamp(castnWaterY + waterDelta * sd, 0, 50);
          if (f->type == 0) {
            play(HIT);
          } else {
            play(COIN);
          }
          f->isAlive = false;
          continue;
        }
      }
      if (!c.rect[LIGHT_YELLOW]) {
        f->pos.y += difficulty * 0.3;
      }
      if (rnd(0, 1) < 0.1) {
        f->vel.x *= -1;
      }
      continue;
    }
    if (f->pos.y < castnWaterY + 3) {
      f->vel.y += 0.1;
    } else {
      f->vel.y = 0;
    }
    vectorAdd(&f->pos, f->vel.x, f->vel.y);
    f->vel.x += rnd(0, sd) * RNDPM() * 0.01;
    if (c.rect[LIGHT_YELLOW] && f->vel.x < 0) {
      f->vel.x *= -1;
    }
    f->isAlive = !((f->vel.x > 0 && f->pos.x > 153) || f->pos.x < -3);
  }
  characterOptions.isMirrorX = false;
  castnWaterY -= sd * 0.01;
}

void addGameCastN() {
  addGame(castnTitle, castnDescription, castnCharacters,
          castnCharactersCount, &castnOptions, false, &castnUpdate);
}
