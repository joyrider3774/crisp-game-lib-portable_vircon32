#include "../cglp.h"

int* ghosthopTitle = "GHOST HOP";
int* ghosthopDescription = "[Tap]\n Hop";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] ghosthopCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int ghosthopCharactersCount = 0;

Options ghosthopOptions = {100, 100, 0, false};

struct GhosthopTarget {
  float x;
  float y;
  float vx;
  float vy;
  float squash;
};
#define GHOSTHOP_TARGET_COUNT 5
GhosthopTarget[GHOSTHOP_TARGET_COUNT] ghosthopTargets;

struct GhosthopPlayer {
  int idx;
  float squash;
  float stretch;
};
GhosthopPlayer ghosthopPlayer;

struct GhosthopBullet {
  float x;
  float y;
  float vx;
  float vy;
  bool isAlive;
};
#define GHOSTHOP_MAX_BULLET_COUNT 32
GhosthopBullet[GHOSTHOP_MAX_BULLET_COUNT] ghosthopBullets;
int ghosthopBulletIndex;

struct GhosthopTrail {
  float x;
  float y;
  int life;
  bool isAlive;
};
#define GHOSTHOP_MAX_TRAIL_COUNT 16
GhosthopTrail[GHOSTHOP_MAX_TRAIL_COUNT] ghosthopTrails;
int ghosthopTrailIndex;

struct GhosthopParticle {
  float x;
  float y;
  float vx;
  float vy;
  int life;
  int col;
  bool isAlive;
};
#define GHOSTHOP_MAX_PARTICLE_COUNT 64
GhosthopParticle[GHOSTHOP_MAX_PARTICLE_COUNT] ghosthopParticles;
int ghosthopParticleIndex;

bool ghosthopJumping;
int ghosthopJumpTargetIdx;
float ghosthopJumpT;
float ghosthopJumpFromX;
float ghosthopJumpFromY;
float ghosthopMultiplier;

void ghosthopUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(GHOSTHOP_TARGET_COUNT, ti) {
      ghosthopTargets[ti].x = 20 + (ti % 3) * 30;
      ghosthopTargets[ti].y = 25 + (ti / 3) * 40;
      ghosthopTargets[ti].vx = (rnd(0, 1) - 0.5) * 0.4;
      ghosthopTargets[ti].vy = (rnd(0, 1) - 0.5) * 0.4;
      ghosthopTargets[ti].squash = 0;
    }
    ghosthopPlayer.idx = 2;
    ghosthopPlayer.squash = 0;
    ghosthopPlayer.stretch = 0;
    INIT_UNALIVED_ARRAY_FAST(ghosthopBullets);
    ghosthopBulletIndex = 0;
    ghosthopJumping = false;
    ghosthopJumpTargetIdx = 0;
    ghosthopJumpT = 0;
    ghosthopJumpFromX = 50;
    ghosthopJumpFromY = 50;
    ghosthopMultiplier = 1;
    INIT_UNALIVED_ARRAY_FAST(ghosthopTrails);
    ghosthopTrailIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(ghosthopParticles);
    ghosthopParticleIndex = 0;
  }

  float bulletSpd = 0.5 * difficulty;

  int spawnInterval = (int)ceil(50.0 / difficulty);
  if (ticks % spawnInterval == 0) {
    int side = (int)rnd(0, 4);
    float bx;
    float by;
    GhosthopTarget* t = &ghosthopTargets[ghosthopPlayer.idx];
    if (side == 0) {
      bx = -5;
      by = rnd(0, 100);
    } else if (side == 1) {
      bx = 105;
      by = rnd(0, 100);
    } else if (side == 2) {
      bx = rnd(0, 100);
      by = -5;
    } else {
      bx = rnd(0, 100);
      by = 105;
    }
    float ang = atan2(t->y - by, t->x - bx);
    ASSIGN_ARRAY_ITEM(ghosthopBullets, ghosthopBulletIndex, GhosthopBullet, nb);
    nb->x = bx;
    nb->y = by;
    nb->vx = cos(ang) * bulletSpd;
    nb->vy = sin(ang) * bulletSpd;
    nb->isAlive = true;
    ghosthopBulletIndex = cgl_wrap(ghosthopBulletIndex + 1, 0, GHOSTHOP_MAX_BULLET_COUNT);
  }

  if (input.isJustPressed && !ghosthopJumping) {
    int nextIdx = (ghosthopPlayer.idx + 1) % GHOSTHOP_TARGET_COUNT;
    ghosthopJumping = true;
    ghosthopJumpTargetIdx = nextIdx;
    ghosthopJumpT = 0;
    GhosthopTarget* t = &ghosthopTargets[ghosthopPlayer.idx];
    ghosthopJumpFromX = t->x;
    ghosthopJumpFromY = t->y;
    ghosthopPlayer.stretch = 1.0;
    play(JUMP);
  }

  TIMES(GHOSTHOP_TARGET_COUNT, ti2) {
    GhosthopTarget* t = &ghosthopTargets[ti2];
    t->x += t->vx * sqrt(difficulty);
    t->y += t->vy * sqrt(difficulty);
    bool bounced = false;
    if (t->x < 15 || t->x > 85) {
      t->vx *= -1;
      bounced = true;
    }
    if (t->y < 15 || t->y > 85) {
      t->vy *= -1;
      bounced = true;
    }
    t->x = clamp(t->x, 15, 85);
    t->y = clamp(t->y, 15, 85);
    if (bounced) {
      t->squash = 0.5;
    }
    t->squash *= 0.85;
  }

  float px;
  float py;
  float eyeDirX = 0;
  float eyeDirY = 0;
  if (ghosthopJumping) {
    ghosthopJumpT += 0.08 * sqrt(difficulty);
    GhosthopTarget* tgt = &ghosthopTargets[ghosthopJumpTargetIdx];
    px = ghosthopJumpFromX + (tgt->x - ghosthopJumpFromX) * ghosthopJumpT;
    py = ghosthopJumpFromY + (tgt->y - ghosthopJumpFromY) * ghosthopJumpT;
    eyeDirX = tgt->x - px;
    eyeDirY = tgt->y - py;
    ghosthopPlayer.stretch = (1 - ghosthopJumpT) * 0.8;
    if (ticks % 3 == 0) {
      ASSIGN_ARRAY_ITEM(ghosthopTrails, ghosthopTrailIndex, GhosthopTrail, nt);
      nt->x = px;
      nt->y = py;
      nt->life = 16;
      nt->isAlive = true;
      ghosthopTrailIndex = cgl_wrap(ghosthopTrailIndex + 1, 0, GHOSTHOP_MAX_TRAIL_COUNT);
    }
    if (ghosthopJumpT >= 1) {
      ghosthopPlayer.idx = ghosthopJumpTargetIdx;
      ghosthopJumping = false;
      ghosthopPlayer.squash = 0.6;
      ghosthopPlayer.stretch = 0;
      play(POWER_UP);
      addScore(floor(ghosthopMultiplier), px, py);
      ghosthopMultiplier *= 2;
      TIMES(2, pj) {
        ASSIGN_ARRAY_ITEM(ghosthopParticles, ghosthopParticleIndex, GhosthopParticle, np);
        np->x = px;
        np->y = py;
        np->vx = (rnd(0, 1) - 0.5) * 2;
        np->vy = rnd(0, 1) * 1.5;
        np->life = 12;
        np->col = CYAN;
        np->isAlive = true;
        ghosthopParticleIndex = cgl_wrap(ghosthopParticleIndex + 1, 0, GHOSTHOP_MAX_PARTICLE_COUNT);
      }
    }
  } else {
    GhosthopTarget* t = &ghosthopTargets[ghosthopPlayer.idx];
    px = t->x;
    py = t->y;
    GhosthopTarget* nextT = &ghosthopTargets[(ghosthopPlayer.idx + 1) % GHOSTHOP_TARGET_COUNT];
    eyeDirX = nextT->x - px;
    eyeDirY = nextT->y - py;
  }

  FOR_EACH(ghosthopParticles, particleIdx) {
    ASSIGN_ARRAY_ITEM(ghosthopParticles, particleIdx, GhosthopParticle, p);
    SKIP_IS_NOT_ALIVE(p);
    p->x += p->vx;
    p->y += p->vy;
    p->vy += 0.1;
    p->life--;
    color = p->col;
    particle(p->x, p->y, 1, CGLP_PI / 4, CGLP_PI / 4, CGLP_PI * 2);
    p->isAlive = p->life > 0;
  }

  FOR_EACH(ghosthopTrails, tri) {
    ASSIGN_ARRAY_ITEM(ghosthopTrails, tri, GhosthopTrail, tr);
    SKIP_IS_NOT_ALIVE(tr);
    tr->life--;
    float alpha = (float)tr->life / 8;
    color = LIGHT_YELLOW;
    box(tr->x, tr->y, 5 * alpha, 5 * alpha, &scratch);
    tr->isAlive = tr->life > 0;
  }

  ghosthopPlayer.squash *= 0.85;

  TIMES(GHOSTHOP_TARGET_COUNT, ti3) {
    GhosthopTarget* t = &ghosthopTargets[ti3];
    bool isNext = (ti3 == (ghosthopPlayer.idx + 1) % GHOSTHOP_TARGET_COUNT) && !ghosthopJumping;
    if (isNext) {
      color = LIGHT_CYAN;
    } else {
      color = GREEN;
    }
    float baseSize;
    if (ti3 == ghosthopPlayer.idx && !ghosthopJumping) {
      baseSize = 8;
    } else {
      baseSize = 5;
    }
    float breath = sin(ticks * 0.1 + ti3) * 0.3;
    float sw = baseSize * (1 + t->squash * 0.5 + breath * 0.1);
    float sh = baseSize * (1 - t->squash * 0.3 + breath * 0.1);
    box(t->x, t->y, sw, sh, &scratch);
    bool isCurrent = ti3 == ghosthopPlayer.idx && !ghosthopJumping;
    float eyeSpacing = 1.5;
    if (isCurrent || isNext) {
      float lookX = t->vx;
      float lookY = t->vy;
      float len = sqrt(lookX * lookX + lookY * lookY);
      if (len > 0.01) {
        lookX /= len;
        lookY /= len;
      }
      color = WHITE;
      box(t->x - eyeSpacing, t->y - 1, 2, 2, &scratch);
      box(t->x + eyeSpacing, t->y - 1, 2, 2, &scratch);
      color = BLACK;
      box(t->x - eyeSpacing + lookX * 0.5, t->y - 1 + lookY * 0.5, 1, 1, &scratch);
      box(t->x + eyeSpacing + lookX * 0.5, t->y - 1 + lookY * 0.5, 1, 1, &scratch);
    } else {
      color = BLACK;
      box(t->x - eyeSpacing, t->y - 1, 2, 0.5, &scratch);
      box(t->x + eyeSpacing, t->y - 1, 2, 0.5, &scratch);
    }
  }

  if (ghosthopJumping) {
    color = YELLOW;
  } else {
    color = CYAN;
  }
  float breath2 = sin(ticks * 0.15) * 0.2;
  float pw = 5 * (1 + ghosthopPlayer.squash * 0.5 - ghosthopPlayer.stretch * 0.3 + breath2 * 0.1);
  float ph = 5 * (1 - ghosthopPlayer.squash * 0.3 + ghosthopPlayer.stretch * 0.5 + breath2 * 0.1);
  float tiltAngle = 0;
  if (ghosthopJumping) {
    GhosthopTarget* tgt = &ghosthopTargets[ghosthopJumpTargetIdx];
    tiltAngle = atan2(tgt->y - ghosthopJumpFromY, tgt->x - ghosthopJumpFromX);
  }
  if (fabs(tiltAngle) < 0.1) {
    box(px, py, pw, ph, &scratch);
  } else {
    thickness = pw * 0.8;
    barCenterPosRatio = 0.5;
    bar(px, py, ph, tiltAngle, &scratch);
  }
  float eyeLen = sqrt(eyeDirX * eyeDirX + eyeDirY * eyeDirY);
  if (eyeLen > 0.01) {
    eyeDirX /= eyeLen;
    eyeDirY /= eyeLen;
  }
  color = WHITE;
  float playerEyeSpacing = 1.5;
  box(px - playerEyeSpacing, py - 1, 2, 2, &scratch);
  box(px + playerEyeSpacing, py - 1, 2, 2, &scratch);
  color = BLACK;
  box(px - playerEyeSpacing + eyeDirX * 0.6, py - 1 + eyeDirY * 0.6, 1, 1, &scratch);
  box(px + playerEyeSpacing + eyeDirX * 0.6, py - 1 + eyeDirY * 0.6, 1, 1, &scratch);

  FOR_EACH(ghosthopBullets, bi) {
    ASSIGN_ARRAY_ITEM(ghosthopBullets, bi, GhosthopBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    b->x += b->vx;
    b->y += b->vy;
    if (b->x < -10 || b->x > 110 || b->y < -10 || b->y > 110) {
      b->isAlive = false;
      continue;
    }
    color = RED;
    float bulletAngle = atan2(b->vy, b->vx);
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision col;
    bar(b->x, b->y, 4, bulletAngle, &col);
    if (col.isColliding.rect[YELLOW]) {
      play(EXPLOSION);
      TIMES(3, pj2) {
        ASSIGN_ARRAY_ITEM(ghosthopParticles, ghosthopParticleIndex, GhosthopParticle, np2);
        np2->x = b->x;
        np2->y = b->y;
        np2->vx = (rnd(0, 1) - 0.5) * 3;
        np2->vy = (rnd(0, 1) - 0.5) * 3;
        np2->life = 10;
        np2->col = RED;
        np2->isAlive = true;
        ghosthopParticleIndex = cgl_wrap(ghosthopParticleIndex + 1, 0, GHOSTHOP_MAX_PARTICLE_COUNT);
      }
      gameOver();
    }
  }

  ghosthopMultiplier = clamp(ghosthopMultiplier * 0.99, 1, 256);
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar((int)ghosthopMultiplier));
  color = BLACK;
  text(multText, 3, 9, &scratch);
}

void addGameGhosthop() {
  addGame(ghosthopTitle, ghosthopDescription, ghosthopCharacters,
          ghosthopCharactersCount, &ghosthopOptions, false, &ghosthopUpdate);
}
