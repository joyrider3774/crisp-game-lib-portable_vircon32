#include "../cglp.h"

int* marblebounceTitle = "MARBLE BOUNCE";
int* marblebounceDescription = "[Tap] Bounce higher";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] marblebounceCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int marblebounceCharactersCount = 0;

Options marblebounceOptions = {100, 100, 3, false};

struct MarblebounceMarble {
  Vector pos;
  float size;
};
MarblebounceMarble marblebounceMarble;

struct MarblebounceGround {
  Vector pos;
  Vector size;
};
MarblebounceGround marblebounceGround;

struct MarblebouncePin {
  Vector pos;
  float size;
  bool isTarget;
  bool spawning;
  float spawnSize;
  float targetSize;
  int lastHitTime;
  int createdTime;
  float warningTime;
  float explosionTime;
  bool isAlive;
};
// Regular pins never expire upstream either, and the periodic spawner's
// interval floors at 90/sqrt(difficulty) ticks while only one pin is
// consumed per target hit, so population trends upward over a normal long
// session and can reach the old 128-slot cap; raised for headroom.
#define MARBLEBOUNCE_MAX_PIN_COUNT 512
MarblebouncePin[MARBLEBOUNCE_MAX_PIN_COUNT] marblebouncePins;
int marblebouncePinIndex;

Vector marblebounceMarbleVelocity;
float marblebounceGravity;
float marblebouncePinSpawnTimer;
int marblebounceMultiplier;
float marblebouncePinSpawnInterval;
float marblebounceBaseSpawnInterval;

// Shared by both the initial 8-pin setup and each new wave; retries up to 50 times for spacing.
void marblebounceSpawnWaveOfPins(bool markSpawning) {
  int i;
  for (i = 0; i < 8; i++) {
    Vector newPos;
    int attempts = 0;
    bool validPosition = false;
    while (attempts < 50 && !validPosition) {
      vectorSet(&newPos, rnd(10, 90), rnd(20, 80));
      validPosition = true;
      FOR_EACH(marblebouncePins, pinIdx) {
        ASSIGN_ARRAY_ITEM(marblebouncePins, pinIdx, MarblebouncePin, p);
        SKIP_IS_NOT_ALIVE(p);
        if (distanceTo(&newPos, p->pos.x, p->pos.y) < 15) {
          validPosition = false;
        }
      }
      if (distanceTo(&newPos, marblebounceMarble.pos.x, marblebounceMarble.pos.y) < 15) {
        validPosition = false;
      }
      attempts++;
    }
    if (!validPosition) {
      vectorSet(&newPos, rnd(10, 90), rnd(20, 80));
    }
    ASSIGN_ARRAY_ITEM(marblebouncePins, marblebouncePinIndex, MarblebouncePin, np);
    np->pos = newPos;
    np->size = 2;
    np->isTarget = (i == 0);
    np->spawning = markSpawning;
    if (markSpawning) {
      np->spawnSize = 0;
    } else {
      np->spawnSize = 2;
    }
    np->targetSize = 2;
    np->lastHitTime = 0;
    np->createdTime = ticks;
    np->warningTime = 600;
    np->explosionTime = 900;
    np->isAlive = true;
    marblebouncePinIndex = cgl_wrap(marblebouncePinIndex + 1, 0, MARBLEBOUNCE_MAX_PIN_COUNT);
  }
}

