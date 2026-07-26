#include "../cglp.h"

int* missileguideTitle = "MISSILE GUIDE";
int* missileguideDescription = "[Hold]\n Freeze";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] missileguideCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int missileguideCharactersCount = 0;

Options missileguideOptions = {100, 100, 0, false};

struct MissileguideMissile {
  float x;
  float y;
  float vx;
  float vy;
};
MissileguideMissile missileguideMissile;

struct MissileguideOutpost {
  float x;
  float y;
  float pulsePhase;
  bool isAlive;
};
// Concurrent count stays low (~3-4 typical) since spawn interval and travel
// speed both scale with difficulty in offsetting ways - sized well above
// worst-case estimates.
#define MISSILEGUIDE_MAX_OUTPOST_COUNT 32
MissileguideOutpost[MISSILEGUIDE_MAX_OUTPOST_COUNT] missileguideOutposts;
int missileguideOutpostIndex;

struct MissileguideObstacle {
  float y;
  float gapX;
  float gapW;
  bool isAlive;
};
#define MISSILEGUIDE_MAX_OBSTACLE_COUNT 32
MissileguideObstacle[MISSILEGUIDE_MAX_OBSTACLE_COUNT] missileguideObstacles;
int missileguideObstacleIndex;

struct MissileguideTrail {
  float x;
  float y;
  int age;
  bool isAlive;
};
// Trail spawns at most once per frame and lives at most 9 frames.
#define MISSILEGUIDE_MAX_TRAIL_COUNT 32
MissileguideTrail[MISSILEGUIDE_MAX_TRAIL_COUNT] missileguideTrails;
int missileguideTrailIndex;

float missileguideScrollSpeed;
float missileguideNextSpawn;
float missileguideFuel;
int missileguideCombo;
float missileguideLastOutpostX;
bool missileguideWasFrozen;
float missileguideSquashTime;

