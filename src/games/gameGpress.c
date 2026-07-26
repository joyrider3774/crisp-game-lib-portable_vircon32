#include "../cglp.h"

int* gpressTitle = "G PRESS";
int* gpressDescription = "[Tap] Press";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] gpressCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int gpressCharactersCount = 1;

Options gpressOptions = {100, 100, 2, false};

#define GPRESS_PRESS_HEIGHT 20

struct GpressPress {
  float y;
  float vy;
  float width;
  float vw;
};
GpressPress gpressPress;

struct GpressBubble {
  Vector pos;
  Vector vel;
  float size;
  bool isAlive;
};
#define GPRESS_MAX_BUBBLE_COUNT 32
GpressBubble[GPRESS_MAX_BUBBLE_COUNT] gpressBubbles;
int gpressBubbleIndex;
float gpressNextBubbleTicks;

struct GpressDrop {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define GPRESS_MAX_DROP_COUNT 64
GpressDrop[GPRESS_MAX_DROP_COUNT] gpressDrops;
int gpressDropIndex;

float gpressDropTicks;
float gpressWaterY;
float gpressPrevWaterY;
float gpressTargetWaterY;

void gpressUpdate() {
  Collision scratch;
  if (!ticks) {
    gpressPress.y = 30;
    gpressPress.vy = 1;
    gpressPress.width = 80;
    gpressPress.vw = 0;
    INIT_UNALIVED_ARRAY_FAST(gpressBubbles);
    gpressBubbleIndex = 0;
    gpressNextBubbleTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(gpressDrops);
    gpressDropIndex = 0;
    gpressDropTicks = 0;
    gpressWaterY = 70;
    gpressPrevWaterY = 70;
    gpressTargetWaterY = 70;
  }
  if (gpressPress.vw != 0) {
    gpressPress.width += gpressPress.vw * 5 * sqrt(difficulty);
    if (gpressPress.width < 0) {
      play(EXPLOSION);
      gpressPress.width = 0;
      gpressPress.vw = 0.5;
    }
    if (gpressPress.width > 80) {
      gpressPress.width = 80;
      gpressPress.vw = 0;
    }
  } else {
    gpressPress.y += gpressPress.vy * sqrt(difficulty);
    if ((gpressPress.y < GPRESS_PRESS_HEIGHT / 2 && gpressPress.vy < 0) ||
        (gpressPress.y > gpressWaterY - GPRESS_PRESS_HEIGHT / 2 && gpressPress.vy > 0)) {
      gpressPress.vy *= -1;
    }
    if (input.isJustPressed) {
      play(SELECT);
      gpressPrevWaterY = gpressWaterY + 12;
      gpressTargetWaterY = gpressWaterY + 12;
      gpressPress.vw = -1;
    }
  }
  if (gpressPress.vw < 0 || gpressPress.width == 0) {
    color = RED;
  } else {
    color = BLACK;
  }
  rect(50 - gpressPress.width / 2, gpressPress.y - GPRESS_PRESS_HEIGHT / 2, -5,
       GPRESS_PRESS_HEIGHT, &scratch);
  rect(50 + gpressPress.width / 2, gpressPress.y - GPRESS_PRESS_HEIGHT / 2, 5,
       GPRESS_PRESS_HEIGHT, &scratch);
  if (gpressPress.vw < 0 || gpressPress.width == 0) {
    color = PURPLE;
    rect(1, gpressPress.y, 1, 99, &scratch);
    rect(1, gpressPress.y, 44 - gpressPress.width / 2, 1, &scratch);
    rect(98, gpressPress.y, 1, 99, &scratch);
    rect(98, gpressPress.y, -(44 - gpressPress.width / 2), 1, &scratch);
  }
  color = BLACK;
  rect(0, gpressPress.y - 1, 1, 99, &scratch);
  rect(2, gpressPress.y + 1, 1, 99, &scratch);
  rect(0, gpressPress.y - 1, 45 - gpressPress.width / 2, 1, &scratch);
  rect(2, gpressPress.y + 1, 43 - gpressPress.width / 2, 1, &scratch);
  rect(99, gpressPress.y - 1, 1, 99, &scratch);
  rect(97, gpressPress.y + 1, 1, 99, &scratch);
  rect(99, gpressPress.y - 1, -(45 - gpressPress.width / 2), 1, &scratch);
  rect(97, gpressPress.y + 1, -(43 - gpressPress.width / 2), 1, &scratch);
  gpressNextBubbleTicks--;
  if (gpressNextBubbleTicks < 0) {
    float size = rnd(5, 9);
    ASSIGN_ARRAY_ITEM(gpressBubbles, gpressBubbleIndex, GpressBubble, b);
    vectorSet(&b->pos, rnd(20 + size, 80 - size), gpressWaterY + size / 2);
    vectorSet(&b->vel, 0, (-rnd(1, 2) / size) * difficulty);
    b->size = size;
    b->isAlive = true;
    gpressBubbleIndex = cgl_wrap(gpressBubbleIndex + 1, 0, GPRESS_MAX_BUBBLE_COUNT);
    gpressTargetWaterY += size * 0.03;
    gpressNextBubbleTicks += rnd(10, 50) / difficulty;
  }
  color = PURPLE;
  FOR_EACH(gpressBubbles, i) {
    ASSIGN_ARRAY_ITEM(gpressBubbles, i, GpressBubble, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    Collision bc;
    box(b->pos.x, b->pos.y, b->size, b->size, &bc);
    if (bc.isColliding.rect[RED]) {
      if (b->size > gpressPress.width) {
        play(HIT);
        int dropCount = (int)ceil(b->size);
        TIMES(dropCount, k) {
          ASSIGN_ARRAY_ITEM(gpressDrops, gpressDropIndex, GpressDrop, d);
          vectorSet(&d->pos, b->pos.x + rnd(0, b->size / 2) * RNDPM(),
                    b->pos.y + rnd(0, b->size / 2) * RNDPM());
          vectorSet(&d->vel, 0, 0);
          d->isAlive = true;
          gpressDropIndex = cgl_wrap(gpressDropIndex + 1, 0, GPRESS_MAX_DROP_COUNT);
        }
        b->isAlive = false;
        continue;
      } else {
        if (b->pos.x < 50) {
          b->pos.x = 50 - gpressPress.width / 2 + b->size / 2;
        } else {
          b->pos.x = 50 + gpressPress.width / 2 - b->size / 2;
        }
      }
    }
    b->isAlive = b->pos.y >= -b->size / 2;
  }
  color = LIGHT_PURPLE;
  FOR_EACH(gpressDrops, i) {
    ASSIGN_ARRAY_ITEM(gpressDrops, i, GpressDrop, d);
    SKIP_IS_NOT_ALIVE(d);
    d->vel.y += difficulty * 0.1;
    vectorAdd(&d->pos, d->vel.x, d->vel.y);
    box(d->pos.x, d->pos.y, 2, 2, &scratch);
    if (d->pos.y > gpressWaterY + 1) {
      gpressDropTicks = 9 / difficulty;
      play(LASER);
      gpressTargetWaterY -= 0.35;
      d->isAlive = false;
      continue;
    }
  }
  if (gpressDropTicks > 0) {
    gpressDropTicks--;
    if (gpressDropTicks <= 0) {
      if (gpressPrevWaterY > gpressTargetWaterY) {
        play(COIN);
        float s = gpressPrevWaterY - gpressTargetWaterY;
        addScore(ceil(s * sqrt(s)), 50, gpressWaterY);
      }
    }
  }
  if (gpressTargetWaterY < 50) {
    gpressTargetWaterY += (50 - gpressTargetWaterY) * 0.05;
  }
  gpressWaterY += (gpressTargetWaterY - gpressWaterY) * 0.2;
  color = PURPLE;
  rect(0, gpressWaterY, 100, 101 - gpressWaterY, &scratch);
  COUNT_IS_ALIVE(gpressDrops, aliveDropCount);
  if (gpressPress.vw == 0 && aliveDropCount == 0 && gpressWaterY >= 100 &&
      gpressTargetWaterY >= 100) {
    play(RANDOM);
    gameOver();
  }
}

void addGameGpress() {
  addGame(gpressTitle, gpressDescription, gpressCharacters,
          gpressCharactersCount, &gpressOptions, false, &gpressUpdate);
}
