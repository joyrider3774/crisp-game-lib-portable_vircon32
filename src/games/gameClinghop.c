#include "../cglp.h"

int* clinghopTitle = "CLING HOP";
int* clinghopDescription = "[Tap] Flap";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] clinghopCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int clinghopCharactersCount = 0;

Options clinghopOptions = {100, 100, 3, false};

struct ClinghopPlayer {
  Vector pos;
  float vy;
  float vx;
};
ClinghopPlayer clinghopPlayer;

struct ClinghopPlatform {
  int id;
  Vector pos;
  float w;
  float vx;
  float phase;
  bool isAlive;
};
// Lifetime (~383/spd frames) and spawn interval (~90/spd frames, floor'd)
// both scale with spd=sqrt(difficulty), so concurrent count stays near a
// constant ~4.3 - sized with generous headroom.
#define CLINGHOP_MAX_PLATFORM_COUNT 16
ClinghopPlatform[CLINGHOP_MAX_PLATFORM_COUNT] clinghopPlatforms;
int clinghopPlatformIndex;

struct ClinghopObstacle {
  Vector pos;
  float vx;
  float vy;
  float spin;
  float squashX;
  float squashY;
  Vector[4] trail;
  int trailCount;
  bool isAlive;
};
// Obstacles bounce forever until popped by the player (not on a timer), and
// spawn interval shrinks to 180/sqrt(difficulty) ticks - a normal session can
// reach the old 48-slot cap, at which point the ring buffer's "count < cap"
// spawn guard can still overwrite a still-alive slot out of FIFO order;
// raised well above the realistic accumulation for headroom.
#define CLINGHOP_MAX_OBSTACLE_COUNT 128
ClinghopObstacle[CLINGHOP_MAX_OBSTACLE_COUNT] clinghopObstacles;
int clinghopObstacleIndex;

float clinghopNextObstacleTicks;
int clinghopClingTargetId;
int clinghopLastPlatformId;
Vector[5] clinghopTrail;
int clinghopTrailCount;
bool clinghopWasClinging;

void clinghopPushObstacleTrail(ClinghopObstacle* obs, float x, float y) {
  int i;
  for (i = 3; i > 0; i--) {
    obs->trail[i] = obs->trail[i - 1];
  }
  vectorSet(&obs->trail[0], x, y);
  if (obs->trailCount < 4) {
    obs->trailCount++;
  }
}

void clinghopUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&clinghopPlayer.pos, 50, 50);
    clinghopPlayer.vy = 0;
    clinghopPlayer.vx = 0;
    INIT_UNALIVED_ARRAY_FAST(clinghopPlatforms);
    clinghopPlatformIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(clinghopObstacles);
    clinghopObstacleIndex = 0;
    clinghopNextObstacleTicks = 0;
    clinghopClingTargetId = -1;
    clinghopLastPlatformId = -1;
    clinghopTrailCount = 0;
    clinghopWasClinging = false;
  }

  float spd = sqrt(difficulty);

  int platformSpawnInterval = (int)floor(90 / spd);
  if (platformSpawnInterval < 1) {
    platformSpawnInterval = 1;
  }
  if (ticks % platformSpawnInterval == 0) {
    float w = rnd(15, 30) / sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(clinghopPlatforms, clinghopPlatformIndex, ClinghopPlatform, np);
    np->id = ticks;
    vectorSet(&np->pos, rnd(20, 80), -5);
    np->w = w;
    np->vx = rnd(-0.3, 0.3) * spd;
    np->phase = rnd(0, CGLP_PI * 2);
    np->isAlive = true;
    clinghopPlatformIndex = cgl_wrap(clinghopPlatformIndex + 1, 0, CLINGHOP_MAX_PLATFORM_COUNT);
  }

  clinghopNextObstacleTicks--;
  COUNT_IS_ALIVE(clinghopObstacles, obstacleAliveCount);
  if ((clinghopNextObstacleTicks < 0 || obstacleAliveCount == 0) &&
      obstacleAliveCount < CLINGHOP_MAX_OBSTACLE_COUNT) {
    bool side = clinghopPlayer.pos.x > 50;
    ASSIGN_ARRAY_ITEM(clinghopObstacles, clinghopObstacleIndex, ClinghopObstacle, no);
    float sideSign;
    float startX;
    if (side) {
      startX = -5;
      sideSign = 1;
    } else {
      startX = 105;
      sideSign = -1;
    }
    vectorSet(&no->pos, startX, rnd(20, 60));
    no->vx = sideSign * rnd(0.5, 1.2) * spd;
    no->vy = rnd(-1, 1) * spd;
    no->spin = 0;
    no->squashX = 1;
    no->squashY = 1;
    no->trailCount = 0;
    no->isAlive = true;
    clinghopObstacleIndex = cgl_wrap(clinghopObstacleIndex + 1, 0, CLINGHOP_MAX_OBSTACLE_COUNT);
    clinghopNextObstacleTicks = 180 / spd;
  }

  float oldX = clinghopPlayer.pos.x;

  if (clinghopClingTargetId < 0) {
    clinghopPlayer.vy += 0.12;
    if (input.isJustPressed) {
      clinghopPlayer.vy = -2.2;
      play(JUMP);
      color = YELLOW;
      particle(clinghopPlayer.pos.x, clinghopPlayer.pos.y, 8, 1.5, CGLP_PI / 2, CGLP_PI / 3);
    }
    clinghopPlayer.pos.y += clinghopPlayer.vy;
    clinghopPlayer.pos.x = clamp(clinghopPlayer.pos.x, 5, 95);
  } else {
    ClinghopPlatform* plat = NULL;
    FOR_EACH(clinghopPlatforms, platIdx) {
      ASSIGN_ARRAY_ITEM(clinghopPlatforms, platIdx, ClinghopPlatform, p);
      SKIP_IS_NOT_ALIVE(p);
      if (p->id == clinghopClingTargetId) {
        plat = p;
      }
    }
    if (plat != NULL) {
      clinghopPlayer.pos.x = clamp(plat->pos.x + sin(ticks * 0.05 + plat->phase) * 20, 5, 95);
      clinghopPlayer.pos.y = plat->pos.y + 7;
      if (!clinghopWasClinging) {
        color = CYAN;
        particle(clinghopPlayer.pos.x, clinghopPlayer.pos.y, 12, 2, -CGLP_PI / 2, CGLP_PI / 2);
      }
      clinghopPlayer.vy = 0;
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      if (input.isJustPressed) {
        clinghopClingTargetId = -1;
        clinghopPlayer.vy = -2.5;
        play(JUMP);
        color = YELLOW;
        particle(clinghopPlayer.pos.x, clinghopPlayer.pos.y, 8, 1.5, CGLP_PI / 2, CGLP_PI / 3);
      }
    } else {
      clinghopClingTargetId = -1;
    }
  }

  clinghopPlayer.vx = clinghopPlayer.pos.x - oldX;
  clinghopWasClinging = clinghopClingTargetId >= 0;

  int i2;
  for (i2 = 4; i2 > 0; i2--) {
    clinghopTrail[i2] = clinghopTrail[i2 - 1];
  }
  vectorSet(&clinghopTrail[0], clinghopPlayer.pos.x, clinghopPlayer.pos.y);
  if (clinghopTrailCount < 5) {
    clinghopTrailCount++;
  }

  if (clinghopPlayer.pos.y > 105) {
    play(EXPLOSION);
    gameOver();
  }

  color = GREEN;
  FOR_EACH(clinghopPlatforms, pi2) {
    ASSIGN_ARRAY_ITEM(clinghopPlatforms, pi2, ClinghopPlatform, plat2);
    SKIP_IS_NOT_ALIVE(plat2);
    plat2->pos.y += 0.3 * spd;
    float drawX = plat2->pos.x + sin(ticks * 0.05 + plat2->phase) * 20;
    drawX = clamp(drawX, plat2->w / 2 + 2, 98 - plat2->w / 2);
    thickness = 4;
    bar(drawX, plat2->pos.y, plat2->w, 0, &scratch);

    if (clinghopClingTargetId < 0 && clinghopPlayer.vy > 0) {
      float dx = fabs(clinghopPlayer.pos.x - drawX);
      float dy = clinghopPlayer.pos.y - plat2->pos.y;
      if (dx < plat2->w / 2 + 3 && dy > -3 && dy < 8) {
        clinghopClingTargetId = plat2->id;
        if (plat2->id != clinghopLastPlatformId) {
          COUNT_IS_ALIVE(clinghopObstacles, obstacleCountAtLanding);
          addScore(obstacleCountAtLanding, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
          clinghopLastPlatformId = plat2->id;
          play(COIN);
        }
      }
    }

    if (plat2->pos.y >= 110) {
      plat2->isAlive = false;
      continue;
    }
  }

  color = LIGHT_YELLOW;
  int ti;
  for (ti = clinghopTrailCount - 1; ti >= 1; ti--) {
    float alpha = (float)(clinghopTrailCount - ti) / clinghopTrailCount;
    float size = 3 * alpha;
    box(clinghopTrail[ti].x, clinghopTrail[ti].y, size, size, &scratch);
  }

  float scaleX = 1;
  float scaleY = 1;
  float tilt = 0;
  if (clinghopClingTargetId >= 0) {
    float breath = sin(ticks * 0.1) * 0.08;
    scaleX = 1.4 + breath;
    scaleY = 0.75 - breath;
    tilt = 0;
  } else {
    float spinRate;
    if (clinghopPlayer.vy < 0) {
      spinRate = 1;
    } else {
      spinRate = 0.5;
    }
    tilt = ticks * 15 * spinRate;
    if (fabs(clinghopPlayer.vy) > 1.5) {
      scaleX = 1;
      scaleY = 1;
    } else {
      scaleX = 1 + fabs(clinghopPlayer.vx) * 0.1;
      scaleY = 1 - fabs(clinghopPlayer.vy) * 0.05;
    }
  }

  if (clinghopClingTargetId >= 0) {
    color = CYAN;
  } else {
    color = YELLOW;
  }
  float pw = 5 * scaleX;
  float ph = 7 * scaleY;
  float px = clinghopPlayer.pos.x;
  float py = clinghopPlayer.pos.y;

  thickness = ph;
  bar(px, py, pw, tilt, &scratch);

  if (clinghopClingTargetId >= 0) {
    float eyeOffsetX = (1.8 * scaleX) / 1.4;
    float eyeOffsetY = -0.3;
    float lookX = clamp(clinghopPlayer.vx * 2, -1, 1);

    color = WHITE;
    box(px - eyeOffsetX, py + eyeOffsetY, 2, 1.5, &scratch);
    box(px + eyeOffsetX, py + eyeOffsetY, 2, 1.5, &scratch);

    color = BLACK;
    box(px - eyeOffsetX + lookX * 0.5, py + eyeOffsetY, 1, 1, &scratch);
    box(px + eyeOffsetX + lookX * 0.5, py + eyeOffsetY, 1, 1, &scratch);
  }

  FOR_EACH(clinghopObstacles, oi) {
    ASSIGN_ARRAY_ITEM(clinghopObstacles, oi, ClinghopObstacle, obs);
    SKIP_IS_NOT_ALIVE(obs);
    obs->pos.x += obs->vx;
    obs->pos.y += obs->vy;

    clinghopPushObstacleTrail(obs, obs->pos.x, obs->pos.y);

    obs->spin += (fabs(obs->vx) + fabs(obs->vy)) * 0.1;

    obs->squashX += (1 - obs->squashX) * 0.2;
    obs->squashY += (1 - obs->squashY) * 0.2;

    color = RED;
    if (obs->pos.x < 3 || obs->pos.x > 97) {
      obs->vx *= -1;
      obs->pos.x = clamp(obs->pos.x, 3, 97);
      obs->squashX = 0.5;
      obs->squashY = 1.4;
      particle(obs->pos.x, obs->pos.y, 5, 1, 0, CGLP_PI * 2);
    }
    if (obs->pos.y < 3 || obs->pos.y > 97) {
      obs->vy *= -1;
      obs->pos.y = clamp(obs->pos.y, 3, 97);
      obs->squashX = 1.4;
      obs->squashY = 0.5;
      particle(obs->pos.x, obs->pos.y, 5, 1, 0, CGLP_PI * 2);
    }

    color = LIGHT_RED;
    int oti;
    for (oti = obs->trailCount - 1; oti >= 1; oti--) {
      float osize = (3.0 * (obs->trailCount - oti)) / obs->trailCount;
      box(obs->trail[oti].x, obs->trail[oti].y, osize, osize, &scratch);
    }

    color = RED;
    float ow = 5 * obs->squashX;
    float oh = 4 * obs->squashY;
    thickness = oh;
    Collision obsCollision;
    bar(obs->pos.x, obs->pos.y, ow, obs->spin, &obsCollision);

    float lookX2;
    if (obs->vx > 0) {
      lookX2 = 1;
    } else {
      lookX2 = -1;
    }
    float lookY2 = clamp(obs->vy * 0.5, -1, 1);
    color = WHITE;
    box(obs->pos.x - 1.2, obs->pos.y - 0.5, 1.5, 1.5, &scratch);
    box(obs->pos.x + 1.2, obs->pos.y - 0.5, 1.5, 1.5, &scratch);
    color = BLACK;
    box(obs->pos.x - 1.2 + lookX2 * 0.3, obs->pos.y - 0.5 + lookY2 * 0.3, 0.8, 0.8, &scratch);
    box(obs->pos.x + 1.2 + lookX2 * 0.3, obs->pos.y - 0.5 + lookY2 * 0.3, 0.8, 0.8, &scratch);

    if (obsCollision.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    } else if (obsCollision.isColliding.rect[YELLOW]) {
      play(POWER_UP);
      color = RED;
      particle(obs->pos.x, obs->pos.y, 20, 3, 0, CGLP_PI * 2);
      obs->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  COUNT_IS_ALIVE(clinghopObstacles, obstacleCountForText);
  int[16] countText;
  strcpy(countText, "x");
  strcat(countText, intToChar(obstacleCountForText));
  text(countText, 3, 9, &scratch);
}

void addGameClinghop() {
  addGame(clinghopTitle, clinghopDescription, clinghopCharacters,
          clinghopCharactersCount, &clinghopOptions, false, &clinghopUpdate);
}
