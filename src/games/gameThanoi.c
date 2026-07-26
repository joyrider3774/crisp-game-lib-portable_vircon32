#include "../cglp.h"

int* thanoiTitle = "T HANOI";
int* thanoiDescription = "[Tap]\n Take/Place disk\n[Hold]\n Shorten rod";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] thanoiCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int thanoiCharactersCount = 1;

Options thanoiOptions = {100, 100, 5, false};

#define THANOI_BAR_COUNT 3
#define THANOI_DISK_WIDTH 7
#define THANOI_DISK_HEIGHT 6
#define THANOI_MAX_DISK_COUNT 32

struct ThanoiRod {
  int[THANOI_MAX_DISK_COUNT] disks;
  int diskCount;
  float height;
};
ThanoiRod[THANOI_BAR_COUNT] thanoiRods;
float thanoiNextDiskTicks;
int thanoiSelectedRodIndex;
int thanoiHoldTicks;
float thanoiMultiplier;

void thanoiUnshift(ThanoiRod* r, int value) {
  for (int i = r->diskCount; i > 0; i--) {
    r->disks[i] = r->disks[i - 1];
  }
  r->disks[0] = value;
  r->diskCount++;
}

void thanoiPush(ThanoiRod* r, int value) {
  r->disks[r->diskCount] = value;
  r->diskCount++;
}

int thanoiPop(ThanoiRod* r) {
  r->diskCount--;
  return r->disks[r->diskCount];
}

void thanoiUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - the tapped rod
  // is computed directly from input.pos via grid math (see the "i =
  // clamp(floor(input.pos.x / ...))" lines below), so the engine's own
  // O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure waste here.
  // Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    TIMES(THANOI_BAR_COUNT, i) {
      thanoiRods[i].diskCount = 0;
      thanoiRods[i].height = 90;
    }
    thanoiNextDiskTicks = 0;
    thanoiSelectedRodIndex = -1;
    thanoiHoldTicks = 0;
    thanoiMultiplier = 1;
  }
  if (thanoiRods[0].diskCount == 0) {
    thanoiNextDiskTicks = 0;
  }
  thanoiNextDiskTicks--;
  if (thanoiNextDiskTicks < 0) {
    play(LASER);
    thanoiUnshift(&thanoiRods[0], rndi(1, 5));
    thanoiNextDiskTicks = (99 * sqrt(thanoiRods[0].diskCount)) / sqrt(difficulty);
  }
  if (input.isJustPressed) {
    int i = (int)clamp(floor(input.pos.x / (100.0 / THANOI_BAR_COUNT)), 0, THANOI_BAR_COUNT);
    if (thanoiSelectedRodIndex < 0) {
      if (thanoiRods[i].diskCount > 0) {
        play(SELECT);
        thanoiSelectedRodIndex = i;
      }
    } else {
      ThanoiRod* fb = &thanoiRods[thanoiSelectedRodIndex];
      ThanoiRod* tb = &thanoiRods[i];
      int fp = fb->disks[fb->diskCount - 1];
      if (i == thanoiSelectedRodIndex) {
        play(HIT);
      } else if (tb->diskCount == 0 || i == 0) {
        play(COIN);
        thanoiPop(fb);
        thanoiPush(tb, fp);
      } else {
        int tp = tb->disks[tb->diskCount - 1];
        if (fp <= tp) {
          play(COIN);
          thanoiPop(fb);
          thanoiPush(tb, fp);
        } else {
          play(HIT);
        }
      }
      thanoiSelectedRodIndex = -1;
    }
  }
  if (input.isPressed) {
    thanoiHoldTicks++;
    int i = (int)clamp(floor(input.pos.x / (100.0 / THANOI_BAR_COUNT)), 0, THANOI_BAR_COUNT);
    if (i > 0) {
      float h = (thanoiHoldTicks * thanoiHoldTicks) / 10000.0;
      thanoiRods[i].height -= h;
      if (h > 0.5) {
        play(JUMP);
      }
    }
  } else {
    thanoiHoldTicks = 0;
  }
  color = LIGHT_BLACK;
  rect(0, 90, 100, 10, &scratch);
  if (thanoiSelectedRodIndex >= 0) {
    color = LIGHT_CYAN;
    rect((thanoiSelectedRodIndex * 100.0) / THANOI_BAR_COUNT, 0,
         100.0 / THANOI_BAR_COUNT, 90, &scratch);
  }
  TIMES(THANOI_BAR_COUNT, i) {
    ThanoiRod* r = &thanoiRods[i];
    float x = ((i + 0.5) * 100.0) / THANOI_BAR_COUNT;
    r->height -= 0.01;
    if (r->height < r->diskCount * THANOI_DISK_HEIGHT) {
      if (i == 0) {
        play(EXPLOSION);
        gameOver();
      } else {
        play(POWER_UP);
        int sc = r->diskCount * r->diskCount;
        addScore(sc * floor(thanoiMultiplier), x, 90 - r->height);
        thanoiMultiplier += sc / 100.0;
        r->diskCount = 0;
        thanoiRods[0].height = clamp(thanoiRods[0].height + sc / 4.0 / sqrt(difficulty), 0, 90);
        r->height = 90;
        if (thanoiSelectedRodIndex == i) {
          thanoiSelectedRodIndex = -1;
        }
        thanoiHoldTicks = 0;
      }
    }
    if (i == 0) {
      color = LIGHT_RED;
    } else {
      color = LIGHT_BLACK;
    }
    rect(x - 1, 90, 3, -r->height, &scratch);
    float y = 90 - THANOI_DISK_HEIGHT / 2.0 + 1;
    TIMES(r->diskCount, j) {
      float w = r->disks[j] * THANOI_DISK_WIDTH;
      if (i == thanoiSelectedRodIndex && j == r->diskCount - 1) {
        color = CYAN;
        box(x + 0.5, y, w, THANOI_DISK_HEIGHT - 1, &scratch);
        color = RED;
        box(x + 0.5, y, w - 2, THANOI_DISK_HEIGHT - 3, &scratch);
      } else {
        color = RED;
        box(x + 0.5, y, w, THANOI_DISK_HEIGHT - 1, &scratch);
      }
      y -= THANOI_DISK_HEIGHT;
    }
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(thanoiMultiplier)));
  text(multText, 3, 9, &scratch);
}

void addGameThanoi() {
  addGame(thanoiTitle, thanoiDescription, thanoiCharacters,
          thanoiCharactersCount, &thanoiOptions, true, &thanoiUpdate);
}