void missileguideUpdate() {
  Collision scratch;
  if (!ticks) {
    missileguideMissile.x = 50;
    missileguideMissile.y = 65;
    missileguideMissile.vx = 0;
    missileguideMissile.vy = 0;
    INIT_UNALIVED_ARRAY_FAST(missileguideOutposts);
    missileguideOutpostIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(missileguideObstacles);
    missileguideObstacleIndex = 0;
    missileguideScrollSpeed = 0.6;
    missileguideNextSpawn = 30;
    missileguideFuel = 100;
    missileguideCombo = 0;
    missileguideLastOutpostX = 50;
    INIT_UNALIVED_ARRAY_FAST(missileguideTrails);
    missileguideTrailIndex = 0;
    missileguideWasFrozen = false;
    missileguideSquashTime = 0;
  }

  missileguideScrollSpeed = 0.6 * difficulty;
  bool frozen = input.isPressed;

  if (frozen && !missileguideWasFrozen) {
    missileguideSquashTime = 10;
    color = LIGHT_CYAN;
    particle(missileguideMissile.x, missileguideMissile.y, 12, 1, 0, CGLP_PI * 2);
  }
  if (!frozen && missileguideWasFrozen) {
    missileguideSquashTime = -10;
    color = CYAN;
    particle(missileguideMissile.x, missileguideMissile.y, 8, 2, -CGLP_PI / 2, CGLP_PI / 4);
  }
  missileguideWasFrozen = frozen;

  if (missileguideSquashTime > 0) {
    missileguideSquashTime--;
  }
  if (missileguideSquashTime < 0) {
    missileguideSquashTime++;
  }

  if (frozen) {
    missileguideFuel -= 0.4;
  } else {
    missileguideFuel = fmin(missileguideFuel + 0.1, 100);
  }

  if (missileguideFuel <= 0) {
    play(EXPLOSION);
    color = RED;
    particle(missileguideMissile.x, missileguideMissile.y, 30, 3, 0, CGLP_PI * 2);
    gameOver();
  }

  if (missileguideFuel < 20 && ticks % 10 == 0) {
    color = RED;
    particle(missileguideMissile.x, missileguideMissile.y + 5, 3, 0.5, CGLP_PI / 2, CGLP_PI / 4);
  }

  color = LIGHT_BLACK;
  rect(3, 97, missileguideFuel * 0.94, 2, &scratch);

  missileguideNextSpawn--;
  if (missileguideNextSpawn <= 0) {
    float newX = missileguideLastOutpostX + rnd(-40, 40);
    newX = clamp(newX, 12, 88);
    if (rnd(0, 1) < 0.8) {
      ASSIGN_ARRAY_ITEM(missileguideOutposts, missileguideOutpostIndex, MissileguideOutpost, no);
      no->x = newX;
      no->y = -5;
      no->pulsePhase = rnd(0, CGLP_PI * 2);
      no->isAlive = true;
      missileguideOutpostIndex = cgl_wrap(missileguideOutpostIndex + 1, 0, MISSILEGUIDE_MAX_OUTPOST_COUNT);
      missileguideLastOutpostX = newX;
    }

    if (ticks > 60 && rnd(0, 1) < 0.6) {
      float gapW = clamp(30, 16, 30);
      float gapX = newX + rnd(-20, 20);
      gapX = clamp(gapX, gapW / 2 + 5, 100 - gapW / 2 - 5);
      ASSIGN_ARRAY_ITEM(missileguideObstacles, missileguideObstacleIndex, MissileguideObstacle, nob);
      nob->y = -10;
      nob->gapX = gapX;
      nob->gapW = gapW;
      nob->isAlive = true;
      missileguideObstacleIndex = cgl_wrap(missileguideObstacleIndex + 1, 0, MISSILEGUIDE_MAX_OBSTACLE_COUNT);
    }
    missileguideNextSpawn = clamp(50 / difficulty, 25, 50);
  }

  float prevX = missileguideMissile.x;
  float prevY = missileguideMissile.y;

  if (!frozen) {
    bool targetFound = false;
    float targetX = 0;
    float targetY = 0;
    float minDist = 999;
    FOR_EACH(missileguideOutposts, oi) {
      ASSIGN_ARRAY_ITEM(missileguideOutposts, oi, MissileguideOutpost, o);
      SKIP_IS_NOT_ALIVE(o);
      if (o->y > missileguideMissile.y - 60 && o->y < missileguideMissile.y + 40) {
        float d = fabs(o->y - missileguideMissile.y);
        if (d < minDist) {
          minDist = d;
          targetX = o->x;
          targetY = o->y;
          targetFound = true;
        }
      }
    }
    if (targetFound) {
      float dx = targetX - missileguideMissile.x;
      float dy = targetY - missileguideMissile.y;
      missileguideMissile.x += clamp(dx * 0.03 * difficulty, -0.8, 0.8);
      missileguideMissile.y += clamp(dy * 0.01 * difficulty, -0.8, 0.8);
    }
  }

  missileguideMissile.x = clamp(missileguideMissile.x, 6, 94);
  missileguideMissile.y = clamp(missileguideMissile.y, 60, 90);

  missileguideMissile.vx = missileguideMissile.x - prevX;
  missileguideMissile.vy = missileguideMissile.y - prevY;

  if (!frozen && (fabs(missileguideMissile.vx) > 0.1 || fabs(missileguideMissile.vy) > 0.1)) {
    ASSIGN_ARRAY_ITEM(missileguideTrails, missileguideTrailIndex, MissileguideTrail, nt);
    nt->x = missileguideMissile.x;
    nt->y = missileguideMissile.y;
    nt->age = 0;
    nt->isAlive = true;
    missileguideTrailIndex = cgl_wrap(missileguideTrailIndex + 1, 0, MISSILEGUIDE_MAX_TRAIL_COUNT);
  }
  FOR_EACH(missileguideTrails, ti) {
    ASSIGN_ARRAY_ITEM(missileguideTrails, ti, MissileguideTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->age++;
    if (t->age > 8) {
      t->isAlive = false;
      continue;
    }
    float alpha = 1 - t->age / 8.0;
    color = LIGHT_CYAN;
    float size = 3 * alpha;
    box(t->x, t->y + t->age * 0.5, size, size * 1.5, &scratch);
  }

  float mWidth = 4;
  float mHeight = 7;
  if (missileguideSquashTime > 0) {
    mWidth = 4 + missileguideSquashTime * 0.3;
    mHeight = 7 - missileguideSquashTime * 0.4;
  } else if (missileguideSquashTime < 0) {
    mWidth = 4 + missileguideSquashTime * 0.2;
    mHeight = 7 - missileguideSquashTime * 0.5;
  }

  float tilt = clamp(missileguideMissile.vx * 0.3, -0.4, 0.4);

  color = CYAN;
  thickness = mWidth;
  barCenterPosRatio = 0.5;
  bar(missileguideMissile.x, missileguideMissile.y, mHeight, -CGLP_PI / 2 + tilt, &scratch);

  if (frozen) {
    color = LIGHT_CYAN;
    float pulseSize = 8 + sin(ticks * 0.2) * 2;
    thickness = 2;
    arc(missileguideMissile.x, missileguideMissile.y, pulseSize, 0, CGLP_PI * 2, &scratch);
  }

  FOR_EACH(missileguideOutposts, oi2) {
    ASSIGN_ARRAY_ITEM(missileguideOutposts, oi2, MissileguideOutpost, o2);
    SKIP_IS_NOT_ALIVE(o2);
    o2->y += missileguideScrollSpeed;
    o2->pulsePhase += 0.15;
    float pulse = 1 + sin(o2->pulsePhase) * 0.15;
    float size = 6 * pulse;
    color = YELLOW;
    Collision col;
    box(o2->x, o2->y, size, size, &col);
    if (!frozen && col.isColliding.rect[CYAN]) {
      play(COIN);
      missileguideCombo++;
      addScore(1 + floor(missileguideCombo / 3.0), SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      missileguideFuel = fmin(missileguideFuel + 12, 100);
      particle(o2->x, o2->y, 12, 2, 0, CGLP_PI * 2);
      o2->isAlive = false;
      continue;
    }
    if (o2->y > 105) {
      missileguideCombo = 0;
      o2->isAlive = false;
      continue;
    }
  }

  FOR_EACH(missileguideObstacles, oi3) {
    ASSIGN_ARRAY_ITEM(missileguideObstacles, oi3, MissileguideObstacle, ob);
    SKIP_IS_NOT_ALIVE(ob);
    ob->y += missileguideScrollSpeed;
    color = RED;
    float leftW = ob->gapX - ob->gapW / 2;
    float rightX = ob->gapX + ob->gapW / 2;
    float rightW = 100 - rightX;
    bool hitLeft = false;
    bool hitRight = false;
    if (leftW > 2) {
      Collision colL;
      box(leftW / 2, ob->y, leftW, 5, &colL);
      hitLeft = colL.isColliding.rect[CYAN];
    }
    if (rightW > 2) {
      Collision colR;
      box(rightX + rightW / 2, ob->y, rightW, 5, &colR);
      hitRight = colR.isColliding.rect[CYAN];
    }
    if (!frozen && (hitLeft || hitRight)) {
      play(EXPLOSION);
      color = YELLOW;
      particle(missileguideMissile.x, missileguideMissile.y, 25, 3, 0, CGLP_PI * 2);
      gameOver();
    }
    if (ob->y > 105) {
      ob->isAlive = false;
      continue;
    }
  }
}

void addGameMissileguide() {
  addGame(missileguideTitle, missileguideDescription, missileguideCharacters,
          missileguideCharactersCount, &missileguideOptions, false, &missileguideUpdate);
}
