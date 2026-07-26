#include "../cglp.h"

int* tapejTitle = "TAPE J";
int* tapejDescription = "[Hold]\n Pull\n[Release]\n Release";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tapejCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int tapejCharactersCount = 0;

Options tapejOptions = {200, 100, 3, false};

#define TAPEJ_GROUND_Y 90

struct TapejRect {
  float x;
  Vector size;
  bool isAlive;
};
#define TAPEJ_MAX_RECT_COUNT 64
TapejRect[TAPEJ_MAX_RECT_COUNT] tapejRects;
int tapejRectIndex;
float tapejNextRectDist;

struct TapejTape {
  Vector from;
  Vector to;
};
// Real order-sensitive queue (front = oldest tape, animated/dismissed
// first) - generous fixed capacity with shift-from-front on completion.
#define TAPEJ_MAX_TAPE_COUNT 64
TapejTape[TAPEJ_MAX_TAPE_COUNT] tapejTapes;
int tapejTapeCount;

#define TAPEJ_HEAD_TYPE_GROUND 0
#define TAPEJ_HEAD_TYPE_UP 1
#define TAPEJ_HEAD_TYPE_TOP 2
#define TAPEJ_HEAD_TYPE_DOWN 3
struct TapejHead {
  Vector from;
  Vector to;
  int type;
  int rectIndex;
};
TapejHead tapejHead;

struct TapejFire {
  Vector pos;
  Vector vel;
  float size;
  bool isAlive;
};
#define TAPEJ_MAX_FIRE_COUNT 64
TapejFire[TAPEJ_MAX_FIRE_COUNT] tapejFires;
int tapejFireIndex;
float tapejNextFireTicks;

float tapejScr;
float tapejDist;

void tapejUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(tapejRects);
    tapejRectIndex = 0;
    tapejNextRectDist = 0;
    tapejTapeCount = 0;
    vectorSet(&tapejHead.from, 100, TAPEJ_GROUND_Y - 1);
    vectorSet(&tapejHead.to, 100, TAPEJ_GROUND_Y - 1);
    tapejHead.type = TAPEJ_HEAD_TYPE_GROUND;
    tapejHead.rectIndex = -1;
    INIT_UNALIVED_ARRAY_FAST(tapejFires);
    tapejFireIndex = 0;
    tapejNextFireTicks = 100;
    tapejScr = 0;
    tapejDist = 0;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    float spd = difficulty;
    tapejDist += spd;
    if (tapejHead.type == TAPEJ_HEAD_TYPE_GROUND) {
      tapejHead.to.x += spd;
    } else if (tapejHead.type == TAPEJ_HEAD_TYPE_UP) {
      tapejHead.to.y -= spd;
      if (tapejHead.to.y < TAPEJ_GROUND_Y - tapejRects[tapejHead.rectIndex].size.y) {
        tapejHead.to.y = TAPEJ_GROUND_Y - tapejRects[tapejHead.rectIndex].size.y;
        if (tapejTapeCount < TAPEJ_MAX_TAPE_COUNT) {
          tapejTapes[tapejTapeCount].from = tapejHead.from;
          tapejTapes[tapejTapeCount].to = tapejHead.to;
          tapejTapeCount++;
        }
        tapejHead.type = TAPEJ_HEAD_TYPE_TOP;
        play(COIN);
        tapejHead.from = tapejHead.to;
      }
    } else if (tapejHead.type == TAPEJ_HEAD_TYPE_TOP) {
      tapejHead.to.x += spd;
      float edgeX =
          tapejRects[tapejHead.rectIndex].x + tapejRects[tapejHead.rectIndex].size.x + 1;
      if (tapejHead.to.x > edgeX) {
        tapejHead.to.x = edgeX;
        if (tapejTapeCount < TAPEJ_MAX_TAPE_COUNT) {
          tapejTapes[tapejTapeCount].from = tapejHead.from;
          tapejTapes[tapejTapeCount].to = tapejHead.to;
          tapejTapeCount++;
        }
        tapejHead.type = TAPEJ_HEAD_TYPE_DOWN;
        play(COIN);
        tapejHead.from = tapejHead.to;
      }
    } else if (tapejHead.type == TAPEJ_HEAD_TYPE_DOWN) {
      tapejHead.to.y += spd;
      if (tapejHead.to.y > TAPEJ_GROUND_Y - 1) {
        tapejHead.to.y = TAPEJ_GROUND_Y - 1;
        if (tapejTapeCount < TAPEJ_MAX_TAPE_COUNT) {
          tapejTapes[tapejTapeCount].from = tapejHead.from;
          tapejTapes[tapejTapeCount].to = tapejHead.to;
          tapejTapeCount++;
        }
        tapejHead.type = TAPEJ_HEAD_TYPE_GROUND;
        play(COIN);
        tapejHead.from = tapejHead.to;
        tapejHead.to.x += 3;
        tapejHead.rectIndex = -1;
      }
    }
  } else {
    if (tapejDist > 0) {
      play(POWER_UP);
      addScore(floor(sqrt(tapejDist) * tapejDist * 0.1 + 1), tapejHead.to.x, tapejHead.to.y);
      tapejDist = 0;
    }
    if (tapejTapeCount > 0) {
      TapejTape* t = &tapejTapes[0];
      t->from.x += (t->to.x - t->from.x) * 0.2 * sqrt(difficulty);
      t->from.y += (t->to.y - t->from.y) * 0.2 * sqrt(difficulty);
      if (distanceTo(&t->from, t->to.x, t->to.y) < 3) {
        memcpy(&tapejTapes[0], &tapejTapes[1], (tapejTapeCount - 1) * sizeof(tapejTapes[0]));
        tapejTapeCount--;
      }
    } else {
      tapejHead.from.x += (tapejHead.to.x - tapejHead.from.x) * 0.2 * sqrt(difficulty);
      tapejHead.from.y += (tapejHead.to.y - tapejHead.from.y) * 0.2 * sqrt(difficulty);
    }
  }
  tapejHead.from.x -= tapejScr;
  if (tapejHead.from.x < 0) {
    tapejHead.from.x = 0;
  }
  if (tapejHead.to.x < 0) {
    play(EXPLOSION);
    gameOver();
  }
  tapejHead.to.x -= tapejScr;
  color = BLACK;
  int ti = 0;
  while (ti < tapejTapeCount) {
    TapejTape* t = &tapejTapes[ti];
    t->from.x -= tapejScr;
    if (t->from.x < 0) {
      t->from.x = 0;
    }
    t->to.x -= tapejScr;
    thickness = 3;
    line(t->from.x, t->from.y, t->to.x, t->to.y, &scratch);
    if (t->to.x < 0) {
      memcpy(&tapejTapes[ti], &tapejTapes[ti + 1], (tapejTapeCount - 1 - ti) * sizeof(tapejTapes[0]));
      tapejTapeCount--;
    } else {
      ti++;
    }
  }
  thickness = 3;
  line(tapejHead.from.x, tapejHead.from.y, tapejHead.to.x, tapejHead.to.y, &scratch);
  color = LIGHT_YELLOW;
  Vector j;
  if (tapejTapeCount > 0) {
    j = tapejTapes[0].from;
  } else {
    j = tapejHead.from;
  }
  box(j.x - 1, j.y - 1, 5, 5, &scratch);
  color = PURPLE;
  box(tapejHead.to.x, tapejHead.to.y, 3, 3, &scratch);
  tapejScr = difficulty * 0.1;
  if (tapejHead.to.x > 50) {
    tapejScr += (tapejHead.to.x - 50) * 0.1;
  }
  tapejNextRectDist -= tapejScr;
  if (tapejNextRectDist < 0) {
    float w = rnd(20, 40);
    ASSIGN_ARRAY_ITEM(tapejRects, tapejRectIndex, TapejRect, nr);
    nr->x = 200;
    vectorSet(&nr->size, w, rnd(10, 50));
    nr->isAlive = true;
    tapejRectIndex = cgl_wrap(tapejRectIndex + 1, 0, TAPEJ_MAX_RECT_COUNT);
    tapejNextRectDist = w + rnd(20, 70);
  }
  color = LIGHT_BLACK;
  rect(0, TAPEJ_GROUND_Y, 200, 100 - TAPEJ_GROUND_Y, &scratch);
  FOR_EACH(tapejRects, ri) {
    ASSIGN_ARRAY_ITEM(tapejRects, ri, TapejRect, r);
    SKIP_IS_NOT_ALIVE(r);
    Collision rc;
    rect(r->x, 90, r->size.x, -r->size.y, &rc);
    if (rc.isColliding.rect[PURPLE] && tapejHead.type == TAPEJ_HEAD_TYPE_GROUND) {
      Vector to;
      vectorSet(&to, r->x - 1, TAPEJ_GROUND_Y - 1);
      if (tapejTapeCount < TAPEJ_MAX_TAPE_COUNT) {
        tapejTapes[tapejTapeCount].from = tapejHead.from;
        tapejTapes[tapejTapeCount].to = to;
        tapejTapeCount++;
      }
      tapejHead.from = to;
      tapejHead.to = to;
      tapejHead.type = TAPEJ_HEAD_TYPE_UP;
      play(COIN);
      tapejHead.rectIndex = ri;
    }
    r->x -= tapejScr;
    if (r->x < -r->size.x) {
      r->isAlive = false;
      continue;
    }
  }
  tapejNextFireTicks--;
  if (tapejNextFireTicks < 0) {
    play(LASER);
    float size = rnd(5, 15);
    ASSIGN_ARRAY_ITEM(tapejFires, tapejFireIndex, TapejFire, nf);
    vectorSet(&nf->pos, rnd(150, 220), -size);
    vectorSet(&nf->vel, -rnd(0.5, 1), rnd(0.8, 1.2));
    vectorMul(&nf->vel, difficulty);
    nf->size = size;
    nf->isAlive = true;
    tapejFireIndex = cgl_wrap(tapejFireIndex + 1, 0, TAPEJ_MAX_FIRE_COUNT);
    tapejNextFireTicks = rnd(40, 60) / difficulty;
  }
  color = RED;
  FOR_EACH(tapejFires, fi) {
    ASSIGN_ARRAY_ITEM(tapejFires, fi, TapejFire, f);
    SKIP_IS_NOT_ALIVE(f);
    vectorAdd(&f->pos, f->vel.x, f->vel.y);
    f->pos.x -= tapejScr;
    Collision cl;
    box(f->pos.x, f->pos.y, f->size, f->size, &cl);
    particle(f->pos.x + rnd(0, f->size / 2) * RNDPM(), f->pos.y + rnd(0, f->size / 2) * RNDPM(),
             rnd(0, 1) * f->size / 2, -vectorLength(&f->vel), vectorAngle(&f->vel), 0.3);
    if (cl.isColliding.rect[BLACK] || cl.isColliding.rect[PURPLE] ||
        cl.isColliding.rect[LIGHT_YELLOW]) {
      play(EXPLOSION);
      gameOver();
    }
    if (cl.isColliding.rect[LIGHT_BLACK] || f->pos.y > TAPEJ_GROUND_Y) {
      play(HIT);
      particle(f->pos.x, f->pos.y, f->size * 5, 1, 0, CGLP_PI * 2);
      f->isAlive = false;
      continue;
    }
  }
}

void addGameTapej() {
  addGame(tapejTitle, tapejDescription, tapejCharacters, tapejCharactersCount, &tapejOptions,
          false, &tapejUpdate);
}
