#include "../cglp.h"

int* pulsestreamTitle = "PULSE STREAM";
int* pulsestreamDescription = "[Tap]\n Push back\n drifting objects.\n Don't lose 3 lanes!";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pulsestreamCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int pulsestreamCharactersCount = 0;

Options pulsestreamOptions = {100, 100, 8, false};

#define PULSESTREAM_LANE_COUNT 5
#define PULSESTREAM_TAPS_TO_RECOVER 36
#define PULSESTREAM_SEQUENCE_LEN 15

struct PulsestreamLane {
  float y;
  float width;
  float height;
};
PulsestreamLane[PULSESTREAM_LANE_COUNT] pulsestreamLanes;

struct PulsestreamWave {
  Vector pos;
  Vector velocity;
  float amplitude;
  int laneIndex;
  float tapExpansion;
  bool isAlive;
};
// Spawn interval is a constant 36 frames and lifetime is a constant
// ~137/0.8 frames (not difficulty-scaled), so concurrent count stays near a
// constant ~4 - sized with generous headroom.
#define PULSESTREAM_MAX_WAVE_COUNT 32
PulsestreamWave[PULSESTREAM_MAX_WAVE_COUNT] pulsestreamWaves;
int pulsestreamWaveIndex;

struct PulsestreamObject {
  Vector pos;
  Vector velocity;
  float size;
  int laneIndex;
  bool isAlive;
};
// Spawn probability is 0.01*difficulty per tick (uncapped, linear in
// difficulty) and an object that never gets pushed by a wave just decays
// toward a near-stop without ever reaching the despawn edges, so population
// can hit the old 128-slot cap within a normal ~15-20 minute session;
// raised for headroom.
#define PULSESTREAM_MAX_OBJECT_COUNT 1024
PulsestreamObject[PULSESTREAM_MAX_OBJECT_COUNT] pulsestreamObjects;
int pulsestreamObjectIndex;

float pulsestreamStreamPhase;
int pulsestreamBubbleSpawnIndex;
bool[PULSESTREAM_LANE_COUNT] pulsestreamDisabledLanes;
int[PULSESTREAM_LANE_COUNT] pulsestreamLaneRecoveryProgress;
int pulsestreamLastTapTime;
int pulsestreamAutoTapTimer;
float pulsestreamRapidTapRate;
int[PULSESTREAM_SEQUENCE_LEN] pulsestreamLaneSequence;
int pulsestreamLaneIndex;

void pulsestreamShuffleSequence() {
  int i;
  for (i = PULSESTREAM_SEQUENCE_LEN - 1; i > 0; i--) {
    int j = rndi(0, i + 1);
    int tmp = pulsestreamLaneSequence[i];
    pulsestreamLaneSequence[i] = pulsestreamLaneSequence[j];
    pulsestreamLaneSequence[j] = tmp;
  }
}

void pulsestreamCreateLaneSequence() {
  int idx = 0;
  int repeat;
  for (repeat = 0; repeat < 3; repeat++) {
    int lane;
    for (lane = 0; lane < PULSESTREAM_LANE_COUNT; lane++) {
      pulsestreamLaneSequence[idx] = lane;
      idx++;
    }
  }
  pulsestreamShuffleSequence();
}

