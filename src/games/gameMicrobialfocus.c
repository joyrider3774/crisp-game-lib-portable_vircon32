#include "../cglp.h"

int* microbialfocusTitle = "MICROBIAL FOCUS";
int* microbialfocusDescription = "[Hold]\n Adjust focus depth";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] microbialfocusCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int microbialfocusCharactersCount = 0;

Options microbialfocusOptions = {100, 100, 9, false};

struct MicrobialfocusMicrobe {
  Vector pos;
  float depth;
  float size;
  bool moving;
  Vector moveDir;
  Vector velocity;
  float anger;
  float maxAnger;
  bool isAlive;
};
// Vircon32 port note: upstream itself caps concurrent microbes at 24 (see
// the spawn gate below) - this array is sized to that exact, explicit cap.
#define MICROBIALFOCUS_MAX_MICROBE_COUNT 24
MicrobialfocusMicrobe[MICROBIALFOCUS_MAX_MICROBE_COUNT] microbialfocusMicrobes;
int microbialfocusMicrobeIndex;

float microbialfocusFocusDepth;
int microbialfocusFocusDirection;
int microbialfocusHoldCycle;
float microbialfocusNextMicrobeSpawn;
float microbialfocusMultiplier;

void microbialfocusUpdate() {
  Collision scratch;
  // Never reads a Collision result - focus matches are decided by depth comparison.
  hasCollision = false;
  if (!ticks) {
    microbialfocusFocusDepth = 0.5;
    microbialfocusFocusDirection = 1;
    INIT_UNALIVED_ARRAY_FAST(microbialfocusMicrobes);
    microbialfocusMicrobeIndex = 0;
    microbialfocusHoldCycle = 1;
    microbialfocusNextMicrobeSpawn = 0;
    microbialfocusMultiplier = 1;
  }

  // Focus control system
  if (input.isJustPressed) {
    if (microbialfocusHoldCycle == 0) {
      microbialfocusFocusDirection = 1;
      microbialfocusHoldCycle = 1;
    } else {
      microbialfocusFocusDirection = -1;
      microbialfocusHoldCycle = 0;
    }
    play(SELECT);
  }

  if (input.isPressed) {
    microbialfocusFocusDepth += microbialfocusFocusDirection * 0.015 * sqrt(difficulty);
    if (microbialfocusFocusDepth < 0 || microbialfocusFocusDepth > 1) {
      microbialfocusFocusDepth = clamp(microbialfocusFocusDepth, 0, 1);
      play(LASER);
      microbialfocusMultiplier = 1;
    }
  }

  microbialfocusMultiplier *= 0.999;

  // Add new microbes periodically from screen edge
  COUNT_IS_ALIVE(microbialfocusMicrobes, microbeCountNow);
  if (ticks >= microbialfocusNextMicrobeSpawn && microbeCountNow < 24) {
    microbialfocusNextMicrobeSpawn = ticks + (50 / difficulty / difficulty) * sqrt(microbeCountNow + 1);
    float newDepth = 0.5;
    int di;
    for (di = 0; di < 8; di++) {
      newDepth = rnd(0.05, 0.95);
      if (fabs(newDepth - microbialfocusFocusDepth) > 0.15) {
        break;
      }
    }

    int edge = (int)rnd(0, 4);
    float microbeSize = rnd(4, 8);
    Vector newPos;
    Vector initialVelocity;
    if (edge == 0) {
      vectorSet(&newPos, rnd(10, 90), -microbeSize + 1);
      vectorSet(&initialVelocity, rnd(-0.2, 0.2), rnd(0.5, 1));
    } else if (edge == 1) {
      vectorSet(&newPos, 100 + microbeSize - 1, rnd(10, 90));
      vectorSet(&initialVelocity, rnd(-1, -0.5), rnd(-0.2, 0.2));
    } else if (edge == 2) {
      vectorSet(&newPos, rnd(10, 90), 100 + microbeSize - 1);
      vectorSet(&initialVelocity, rnd(-0.2, 0.2), rnd(-1, -0.5));
    } else {
      vectorSet(&newPos, -microbeSize + 1, rnd(10, 90));
      vectorSet(&initialVelocity, rnd(0.5, 1), rnd(-0.2, 0.2));
    }

    ASSIGN_ARRAY_ITEM(microbialfocusMicrobes, microbialfocusMicrobeIndex, MicrobialfocusMicrobe, nm);
    nm->pos = newPos;
    nm->depth = newDepth;
    nm->size = microbeSize;
    nm->moving = false;
    vectorSet(&nm->moveDir, 0, 0);
    nm->velocity = initialVelocity;
    nm->anger = 0;
    nm->maxAnger = floor(600 / sqrt(sqrt(difficulty)));
    nm->isAlive = true;
    microbialfocusMicrobeIndex = cgl_wrap(microbialfocusMicrobeIndex + 1, 0, MICROBIALFOCUS_MAX_MICROBE_COUNT);

    play(SELECT);
  }

  // Update microbes
  FOR_EACH(microbialfocusMicrobes, mi) {
    ASSIGN_ARRAY_ITEM(microbialfocusMicrobes, mi, MicrobialfocusMicrobe, m);
    SKIP_IS_NOT_ALIVE(m);
    float depthDiff = fabs(m->depth - microbialfocusFocusDepth);
    bool isInFocus = depthDiff < 0.1;

    m->anger++;

    float angerRatio = m->anger / m->maxAnger;
    if (angerRatio >= 0.8 && ((int)m->anger) % 30 == 0) {
      play(HIT);
    } else if (angerRatio >= 0.6 && ((int)m->anger) % 60 == 0) {
      play(HIT);
    }

    if (isInFocus) {
      if (!m->moving) {
        m->moving = true;
        Vector directionFromCenter;
        vectorSet(&directionFromCenter, m->pos.x - 50, m->pos.y - 50);
        float length = vectorLength(&directionFromCenter);
        if (length > 0) {
          vectorSet(&m->moveDir, directionFromCenter.x / length, directionFromCenter.y / length);
        } else {
          float angle = rnd(0, CGLP_PI * 2);
          vectorSet(&m->moveDir, cos(angle), sin(angle));
        }
      }
      float acceleration = 0.02;
      m->velocity.x += m->moveDir.x * acceleration;
      m->velocity.y += m->moveDir.y * acceleration;
      vectorAdd(&m->pos, m->velocity.x, m->velocity.y);
    } else {
      m->velocity.x *= 0.99;
      m->velocity.y *= 0.99;
      vectorAdd(&m->pos, m->velocity.x, m->velocity.y);
      if (fabs(m->velocity.x) < 0.01 && fabs(m->velocity.y) < 0.01) {
        m->moving = false;
        vectorSet(&m->velocity, 0, 0);
      }
    }
  }

  // Remove off-screen microbes and award scores
  FOR_EACH(microbialfocusMicrobes, mri) {
    ASSIGN_ARRAY_ITEM(microbialfocusMicrobes, mri, MicrobialfocusMicrobe, mr);
    SKIP_IS_NOT_ALIVE(mr);
    if (mr->pos.x < -mr->size || mr->pos.x > 100 + mr->size || mr->pos.y < -mr->size ||
        mr->pos.y > 100 + mr->size) {
      int roundedMultiplier = (int)round(microbialfocusMultiplier);
      float scoreX = clamp(mr->pos.x, 10, 90);
      float scoreY = clamp(mr->pos.y, 20, 99);
      addScore(roundedMultiplier, scoreX, scoreY);
      microbialfocusMultiplier += 1;
      play(POWER_UP);
      mr->isAlive = false;
      continue;
    }
  }

  // Draw microbes
  FOR_EACH(microbialfocusMicrobes, mdi) {
    ASSIGN_ARRAY_ITEM(microbialfocusMicrobes, mdi, MicrobialfocusMicrobe, md);
    SKIP_IS_NOT_ALIVE(md);
    float depthDiff2 = fabs(md->depth - microbialfocusFocusDepth);
    float blur = depthDiff2 * 15;
    bool isInFocus2 = depthDiff2 < 0.1;

    float angerRatio2 = md->anger / md->maxAnger;
    float angerRadius = angerRatio2 * md->size;

    bool shouldFlash = angerRatio2 > 0.6;
    float flashPeriod = 21 - (angerRatio2 - 0.6) * 20;
    bool isFlashFrame = false;
    if (shouldFlash) {
      isFlashFrame = ((int)(md->anger / flashPeriod)) % 2 == 0;
    }
    int angerColor = RED;
    int blurredAngerColor = LIGHT_RED;
    if (isFlashFrame) {
      angerColor = YELLOW;
      blurredAngerColor = LIGHT_YELLOW;
    }

    if (isInFocus2) {
      // Sharp, in-focus microbe
      color = GREEN;
      arc(md->pos.x, md->pos.y, md->size, 0, CGLP_PI * 2, &scratch);

      if (rnd(0, 1) < 0.3) {
        particle(md->pos.x, md->pos.y, 1, rnd(0.5, 1.5), 0, CGLP_PI * 2);
      }

      if (angerRadius > 0.5) {
        color = angerColor;
        arc(md->pos.x, md->pos.y, angerRadius, 0, CGLP_PI * 2, &scratch);

        if (angerRatio2 > 0.7 && rnd(0, 1) < angerRatio2) {
          particle(md->pos.x, md->pos.y, rndi(2, 5), rnd(1, 3), 0, CGLP_PI * 2);
        }
      }
    } else {
      // Blurred, out-of-focus microbe (fixed two-copy offset pattern)
      float blurAmount = blur * 0.5;

      color = LIGHT_GREEN;
      arc(md->pos.x - blurAmount, md->pos.y, md->size * (0.8 + blurAmount * 0.1), 0, CGLP_PI * 2, &scratch);
      arc(md->pos.x + blurAmount, md->pos.y, md->size * (0.8 + blurAmount * 0.1), 0, CGLP_PI * 2, &scratch);

      if (angerRadius > 0.5) {
        color = blurredAngerColor;
        arc(md->pos.x - blurAmount, md->pos.y, angerRadius * (0.8 + blurAmount * 0.1), 0, CGLP_PI * 2,
            &scratch);
        arc(md->pos.x + blurAmount, md->pos.y, angerRadius * (0.8 + blurAmount * 0.1), 0, CGLP_PI * 2,
            &scratch);

        if (angerRatio2 > 0.7 && rnd(0, 1) < angerRatio2 * 0.5) {
          particle(md->pos.x, md->pos.y, rnd(1, 3), rnd(0.8, 2), rnd(0, CGLP_PI * 2), CGLP_PI * 2);
        }
      }
    }

    // Check for game over
    if (md->anger >= md->maxAnger) {
      color = RED;
      float tx = clamp(md->pos.x, 3, 97);
      float ty = clamp(md->pos.y, 3, 97);
      text("X", tx, ty, &scratch);
      play(EXPLOSION);
      gameOver();
    }
  }

  // Draw focus depth indicator
  color = BLUE;
  rect(10, 90, 80, 3, &scratch);
  color = CYAN;
  rect(10 + microbialfocusFocusDepth * 80 - 2, 89, 4, 5, &scratch);

  // Draw multiplier
  // isSmallText dropped (no small-font variant in this port); drawn at normal size.
  color = BLACK;
  int[16] multiplierText;
  strcpy(multiplierText, "x");
  strcat(multiplierText, intToChar((int)round(microbialfocusMultiplier)));
  text(multiplierText, 3, 9, &scratch);
}

void addGameMicrobialfocus() {
  addGame(microbialfocusTitle, microbialfocusDescription, microbialfocusCharacters,
          microbialfocusCharactersCount, &microbialfocusOptions, false, &microbialfocusUpdate);
}
