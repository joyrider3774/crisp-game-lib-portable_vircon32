#include "../cglp.h"

int* kitemasterTitle = "KITE MASTER";
int* kitemasterDescription = "[Hold]\n Pull string\n Catch clouds with kite";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] kitemasterCharacters = {
    {
        "  ll  ",
        "  l   ",
        "  l  l",
        "lllll ",
        "  l   ",
        "l  l  ",
    },
    {
        " ll   ",
        " l    ",
        "  l ll",
        "lllll ",
        " l    ",
        "l ll  ",
    },
};
int kitemasterCharactersCount = 2;

Options kitemasterOptions = {100, 100, 3, false};

struct KitemasterPlayer {
  Vector pos;
};
KitemasterPlayer kitemasterPlayer;

struct KitemasterKite {
  Vector pos;
  Vector vel;
};
KitemasterKite kitemasterKite;

struct KitemasterTailSegment {
  Vector pos;
  Vector vel;
};
#define KITEMASTER_TAIL_COUNT 5
KitemasterTailSegment[KITEMASTER_TAIL_COUNT] kitemasterTail;

struct KitemasterCloud {
  Vector pos;
  Vector vel;
  bool exists;
};
KitemasterCloud kitemasterCloud;

float kitemasterStringLength;
float kitemasterMaxStringLength;
float kitemasterWind;
float kitemasterWindTicks;

// Vircon32 port note: vector.h has no normalize() - divide by length here,
// guarding the (rare, but possible when kite and player exactly coincide)
// zero-length case since dividing by zero hard-traps this CPU.
void kitemasterNormalize(Vector* v) {
  float len = vectorLength(v);
  if (len > 0) {
    vectorMul(v, 1.0 / len);
  }
}

void kitemasterUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&kitemasterPlayer.pos, 20, 87);
    vectorSet(&kitemasterKite.pos, 35, 70);
    vectorSet(&kitemasterKite.vel, 0, 0);
    kitemasterStringLength = 20;
    kitemasterMaxStringLength = 120;
    kitemasterWind = 0.8;
    kitemasterWindTicks = 0;
    TIMES(KITEMASTER_TAIL_COUNT, i) {
      vectorSet(&kitemasterTail[i].pos, 35, 70 + (i + 1) * 3);
      vectorSet(&kitemasterTail[i].vel, 0, 0);
    }
    kitemasterCloud.exists = false;
  }

  kitemasterWind = 1 + 0.5 * sin(kitemasterWindTicks);
  kitemasterWindTicks += 0.01 * difficulty;

  Vector forces;
  vectorSet(&forces, 0, 0);

  Vector stringDirection;
  vectorSet(&stringDirection, kitemasterKite.pos.x - kitemasterPlayer.pos.x,
            kitemasterKite.pos.y - kitemasterPlayer.pos.y);
  kitemasterNormalize(&stringDirection);
  vectorAdd(&forces, stringDirection.x * (kitemasterWind * 0.81), stringDirection.y * (kitemasterWind * 0.81));
  vectorAdd(&forces, kitemasterWind * 0.19, 0);

  if (input.isPressed) {
    kitemasterStringLength = fmax(5, kitemasterStringLength - 0.2 * difficulty);
    vectorAdd(&forces, 0, -1);
  }
  if (input.isJustPressed) {
    play(LASER);
  }
  if (input.isJustReleased) {
    play(HIT);
  }

  float distanceToPlayer = distanceTo(&kitemasterKite.pos, kitemasterPlayer.pos.x, kitemasterPlayer.pos.y);
  if (distanceToPlayer > kitemasterStringLength) {
    kitemasterStringLength = fmin(kitemasterMaxStringLength,
                                  kitemasterStringLength + kitemasterWind * 0.1 * difficulty);
  }

  vectorAdd(&forces, 0, 0.3);

  if (distanceToPlayer > kitemasterStringLength) {
    Vector tensionDirection;
    vectorSet(&tensionDirection, kitemasterPlayer.pos.x - kitemasterKite.pos.x,
              kitemasterPlayer.pos.y - kitemasterKite.pos.y);
    kitemasterNormalize(&tensionDirection);
    float tensionForce = (distanceToPlayer - kitemasterStringLength) * 0.1;
    vectorAdd(&forces, tensionDirection.x * tensionForce, tensionDirection.y * tensionForce);
  }

  vectorMul(&forces, 0.02);
  vectorAdd(&kitemasterKite.vel, forces.x, forces.y);
  vectorMul(&kitemasterKite.vel, 0.98);

  vectorAdd(&kitemasterKite.pos, kitemasterKite.vel.x * difficulty, kitemasterKite.vel.y * difficulty);

  kitemasterKite.pos.x = clamp(kitemasterKite.pos.x, 5, 95);
  kitemasterKite.pos.y = clamp(kitemasterKite.pos.y, 5, 95);

  if (!kitemasterCloud.exists) {
    vectorSet(&kitemasterCloud.pos, rnd(25, 75), -5);
    vectorSet(&kitemasterCloud.vel, rnd(-0.1, 0.1), 0.15);
    kitemasterCloud.exists = true;
  }

  if (kitemasterCloud.exists) {
    vectorAdd(&kitemasterCloud.pos, kitemasterCloud.vel.x * difficulty, kitemasterCloud.vel.y * difficulty);
    if ((kitemasterCloud.pos.x <= 20 && kitemasterCloud.vel.x < 0) ||
        (kitemasterCloud.pos.x >= 80 && kitemasterCloud.vel.x > 0)) {
      kitemasterCloud.vel.x *= -1;
    }
    if (kitemasterCloud.pos.y > 90) {
      play(EXPLOSION);
      gameOver();
    }
  }

  float stringAngle = cgl_atan2(kitemasterKite.pos.y - kitemasterPlayer.pos.y,
                                 kitemasterKite.pos.x - kitemasterPlayer.pos.x);
  float windInfluence = kitemasterWind * 0.3;
  float kiteAngle = stringAngle + windInfluence + CGLP_PI / 2;

  TIMES(KITEMASTER_TAIL_COUNT, ti) {
    KitemasterTailSegment* segment = &kitemasterTail[ti];
    vectorAdd(&segment->vel, 0, 0.1);
    float windForce = kitemasterWind * 0.05 * (ti + 1);
    vectorAdd(&segment->vel, windForce + rnd(0, 0.25) * RNDPM(), rnd(0, 0.25) * RNDPM());
    vectorMul(&segment->vel, 0.99);

    Vector anchor;
    if (ti == 0) {
      vectorSet(&anchor, kitemasterKite.pos.x, kitemasterKite.pos.y);
      addWithAngle(&anchor, kiteAngle, 6);
    } else {
      vectorSet(&anchor, kitemasterTail[ti - 1].pos.x, kitemasterTail[ti - 1].pos.y);
    }

    float tailDistance = distanceTo(&segment->pos, anchor.x, anchor.y);
    float maxDistance = 4;
    if (tailDistance > maxDistance) {
      float dirX = (anchor.x - segment->pos.x) / tailDistance;
      float dirY = (anchor.y - segment->pos.y) / tailDistance;
      segment->pos.x = anchor.x - dirX * maxDistance;
      segment->pos.y = anchor.y - dirY * maxDistance;
    }

    vectorMul(&segment->vel, 0.85);
    vectorAdd(&segment->pos, segment->vel.x, segment->vel.y);
    segment->pos.x = clamp(segment->pos.x, 2, 98);
    segment->pos.y = clamp(segment->pos.y, 2, 90);
  }

  color = GREEN;
  rect(0, 90, 100, 10, &scratch);

  color = BLACK;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  int[2] playerChar;
  if (input.isPressed) {
    playerChar[0] = 'b';
  } else {
    playerChar[0] = 'a';
  }
  playerChar[1] = 0;
  character(playerChar, kitemasterPlayer.pos.x, kitemasterPlayer.pos.y, &scratch);

  color = LIGHT_BLACK;
  thickness = 1;
  float lineEndX = kitemasterKite.pos.x - stringDirection.x * 6;
  float lineEndY = kitemasterKite.pos.y - stringDirection.y * 6;
  line(kitemasterPlayer.pos.x + 3, kitemasterPlayer.pos.y, lineEndX, lineEndY, &scratch);

  color = BLUE;
  thickness = 3;
  barCenterPosRatio = 0.5;
  bar(kitemasterKite.pos.x, kitemasterKite.pos.y, 12, kiteAngle, &scratch);

  color = LIGHT_BLUE;
  TIMES(KITEMASTER_TAIL_COUNT, si) {
    float size = fmax(2, 4 - si * 0.5);
    box(kitemasterTail[si].pos.x, kitemasterTail[si].pos.y, size, size, &scratch);
  }

  if (kitemasterCloud.exists) {
    color = CYAN;
    thickness = 3;
    Collision c1;
    arc(kitemasterCloud.pos.x - 3, kitemasterCloud.pos.y, 3, 0, CGLP_PI * 2, &c1);
    Collision c2;
    arc(kitemasterCloud.pos.x + 3, kitemasterCloud.pos.y, 3, 0, CGLP_PI * 2, &c2);

    if (c1.isColliding.rect[BLUE] || c1.isColliding.character['a'] || c1.isColliding.character['b'] ||
        c2.isColliding.rect[BLUE] || c2.isColliding.character['a'] || c2.isColliding.character['b']) {
      float cloudScoreValue = 100 - kitemasterCloud.pos.y;
      kitemasterCloud.pos.x = clamp(kitemasterCloud.pos.x, 10, 90);
      kitemasterCloud.pos.y = clamp(kitemasterCloud.pos.y, 10, 90);
      addScore(cloudScoreValue, kitemasterCloud.pos.x, kitemasterCloud.pos.y);
      play(COIN);
      kitemasterCloud.exists = false;
    }
  }

  color = CYAN;
  rect(5, 5, kitemasterWind * 30, 3, &scratch);

  if (kitemasterKite.pos.y > 85) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameKitemaster() {
  addGame(kitemasterTitle, kitemasterDescription, kitemasterCharacters,
          kitemasterCharactersCount, &kitemasterOptions, false, &kitemasterUpdate);
}