void marblebounceUpdate() {
  Collision scratch;
  if (!ticks) {
    // Upstream's sss.setQuantize(0) has no equivalent in this port's sound engine; dropped.
    vectorSet(&marblebounceMarble.pos, 50, 80);
    marblebounceMarble.size = 3;
    INIT_UNALIVED_ARRAY_FAST(marblebouncePins);
    marblebouncePinIndex = 0;
    vectorSet(&marblebounceGround.pos, 50, 95);
    vectorSet(&marblebounceGround.size, 100, 5);
    vectorSet(&marblebounceMarbleVelocity, 0, 0);
    marblebounceGravity = 0.15;
    marblebouncePinSpawnTimer = 0;
    marblebounceMultiplier = 1;
    marblebounceBaseSpawnInterval = 360;
    marblebouncePinSpawnInterval = marblebounceBaseSpawnInterval;
    marblebounceSpawnWaveOfPins(false);
  }

  marblebounceMarbleVelocity.y += marblebounceGravity;

  marblebouncePinSpawnTimer += sqrt(difficulty);
  if (marblebouncePinSpawnTimer >= marblebouncePinSpawnInterval) {
    marblebouncePinSpawnTimer = 0;
    int attempts = 0;
    bool spawned = false;
    while (attempts < 20 && !spawned) {
      Vector newPos;
      vectorSet(&newPos, rnd(10, 90), rnd(20, 80));
      bool tooClose = false;
      FOR_EACH(marblebouncePins, pi2) {
        ASSIGN_ARRAY_ITEM(marblebouncePins, pi2, MarblebouncePin, p2);
        SKIP_IS_NOT_ALIVE(p2);
        if (distanceTo(&newPos, p2->pos.x, p2->pos.y) < 15) {
          tooClose = true;
        }
      }
      if (distanceTo(&newPos, marblebounceMarble.pos.x, marblebounceMarble.pos.y) < 15) {
        tooClose = true;
      }
      if (!tooClose) {
        COUNT_IS_ALIVE(marblebouncePins, pinAliveCount);
        if (pinAliveCount < MARBLEBOUNCE_MAX_PIN_COUNT) {
          ASSIGN_ARRAY_ITEM(marblebouncePins, marblebouncePinIndex, MarblebouncePin, np2);
          np2->pos = newPos;
          np2->size = 2;
          np2->isTarget = false;
          np2->spawning = true;
          np2->spawnSize = 0;
          np2->targetSize = 2;
          np2->lastHitTime = 0;
          np2->createdTime = ticks;
          np2->warningTime = 600;
          np2->explosionTime = 900;
          np2->isAlive = true;
          marblebouncePinIndex = cgl_wrap(marblebouncePinIndex + 1, 0, MARBLEBOUNCE_MAX_PIN_COUNT);
          play(LASER);
          marblebouncePinSpawnInterval = clamp(marblebouncePinSpawnInterval - 30, 90, marblebounceBaseSpawnInterval);
        }
        spawned = true;
      }
      attempts++;
    }
  }

  if (input.isJustPressed) {
    marblebounceMarbleVelocity.y = clamp(marblebounceMarbleVelocity.y - 3, -5, 5);
    play(JUMP);
  }

  vectorAdd(&marblebounceMarble.pos, marblebounceMarbleVelocity.x, marblebounceMarbleVelocity.y);

  color = GREEN;
  rect(marblebounceGround.pos.x - 50, marblebounceGround.pos.y - 2.5, 100, 5, &scratch);

  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(marblebounceMultiplier));
  text(multText, 3, 9, &scratch);

  if (marblebounceMarble.pos.y + marblebounceMarble.size >= marblebounceGround.pos.y - 2.5) {
    marblebounceMarble.pos.y = marblebounceGround.pos.y - 2.5 - marblebounceMarble.size;
    marblebounceMarbleVelocity.y = -marblebounceMarbleVelocity.y * 0.7;
    if (fabs(marblebounceMarbleVelocity.x) < 0.5) {
      marblebounceMarbleVelocity.x += rnd(1, 2) * RNDPM();
    }
  }

  FOR_EACH(marblebouncePins, pi3) {
    ASSIGN_ARRAY_ITEM(marblebouncePins, pi3, MarblebouncePin, pin);
    SKIP_IS_NOT_ALIVE(pin);

    if (pin->spawning) {
      pin->spawnSize += 0.1;
      if (pin->spawnSize >= pin->targetSize) {
        pin->spawnSize = pin->targetSize;
        pin->spawning = false;
      }
      pin->size = pin->spawnSize;
    }

    if (pin->isTarget) {
      float timeAlive = (ticks - pin->createdTime) * sqrt(difficulty);

      if (timeAlive > pin->explosionTime) {
        play(EXPLOSION);
        gameOver();
        continue;
      }

      if (timeAlive > pin->warningTime) {
        float warningProgress = (timeAlive - pin->warningTime) / (pin->explosionTime - pin->warningTime);
        float intensity = warningProgress;

        float vibrationStrength = 0.2 + intensity * 1.0;
        float vibrationSpeed = 0.2 + intensity * 0.8;
        float vibrationX = sin(ticks * vibrationSpeed) * vibrationStrength;
        float vibrationY = cos(ticks * vibrationSpeed * 1.3) * vibrationStrength;

        int particleFrequency = (int)floor(20 - intensity * 15);
        int particleCount = (int)floor(2 + intensity * 6);
        float particleSpeed = 0.5 + intensity * 2;

        int spawnFreq = (int)clamp(particleFrequency, 3, 20);
        if (ticks % spawnFreq == 0) {
          play(HIT);
          particle(pin->pos.x + vibrationX, pin->pos.y + vibrationY, particleCount, particleSpeed, rnd(0, CGLP_PI * 2),
                   CGLP_PI * 2);
        }

        int flashSpeed = (int)floor(20 - intensity * 15);
        int clampedFlash = (int)clamp(flashSpeed, 3, 20);
        int clampedFlashHalf = (int)clamp(flashSpeed / 2, 1, 10);
        if (ticks % clampedFlash < clampedFlashHalf) {
          color = RED;
        } else {
          color = YELLOW;
        }
        arc(pin->pos.x + vibrationX, pin->pos.y + vibrationY, pin->size, 0, CGLP_PI * 2, &scratch);
      } else {
        color = RED;
        arc(pin->pos.x, pin->pos.y, pin->size, 0, CGLP_PI * 2, &scratch);
      }
    } else {
      color = BLACK;
      arc(pin->pos.x, pin->pos.y, pin->size, 0, CGLP_PI * 2, &scratch);
    }
  }

  color = BLUE;
  Collision marbleCollision;
  arc(marblebounceMarble.pos.x, marblebounceMarble.pos.y, marblebounceMarble.size, 0, CGLP_PI * 2, &marbleCollision);

  if (marbleCollision.isColliding.rect[BLACK]) {
    MarblebouncePin* closestPin = NULL;
    float closestDistance = 1000000;
    FOR_EACH(marblebouncePins, pi4) {
      ASSIGN_ARRAY_ITEM(marblebouncePins, pi4, MarblebouncePin, p4);
      SKIP_IS_NOT_ALIVE(p4);
      if (!p4->isTarget) {
        float distance = distanceTo(&marblebounceMarble.pos, p4->pos.x, p4->pos.y);
        if (distance < closestDistance) {
          closestDistance = distance;
          closestPin = p4;
        }
      }
    }

    if (closestPin != NULL) {
      int attempts = 0;
      while (attempts < 10) {
        float separationAngle = angleTo(&closestPin->pos, marblebounceMarble.pos.x, marblebounceMarble.pos.y);
        addWithAngle(&marblebounceMarble.pos, separationAngle, 1);

        color = TRANSPARENT;
        Collision stillCollidingCheck;
        arc(marblebounceMarble.pos.x, marblebounceMarble.pos.y, marblebounceMarble.size, 0, CGLP_PI * 2,
            &stillCollidingCheck);
        if (!stillCollidingCheck.isColliding.rect[BLACK]) {
          break;
        }
        attempts++;
      }

      int hitCooldown = 9;
      if (ticks - closestPin->lastHitTime > hitCooldown) {
        addScore(marblebounceMultiplier, marblebounceMarble.pos.x, marblebounceMarble.pos.y);
        closestPin->lastHitTime = ticks;
        play(HIT);
      }

      marblebounceMarbleVelocity.x = rnd(1, 2) * RNDPM();
      marblebounceMarbleVelocity.y = -fabs(marblebounceMarbleVelocity.y) * 0.8;
    }

    color = BLUE;
    arc(marblebounceMarble.pos.x, marblebounceMarble.pos.y, marblebounceMarble.size, 0, CGLP_PI * 2, &marbleCollision);
  }

  if (marbleCollision.isColliding.rect[RED] || marbleCollision.isColliding.rect[YELLOW]) {
    MarblebouncePin* targetPin = NULL;
    FOR_EACH(marblebouncePins, pi5) {
      ASSIGN_ARRAY_ITEM(marblebouncePins, pi5, MarblebouncePin, p5);
      SKIP_IS_NOT_ALIVE(p5);
      if (p5->isTarget) {
        float distance = distanceTo(&marblebounceMarble.pos, p5->pos.x, p5->pos.y);
        if (distance < marblebounceMarble.size + p5->size) {
          targetPin = p5;
        }
      }
    }

    if (targetPin != NULL) {
      float separationAngle = angleTo(&targetPin->pos, marblebounceMarble.pos.x, marblebounceMarble.pos.y);
      addWithAngle(&marblebounceMarble.pos, separationAngle, 2);
      marblebounceMarbleVelocity.x = rnd(2, 3) * RNDPM();
      marblebounceMarbleVelocity.y = -fabs(marblebounceMarbleVelocity.y) * 0.9;

      addScore(marblebounceMultiplier * 100, marblebounceMarble.pos.x, marblebounceMarble.pos.y);
      play(POWER_UP);

      marblebouncePinSpawnInterval = marblebounceBaseSpawnInterval;
      marblebouncePinSpawnTimer = 0;

      targetPin->isAlive = false;

      COUNT_IS_ALIVE(marblebouncePins, remainingPinCount);
      if (remainingPinCount == 0) {
        marblebounceMultiplier++;
        play(RANDOM);  // Equivalent to "lucky" in JS
        marblebounceSpawnWaveOfPins(true);
      } else {
        int[MARBLEBOUNCE_MAX_PIN_COUNT] regularIndices;
        int regularCount = 0;
        FOR_EACH(marblebouncePins, pi6) {
          ASSIGN_ARRAY_ITEM(marblebouncePins, pi6, MarblebouncePin, p6);
          SKIP_IS_NOT_ALIVE(p6);
          if (!p6->isTarget) {
            regularIndices[regularCount] = pi6;
            regularCount++;
          }
        }
        if (regularCount > 0) {
          int chosen = regularIndices[rndi(0, regularCount)];
          ASSIGN_ARRAY_ITEM(marblebouncePins, chosen, MarblebouncePin, randomPin);
          randomPin->isTarget = true;
          randomPin->createdTime = ticks;
          randomPin->warningTime = 600;
          randomPin->explosionTime = 900;
        } else {
          ASSIGN_ARRAY_ITEM(marblebouncePins, marblebouncePinIndex, MarblebouncePin, newTarget);
          vectorSet(&newTarget->pos, rnd(10, 90), rnd(20, 80));
          newTarget->size = 2;
          newTarget->isTarget = true;
          newTarget->spawning = true;
          newTarget->spawnSize = 0;
          newTarget->targetSize = 2;
          newTarget->lastHitTime = 0;
          newTarget->createdTime = ticks;
          newTarget->warningTime = 600;
          newTarget->explosionTime = 900;
          newTarget->isAlive = true;
          marblebouncePinIndex = cgl_wrap(marblebouncePinIndex + 1, 0, MARBLEBOUNCE_MAX_PIN_COUNT);
        }
      }

      color = BLUE;
      arc(marblebounceMarble.pos.x, marblebounceMarble.pos.y, marblebounceMarble.size, 0, CGLP_PI * 2, &scratch);
    }
  }

  if (marblebounceMarble.pos.x < 5) {
    marblebounceMarble.pos.x = 5;
    marblebounceMarbleVelocity.x = fabs(marblebounceMarbleVelocity.x);
  }
  if (marblebounceMarble.pos.x > 95) {
    marblebounceMarble.pos.x = 95;
    marblebounceMarbleVelocity.x = -fabs(marblebounceMarbleVelocity.x);
  }

  if (marblebounceMarble.pos.y < 5) {
    marblebounceMarble.pos.y = 5;
    marblebounceMarbleVelocity.y = fabs(marblebounceMarbleVelocity.y);
  }
}

void addGameMarblebounce() {
  addGame(marblebounceTitle, marblebounceDescription, marblebounceCharacters,
          marblebounceCharactersCount, &marblebounceOptions, false, &marblebounceUpdate);
}
