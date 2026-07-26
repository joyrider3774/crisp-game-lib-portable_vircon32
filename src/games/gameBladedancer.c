#include "../cglp.h"

int* bladedancerTitle = "BLADE DANCER";
int* bladedancerDescription = "[Tap]\n Jump & Slash";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bladedancerCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int bladedancerCharactersCount = 0;

Options bladedancerOptions = {200, 100, 5, false};

#define BLADEDANCER_GROUND_Y 80
#define BLADEDANCER_JUMP_DURATION 30
#define BLADEDANCER_JUMP_HEIGHT 30
#define BLADEDANCER_SCROLL_SPEED 1
#define BLADEDANCER_SWORD_ROTATION_SPEED 0.3
#define BLADEDANCER_MAX_SWORD_LENGTH 30

struct BladedancerPlayer {
  Vector pos;
  bool isJumping;
  float jumpTicks;
  float swordAngle;
  float swordLength;
};
BladedancerPlayer bladedancerPlayer;

struct BladedancerEnemy {
  Vector pos;
  float size;
  bool isAlive;
};
#define BLADEDANCER_MAX_ENEMY_COUNT 32
BladedancerEnemy[BLADEDANCER_MAX_ENEMY_COUNT] bladedancerEnemies;
int bladedancerEnemyIndex;

struct BladedancerSegment {
  float x;
  float width;
};
#define BLADEDANCER_MAX_SEGMENT_COUNT 16
BladedancerSegment[BLADEDANCER_MAX_SEGMENT_COUNT] bladedancerSegments;
int bladedancerSegmentHead;
int bladedancerSegmentCount;

float bladedancerEnemySpawnChance;
float bladedancerGapFrequency;
int bladedancerMultiplier;

void bladedancerUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&bladedancerPlayer.pos, 35, BLADEDANCER_GROUND_Y);
    bladedancerPlayer.isJumping = false;
    bladedancerPlayer.jumpTicks = 0;
    bladedancerPlayer.swordAngle = CGLP_PI_2;
    bladedancerPlayer.swordLength = 10;
    INIT_UNALIVED_ARRAY_FAST(bladedancerEnemies);
    bladedancerEnemyIndex = 0;
    bladedancerSegments[0].x = 0;
    bladedancerSegments[0].width = 200;
    bladedancerSegmentHead = 0;
    bladedancerSegmentCount = 1;
    bladedancerEnemySpawnChance = 0.03;
    bladedancerGapFrequency = 0.3;
    bladedancerMultiplier = 1;
  }
  if (input.isJustPressed && !bladedancerPlayer.isJumping) {
    bladedancerPlayer.isJumping = true;
    bladedancerPlayer.jumpTicks = BLADEDANCER_JUMP_DURATION;
    play(POWER_UP);
    bladedancerMultiplier = 1;
  }
  if (bladedancerPlayer.isJumping) {
    if (input.isPressed) {
      bladedancerPlayer.jumpTicks -= 0.5;
    } else {
      bladedancerPlayer.jumpTicks -= 1;
    }
    float jumpProgress = bladedancerPlayer.jumpTicks / BLADEDANCER_JUMP_DURATION;
    bladedancerPlayer.pos.y =
        BLADEDANCER_GROUND_Y - sin(jumpProgress * CGLP_PI) * BLADEDANCER_JUMP_HEIGHT;
    bladedancerPlayer.swordAngle -= BLADEDANCER_SWORD_ROTATION_SPEED;
    bladedancerPlayer.swordLength =
        8 + (1 - jumpProgress) * (BLADEDANCER_MAX_SWORD_LENGTH - 8);
    if (bladedancerPlayer.jumpTicks <= 0) {
      play(CLICK);
      bladedancerPlayer.isJumping = false;
      bladedancerPlayer.pos.y = BLADEDANCER_GROUND_Y;
      bladedancerPlayer.swordAngle = CGLP_PI_2;
      bladedancerPlayer.swordLength = 10;
    }
  }
  color = BLUE;
  box(bladedancerPlayer.pos.x, bladedancerPlayer.pos.y, 4, 8, &scratch);
  color = CYAN;
  Vector swordTip;
  vectorSet(&swordTip, bladedancerPlayer.swordLength, 0);
  rotate(&swordTip, bladedancerPlayer.swordAngle);
  if (bladedancerPlayer.isJumping) {
    thickness = 5;
  } else {
    thickness = 2;
  }
  line(bladedancerPlayer.pos.x, bladedancerPlayer.pos.y - 4,
       bladedancerPlayer.pos.x + swordTip.x, bladedancerPlayer.pos.y - 4 + swordTip.y,
       &scratch);
  COUNT_IS_ALIVE(bladedancerEnemies, aliveEnemyCount);
  if (aliveEnemyCount == 0 || rnd(0, 1) < bladedancerEnemySpawnChance) {
    ASSIGN_ARRAY_ITEM(bladedancerEnemies, bladedancerEnemyIndex, BladedancerEnemy, ne);
    vectorSet(&ne->pos, 205, rnd(30, BLADEDANCER_GROUND_Y - 5));
    ne->size = rnd(3, 6);
    ne->isAlive = true;
    bladedancerEnemyIndex = cgl_wrap(bladedancerEnemyIndex + 1, 0, BLADEDANCER_MAX_ENEMY_COUNT);
  }
  FOR_EACH(bladedancerEnemies, i) {
    ASSIGN_ARRAY_ITEM(bladedancerEnemies, i, BladedancerEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.x -= BLADEDANCER_SCROLL_SPEED * sqrt(difficulty);
    color = RED;
    box(e->pos.x, e->pos.y, e->size, e->size, &scratch);
    bool isCollidingWithSword = scratch.isColliding.rect[CYAN];
    if (isCollidingWithSword) {
      play(HIT);
      addScore(bladedancerMultiplier, e->pos.x, e->pos.y);
      bladedancerMultiplier++;
      e->isAlive = false;
      continue;
    }
    box(e->pos.x, e->pos.y, e->size, e->size, &scratch);
    if (scratch.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
    if (e->pos.x < -10) {
      e->isAlive = false;
      continue;
    }
  }
  color = GREEN;
  TIMES(bladedancerSegmentCount, k) {
    int idx = (bladedancerSegmentHead + k) % BLADEDANCER_MAX_SEGMENT_COUNT;
    BladedancerSegment* segment = &bladedancerSegments[idx];
    segment->x -= BLADEDANCER_SCROLL_SPEED * sqrt(difficulty);
    rect(segment->x, BLADEDANCER_GROUND_Y, segment->width - 2, 20, &scratch);
    if (segment->x + segment->width < 200 && k == bladedancerSegmentCount - 1) {
      int newIdx = (bladedancerSegmentHead + bladedancerSegmentCount) % BLADEDANCER_MAX_SEGMENT_COUNT;
      if (rnd(0, 1) < bladedancerGapFrequency) {
        float gapWidth = rnd(15, 30);
        bladedancerSegments[newIdx].x = segment->x + segment->width + gapWidth;
        bladedancerSegments[newIdx].width = rnd(30, 100);
      } else {
        bladedancerSegments[newIdx].x = segment->x + segment->width;
        bladedancerSegments[newIdx].width = rnd(50, 150);
      }
      bladedancerSegmentCount++;
    }
  }
  while (bladedancerSegmentCount > 0) {
    BladedancerSegment* segment = &bladedancerSegments[bladedancerSegmentHead];
    if (segment->x + segment->width < 0) {
      bladedancerSegmentHead = (bladedancerSegmentHead + 1) % BLADEDANCER_MAX_SEGMENT_COUNT;
      bladedancerSegmentCount--;
    } else {
      break;
    }
  }
  TIMES(bladedancerSegmentCount, k) {
    int idx = (bladedancerSegmentHead + k) % BLADEDANCER_MAX_SEGMENT_COUNT;
    BladedancerSegment* segment = &bladedancerSegments[idx];
    if (!bladedancerPlayer.isJumping && bladedancerPlayer.pos.x > segment->x &&
        bladedancerPlayer.pos.x < segment->x + segment->width) {
      bladedancerPlayer.pos.y = BLADEDANCER_GROUND_Y;
    }
  }
  bool isOverGap = true;
  TIMES(bladedancerSegmentCount, k) {
    int idx = (bladedancerSegmentHead + k) % BLADEDANCER_MAX_SEGMENT_COUNT;
    BladedancerSegment* segment = &bladedancerSegments[idx];
    if (bladedancerPlayer.pos.x >= segment->x && bladedancerPlayer.pos.x <= segment->x + segment->width) {
      isOverGap = false;
    }
  }
  if (isOverGap && !bladedancerPlayer.isJumping) {
    play(EXPLOSION);
    gameOver();
  }
  bladedancerEnemySpawnChance = clamp(0.03 + difficulty * 0.01, 0.02, 0.1);
  bladedancerGapFrequency = clamp(0.3 + difficulty * 0.02, 0.3, 0.7);
}

void addGameBladedancer() {
  addGame(bladedancerTitle, bladedancerDescription, bladedancerCharacters,
          bladedancerCharactersCount, &bladedancerOptions, false,
          &bladedancerUpdate);
}
