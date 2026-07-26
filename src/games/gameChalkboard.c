#include "../cglp.h"

int* chalkboardTitle = "CHALKBOARD";
int* chalkboardDescription = "[Hold]\n Write\n[Release]\n Save\n Overwriting costs a lot";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] chalkboardCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int chalkboardCharactersCount = 0;

Options chalkboardOptions = {100, 100, 5, false};

#define CHALKBOARD_MAX_ENERGY 100
#define CHALKBOARD_WRITE_GAIN 0.1
#define CHALKBOARD_IDLE_DECAY 0.1
#define CHALKBOARD_OVERLAP_PENALTY 10
#define CHALKBOARD_THICKNESS 3
#define CHALKBOARD_SEGMENT_SPACING 2
#define CHALKBOARD_FRESH_FRAMES 4

Vector chalkboardPos;
int chalkboardDirIndex;
float chalkboardSpeed;
int chalkboardTurnTicks;
float chalkboardNextTurnTicks;
Vector chalkboardCam;
float chalkboardEnergy;
Vector chalkboardLastWritePos;
float chalkboardScoreCarry;
bool chalkboardBlinkWasOn;
int chalkboardHoldFrames;
int chalkboardReleaseFrames;
Vector chalkboardNibScreen;

struct ChalkboardSeg {
  Vector p1;
  Vector p2;
  int age;
  bool isAlive;
};
// Matches upstream's 1000-entry cap; its extra distance-based pruning pass is redundant here.
#define CHALKBOARD_MAX_SEG_COUNT 1000
ChalkboardSeg[CHALKBOARD_MAX_SEG_COUNT] chalkboardSegs;
int chalkboardSegIndex;

float chalkboardGetTurnInterval() { return rndi(30, 150) / sqrt(difficulty); }

void chalkboardInitGame() {
  vectorSet(&chalkboardPos, 0, 0);
  chalkboardDirIndex = 0;
  chalkboardSpeed = 0.9;
  chalkboardTurnTicks = 0;
  chalkboardNextTurnTicks = 120;
  vectorSet(&chalkboardCam, 0, 0);
  chalkboardEnergy = 60;
  INIT_UNALIVED_ARRAY_FAST(chalkboardSegs);
  chalkboardSegIndex = 0;
  vectorSet(&chalkboardLastWritePos, chalkboardPos.x, chalkboardPos.y);
  int sy = 10;
  while (sy < 90) {
    ASSIGN_ARRAY_ITEM(chalkboardSegs, chalkboardSegIndex, ChalkboardSeg, s);
    vectorSet(&s->p1, 120 - 50, sy - 50);
    vectorSet(&s->p2, 120 - 50, sy + 10 - 50);
    s->age = 0;
    s->isAlive = true;
    chalkboardSegIndex = cgl_wrap(chalkboardSegIndex + 1, 0, CHALKBOARD_MAX_SEG_COUNT);
    sy += 10;
  }
  chalkboardScoreCarry = 0;
  chalkboardHoldFrames = 0;
  chalkboardReleaseFrames = 0;
  chalkboardBlinkWasOn = false;
}

void chalkboardStepMovement() {
  chalkboardTurnTicks++;
  if (chalkboardTurnTicks >= chalkboardNextTurnTicks) {
    chalkboardTurnTicks = 0;
    chalkboardNextTurnTicks = chalkboardGetTurnInterval();
    chalkboardDirIndex = (chalkboardDirIndex + 1) % 4;
    play(LASER);
  }
  chalkboardSpeed = 0.9 * sqrt(difficulty);
  if (chalkboardDirIndex == 0) {
    chalkboardPos.x += chalkboardSpeed;
  } else if (chalkboardDirIndex == 1) {
    chalkboardPos.y += chalkboardSpeed;
  } else if (chalkboardDirIndex == 2) {
    chalkboardPos.x -= chalkboardSpeed;
  } else {
    chalkboardPos.y -= chalkboardSpeed;
  }
  vectorSet(&chalkboardCam, chalkboardPos.x - 50, chalkboardPos.y - 50);
}

