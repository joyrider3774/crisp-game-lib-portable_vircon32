#include "../cglp.h"

int* outpostpatrolTitle = "OUTPOST\nPATROL";
int* outpostpatrolDescription = "[Tap] Jump";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] outpostpatrolCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int outpostpatrolCharactersCount = 0;

Options outpostpatrolOptions = {100, 100, 7, false};

#define OUTPOSTPATROL_TRACK_RADIUS 40
#define OUTPOSTPATROL_JUMP_DURATION 30

struct OutpostpatrolSentry {
  Vector pos;
  float angle;
  bool isJumping;
  float jumpHeight;
};
OutpostpatrolSentry outpostpatrolSentry;
float outpostpatrolJumpTicks;

struct OutpostpatrolObstacle {
  Vector pos;
  float angle;
  bool isAlive;
};
#define OUTPOSTPATROL_MAX_OBSTACLE_COUNT 16
OutpostpatrolObstacle[OUTPOSTPATROL_MAX_OBSTACLE_COUNT] outpostpatrolObstacles;
int outpostpatrolObstacleIndex;
float outpostpatrolNextObstacleTicks;

struct OutpostpatrolBomb {
  Vector pos;
  float angle;
  float height;
  bool isAlive;
};
// 16->1024: bombs never expire on their own (only removed by a precisely-timed
// jump hit) and spawn every ~105/difficulty ticks, so uncleared bombs pile up
// close to linearly with play time - simulation shows the count already blows
// past 16 within the first minute even with zero bombs ever destroyed.
#define OUTPOSTPATROL_MAX_BOMB_COUNT 1024
OutpostpatrolBomb[OUTPOSTPATROL_MAX_BOMB_COUNT] outpostpatrolBombs;
int outpostpatrolBombIndex;
float outpostpatrolNextBombTicks;

struct OutpostpatrolWave {
  float angle;
  float width;
  bool isAlive;
};
#define OUTPOSTPATROL_MAX_WAVE_COUNT 16
OutpostpatrolWave[OUTPOSTPATROL_MAX_WAVE_COUNT] outpostpatrolWaves;
int outpostpatrolWaveIndex;

void outpostpatrolUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&outpostpatrolSentry.pos, 50, 50);
    outpostpatrolSentry.angle = 0;
    outpostpatrolSentry.isJumping = false;
    outpostpatrolSentry.jumpHeight = 0;
    INIT_UNALIVED_ARRAY_FAST(outpostpatrolObstacles);
    outpostpatrolObstacleIndex = 0;
    outpostpatrolNextObstacleTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(outpostpatrolBombs);
    outpostpatrolBombIndex = 0;
    outpostpatrolNextBombTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(outpostpatrolWaves);
    outpostpatrolWaveIndex = 0;
  }
  outpostpatrolSentry.angle += 0.02 * difficulty;
  vectorSet(&outpostpatrolSentry.pos, 50, 50);
  Vector sentryOfs;
  vectorSet(&sentryOfs, OUTPOSTPATROL_TRACK_RADIUS, 0);
  rotate(&sentryOfs, outpostpatrolSentry.angle);
  vectorAdd(&outpostpatrolSentry.pos, sentryOfs.x, sentryOfs.y);
  if (input.isJustPressed && !outpostpatrolSentry.isJumping) {
    play(JUMP);
    outpostpatrolSentry.isJumping = true;
    outpostpatrolJumpTicks = 0;
  }
  if (outpostpatrolSentry.isJumping) {
    float jumpAdd;
    if (input.isPressed) {
      jumpAdd = 1;
    } else {
      jumpAdd = 2;
    }
    outpostpatrolJumpTicks += jumpAdd * difficulty;
    outpostpatrolSentry.jumpHeight =
        10 * sin(outpostpatrolJumpTicks / OUTPOSTPATROL_JUMP_DURATION * CGLP_PI);
    if (outpostpatrolJumpTicks >= OUTPOSTPATROL_JUMP_DURATION) {
      play(HIT);
      outpostpatrolSentry.isJumping = false;
      outpostpatrolSentry.jumpHeight = 0;
    }
  }
  outpostpatrolNextObstacleTicks -= difficulty;
  if (outpostpatrolNextObstacleTicks < 0) {
    float angle = outpostpatrolSentry.angle + CGLP_PI;
    float minAo = CGLP_PI;
    FOR_EACH(outpostpatrolObstacles, i) {
      ASSIGN_ARRAY_ITEM(outpostpatrolObstacles, i, OutpostpatrolObstacle, ob);
      SKIP_IS_NOT_ALIVE(ob);
      float ao = fabs(cgl_wrap(ob->angle - angle, -CGLP_PI, CGLP_PI));
      if (ao < minAo) {
        minAo = ao;
      }
    }
    if (minAo > CGLP_PI * 0.2) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(outpostpatrolObstacles, outpostpatrolObstacleIndex, OutpostpatrolObstacle,
                         no);
      vectorSet(&no->pos, 50, 50);
      no->angle = angle;
      no->isAlive = true;
      outpostpatrolObstacleIndex =
          cgl_wrap(outpostpatrolObstacleIndex + 1, 0, OUTPOSTPATROL_MAX_OBSTACLE_COUNT);
    }
    outpostpatrolNextObstacleTicks += rnd(60, 150);
  }
  FOR_EACH(outpostpatrolObstacles, i) {
    ASSIGN_ARRAY_ITEM(outpostpatrolObstacles, i, OutpostpatrolObstacle, ob);
    SKIP_IS_NOT_ALIVE(ob);
    ob->angle -= 0.01 * difficulty;
    vectorSet(&ob->pos, 50, 50);
    Vector ofs;
    vectorSet(&ofs, OUTPOSTPATROL_TRACK_RADIUS, 0);
    rotate(&ofs, ob->angle);
    vectorAdd(&ob->pos, ofs.x, ofs.y);
  }
  outpostpatrolNextBombTicks -= difficulty;
  if (outpostpatrolNextBombTicks < 0) {
    play(CLICK);
    ASSIGN_ARRAY_ITEM(outpostpatrolBombs, outpostpatrolBombIndex, OutpostpatrolBomb, nb);
    vectorSet(&nb->pos, 50, 50);
    nb->angle = rnd(0, 2 * CGLP_PI);
    nb->height = 10;
    nb->isAlive = true;
    outpostpatrolBombIndex = cgl_wrap(outpostpatrolBombIndex + 1, 0, OUTPOSTPATROL_MAX_BOMB_COUNT);
    outpostpatrolNextBombTicks += rnd(90, 120);
  }
  FOR_EACH(outpostpatrolBombs, i) {
    ASSIGN_ARRAY_ITEM(outpostpatrolBombs, i, OutpostpatrolBomb, b);
    SKIP_IS_NOT_ALIVE(b);
    b->angle -= 0.015 * difficulty;
    vectorSet(&b->pos, 50, 50);
    Vector ofs;
    vectorSet(&ofs, OUTPOSTPATROL_TRACK_RADIUS, 0);
    rotate(&ofs, b->angle);
    vectorAdd(&b->pos, ofs.x, ofs.y);
  }
  color = LIGHT_BLACK;
  thickness = 3;
  arc(50, 50, OUTPOSTPATROL_TRACK_RADIUS, 0, CGLP_PI * 2, &scratch);
  color = YELLOW;
  FOR_EACH(outpostpatrolWaves, i) {
    ASSIGN_ARRAY_ITEM(outpostpatrolWaves, i, OutpostpatrolWave, w);
    SKIP_IS_NOT_ALIVE(w);
    w->width += 0.1 * difficulty;
    thickness = 4;
    arc(50, 50, OUTPOSTPATROL_TRACK_RADIUS, w->angle - w->width / 2, w->angle + w->width / 2,
        &scratch);
    if (w->width > 1) {
      w->isAlive = false;
    }
  }
  color = RED;
  FOR_EACH(outpostpatrolObstacles, i) {
    ASSIGN_ARRAY_ITEM(outpostpatrolObstacles, i, OutpostpatrolObstacle, ob);
    SKIP_IS_NOT_ALIVE(ob);
    box(ob->pos.x, ob->pos.y, 5, 5, &scratch);
    if (scratch.isColliding.rect[YELLOW]) {
      play(COIN);
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      ob->isAlive = false;
      continue;
    }
  }
  if (outpostpatrolSentry.isJumping) {
    color = CYAN;
  } else {
    color = BLUE;
  }
  box(outpostpatrolSentry.pos.x, outpostpatrolSentry.pos.y - outpostpatrolSentry.jumpHeight, 7, 7,
      &scratch);
  if (scratch.isColliding.rect[RED] && !outpostpatrolSentry.isJumping) {
    play(EXPLOSION);
    gameOver();
  }
  color = YELLOW;
  FOR_EACH(outpostpatrolBombs, i) {
    ASSIGN_ARRAY_ITEM(outpostpatrolBombs, i, OutpostpatrolBomb, b);
    SKIP_IS_NOT_ALIVE(b);
    box(b->pos.x, b->pos.y - b->height, 6, 6, &scratch);
    if ((scratch.isColliding.rect[BLUE] || scratch.isColliding.rect[CYAN]) &&
        outpostpatrolSentry.isJumping) {
      play(POWER_UP);
      ASSIGN_ARRAY_ITEM(outpostpatrolWaves, outpostpatrolWaveIndex, OutpostpatrolWave, nw);
      nw->angle = b->angle;
      nw->width = 0;
      nw->isAlive = true;
      outpostpatrolWaveIndex = cgl_wrap(outpostpatrolWaveIndex + 1, 0, OUTPOSTPATROL_MAX_WAVE_COUNT);
      b->isAlive = false;
      continue;
    }
  }
}

void addGameOutpostpatrol() {
  addGame(outpostpatrolTitle, outpostpatrolDescription, outpostpatrolCharacters,
          outpostpatrolCharactersCount, &outpostpatrolOptions, false,
          &outpostpatrolUpdate);
}
