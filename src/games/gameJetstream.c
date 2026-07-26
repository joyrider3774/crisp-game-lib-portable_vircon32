#include "../cglp.h"

int* jetstreamTitle = "JET STREAM";
int* jetstreamDescription = "[Tap]\n Cycle lanes";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] jetstreamCharacters = {
    {
        "  ll  ",
        "   l l",
        "  lll ",
        " l l  ",
        "  ll  ",
        " l  l ",
    },
    {
        "  ll  ",
        "   l  ",
        " lllll",
        "  l   ",
        " l l  ",
        "l  l  ",
    },
    {
        "  ll  ",
        " l l  ",
        "  lll ",
        "   l l",
        "  ll  ",
        "  l l ",
    },
};
int jetstreamCharactersCount = 3;

Options jetstreamOptions = {100, 100, 6, false};

struct JetstreamSurfer {
  Vector pos;
  int lane;
  int step;
  float animFrame;
  float boardSway;
  Vector knockbackVel;
  float buttonKnockbackRatio;
};
JetstreamSurfer jetstreamSurfer;

struct JetstreamWakeParticle {
  Vector pos;
  Vector vel;
  float size;
  float life;
  float maxLife;
  bool isAlive;
};
#define JETSTREAM_MAX_WAKE_PARTICLE_COUNT 32
JetstreamWakeParticle[JETSTREAM_MAX_WAKE_PARTICLE_COUNT] jetstreamWakeParticles;
int jetstreamWakeParticleIndex;

struct JetstreamWindBarrier {
  Vector pos;
  int lane;
  float rotation;
  float pulsePhase;
  bool shrinking;
  float shrinkScale;
  float shrinkSpeed;
  bool destroyed;
  float destroyScale;
  float destroyExpansion;
  bool isAlive;
};
// Concurrent count grows roughly as 1.3*sqrt(difficulty) (barrier speed and
// spawn cadence both scale with difficulty) - generous headroom applied for
// a long session.
#define JETSTREAM_MAX_WIND_BARRIER_COUNT 64
JetstreamWindBarrier[JETSTREAM_MAX_WIND_BARRIER_COUNT] jetstreamWindBarriers;
int jetstreamWindBarrierIndex;

float jetstreamStreamOffset;
float jetstreamBarrierSpawnTimer;
float jetstreamBaseSpawnInterval;
int jetstreamWakeParticleTimer;
int jetstreamWakeParticleInterval;
int jetstreamMultiplier;

void jetstreamUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&jetstreamSurfer.pos, 50, 25);
    jetstreamSurfer.lane = 0;
    jetstreamSurfer.step = 0;
    jetstreamSurfer.animFrame = 0;
    jetstreamSurfer.boardSway = 0;
    vectorSet(&jetstreamSurfer.knockbackVel, 0, 0);
    jetstreamSurfer.buttonKnockbackRatio = 0;
    INIT_UNALIVED_ARRAY_FAST(jetstreamWindBarriers);
    jetstreamWindBarrierIndex = 0;
    jetstreamStreamOffset = 0;
    INIT_UNALIVED_ARRAY_FAST(jetstreamWakeParticles);
    jetstreamWakeParticleIndex = 0;
    jetstreamBarrierSpawnTimer = 0;
    jetstreamBaseSpawnInterval = 120;
    jetstreamWakeParticleTimer = 0;
    jetstreamWakeParticleInterval = 3;
    jetstreamMultiplier = 1;
  }

  if (input.isJustPressed) {
    jetstreamSurfer.step = (jetstreamSurfer.step + 1) % 4;

    jetstreamSurfer.knockbackVel.x += jetstreamSurfer.buttonKnockbackRatio;
    jetstreamSurfer.knockbackVel.y -= 1;
    jetstreamSurfer.buttonKnockbackRatio += 1;

    if (jetstreamSurfer.step == 0) {
      jetstreamSurfer.lane = 0;
    } else if (jetstreamSurfer.step == 1) {
      jetstreamSurfer.lane = 1;
    } else if (jetstreamSurfer.step == 2) {
      jetstreamSurfer.lane = 2;
    } else if (jetstreamSurfer.step == 3) {
      jetstreamSurfer.lane = 1;
    }

    play(SELECT);
  }
  jetstreamSurfer.buttonKnockbackRatio *= 0.9;

  float targetY;
  if (jetstreamSurfer.lane == 0) {
    targetY = 21;
  } else if (jetstreamSurfer.lane == 1) {
    targetY = 46;
  } else {
    targetY = 71;
  }
  jetstreamSurfer.pos.y += (targetY - jetstreamSurfer.pos.y) * 0.1;

  vectorAdd(&jetstreamSurfer.pos, jetstreamSurfer.knockbackVel.x, jetstreamSurfer.knockbackVel.y);
  vectorMul(&jetstreamSurfer.knockbackVel, 0.9);

  float laneSpeed = 0.5 * sqrt(difficulty);
  if (jetstreamSurfer.lane == 0 || jetstreamSurfer.lane == 2) {
    jetstreamSurfer.pos.x -= laneSpeed;
  } else {
    jetstreamSurfer.pos.x += laneSpeed;
  }

  jetstreamStreamOffset += 2 * sqrt(difficulty);
  color = LIGHT_BLUE;
  thickness = 2;
  int si;
  for (si = 0; si < 5; si++) {
    float xUpper = 100 - (fmod(si * 25 + jetstreamStreamOffset, 125) - 25);
    line(xUpper, 25, xUpper - 15, 25, &scratch);

    float xCenter = fmod(si * 25 + jetstreamStreamOffset, 125) - 25;
    thickness = 2;
    line(xCenter, 50, xCenter + 15, 50, &scratch);

    float xLower = 100 - (fmod(si * 25 + jetstreamStreamOffset, 125) - 25);
    thickness = 2;
    line(xLower, 75, xLower - 15, 75, &scratch);
  }

  if (rnd(0, 1) < 0.05) {
    jetstreamSurfer.animFrame++;
  }
  jetstreamSurfer.boardSway += 0.1;

  color = BLUE;
  int[4] jetstreamAnimLookup;
  jetstreamAnimLookup[0] = 0;
  jetstreamAnimLookup[1] = 1;
  jetstreamAnimLookup[2] = 2;
  jetstreamAnimLookup[3] = 1;
  int charIndex = jetstreamAnimLookup[(int)floor(jetstreamSurfer.animFrame) % 4];

  int[2] surferChar;
  surferChar[0] = 'a' + charIndex;
  surferChar[1] = 0;
  character(surferChar, jetstreamSurfer.pos.x, jetstreamSurfer.pos.y, &scratch);

  color = YELLOW;
  float boardY = jetstreamSurfer.pos.y + 4;
  float swayAmount;
  if (jetstreamSurfer.lane == 0 || jetstreamSurfer.lane == 2) {
    swayAmount = sin(jetstreamSurfer.boardSway + jetstreamStreamOffset * 0.02) * 1;
  } else {
    swayAmount = sin(jetstreamSurfer.boardSway - jetstreamStreamOffset * 0.02) * 1;
  }

  float boardStartX = jetstreamSurfer.pos.x - 4;
  float boardEndX = jetstreamSurfer.pos.x + 4;
  float boardStartY = boardY + swayAmount;
  float boardEndY = boardY - swayAmount;

  thickness = 2;
  line(boardStartX, boardStartY, boardEndX, boardEndY, &scratch);

  jetstreamWakeParticleTimer++;

  if (jetstreamWakeParticleTimer >= jetstreamWakeParticleInterval) {
    float wakeX;
    float velocityX;
    if (jetstreamSurfer.lane == 0 || jetstreamSurfer.lane == 2) {
      wakeX = jetstreamSurfer.pos.x + 5;
      velocityX = 0.3;
    } else {
      wakeX = jetstreamSurfer.pos.x - 5;
      velocityX = -0.3;
    }

    ASSIGN_ARRAY_ITEM(jetstreamWakeParticles, jetstreamWakeParticleIndex, JetstreamWakeParticle, nw);
    vectorSet(&nw->pos, wakeX, boardY);
    vectorSet(&nw->vel, velocityX, -0.1);
    nw->size = 1;
    nw->life = 24;
    nw->maxLife = 24;
    nw->isAlive = true;
    jetstreamWakeParticleIndex = cgl_wrap(jetstreamWakeParticleIndex + 1, 0, JETSTREAM_MAX_WAKE_PARTICLE_COUNT);

    jetstreamWakeParticleTimer = 0;
  }

  color = CYAN;
  FOR_EACH(jetstreamWakeParticles, wpi) {
    ASSIGN_ARRAY_ITEM(jetstreamWakeParticles, wpi, JetstreamWakeParticle, p);
    SKIP_IS_NOT_ALIVE(p);
    vectorAdd(&p->pos, p->vel.x, p->vel.y);
    p->life--;

    p->size = 1 + (1 - p->life / p->maxLife) * 2;

    if (p->life > 0) {
      rect(p->pos.x - p->size / 2, p->pos.y - p->size / 2, p->size, p->size, &scratch);
    }

    if (p->life <= 0 || p->pos.x < -5 || p->pos.x > 105) {
      p->isAlive = false;
    }
  }

  jetstreamBarrierSpawnTimer += difficulty;

  float nextSpawnInterval = jetstreamBaseSpawnInterval + rnd(-30, 30);

  if (jetstreamBarrierSpawnTimer >= nextSpawnInterval) {
    int lane = rndi(0, 3);
    float yPos;
    if (lane == 0) {
      yPos = 25;
    } else if (lane == 1) {
      yPos = 50;
    } else {
      yPos = 75;
    }
    float spawnX;
    if (lane == 0 || lane == 2) {
      spawnX = -10;
    } else {
      spawnX = 110;
    }
    ASSIGN_ARRAY_ITEM(jetstreamWindBarriers, jetstreamWindBarrierIndex, JetstreamWindBarrier, nb);
    vectorSet(&nb->pos, spawnX, yPos);
    nb->lane = lane;
    nb->rotation = 0;
    nb->pulsePhase = rnd(0, 2 * CGLP_PI);
    nb->shrinking = false;
    nb->shrinkScale = 1.0;
    nb->shrinkSpeed = 0.15;
    nb->destroyed = false;
    nb->destroyScale = 1.0;
    nb->destroyExpansion = 1.0;
    nb->isAlive = true;
    jetstreamWindBarrierIndex = cgl_wrap(jetstreamWindBarrierIndex + 1, 0, JETSTREAM_MAX_WIND_BARRIER_COUNT);

    jetstreamBarrierSpawnTimer = 0;
    jetstreamBaseSpawnInterval = 120 + rnd(-20, 20);
  }

  FOR_EACH(jetstreamWindBarriers, bi) {
    ASSIGN_ARRAY_ITEM(jetstreamWindBarriers, bi, JetstreamWindBarrier, b);
    SKIP_IS_NOT_ALIVE(b);

    if (!b->shrinking) {
      float barrierSpeed = 1 * sqrt(difficulty);
      if (b->lane == 0 || b->lane == 2) {
        b->pos.x += barrierSpeed;
        b->rotation += 0.15;
      } else {
        b->pos.x -= barrierSpeed;
        b->rotation -= 0.15;
      }
    }

    b->pulsePhase += 0.2;

    if (b->shrinking) {
      b->shrinkScale -= b->shrinkSpeed;
      if (b->shrinkScale <= 0) {
        b->isAlive = false;
        continue;
      }
    }

    if (b->destroyed) {
      b->destroyScale -= 0.1;
      b->destroyExpansion += 0.9;
      if (b->destroyScale <= 0) {
        b->isAlive = false;
        continue;
      }
    }

    float currentScale;
    if (b->shrinking) {
      currentScale = b->shrinkScale;
    } else if (b->destroyed) {
      currentScale = b->destroyScale;
    } else {
      currentScale = 1.0;
    }
    float expansionScale;
    if (b->destroyed) {
      expansionScale = b->destroyExpansion;
    } else {
      expansionScale = 1.0;
    }

    float pulseSize = (1 + sin(b->pulsePhase) * 0.3) * currentScale;

    color = LIGHT_PURPLE;
    bool windCollision = false;
    int wi;
    for (wi = 0; wi < 4; wi++) {
      float angle = b->rotation + (wi * CGLP_PI) / 2;
      float windX = b->pos.x + cos(angle) * 6 * currentScale * expansionScale;
      float windY = b->pos.y + sin(angle) * 6 * currentScale * expansionScale;
      thickness = 3;
      Collision windCol;
      arc(windX, windY, 2 * currentScale, 0, CGLP_PI * 2, &windCol);

      if (!b->shrinking && !b->destroyed && windCol.isColliding.rect[CYAN]) {
        windCollision = true;
      }
    }

    if (windCollision) {
      b->destroyed = true;

      addScore(jetstreamMultiplier, b->pos.x, b->pos.y);

      // Vircon32 port note: upstream picks between three sound-seed
      // variants of the same effect by multiplier tier - this engine's
      // play() has no per-call seed, so all three tiers just play POWER_UP.
      play(POWER_UP);
      jetstreamMultiplier = clamp(jetstreamMultiplier + 1, 1, 16);
    }

    color = PURPLE;
    thickness = 3;
    Collision coreCol;
    arc(b->pos.x, b->pos.y, 3 * pulseSize, 0, CGLP_PI * 2, &coreCol);

    if (!b->shrinking && !b->destroyed &&
        (coreCol.isColliding.character['a'] || coreCol.isColliding.character['b'] ||
         coreCol.isColliding.character['c'])) {
      if (b->lane == 0 || b->lane == 2) {
        jetstreamSurfer.knockbackVel.x += 3 * sqrt(difficulty);
      } else {
        jetstreamSurfer.knockbackVel.x -= 3 * sqrt(difficulty);
      }

      jetstreamSurfer.knockbackVel.y += rnd(-0.5, 0.5);

      b->shrinking = true;

      play(EXPLOSION);
    }

    if (!b->shrinking && !b->destroyed && (b->pos.x < -15 || b->pos.x > 115)) {
      jetstreamMultiplier = clamp(jetstreamMultiplier - 1, 1, 16);
      b->isAlive = false;
      continue;
    }
  }

  if (jetstreamSurfer.pos.x < -3 || jetstreamSurfer.pos.x > 103) {
    play(EXPLOSION);
    gameOver();
  }

  color = BLACK;
  int[16] jetstreamMultText;
  strcpy(jetstreamMultText, "x");
  strcat(jetstreamMultText, intToChar(jetstreamMultiplier));
  // Vircon32 port note: JS drew this with an unsupported isSmallText option;
  // drawn at normal text size instead (see gameSlimestretcher.c precedent).
  text(jetstreamMultText, 3, 9, &scratch);
}

void addGameJetstream() {
  addGame(jetstreamTitle, jetstreamDescription, jetstreamCharacters,
          jetstreamCharactersCount, &jetstreamOptions, false, &jetstreamUpdate);
}