void pulsestreamUpdate() {
  Collision scratch;
  // Never reads a Collision result - pushes/lane logic are decided by lane/position math.
  hasCollision = false;
  if (!ticks) {
    pulsestreamLanes[0].y = 18;
    pulsestreamLanes[1].y = 36;
    pulsestreamLanes[2].y = 54;
    pulsestreamLanes[3].y = 72;
    pulsestreamLanes[4].y = 90;
    int li;
    for (li = 0; li < PULSESTREAM_LANE_COUNT; li++) {
      pulsestreamLanes[li].width = 100;
      pulsestreamLanes[li].height = 9;
    }

    INIT_UNALIVED_ARRAY_FAST(pulsestreamWaves);
    pulsestreamWaveIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(pulsestreamObjects);
    pulsestreamObjectIndex = 0;

    ASSIGN_ARRAY_ITEM(pulsestreamObjects, pulsestreamObjectIndex, PulsestreamObject, initialObj);
    vectorSet(&initialObj->pos, 98, pulsestreamLanes[2].y + rnd(0, 2) * RNDPM());
    vectorSet(&initialObj->velocity, -0.2, 0);
    initialObj->size = 3;
    initialObj->laneIndex = 2;
    initialObj->isAlive = true;
    pulsestreamObjectIndex = cgl_wrap(pulsestreamObjectIndex + 1, 0, PULSESTREAM_MAX_OBJECT_COUNT);

    pulsestreamStreamPhase = 0;
    pulsestreamBubbleSpawnIndex = 0;
    int di;
    for (di = 0; di < PULSESTREAM_LANE_COUNT; di++) {
      pulsestreamDisabledLanes[di] = false;
      pulsestreamLaneRecoveryProgress[di] = 0;
    }
    pulsestreamLastTapTime = ticks;
    pulsestreamAutoTapTimer = 0;
    pulsestreamRapidTapRate = 0;
    pulsestreamCreateLaneSequence();
    pulsestreamLaneIndex = 0;
  }

  pulsestreamStreamPhase += 0.05;

  if (ticks % 36 == 0) {
    int selectedLane = pulsestreamLaneSequence[pulsestreamLaneIndex];
    pulsestreamLaneIndex++;

    if (pulsestreamLaneIndex >= PULSESTREAM_SEQUENCE_LEN) {
      pulsestreamCreateLaneSequence();
      pulsestreamLaneIndex = 0;
      selectedLane = pulsestreamLaneSequence[pulsestreamLaneIndex];
      pulsestreamLaneIndex++;
    }

    if (pulsestreamDisabledLanes[selectedLane]) {
      bool foundActiveLane = false;
      int i;
      for (i = 0; i < PULSESTREAM_LANE_COUNT; i++) {
        selectedLane = (selectedLane + i) % PULSESTREAM_LANE_COUNT;
        if (!pulsestreamDisabledLanes[selectedLane]) {
          foundActiveLane = true;
          break;
        }
      }
      if (!foundActiveLane) {
        selectedLane = -1;
      }
    }

    if (selectedLane != -1) {
      ASSIGN_ARRAY_ITEM(pulsestreamWaves, pulsestreamWaveIndex, PulsestreamWave, nw);
      vectorSet(&nw->pos, -5, pulsestreamLanes[selectedLane].y);
      vectorSet(&nw->velocity, 0.8, 0);
      nw->amplitude = rnd(1, 1.7);
      nw->laneIndex = selectedLane;
      nw->tapExpansion = 0;
      nw->isAlive = true;
      pulsestreamWaveIndex = cgl_wrap(pulsestreamWaveIndex + 1, 0, PULSESTREAM_MAX_WAVE_COUNT);
    }
  }

  if (rnd(0, 1) < 0.01 * difficulty) {
    int selectedLane;
    if (rnd(0, 1) < 0.7) {
      selectedLane = pulsestreamBubbleSpawnIndex;
      pulsestreamBubbleSpawnIndex = (pulsestreamBubbleSpawnIndex + 1) % PULSESTREAM_LANE_COUNT;
    } else {
      selectedLane = rndi(0, PULSESTREAM_LANE_COUNT);
    }

    if (pulsestreamDisabledLanes[selectedLane]) {
      bool foundActiveLane = false;
      int i;
      for (i = 0; i < PULSESTREAM_LANE_COUNT; i++) {
        selectedLane = (selectedLane + 1) % PULSESTREAM_LANE_COUNT;
        if (!pulsestreamDisabledLanes[selectedLane]) {
          foundActiveLane = true;
          break;
        }
      }
      if (!foundActiveLane) {
        selectedLane = -1;
      }
    }

    if (selectedLane != -1) {
      COUNT_IS_ALIVE(pulsestreamObjects, objectAliveCount);
      if (objectAliveCount < PULSESTREAM_MAX_OBJECT_COUNT) {
        ASSIGN_ARRAY_ITEM(pulsestreamObjects, pulsestreamObjectIndex, PulsestreamObject, no);
        vectorSet(&no->pos, 98, pulsestreamLanes[selectedLane].y + rnd(0, 2) * RNDPM());
        vectorSet(&no->velocity, rnd(-0.3, -0.2), 0);
        no->size = rnd(2.5, 4);
        no->laneIndex = selectedLane;
        no->isAlive = true;
        pulsestreamObjectIndex = cgl_wrap(pulsestreamObjectIndex + 1, 0, PULSESTREAM_MAX_OBJECT_COUNT);
      }
    }
  }

  bool simulatedTap = false;
  if (input.isJustPressed) {
    pulsestreamLastTapTime = ticks;
    pulsestreamAutoTapTimer = 0;
  } else {
    if (ticks - pulsestreamLastTapTime >= 600) {
      pulsestreamAutoTapTimer++;
      if (pulsestreamAutoTapTimer >= 60) {
        simulatedTap = true;
        pulsestreamAutoTapTimer = 0;
      }
    }
  }

  bool tapNow = input.isJustPressed || simulatedTap;
  if (tapNow) {
    pulsestreamRapidTapRate += 0.3;
    FOR_EACH(pulsestreamWaves, wi) {
      ASSIGN_ARRAY_ITEM(pulsestreamWaves, wi, PulsestreamWave, w);
      SKIP_IS_NOT_ALIVE(w);
      w->tapExpansion = 1.5;
    }

    int li2;
    for (li2 = 0; li2 < PULSESTREAM_LANE_COUNT; li2++) {
      if (pulsestreamDisabledLanes[li2]) {
        pulsestreamLaneRecoveryProgress[li2]++;
        if (pulsestreamLaneRecoveryProgress[li2] >= PULSESTREAM_TAPS_TO_RECOVER) {
          pulsestreamDisabledLanes[li2] = false;
          pulsestreamLaneRecoveryProgress[li2] = 0;
          play(POWER_UP);
        }
      }
    }

    play(CLICK);
  }
  pulsestreamRapidTapRate *= 0.98;

  color = CYAN;
  FOR_EACH(pulsestreamWaves, wi2) {
    ASSIGN_ARRAY_ITEM(pulsestreamWaves, wi2, PulsestreamWave, wave);
    SKIP_IS_NOT_ALIVE(wave);
    vectorAdd(&wave->pos, wave->velocity.x, wave->velocity.y);
    wave->pos.y = pulsestreamLanes[wave->laneIndex].y;

    if (wave->tapExpansion > 0) {
      wave->tapExpansion -= 0.08;
      if (wave->tapExpansion < 0) {
        wave->tapExpansion = 0;
      }
    }

    float waveHeight = sin(pulsestreamStreamPhase + wave->pos.x * 0.1) * wave->amplitude;
    float expandedRadius = wave->amplitude * 3 + wave->tapExpansion * 4;
    arc(wave->pos.x, wave->pos.y + waveHeight, expandedRadius, 0, CGLP_PI * 2, &scratch);

    if (wave->pos.x > 105) {
      wave->isAlive = false;
      continue;
    }
  }

  color = RED;
  FOR_EACH(pulsestreamObjects, oi) {
    ASSIGN_ARRAY_ITEM(pulsestreamObjects, oi, PulsestreamObject, obj);
    SKIP_IS_NOT_ALIVE(obj);
    vectorAdd(&obj->pos, obj->velocity.x, obj->velocity.y);

    if (tapNow) {
      bool accelerated = false;
      FOR_EACH(pulsestreamWaves, wi3) {
        ASSIGN_ARRAY_ITEM(pulsestreamWaves, wi3, PulsestreamWave, pulse);
        SKIP_IS_NOT_ALIVE(pulse);
        if (obj->laneIndex == pulse->laneIndex &&
            distanceTo(&obj->pos, pulse->pos.x, pulse->pos.y) < pulse->amplitude * 6) {
          obj->velocity.x += 0.8;
          obj->velocity.y += rnd(0, 0.2) * RNDPM();
          accelerated = true;
          particle(obj->pos.x, obj->pos.y, 4, 1, 0, 2 * CGLP_PI);
          addScore(1, obj->pos.x, obj->pos.y);
          play(HIT);
        }
      }
      if (!accelerated) {
        obj->velocity.x -= 0.1 * (0.25 + pulsestreamRapidTapRate);
      }
    }

    obj->velocity.x *= 0.993;
    obj->velocity.y -= obj->velocity.y * 0.01;
    obj->velocity.y *= 0.9;

    float targetY = pulsestreamLanes[obj->laneIndex].y;
    obj->pos.y += (targetY - obj->pos.y) * 0.05;

    box(obj->pos.x, obj->pos.y, obj->size, obj->size * 2, &scratch);

    if (obj->pos.x < 0) {
      particle(obj->pos.x, obj->pos.y, 8, 2, 0, CGLP_PI);
      play(EXPLOSION);

      int failedLaneIndex = obj->laneIndex;
      pulsestreamDisabledLanes[failedLaneIndex] = true;
      pulsestreamLaneRecoveryProgress[failedLaneIndex] = 0;

      FOR_EACH(pulsestreamObjects, oi2) {
        ASSIGN_ARRAY_ITEM(pulsestreamObjects, oi2, PulsestreamObject, otherObj);
        SKIP_IS_NOT_ALIVE(otherObj);
        if (otherObj->laneIndex == failedLaneIndex) {
          otherObj->isAlive = false;
        }
      }
      FOR_EACH(pulsestreamWaves, wi4) {
        ASSIGN_ARRAY_ITEM(pulsestreamWaves, wi4, PulsestreamWave, otherWave);
        SKIP_IS_NOT_ALIVE(otherWave);
        if (otherWave->laneIndex == failedLaneIndex) {
          otherWave->isAlive = false;
        }
      }
      continue;
    }

    if (obj->pos.x > 100) {
      addScore(2, 95, obj->pos.y);
      particle(95, obj->pos.y, 6, 2, CGLP_PI, CGLP_PI / 4);
      play(COIN);
      obj->isAlive = false;
      continue;
    }
  }

  int disabledCount = 0;
  int dci;
  for (dci = 0; dci < PULSESTREAM_LANE_COUNT; dci++) {
    if (pulsestreamDisabledLanes[dci]) {
      disabledCount++;
    }
  }
  if (disabledCount >= 3) {
    play(EXPLOSION);
    gameOver();
  }

  int lci;
  for (lci = 0; lci < PULSESTREAM_LANE_COUNT; lci++) {
    PulsestreamLane* streamCurrent = &pulsestreamLanes[lci];
    if (pulsestreamDisabledLanes[lci]) {
      float recoveryProgress = (float)pulsestreamLaneRecoveryProgress[lci] / PULSESTREAM_TAPS_TO_RECOVER;
      float recoveryWidth = recoveryProgress * streamCurrent->width;

      if (recoveryWidth > 0) {
        color = BLUE;
        rect(0, streamCurrent->y, recoveryWidth, streamCurrent->height, &scratch);
      }
      if (recoveryWidth < streamCurrent->width) {
        color = LIGHT_BLUE;
        rect(recoveryWidth, streamCurrent->y, streamCurrent->width - recoveryWidth, streamCurrent->height, &scratch);
      }
    } else {
      color = BLUE;
      rect(0, streamCurrent->y, streamCurrent->width, streamCurrent->height, &scratch);
    }
  }
}

void addGamePulsestream() {
  addGame(pulsestreamTitle, pulsestreamDescription, pulsestreamCharacters,
          pulsestreamCharactersCount, &pulsestreamOptions, false, &pulsestreamUpdate);
}