void chalkboardDrawBackground() {
  Collision scratch;
  color = GREEN;
  rect(0, 0, 100, 100, &scratch);
}

void chalkboardDrawSegments() {
  Collision scratch;
  float minX = -20;
  float minY = -20;
  float maxX = 120;
  float maxY = 120;
  FOR_EACH(chalkboardSegs, i) {
    ASSIGN_ARRAY_ITEM(chalkboardSegs, i, ChalkboardSeg, s);
    SKIP_IS_NOT_ALIVE(s);
    float centerX = (s->p1.x + s->p2.x) / 2 - chalkboardCam.x;
    float centerY = (s->p1.y + s->p2.y) / 2 - chalkboardCam.y;
    if (centerX < minX || centerX > maxX || centerY < minY || centerY > maxY) {
      continue;
    }
    float len = distanceTo(&s->p1, s->p2.x, s->p2.y);
    float ang = angleTo(&s->p1, s->p2.x, s->p2.y);
    if (s->age < CHALKBOARD_FRESH_FRAMES) {
      color = LIGHT_BLACK;
    } else {
      color = WHITE;
    }
    thickness = CHALKBOARD_THICKNESS;
    bar(centerX, centerY, len, ang, &scratch);
    s->age++;
  }
}

void chalkboardMaybeAddSegment() {
  float d = distanceTo(&chalkboardLastWritePos, chalkboardPos.x, chalkboardPos.y);
  if (d >= CHALKBOARD_SEGMENT_SPACING) {
    ASSIGN_ARRAY_ITEM(chalkboardSegs, chalkboardSegIndex, ChalkboardSeg, s);
    vectorSet(&s->p1, chalkboardLastWritePos.x, chalkboardLastWritePos.y);
    vectorSet(&s->p2, chalkboardPos.x, chalkboardPos.y);
    s->age = 0;
    s->isAlive = true;
    chalkboardSegIndex = cgl_wrap(chalkboardSegIndex + 1, 0, CHALKBOARD_MAX_SEG_COUNT);
    vectorSet(&chalkboardLastWritePos, chalkboardPos.x, chalkboardPos.y);
  }
}

void chalkboardDrawHud() {
  Collision scratch;
  float w = clamp((chalkboardEnergy / CHALKBOARD_MAX_ENERGY) * 92, 0, 92);
  float y = 91;
  color = BLACK;
  rect(4, y, 92, 5, &scratch);
  color = YELLOW;
  rect(4, y, w, 5, &scratch);
}

