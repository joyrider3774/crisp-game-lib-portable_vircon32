#include "../cglp.h"

int* podmigrationTitle = "POD MIGRATION";
int* podmigrationDescription = "[Tap]\n Change direction";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] podmigrationCharacters = {
    {
        "  lll ",
        "l lccl",
        "llclcl",
        "lcllll",
        " lwwwl",
        "  lll ",
    },
    {
        "  ll  ",
        " lppl ",
        "llpll ",
        " lll  ",
    },
};
int podmigrationCharactersCount = 2;

Options podmigrationOptions = {100, 100, 4, false};

Vector podmigrationLeadWhalePos;
Vector podmigrationLeadWhaleKnockbackVel;
int podmigrationLeadWhaleKnockbackFrames;

struct PodmigrationWhale {
  Vector pos;
  bool isDestroyed;
};
#define PODMIGRATION_POD_WHALE_COUNT 3
PodmigrationWhale[PODMIGRATION_POD_WHALE_COUNT] podmigrationPodWhales;

struct PodmigrationIce {
  Vector pos;
  Vector size;
  float jaggedness;
  int spawnTime;
  float emergeDuration;
  float lifespan;
  float sinkDuration;
  bool isAlive;
};
// targetIceCount = 3 + floor(difficulty/2) grows unbounded with difficulty (parity with the old
// cap of 48 hit at difficulty~90, i.e. ~90 min in) - raised for long sessions.
#define PODMIGRATION_MAX_ICE_COUNT 256
PodmigrationIce[PODMIGRATION_MAX_ICE_COUNT] podmigrationIceFloes;
int podmigrationIceIndex;

int podmigrationDirection;
float podmigrationSpeed;
int podmigrationRespawnTimer;
float podmigrationMultiplier;

void podmigrationUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&podmigrationLeadWhalePos, 20, 50);
    vectorSet(&podmigrationLeadWhaleKnockbackVel, 0, 0);
    podmigrationLeadWhaleKnockbackFrames = 0;
    vectorSet(&podmigrationPodWhales[0].pos, 10, 45);
    podmigrationPodWhales[0].isDestroyed = false;
    vectorSet(&podmigrationPodWhales[1].pos, 10, 55);
    podmigrationPodWhales[1].isDestroyed = false;
    vectorSet(&podmigrationPodWhales[2].pos, 5, 50);
    podmigrationPodWhales[2].isDestroyed = false;
    INIT_UNALIVED_ARRAY_FAST(podmigrationIceFloes);
    podmigrationIceIndex = 0;
    podmigrationDirection = 0;
    podmigrationSpeed = 1;
    podmigrationRespawnTimer = 0;
    podmigrationMultiplier = 1;
  }

  float currentSpeed = podmigrationSpeed + (difficulty - 1) * 0.3;
  float baseIceSpawnRate = 0.01 + (difficulty - 1) * 0.005;

  int targetIceCount = 3 + (int)floor(difficulty / 2);
  COUNT_IS_ALIVE(podmigrationIceFloes, curIceCount);
  float iceCountMultiplier = 1;
  if (curIceCount < targetIceCount) {
    iceCountMultiplier = 3;
  } else if (curIceCount < targetIceCount + 2) {
    iceCountMultiplier = 2;
  }
  float iceSpawnRate = baseIceSpawnRate * iceCountMultiplier;

  podmigrationMultiplier *= 0.998;
  podmigrationMultiplier = fmax(podmigrationMultiplier, 1);

  color = LIGHT_BLUE;
  for (int wy = 0; wy < 100; wy += 12) {
    for (int wx = 0; wx < 100; wx += 15) {
      float waveHeight = sin(ticks * 0.01 + wx * 0.1 + wy * 0.05) * 0.8;
      rect(wx, wy + waveHeight, 9, 2, &scratch);
    }
  }

  podmigrationRespawnTimer++;
  int respawnInterval = 300;
  if (podmigrationRespawnTimer >= respawnInterval) {
    int destroyedIdx = -1;
    TIMES(PODMIGRATION_POD_WHALE_COUNT, fi) {
      if (destroyedIdx < 0 && podmigrationPodWhales[fi].isDestroyed) {
        destroyedIdx = fi;
      }
    }
    if (destroyedIdx >= 0) {
      podmigrationPodWhales[destroyedIdx].isDestroyed = false;
      particle(podmigrationPodWhales[destroyedIdx].pos.x, podmigrationPodWhales[destroyedIdx].pos.y, 6, 1, 0, CGLP_PI * 2);
      play(COIN);
      podmigrationRespawnTimer = 0;
    }
  }

  if (input.isJustPressed) {
    podmigrationDirection = (podmigrationDirection + 1) % 4;
  }

  Vector vel;
  vectorSet(&vel, 0, 0);
  if (podmigrationDirection == 0) {
    vel.x = currentSpeed;
  } else if (podmigrationDirection == 1) {
    vel.y = currentSpeed;
  } else if (podmigrationDirection == 2) {
    vel.x = -currentSpeed;
  } else {
    vel.y = -currentSpeed;
  }

  bool canMove = true;

  TIMES(PODMIGRATION_POD_WHALE_COUNT, wi) {
    Vector target;
    if (wi == 0) {
      target = podmigrationLeadWhalePos;
    } else {
      target = podmigrationPodWhales[wi - 1].pos;
    }
    Vector offset;
    vectorSet(&offset, target.x - podmigrationPodWhales[wi].pos.x, target.y - podmigrationPodWhales[wi].pos.y);
    vectorMul(&offset, 0.05);
    vectorAdd(&podmigrationPodWhales[wi].pos, offset.x, offset.y);
  }

  color = BLACK;
  characterOptions.isMirrorX = podmigrationDirection >= 2;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  int[2] leadChar;
  leadChar[0] = 'a';
  leadChar[1] = 0;
  character(leadChar, podmigrationLeadWhalePos.x, podmigrationLeadWhalePos.y, &scratch);
  color = LIGHT_BLUE;
  particle(podmigrationLeadWhalePos.x, podmigrationLeadWhalePos.y, 1, 1, vectorAngle(&vel) + CGLP_PI, 0.2);

  color = BLACK;
  TIMES(PODMIGRATION_POD_WHALE_COUNT, wi2) {
    Vector target2;
    if (wi2 == 0) {
      target2 = podmigrationLeadWhalePos;
    } else {
      target2 = podmigrationPodWhales[wi2 - 1].pos;
    }
    PodmigrationWhale* whale = &podmigrationPodWhales[wi2];
    characterOptions.isMirrorX = whale->pos.x > target2.x;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    int[2] podChar;
    podChar[0] = 'b';
    podChar[1] = 0;
    if (!whale->isDestroyed) {
      Collision pc;
      character(podChar, whale->pos.x, whale->pos.y, &pc);
      if (pc.isColliding.character['a']) {
        whale->isDestroyed = true;
        particle(whale->pos.x, whale->pos.y, 4, 2, 0, CGLP_PI * 2);
        play(EXPLOSION);
        podmigrationRespawnTimer = 0;
      }
    } else {
      color = TRANSPARENT;
      character(podChar, whale->pos.x, whale->pos.y, &scratch);
      color = BLACK;
    }
  }

  if (rnd(0, 1) < iceSpawnRate) {
    Vector newIcePos;
    vectorSet(&newIcePos, rnd(10, 90), rnd(10, 90));
    float safeDistance = 20;
    bool isSafe = true;
    TIMES(PODMIGRATION_POD_WHALE_COUNT, wi3) {
      if (distanceTo(&newIcePos, podmigrationPodWhales[wi3].pos.x, podmigrationPodWhales[wi3].pos.y) < safeDistance) {
        isSafe = false;
      }
    }
    if (isSafe) {
      play(CLICK);
      ASSIGN_ARRAY_ITEM(podmigrationIceFloes, podmigrationIceIndex, PodmigrationIce, ice);
      ice->pos = newIcePos;
      vectorSet(&ice->size, rnd(9, 16), rnd(6, 10));
      ice->jaggedness = rnd(0.3, 0.8);
      ice->spawnTime = ticks;
      ice->emergeDuration = 150;
      ice->lifespan = rnd(500, 600);
      ice->sinkDuration = 120;
      ice->isAlive = true;
      podmigrationIceIndex = cgl_wrap(podmigrationIceIndex + 1, 0, PODMIGRATION_MAX_ICE_COUNT);
    }
  }

  FOR_EACH(podmigrationIceFloes, icei) {
    ASSIGN_ARRAY_ITEM(podmigrationIceFloes, icei, PodmigrationIce, ice);
    SKIP_IS_NOT_ALIVE(ice);
    bool iceDestroyed = false;

    float ageFrames = ticks - ice->spawnTime;
    float emergeProgress = fmin(ageFrames / ice->emergeDuration, 1);

    float sinkStartTime = ice->lifespan - ice->sinkDuration;
    float heightMultiplier = 1;
    bool isFullyEmerged = emergeProgress >= 1;
    bool isSinking = ageFrames >= sinkStartTime;

    if (ageFrames >= ice->lifespan) {
      ice->isAlive = false;
      continue;
    } else if (isSinking) {
      float sinkProgress = (ageFrames - sinkStartTime) / ice->sinkDuration;
      float sinkEase = pow(sinkProgress, 2);
      heightMultiplier = 1 - sinkEase;
    }

    float emergeEase = 1 - pow(1 - emergeProgress, 3);

    float finalHeightMultiplier = emergeEase * heightMultiplier;
    float visibleHeight = ice->size.y * finalHeightMultiplier;
    float emergeOffset = ice->size.y - visibleHeight;

    int jaggedParts = (int)floor(ice->size.x / 4);

    color = LIGHT_BLUE;
    TIMES(jaggedParts, jp1) {
      float partWidth = ice->size.x / jaggedParts;
      float waveOffset = sin(ticks * 0.02 + jp1 * 0.5) * 1.5;
      float partHeight = ice->size.y * 0.7 * finalHeightMultiplier;
      if (partHeight > 0) {
        rect(ice->pos.x - ice->size.x / 2 + jp1 * partWidth - 1,
             ice->pos.y + emergeOffset - partHeight / 2 + 2 + waveOffset,
             partWidth + 2, partHeight, &scratch);
      }
    }

    color = CYAN;
    bool iceCollidesA = false;
    bool iceCollidesB = false;
    TIMES(jaggedParts, jp2) {
      float partWidth = ice->size.x / jaggedParts;
      float heightVariation = ice->jaggedness * (sin(ice->pos.x * 0.1 + jp2) * 2);
      float fullPartHeight = ice->size.y + heightVariation;
      float partHeight = fullPartHeight * finalHeightMultiplier;
      if (partHeight > 0) {
        Collision partCollision;
        rect(ice->pos.x - ice->size.x / 2 + jp2 * partWidth - 1,
             ice->pos.y + emergeOffset - partHeight / 2,
             partWidth + 2, partHeight, &partCollision);
        if (partCollision.isColliding.character['a']) {
          iceCollidesA = true;
        }
        if (partCollision.isColliding.character['b']) {
          iceCollidesB = true;
        }
      }
    }

    color = LIGHT_CYAN;
    TIMES(jaggedParts, jp3) {
      float partWidth3 = (ice->size.x / jaggedParts) * 0.8;
      float heightVariation3 = ice->jaggedness * (sin(ice->pos.x * 0.1 + jp3) * 1);
      float fullPartHeight3 = ice->size.y * 0.6 + heightVariation3;
      float partHeight3 = fullPartHeight3 * finalHeightMultiplier;
      if (partHeight3 > 0) {
        rect(ice->pos.x - ice->size.x / 2 + jp3 * (ice->size.x / jaggedParts) - 1,
             ice->pos.y + emergeOffset - partHeight3 / 2 - 1,
             partWidth3, partHeight3, &scratch);
      }
    }

    if (iceCollidesB) {
      particle(ice->pos.x, ice->pos.y, 5, 2, 0, CGLP_PI);
      float points = round(podmigrationMultiplier);
      addScore(points, ice->pos.x, ice->pos.y);
      podmigrationMultiplier += 1;
      play(POWER_UP);
      iceDestroyed = true;
    }

    if (!iceDestroyed && isFullyEmerged && !isSinking) {
      if (iceCollidesA) {
        canMove = false;
      }
    }

    if (iceDestroyed) {
      ice->isAlive = false;
      continue;
    }
  }

  if (podmigrationLeadWhaleKnockbackFrames > 0) {
    vectorAdd(&podmigrationLeadWhalePos, podmigrationLeadWhaleKnockbackVel.x, podmigrationLeadWhaleKnockbackVel.y);
    vectorMul(&podmigrationLeadWhaleKnockbackVel, 0.88);
    podmigrationLeadWhaleKnockbackFrames--;
  }

  if (canMove) {
    vectorAdd(&podmigrationLeadWhalePos, vel.x, vel.y);
  } else {
    vectorSet(&podmigrationLeadWhaleKnockbackVel, vel.x * -1.8, vel.y * -1.8);
    podmigrationLeadWhaleKnockbackFrames = 25;
    play(HIT);
  }

  bool allPodWhalesDestroyed = true;
  TIMES(PODMIGRATION_POD_WHALE_COUNT, wi4) {
    if (!podmigrationPodWhales[wi4].isDestroyed) {
      allPodWhalesDestroyed = false;
    }
  }
  if (allPodWhalesDestroyed) {
    play(EXPLOSION);
    gameOver();
  }

  podmigrationLeadWhalePos.x = clamp(podmigrationLeadWhalePos.x, 3, 97);
  podmigrationLeadWhalePos.y = clamp(podmigrationLeadWhalePos.y, 3, 97);

  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar((int)round(podmigrationMultiplier)));
  text(multText, 3, 9, &scratch);
}

void addGamePodmigration() {
  addGame(podmigrationTitle, podmigrationDescription, podmigrationCharacters,
          podmigrationCharactersCount, &podmigrationOptions, false, &podmigrationUpdate);
}
