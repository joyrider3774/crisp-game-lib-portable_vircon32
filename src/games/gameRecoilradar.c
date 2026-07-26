#include "../cglp.h"

int* recoilradarTitle = "RECOIL RADAR";
int* recoilradarDescription = "[Hold] Charge Scan\n[Release] Fire + Recoil";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] recoilradarCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int recoilradarCharactersCount = 0;

Options recoilradarOptions = {100, 100, 2026071102, true};

struct RecoilradarPlayer {
  Vector pos;
  Vector vel;
};
RecoilradarPlayer recoilradarPlayer;

struct RecoilradarHazard {
  Vector pos;
  Vector vel;
  float size;
  bool isAlive;
};
// Concurrent hazards work out to roughly 3-8 at any realistic difficulty
// (spawn interval and drift speed both scale with difficulty in a way that
// keeps the ratio bounded) - sized with generous headroom.
#define RECOILRADAR_MAX_HAZARD_COUNT 48
RecoilradarHazard[RECOILRADAR_MAX_HAZARD_COUNT] recoilradarHazards;
int recoilradarHazardIndex;

float recoilradarScanRadius;
int recoilradarScanChargeTicks;
float recoilradarNextHazardTicks;
int recoilradarShotTicks;
Vector recoilradarShotFrom;
Vector recoilradarShotTo;
int recoilradarPlayerFlashTicks;

void recoilradarUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&recoilradarPlayer.pos, 50, 50);
    vectorSet(&recoilradarPlayer.vel, 0, 0);
    INIT_UNALIVED_ARRAY_FAST(recoilradarHazards);
    recoilradarHazardIndex = 0;
    recoilradarScanRadius = 8;
    recoilradarScanChargeTicks = 0;
    recoilradarNextHazardTicks = 45;
    recoilradarShotTicks = 0;
    vectorSet(&recoilradarShotFrom, 50, 50);
    vectorSet(&recoilradarShotTo, 50, 50);
    recoilradarPlayerFlashTicks = 0;
  }

  if (input.isPressed) {
    recoilradarScanChargeTicks++;
    recoilradarScanRadius = fmin(recoilradarScanRadius + 0.55, 46);
  } else if (!input.isJustReleased) {
    recoilradarScanChargeTicks = 0;
    recoilradarScanRadius = fmax(recoilradarScanRadius - 1.5, 8);
  }
  bool isScanCharged = recoilradarScanChargeTicks >= 10;
  if (recoilradarScanChargeTicks == 10) {
    color = LIGHT_CYAN;
    particle(recoilradarPlayer.pos.x, recoilradarPlayer.pos.y, 6, 0.8, 0, CGLP_PI * 2);
    play(SELECT);
  }

  if (input.isJustReleased) {
    int targetIndex = -1;
    float targetDistance = recoilradarScanRadius;
    if (isScanCharged) {
      FOR_EACH(recoilradarHazards, i) {
        ASSIGN_ARRAY_ITEM(recoilradarHazards, i, RecoilradarHazard, h);
        SKIP_IS_NOT_ALIVE(h);
        float d = distanceTo(&recoilradarPlayer.pos, h->pos.x, h->pos.y);
        if (d <= targetDistance) {
          targetDistance = d;
          targetIndex = i;
        }
      }
    }
    if (targetIndex >= 0) {
      ASSIGN_ARRAY_ITEM(recoilradarHazards, targetIndex, RecoilradarHazard, target);
      vectorSet(&recoilradarShotFrom, recoilradarPlayer.pos.x, recoilradarPlayer.pos.y);
      vectorSet(&recoilradarShotTo, target->pos.x, target->pos.y);
      recoilradarShotTicks = 5;
      float recoilAngle = angleTo(&target->pos, recoilradarPlayer.pos.x, recoilradarPlayer.pos.y);
      float kick = 1.2 + (46 - targetDistance) * 0.025;
      addWithAngle(&recoilradarPlayer.vel, recoilAngle, kick);
      addScore(round(55 - targetDistance), target->pos.x, target->pos.y);
      color = YELLOW;
      particle(target->pos.x, target->pos.y, 12, 2.2, recoilAngle, CGLP_PI * 2);
      color = CYAN;
      particle(recoilradarPlayer.pos.x, recoilradarPlayer.pos.y, 4, 1.4, recoilAngle + CGLP_PI, 0.5);
      target->isAlive = false;
      play(LASER);
    } else {
      play(CLICK);
    }
    recoilradarScanChargeTicks = 0;
    recoilradarScanRadius = 8;
  }

  vectorAdd(&recoilradarPlayer.pos, recoilradarPlayer.vel.x, recoilradarPlayer.vel.y);
  vectorMul(&recoilradarPlayer.vel, 0.985);
  bool didBounce = false;
  if (recoilradarPlayer.pos.x < 8) {
    recoilradarPlayer.pos.x = 8;
    recoilradarPlayer.vel.x = fabs(recoilradarPlayer.vel.x) * 0.8;
    didBounce = true;
  } else if (recoilradarPlayer.pos.x > 92) {
    recoilradarPlayer.pos.x = 92;
    recoilradarPlayer.vel.x = -fabs(recoilradarPlayer.vel.x) * 0.8;
    didBounce = true;
  }
  if (recoilradarPlayer.pos.y < 9) {
    recoilradarPlayer.pos.y = 9;
    recoilradarPlayer.vel.y = fabs(recoilradarPlayer.vel.y) * 0.8;
    didBounce = true;
  } else if (recoilradarPlayer.pos.y > 92) {
    recoilradarPlayer.pos.y = 92;
    recoilradarPlayer.vel.y = -fabs(recoilradarPlayer.vel.y) * 0.8;
    didBounce = true;
  }
  if (didBounce) {
    recoilradarPlayerFlashTicks = 2;
    color = CYAN;
    particle(recoilradarPlayer.pos.x, recoilradarPlayer.pos.y, 5, 1.2,
             vectorAngle(&recoilradarPlayer.vel) + CGLP_PI, CGLP_PI / 2);
    play(HIT);
  }

  recoilradarNextHazardTicks--;
  if (recoilradarNextHazardTicks < 0) {
    int edge = rndi(0, 4);
    Vector p;
    if (edge == 0) {
      vectorSet(&p, rnd(10, 90), -5);
    } else if (edge == 1) {
      vectorSet(&p, 105, rnd(10, 90));
    } else if (edge == 2) {
      vectorSet(&p, rnd(10, 90), 105);
    } else {
      vectorSet(&p, -5, rnd(10, 90));
    }
    float a = angleTo(&p, recoilradarPlayer.pos.x, recoilradarPlayer.pos.y) + rnd(-0.45, 0.45);
    ASSIGN_ARRAY_ITEM(recoilradarHazards, recoilradarHazardIndex, RecoilradarHazard, nh);
    nh->pos = p;
    vectorSet(&nh->vel, cos(a), sin(a));
    nh->size = rnd(5, 8);
    nh->isAlive = true;
    recoilradarHazardIndex = cgl_wrap(recoilradarHazardIndex + 1, 0, RECOILRADAR_MAX_HAZARD_COUNT);
    recoilradarNextHazardTicks += fmax(24, 72 / sqrt(difficulty));
  }

  if (input.isPressed) {
    if (isScanCharged) {
      color = LIGHT_CYAN;
    } else {
      color = LIGHT_BLACK;
    }
    thickness = 1.5;
    arc(recoilradarPlayer.pos.x, recoilradarPlayer.pos.y, recoilradarScanRadius, 0, CGLP_PI * 2, &scratch);
  }

  if (recoilradarShotTicks > 0) {
    color = YELLOW;
    thickness = 2;
    line(recoilradarShotFrom.x, recoilradarShotFrom.y, recoilradarShotTo.x, recoilradarShotTo.y, &scratch);
    recoilradarShotTicks--;
  }

  color = CYAN;
  box(recoilradarPlayer.pos.x, recoilradarPlayer.pos.y, 6, 6, &scratch);
  if (recoilradarPlayerFlashTicks > 0) {
    color = LIGHT_CYAN;
    box(recoilradarPlayer.pos.x, recoilradarPlayer.pos.y, 4, 4, &scratch);
    recoilradarPlayerFlashTicks--;
  }
  color = BLUE;
  thickness = 2;
  bar(recoilradarPlayer.pos.x, recoilradarPlayer.pos.y, 9, vectorAngle(&recoilradarPlayer.vel), &scratch);

  float rockSpeed = fmin(1.8, 0.45 + 0.18 * sqrt(difficulty));
  FOR_EACH(recoilradarHazards, hi) {
    ASSIGN_ARRAY_ITEM(recoilradarHazards, hi, RecoilradarHazard, h2);
    SKIP_IS_NOT_ALIVE(h2);
    vectorAdd(&h2->pos, h2->vel.x * rockSpeed, h2->vel.y * rockSpeed);
    bool isRevealed = input.isPressed && isScanCharged &&
        distanceTo(&recoilradarPlayer.pos, h2->pos.x, h2->pos.y) <= recoilradarScanRadius;
    if (isRevealed) {
      color = RED;
    } else {
      color = LIGHT_BLACK;
    }
    Collision hit;
    box(h2->pos.x, h2->pos.y, h2->size, h2->size, &hit);
    if (hit.isColliding.rect[CYAN] || hit.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
    if (h2->pos.x < -12 || h2->pos.x > 112 || h2->pos.y < -12 || h2->pos.y > 112) {
      h2->isAlive = false;
      continue;
    }
  }
}

void addGameRecoilradar() {
  addGame(recoilradarTitle, recoilradarDescription, recoilradarCharacters,
          recoilradarCharactersCount, &recoilradarOptions, false, &recoilradarUpdate);
}
