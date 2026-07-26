#include "../cglp.h"

int* splitzigTitle = "SPLITZIG";
int* splitzigDescription = "[Tap] Turn";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] splitzigCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int splitzigCharactersCount = 0;

Options splitzigOptions = {100, 100, 0, false};

struct SplitzigPlayer {
  Vector pos;
  float dir;
  float speed;
  float squash;
  float tilt;
};
SplitzigPlayer splitzigPlayer;

struct SplitzigBullet {
  Vector pos;
  float vy;
  bool isAlive;
};
// Fire interval floor(7/sqrt(difficulty)) plateaus at 1 tick once difficulty
// is high enough; bullet lifetime is a fixed ~28 ticks (vy is a constant -3,
// not difficulty-scaled), so worst case concurrent count plateaus near 28.
#define SPLITZIG_MAX_BULLET_COUNT 64
SplitzigBullet[SPLITZIG_MAX_BULLET_COUNT] splitzigBullets;
int splitzigBulletIndex;

struct SplitzigBlock {
  Vector pos;
  float vx;
  float vy;
  int size;
  float squash;
  float rot;
  bool isAlive;
};
// Sized with large headroom above the worst-case split-chain burst (2 children + 4 grandchildren).
#define SPLITZIG_MAX_BLOCK_COUNT 64
SplitzigBlock[SPLITZIG_MAX_BLOCK_COUNT] splitzigBlocks;
int splitzigBlockIndex;

struct SplitzigTrail {
  Vector pos;
  float life;
  bool isAlive;
};
// Up to ~28 bullets each spawning a trail every 2 ticks with a ~7 tick
// lifetime -> worst case around 90-100 concurrent; sized with headroom.
#define SPLITZIG_MAX_TRAIL_COUNT 200
SplitzigTrail[SPLITZIG_MAX_TRAIL_COUNT] splitzigTrails;
int splitzigTrailIndex;

float splitzigStackHeight;
float splitzigSpawnTimer;

void splitzigUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&splitzigPlayer.pos, 50, 80);
    splitzigPlayer.dir = 1;
    splitzigPlayer.speed = 1.5;
    splitzigPlayer.squash = 1;
    splitzigPlayer.tilt = 0;
    INIT_UNALIVED_ARRAY_FAST(splitzigBullets);
    splitzigBulletIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(splitzigBlocks);
    splitzigBlockIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(splitzigTrails);
    splitzigTrailIndex = 0;
    splitzigStackHeight = 0;
    splitzigSpawnTimer = 0;
  }

  if (input.isJustPressed) {
    splitzigPlayer.dir *= -1;
    splitzigPlayer.squash = 0.5;
    play(SELECT);
    color = CYAN;
    particle(splitzigPlayer.pos.x, splitzigPlayer.pos.y, 4, 1.5, CGLP_PI / 2, CGLP_PI);
  }

  splitzigPlayer.squash += (1 - splitzigPlayer.squash) * 0.15;
  splitzigPlayer.tilt += (splitzigPlayer.dir * 0.3 - splitzigPlayer.tilt) * 0.2;

  splitzigPlayer.pos.x += splitzigPlayer.dir * splitzigPlayer.speed * sqrt(difficulty);
  splitzigPlayer.pos.x = cgl_wrap(splitzigPlayer.pos.x, 0, 100);

  splitzigStackHeight *= 0.998;
  splitzigPlayer.pos.y = 90 - splitzigStackHeight;
  if (splitzigPlayer.pos.y < 0) {
    play(EXPLOSION);
    gameOver();
  }

  int fireInterval = (int)floor(7 / sqrt(difficulty));
  if (fireInterval < 1) {
    fireInterval = 1;
  }
  if (ticks % fireInterval == 0) {
    ASSIGN_ARRAY_ITEM(splitzigBullets, splitzigBulletIndex, SplitzigBullet, nb);
    vectorSet(&nb->pos, splitzigPlayer.pos.x, splitzigPlayer.pos.y - 5);
    nb->vy = -3;
    nb->isAlive = true;
    splitzigBulletIndex = cgl_wrap(splitzigBulletIndex + 1, 0, SPLITZIG_MAX_BULLET_COUNT);
    play(LASER);
  }

  splitzigSpawnTimer++;
  float spawnRate = rnd(88, 111) / sqrt(difficulty);
  if (splitzigSpawnTimer >= spawnRate) {
    splitzigSpawnTimer = 0;
    ASSIGN_ARRAY_ITEM(splitzigBlocks, splitzigBlockIndex, SplitzigBlock, newBlock);
    vectorSet(&newBlock->pos, rnd(15, 85), -10);
    newBlock->vx = rnd(-0.3, 0.3) * sqrt(difficulty);
    newBlock->vy = rnd(0.2, 0.8) * sqrt(difficulty);
    newBlock->size = 2;
    newBlock->squash = 1;
    newBlock->rot = 0;
    newBlock->isAlive = true;
    splitzigBlockIndex = cgl_wrap(splitzigBlockIndex + 1, 0, SPLITZIG_MAX_BLOCK_COUNT);
  }

  FOR_EACH(splitzigTrails, ti) {
    ASSIGN_ARRAY_ITEM(splitzigTrails, ti, SplitzigTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life -= 0.15;
    color = LIGHT_YELLOW;
    box(t->pos.x, t->pos.y, 2 * t->life, 3 * t->life, &scratch);
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
  }

  color = GREEN;
  if (splitzigStackHeight > 0) {
    rect(0, 100 - splitzigStackHeight, 100, splitzigStackHeight, &scratch);
  }

  color = CYAN;
  float pw = 6 * splitzigPlayer.squash;
  float ph = 6 / splitzigPlayer.squash;
  thickness = pw;
  barCenterPosRatio = 0.5;
  bar(splitzigPlayer.pos.x, splitzigPlayer.pos.y, ph, splitzigPlayer.tilt, &scratch);
  float eyeOffX = splitzigPlayer.dir * 1.5;
  color = WHITE;
  box(splitzigPlayer.pos.x - 1.5, splitzigPlayer.pos.y - 1, 2, 2, &scratch);
  box(splitzigPlayer.pos.x + 1.5, splitzigPlayer.pos.y - 1, 2, 2, &scratch);
  color = BLACK;
  box(splitzigPlayer.pos.x - 1.5 + eyeOffX * 0.3, splitzigPlayer.pos.y - 1, 1, 1, &scratch);
  box(splitzigPlayer.pos.x + 1.5 + eyeOffX * 0.3, splitzigPlayer.pos.y - 1, 1, 1, &scratch);

  FOR_EACH(splitzigBullets, bi) {
    ASSIGN_ARRAY_ITEM(splitzigBullets, bi, SplitzigBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    if (ticks % 2 == 0) {
      ASSIGN_ARRAY_ITEM(splitzigTrails, splitzigTrailIndex, SplitzigTrail, nt);
      vectorSet(&nt->pos, b->pos.x, b->pos.y);
      nt->life = 1;
      nt->isAlive = true;
      splitzigTrailIndex = cgl_wrap(splitzigTrailIndex + 1, 0, SPLITZIG_MAX_TRAIL_COUNT);
    }
    b->pos.y += b->vy;
    color = YELLOW;
    box(b->pos.x, b->pos.y, 2, 4, &scratch);
    if (b->pos.y < -5) {
      b->isAlive = false;
      continue;
    }
  }

  FOR_EACH(splitzigBlocks, blki) {
    ASSIGN_ARRAY_ITEM(splitzigBlocks, blki, SplitzigBlock, blk);
    SKIP_IS_NOT_ALIVE(blk);
    blk->pos.x += blk->vx;
    blk->pos.y += blk->vy;

    if ((blk->pos.x < 5 && blk->vx < 0) || (blk->pos.x > 95 && blk->vx > 0)) {
      blk->vx *= -1;
      blk->squash = 0.6;
      color = WHITE;
      float bounceAngle;
      if (blk->vx > 0) {
        bounceAngle = 0;
      } else {
        bounceAngle = CGLP_PI;
      }
      particle(blk->pos.x, blk->pos.y, 3, 1, bounceAngle, CGLP_PI / 2);
    }

    blk->squash += (1 - blk->squash) * 0.1;
    blk->rot += blk->vx * 0.05;

    int blockSize;
    int blockColor;
    int points;
    if (blk->size == 2) {
      blockSize = 12;
      blockColor = RED;
      points = 1;
    } else if (blk->size == 1) {
      blockSize = 8;
      blockColor = PURPLE;
      points = 2;
    } else {
      blockSize = 5;
      blockColor = BLUE;
      points = 3;
    }

    color = blockColor;
    float bw = blockSize * blk->squash;
    float bh = blockSize / blk->squash;
    thickness = bw;
    barCenterPosRatio = 0.5;
    Collision col;
    bar(blk->pos.x, blk->pos.y, bh, blk->rot, &col);

    float dx = splitzigPlayer.pos.x - blk->pos.x;
    float dy = splitzigPlayer.pos.y - blk->pos.y;
    float eyeDir = cgl_atan2(dy, dx);
    float es = blockSize * 0.15;
    color = WHITE;
    box(blk->pos.x - es, blk->pos.y, es * 1.5, es * 1.5, &scratch);
    box(blk->pos.x + es, blk->pos.y, es * 1.5, es * 1.5, &scratch);
    color = BLACK;
    box(blk->pos.x - es + cos(eyeDir) * es * 0.4, blk->pos.y + sin(eyeDir) * es * 0.4,
        es * 0.8, es * 0.8, &scratch);
    box(blk->pos.x + es + cos(eyeDir) * es * 0.4, blk->pos.y + sin(eyeDir) * es * 0.4,
        es * 0.8, es * 0.8, &scratch);

    if (col.isColliding.rect[YELLOW]) {
      play(HIT);
      addScore(points, blk->pos.x, blk->pos.y);
      color = blockColor;
      particle(blk->pos.x, blk->pos.y, 8, 2, 0, CGLP_PI * 2);

      if (blk->size > 0) {
        int newSize = blk->size - 1;
        ASSIGN_ARRAY_ITEM(splitzigBlocks, splitzigBlockIndex, SplitzigBlock, child1);
        vectorSet(&child1->pos, blk->pos.x - 5, blk->pos.y);
        child1->vx = -0.8 - rnd(0, 0.3);
        child1->vy = blk->vy * 0.8;
        child1->size = newSize;
        child1->squash = 0.6;
        child1->rot = blk->rot;
        child1->isAlive = true;
        splitzigBlockIndex = cgl_wrap(splitzigBlockIndex + 1, 0, SPLITZIG_MAX_BLOCK_COUNT);

        ASSIGN_ARRAY_ITEM(splitzigBlocks, splitzigBlockIndex, SplitzigBlock, child2);
        vectorSet(&child2->pos, blk->pos.x + 5, blk->pos.y);
        child2->vx = 0.8 + rnd(0, 0.3);
        child2->vy = blk->vy * 0.8;
        child2->size = newSize;
        child2->squash = 0.6;
        child2->rot = blk->rot;
        child2->isAlive = true;
        splitzigBlockIndex = cgl_wrap(splitzigBlockIndex + 1, 0, SPLITZIG_MAX_BLOCK_COUNT);
      } else {
        play(POWER_UP);
      }
      blk->isAlive = false;
      continue;
    }

    if (col.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
      blk->isAlive = false;
      continue;
    }

    float floorY = 100 - splitzigStackHeight;
    if (blk->pos.y > floorY - blockSize / 2.0) {
      splitzigStackHeight += blockSize / 2.0;
      play(HIT);
      color = blockColor;
      particle(blk->pos.x, blk->pos.y, 6, 1, -CGLP_PI / 2, CGLP_PI);
      blk->isAlive = false;
      continue;
    }
  }

  FOR_EACH(splitzigBullets, bi2) {
    ASSIGN_ARRAY_ITEM(splitzigBullets, bi2, SplitzigBullet, b2);
    SKIP_IS_NOT_ALIVE(b2);
    color = TRANSPARENT;
    Collision bcol;
    box(b2->pos.x, b2->pos.y, 2, 4, &bcol);
    if (bcol.isColliding.rect[RED] || bcol.isColliding.rect[PURPLE] || bcol.isColliding.rect[BLUE]) {
      b2->isAlive = false;
      continue;
    }
  }
}

void addGameSplitzig() {
  addGame(splitzigTitle, splitzigDescription, splitzigCharacters,
          splitzigCharactersCount, &splitzigOptions, false, &splitzigUpdate);
}
