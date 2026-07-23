#include "../cglp.h"

char* intowTitle = "IN TOW";
char* intowDescription = "[Tap] Multiple jumps";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] intowCharacters = {
    {
        " bbbb ",
        "bbblwb",
        "bbbbyy",
        "  bb  ",
        "bbbb  ",
        " y y  ",
    },
    {
        " bbbb ",
        "bbblwb",
        "bbbbyy",
        "bbbb  ",
        " bbb  ",
        " y y  ",
    },
    {
        "      ",
        "      ",
        " yy   ",
        " yyl  ",
        "yyyy  ",
        " yy   ",
    },
    {
        " rrr l",
        "rrrr l",
        "rrrr l",
        "rrrr l",
        "rrrr l",
        " rrr l",
    }};
int intowCharactersCount = 4;

Options intowOptions = {200, 100, 50, false};

struct IntowBird {
  Vector pos;
  float vy;
  Vector[100] posHistory;
  bool isJumping;
};
IntowBird intowBird;
struct IntowChick {
  float index;
  int targetIndex;
};
#define IN_TOW_MAX_CHICK_COUNT 30
IntowChick[IN_TOW_MAX_CHICK_COUNT] intowChicks;
int intowChickCount;
struct IntowFallingChick {
  Vector pos;
  float vy;
  bool isAlive;
};
#define IN_TOW_MAX_FALLING_CHICK_COUNT 32
IntowFallingChick[IN_TOW_MAX_FALLING_CHICK_COUNT] intowFallingChicks;
int intowFallingChickIndex;
struct IntowFloor {
  Vector pos;
  float width;
  bool hasChick;
  bool isAlive;
};
#define IN_TOW_MAX_FLOOR_COUNT 32
IntowFloor[IN_TOW_MAX_FLOOR_COUNT] intowFloors;
int intowFloorIndex;
float intowNextFloorDist;
struct IntowBullet {
  Vector pos;
  float vx;
  bool isAlive;
};
#define IN_TOW_MAX_BULLET_COUNT 32
IntowBullet[IN_TOW_MAX_BULLET_COUNT] intowBullets;
int intowBulletIndex;
float intowNextBulletDist;
bool intowIsFalling;

void intowUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&intowBird.pos, 64, 32);
    intowBird.vy = 0;
    intowBird.isJumping = true;
    intowChickCount = 0;
    INIT_UNALIVED_ARRAY_FAST(intowFallingChicks);
    intowFallingChickIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(intowFloors);
    ASSIGN_ARRAY_ITEM(intowFloors, intowFloorIndex, IntowFloor, f);
    vectorSet(&f->pos, 70, 70);
    f->width = 90;
    f->hasChick = false;
    f->isAlive = true;
    intowFloorIndex = cgl_wrap(intowFloorIndex + 1, 0, IN_TOW_MAX_FLOOR_COUNT);
    ASSIGN_ARRAY_ITEM(intowFloors, intowFloorIndex, IntowFloor, f2);
    vectorSet(&f2->pos, 150, 50);
    f2->width = 90;
    f2->hasChick = true;
    f2->isAlive = true;
    intowFloorIndex = cgl_wrap(intowFloorIndex + 1, 0, IN_TOW_MAX_FLOOR_COUNT);
    intowNextFloorDist = 0;
    INIT_UNALIVED_ARRAY_FAST(intowBullets);
    intowBulletIndex = 0;
    intowNextBulletDist = 99;
    intowIsFalling = false;
  }
  float scr = sqrt(difficulty);
  if (intowBird.isJumping) {
    if (intowChickCount > 0 && input.isJustPressed) {
      play(JUMP);
      play(HIT);
      intowBird.vy = -2 * sqrt(difficulty);
      intowChickCount--;
      ASSIGN_ARRAY_ITEM(intowFallingChicks, intowFallingChickIndex, IntowFallingChick, fc);
      vectorSet(&fc->pos, intowBird.posHistory[2].x, intowBird.posHistory[2].y);
      fc->vy = 0;
      fc->isAlive = true;
      intowFallingChickIndex =
          cgl_wrap(intowFallingChickIndex + 1, 0, IN_TOW_MAX_FALLING_CHICK_COUNT);
    }
    Vector pp;
    vectorSet(&pp, intowBird.pos.x, intowBird.pos.y);
    float vyStep;
    if (input.isPressed) {
      vyStep = 0.05;
    } else {
      vyStep = 0.2;
    }
    intowBird.vy += vyStep * difficulty;
    intowBird.pos.y += intowBird.vy;
    Vector op;
    vectorSet(&op, intowBird.pos.x, intowBird.pos.y);
    vectorAdd(&op, -pp.x, -pp.y);
    vectorMul(&op, 1.0 / 9);
    color = WHITE;
    TIMES(9, i) {
      vectorAdd(&pp, op.x, op.y);
      box(pp.x, pp.y, 6, 6, &scratch);
    }
  } else {
    if (input.isJustPressed) {
      play(JUMP);
      intowBird.vy = -2 * sqrt(difficulty);
      intowBird.isJumping = true;
    }
  }
  color = BLACK;
  int[2] pc;
  if (intowBird.vy < 0) {
    pc[0] = 'b';
  } else {
    pc[0] = 'a';
  }
  pc[1] = 0;
  character(pc, intowBird.pos.x, intowBird.pos.y, &scratch);
  intowNextFloorDist -= scr;
  if (intowNextFloorDist < 0) {
    float width = rnd(40, 80);
    ASSIGN_ARRAY_ITEM(intowFloors, intowFloorIndex, IntowFloor, f);
    vectorSet(&f->pos, 200 + width / 2, rndi(30, 90));
    f->width = width;
    f->hasChick = true;
    f->isAlive = true;
    intowFloorIndex = cgl_wrap(intowFloorIndex + 1, 0, IN_TOW_MAX_FLOOR_COUNT);
    intowNextFloorDist += width + rnd(10, 30);
  }
  FOR_EACH(intowFloors, i) {
    ASSIGN_ARRAY_ITEM(intowFloors, i, IntowFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->pos.x -= scr;
    color = LIGHT_YELLOW;
    Collision fcl;
    box(f->pos.x, f->pos.y, f->width, 4, &fcl);
    if (intowBird.vy > 0 && fcl.isColliding.rect[WHITE]) {
      intowBird.pos.y = f->pos.y - 5;
      intowBird.isJumping = false;
      intowBird.vy = 0;
    }
    if (f->hasChick) {
      color = BLACK;
      Collision cl;
      character("c", f->pos.x, f->pos.y - 5, &cl);
      if (cl.isColliding.character['a'] || cl.isColliding.character['b']) {
        if (intowChickCount < 30) {
          IntowChick* chick = &(intowChicks[intowChickCount]);
          chick->index = 0;
          chick->targetIndex = 0;
          intowChickCount++;
        }
        play(SELECT);
        addScore(intowChickCount, f->pos.x, f->pos.y - 5);
        f->hasChick = false;
      }
    }
    f->isAlive = f->pos.x >= -f->width / 2;
  }
  for (int i = 98; i >= 0; i--) {
    intowBird.posHistory[i + 1].x = intowBird.posHistory[i].x - scr;
    intowBird.posHistory[i + 1].y = intowBird.posHistory[i].y;
  }
  intowBird.posHistory[0].x = intowBird.pos.x;
  intowBird.posHistory[0].y = intowBird.pos.y;
  color = TRANSPARENT;
  if (!intowBird.isJumping) {
    Collision cl;
    box(intowBird.pos.x, intowBird.pos.y + 4, 9, 2, &cl);
    if (!cl.isColliding.rect[LIGHT_YELLOW]) {
      intowBird.isJumping = true;
    }
  }
  intowNextBulletDist -= scr;
  if (intowNextBulletDist < 0) {
    ASSIGN_ARRAY_ITEM(intowBullets, intowBulletIndex, IntowBullet, b);
    vectorSet(&b->pos, 203, rndi(10, 90));
    b->vx = rnd(1, difficulty) * 0.3;
    b->isAlive = true;
    intowBulletIndex = cgl_wrap(intowBulletIndex + 1, 0, IN_TOW_MAX_BULLET_COUNT);
    intowNextBulletDist += rnd(50, 80) / sqrt(difficulty);
  }
  color = BLACK;
  FOR_EACH(intowBullets, i) {
    ASSIGN_ARRAY_ITEM(intowBullets, i, IntowBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x -= b->vx + scr;
    Collision cl;
    character("d", b->pos.x, b->pos.y, &cl);
    if (cl.isColliding.character['a'] || cl.isColliding.character['b']) {
      play(EXPLOSION);
      if (intowChickCount > 0) {
        intowIsFalling = true;
        intowBird.vy = 3 * sqrt(difficulty);
      } else {
        gameOver();
      }
      b->isAlive = false;
    } else {
      b->isAlive = b->pos.x >= -3;
    }
  }
  color = BLACK;
  bool isHit = intowIsFalling;
  intowIsFalling = false;
  int nextChickCount = -1;
  for (int i = 0; i < intowChickCount; i++) {
    IntowChick* chick = &(intowChicks[i]);
    chick->targetIndex = 3 * (i + 1);
    chick->index += (chick->targetIndex - chick->index) * 0.05;
    Vector p = intowBird.posHistory[(int)(chick->index)];
    Collision cl;
    character("c", p.x, p.y, &cl);
    if (cl.isColliding.character['d']) {
      play(POWER_UP);
      isHit = true;
    }
    if (isHit) {
      ASSIGN_ARRAY_ITEM(intowFallingChicks, intowFallingChickIndex, IntowFallingChick, fc);
      vectorSet(&fc->pos, p.x, p.y);
      fc->vy = 0;
      fc->isAlive = true;
      intowFallingChickIndex =
          cgl_wrap(intowFallingChickIndex + 1, 0, IN_TOW_MAX_FALLING_CHICK_COUNT);
      if (nextChickCount < 0) {
        nextChickCount = i;
      }
    }
  }
  if (nextChickCount >= 0) {
    intowChickCount = nextChickCount;
  }
  FOR_EACH(intowFallingChicks, i) {
    ASSIGN_ARRAY_ITEM(intowFallingChicks, i, IntowFallingChick, fc);
    SKIP_IS_NOT_ALIVE(fc);
    fc->vy += 0.3 * difficulty;
    fc->pos.y += fc->vy;
    characterOptions.isMirrorY = true;
    character("c", fc->pos.x, fc->pos.y, &scratch);
    characterOptions.isMirrorY = false;
    fc->isAlive = fc->pos.y <= 103;
  }
  color = BLACK;
  int[2] ppc;
  if (intowBird.vy < 0) {
    ppc[0] = 'b';
  } else {
    ppc[0] = 'a';
  }
  ppc[1] = 0;
  character(ppc, intowBird.pos.x, intowBird.pos.y, &scratch);
  if (intowBird.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameInTow() {
  addGame(intowTitle, intowDescription, intowCharacters,
          intowCharactersCount, &intowOptions, false, &intowUpdate);
}