void chalkboardDoWritingAndEnergy() {
  Collision scratch;
  vectorSet(&chalkboardNibScreen, chalkboardPos.x - chalkboardCam.x, chalkboardPos.y - chalkboardCam.y);
  color = TRANSPARENT;
  Collision cWhite;
  box(chalkboardNibScreen.x, chalkboardNibScreen.y, CHALKBOARD_THICKNESS, CHALKBOARD_THICKNESS, &cWhite);

  if (input.isPressed) {
    chalkboardHoldFrames++;
    chalkboardReleaseFrames = 0;
  } else {
    chalkboardReleaseFrames++;
    chalkboardHoldFrames = 0;
  }
  float rampHold = clamp(pow((float)chalkboardHoldFrames / 120.0, 1.5), 0, 1);
  float rampRelease = clamp(pow((float)chalkboardReleaseFrames / 120.0, 1.5), 0, 1);

  if (input.isPressed) {
    if (input.isJustPressed) {
      vectorSet(&chalkboardLastWritePos, chalkboardPos.x, chalkboardPos.y);
    }
    if (cWhite.isColliding.rect[WHITE]) {
      chalkboardEnergy -= CHALKBOARD_OVERLAP_PENALTY;
      play(HIT);
      particle(chalkboardNibScreen.x, chalkboardNibScreen.y, 6, 1, 0, CGLP_PI * 2);
    } else {
      float rawGain = CHALKBOARD_WRITE_GAIN * (3 * rampHold) * sqrt(difficulty);
      float applied = clamp(rawGain, 0, CHALKBOARD_MAX_ENERGY - chalkboardEnergy);
      chalkboardEnergy += applied;
      chalkboardScoreCarry += rawGain;
      while (chalkboardScoreCarry >= 1) {
        addScore(1, chalkboardNibScreen.x, chalkboardNibScreen.y);
        chalkboardScoreCarry -= 1;
      }
    }
    chalkboardEnergy = clamp(chalkboardEnergy, 0, CHALKBOARD_MAX_ENERGY);
    chalkboardMaybeAddSegment();
  } else {
    chalkboardEnergy -= CHALKBOARD_IDLE_DECAY * (3 * rampRelease) * sqrt(difficulty);
  }

  color = RED;
  box(chalkboardNibScreen.x, chalkboardNibScreen.y, CHALKBOARD_THICKNESS, CHALKBOARD_THICKNESS, &scratch);

  float arm = 8;
  float gap;
  if (input.isPressed) {
    gap = 0;
  } else {
    gap = 3;
  }
  float s = CHALKBOARD_THICKNESS;
  color = LIGHT_RED;
  if (chalkboardDirIndex == 0) {
    rect(chalkboardNibScreen.x + s / 2 + gap, chalkboardNibScreen.y - s / 2, arm, s, &scratch);
  } else if (chalkboardDirIndex == 1) {
    rect(chalkboardNibScreen.x - s / 2, chalkboardNibScreen.y + s / 2 + gap, s, arm, &scratch);
  } else if (chalkboardDirIndex == 2) {
    rect(chalkboardNibScreen.x - s / 2 - arm - gap, chalkboardNibScreen.y - s / 2, arm, s, &scratch);
  } else {
    rect(chalkboardNibScreen.x - s / 2, chalkboardNibScreen.y - s / 2 - arm - gap, s, arm, &scratch);
  }

  float warnFrames = 30;
  int blinkPeriod = 8;
  float remaining = chalkboardNextTurnTicks - chalkboardTurnTicks;
  if (remaining <= warnFrames) {
    bool blinkOn = ticks % blinkPeriod < blinkPeriod / 2;
    if (blinkOn && !chalkboardBlinkWasOn) {
      play(JUMP);
    }
    chalkboardBlinkWasOn = blinkOn;
    if (blinkOn) {
      int nextDir = (chalkboardDirIndex + 1) % 4;
      float angle;
      if (nextDir == 0) {
        angle = 0;
      } else if (nextDir == 1) {
        angle = CGLP_PI / 2;
      } else if (nextDir == 2) {
        angle = CGLP_PI;
      } else {
        angle = -CGLP_PI / 2;
      }
      float warnArm = arm + 2;
      float warnGap = gap + 1;
      Vector center;
      vectorSet(&center, chalkboardNibScreen.x, chalkboardNibScreen.y);
      addWithAngle(&center, angle, warnGap + warnArm * 0.5);
      color = RED;
      thickness = s;
      bar(center.x, center.y, warnArm, angle, &scratch);
    }
  } else {
    chalkboardBlinkWasOn = false;
  }

  if (chalkboardEnergy <= 0) {
    gameOver();
    play(EXPLOSION);
  }
}

void chalkboardUpdate() {
  if (!ticks) {
    chalkboardInitGame();
  }
  chalkboardStepMovement();
  chalkboardDrawBackground();
  chalkboardDrawSegments();
  chalkboardDoWritingAndEnergy();
  chalkboardDrawHud();
}

void addGameChalkboard() {
  addGame(chalkboardTitle, chalkboardDescription, chalkboardCharacters,
          chalkboardCharactersCount, &chalkboardOptions, false, &chalkboardUpdate);
}
