#include "../cglp.h"

int* stillfirechoirTitle = "STILLFIRE CHOIR";
int* stillfirechoirDescription = "[Hold] Fix choir\nAuto-ray on pulse";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] stillfirechoirCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int stillfirechoirCharactersCount = 0;

Options stillfirechoirOptions = {100, 100, 5, false};

struct StillfirechoirPlayer {
  Vector pos;
  float vx;
};
StillfirechoirPlayer stillfirechoirPlayer;

struct StillfirechoirAlly {
  Vector pos;
  float vx;
  int spawnTicks;
  bool isAlive;
};
// Allies never expire (unbounded growth): item spawn rate integrates to
// ~220 items by ~5min and ~570 by ~10min of play (interval ~150/sqrt(difficulty),
// difficulty = ticks/3600+1), and a chunk of those get caught since allies are
// the score multiplier - 128 can wrap during a single good run, raised to 512.
#define STILLFIRECHOIR_MAX_ALLY_COUNT 512
StillfirechoirAlly[STILLFIRECHOIR_MAX_ALLY_COUNT] stillfirechoirAllies;
int stillfirechoirAllyIndex;

struct StillfirechoirEnemy {
  Vector pos;
  float speed;
  float size;
  bool isAlive;
};
#define STILLFIRECHOIR_MAX_ENEMY_COUNT 32
StillfirechoirEnemy[STILLFIRECHOIR_MAX_ENEMY_COUNT] stillfirechoirEnemies;
int stillfirechoirEnemyIndex;

struct StillfirechoirItem {
  Vector pos;
  float speed;
  bool isAlive;
};
#define STILLFIRECHOIR_MAX_ITEM_COUNT 16
StillfirechoirItem[STILLFIRECHOIR_MAX_ITEM_COUNT] stillfirechoirItems;
int stillfirechoirItemIndex;

float stillfirechoirNextEnemyTicks;
float stillfirechoirNextItemTicks;
float stillfirechoirNextVolleyTicks;
float stillfirechoirVolleySpan;
float stillfirechoirBeamTicks;
float stillfirechoirBeamAfterglowTicks;
float stillfirechoirLockPulseTicks;

void stillfirechoirUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&stillfirechoirPlayer.pos, 50, 88);
    stillfirechoirPlayer.vx = 0.72;
    INIT_UNALIVED_ARRAY_FAST(stillfirechoirAllies);
    stillfirechoirAllyIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(stillfirechoirEnemies);
    stillfirechoirEnemyIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(stillfirechoirItems);
    stillfirechoirItemIndex = 0;
    stillfirechoirNextEnemyTicks = 25;
    stillfirechoirNextItemTicks = 75;
    stillfirechoirVolleySpan = 62;
    stillfirechoirNextVolleyTicks = stillfirechoirVolleySpan;
    stillfirechoirBeamTicks = 0;
    stillfirechoirBeamAfterglowTicks = 0;
    stillfirechoirLockPulseTicks = 0;
  }

  float pressure = sqrt(difficulty);

  if (input.isJustPressed) {
    stillfirechoirLockPulseTicks = 5;
    play(SELECT);
  }

  if (!input.isPressed) {
    stillfirechoirPlayer.pos.x += stillfirechoirPlayer.vx * pressure;
    if (stillfirechoirPlayer.pos.x < 5 || stillfirechoirPlayer.pos.x > 95) {
      stillfirechoirPlayer.pos.x = clamp(stillfirechoirPlayer.pos.x, 5, 95);
      stillfirechoirPlayer.vx *= -1;
      color = LIGHT_BLUE;
      float bounceAngle = 0;
      if (stillfirechoirPlayer.vx <= 0) {
        bounceAngle = CGLP_PI;
      }
      particle(stillfirechoirPlayer.pos.x, stillfirechoirPlayer.pos.y, 3, 0.8, bounceAngle, CGLP_PI / 3);
      play(CLICK);
    }
    FOR_EACH(stillfirechoirAllies, ai) {
      ASSIGN_ARRAY_ITEM(stillfirechoirAllies, ai, StillfirechoirAlly, ally);
      SKIP_IS_NOT_ALIVE(ally);
      ally->pos.x += ally->vx * pressure;
      if (ally->pos.x < 4 || ally->pos.x > 96) {
        ally->pos.x = clamp(ally->pos.x, 4, 96);
        ally->vx *= -1;
        color = LIGHT_BLUE;
        float allyBounceAngle = 0;
        if (ally->vx <= 0) {
          allyBounceAngle = CGLP_PI;
        }
        particle(ally->pos.x, ally->pos.y, 2, 0.7, allyBounceAngle, CGLP_PI / 3);
      }
    }
  }

  stillfirechoirNextVolleyTicks--;
  if (stillfirechoirNextVolleyTicks <= 0) {
    stillfirechoirBeamTicks = 1;
    // Four includes the live beam frame, leaving three visual-only frames.
    stillfirechoirBeamAfterglowTicks = 4;
    stillfirechoirVolleySpan = fmax(36, 62 / pressure);
    stillfirechoirNextVolleyTicks = stillfirechoirVolleySpan;
    play(LASER);
  }

  stillfirechoirNextEnemyTicks--;
  if (stillfirechoirNextEnemyTicks <= 0) {
    ASSIGN_ARRAY_ITEM(stillfirechoirEnemies, stillfirechoirEnemyIndex, StillfirechoirEnemy, ne);
    vectorSet(&ne->pos, rnd(5, 95), -4);
    ne->speed = rnd(0.45, 0.78);
    ne->size = rnd(5, 9);
    ne->isAlive = true;
    stillfirechoirEnemyIndex = cgl_wrap(stillfirechoirEnemyIndex + 1, 0, STILLFIRECHOIR_MAX_ENEMY_COUNT);
    stillfirechoirNextEnemyTicks = rnd(31, 49) / pressure;
  }

  stillfirechoirNextItemTicks--;
  if (stillfirechoirNextItemTicks <= 0) {
    ASSIGN_ARRAY_ITEM(stillfirechoirItems, stillfirechoirItemIndex, StillfirechoirItem, ni);
    vectorSet(&ni->pos, rnd(7, 93), -3);
    ni->speed = rnd(0.36, 0.52);
    ni->isAlive = true;
    stillfirechoirItemIndex = cgl_wrap(stillfirechoirItemIndex + 1, 0, STILLFIRECHOIR_MAX_ITEM_COUNT);
    stillfirechoirNextItemTicks = rnd(125, 175) / pressure;
  }

  color = BLUE;
  box(stillfirechoirPlayer.pos.x, stillfirechoirPlayer.pos.y, 7, 7, &scratch);
  FOR_EACH(stillfirechoirAllies, ai2) {
    ASSIGN_ARRAY_ITEM(stillfirechoirAllies, ai2, StillfirechoirAlly, ally2);
    SKIP_IS_NOT_ALIVE(ally2);
    box(ally2->pos.x, ally2->pos.y, 5, 5, &scratch);
  }

  // The emitters themselves are the volley gauge: their cores fill upward
  // as the next synchronized ray approaches. Keep the blue body unchanged
  // so the visual charge never alters its collision footprint.
  float volleyCharge = clamp(1 - stillfirechoirNextVolleyTicks / stillfirechoirVolleySpan, 0, 1);
  bool isChargeFlickering = stillfirechoirNextVolleyTicks < 10 && (ticks / 2) % 2 == 0;
  if (isChargeFlickering) {
    color = LIGHT_BLUE;
  } else {
    color = LIGHT_CYAN;
  }
  float playerChargeHeight = 5 * volleyCharge;
  rect(stillfirechoirPlayer.pos.x - 1.5, stillfirechoirPlayer.pos.y + 2.5 - playerChargeHeight, 3,
       playerChargeHeight, &scratch);
  FOR_EACH(stillfirechoirAllies, ai3) {
    ASSIGN_ARRAY_ITEM(stillfirechoirAllies, ai3, StillfirechoirAlly, ally3);
    SKIP_IS_NOT_ALIVE(ally3);
    float allyChargeHeight = 3 * volleyCharge;
    rect(ally3->pos.x - 1, ally3->pos.y + 1.5 - allyChargeHeight, 2, allyChargeHeight, &scratch);
  }

  // A brief ring makes the exact hold edge tactile without enlarging or
  // deforming the collision bodies.
  if (stillfirechoirLockPulseTicks > 0) {
    color = LIGHT_CYAN;
    thickness = 1;
    arc(stillfirechoirPlayer.pos.x, stillfirechoirPlayer.pos.y, 4 + stillfirechoirLockPulseTicks * 0.35, 0,
        CGLP_PI * 2, &scratch);
    FOR_EACH(stillfirechoirAllies, ai4) {
      ASSIGN_ARRAY_ITEM(stillfirechoirAllies, ai4, StillfirechoirAlly, ally4);
      SKIP_IS_NOT_ALIVE(ally4);
      thickness = 1;
      arc(ally4->pos.x, ally4->pos.y, 3 + stillfirechoirLockPulseTicks * 0.25, 0, CGLP_PI * 2, &scratch);
    }
  }

  // Newly collected voices settle into the formation with a short ring.
  color = LIGHT_YELLOW;
  FOR_EACH(stillfirechoirAllies, ai5) {
    ASSIGN_ARRAY_ITEM(stillfirechoirAllies, ai5, StillfirechoirAlly, ally5);
    SKIP_IS_NOT_ALIVE(ally5);
    if (ally5->spawnTicks > 0) {
      thickness = 1;
      arc(ally5->pos.x, ally5->pos.y, 3 + ally5->spawnTicks * 0.35, 0, CGLP_PI * 2, &scratch);
      ally5->spawnTicks--;
    }
  }

  if (stillfirechoirBeamTicks > 0) {
    color = CYAN;
    rect(stillfirechoirPlayer.pos.x - 1, 6, 2, stillfirechoirPlayer.pos.y - 8, &scratch);
    FOR_EACH(stillfirechoirAllies, ai6) {
      ASSIGN_ARRAY_ITEM(stillfirechoirAllies, ai6, StillfirechoirAlly, ally6);
      SKIP_IS_NOT_ALIVE(ally6);
      rect(ally6->pos.x - 1, 6, 2, ally6->pos.y - 8, &scratch);
    }
  } else if (stillfirechoirBeamAfterglowTicks > 0) {
    color = LIGHT_CYAN;
    float afterglowWidth = 0.5 + stillfirechoirBeamAfterglowTicks * 0.25;
    rect(stillfirechoirPlayer.pos.x - afterglowWidth / 2, 6, afterglowWidth, stillfirechoirPlayer.pos.y - 8,
         &scratch);
    FOR_EACH(stillfirechoirAllies, ai7) {
      ASSIGN_ARRAY_ITEM(stillfirechoirAllies, ai7, StillfirechoirAlly, ally7);
      SKIP_IS_NOT_ALIVE(ally7);
      rect(ally7->pos.x - afterglowWidth / 2, 6, afterglowWidth, ally7->pos.y - 8, &scratch);
    }
  }

  color = YELLOW;
  FOR_EACH(stillfirechoirItems, ii) {
    ASSIGN_ARRAY_ITEM(stillfirechoirItems, ii, StillfirechoirItem, item);
    SKIP_IS_NOT_ALIVE(item);
    item->pos.y += item->speed * pressure;
    Collision hit;
    box(item->pos.x, item->pos.y, 5, 5, &hit);
    if (hit.isColliding.rect[BLUE]) {
      int direction = 1;
      if (rndi(0, 2) == 0) {
        direction = -1;
      }
      ASSIGN_ARRAY_ITEM(stillfirechoirAllies, stillfirechoirAllyIndex, StillfirechoirAlly, na);
      vectorSet(&na->pos, item->pos.x, item->pos.y);
      na->vx = direction * rnd(0.48, 0.9);
      na->spawnTicks = 8;
      na->isAlive = true;
      stillfirechoirAllyIndex = cgl_wrap(stillfirechoirAllyIndex + 1, 0, STILLFIRECHOIR_MAX_ALLY_COUNT);
      addScore(5, item->pos.x, item->pos.y);
      particle(item->pos.x, item->pos.y, 8, 1.2, -CGLP_PI_2, CGLP_PI * 2);
      play(POWER_UP);
      item->isAlive = false;
      continue;
    }
    item->isAlive = item->pos.y <= 105;
  }

  COUNT_IS_ALIVE(stillfirechoirAllies, stillfirechoirAllyCount);
  color = RED;
  FOR_EACH(stillfirechoirEnemies, ei) {
    ASSIGN_ARRAY_ITEM(stillfirechoirEnemies, ei, StillfirechoirEnemy, enemy);
    SKIP_IS_NOT_ALIVE(enemy);
    enemy->pos.y += enemy->speed * pressure;
    Collision hit2;
    box(enemy->pos.x, enemy->pos.y, enemy->size, enemy->size, &hit2);
    if (hit2.isColliding.rect[CYAN]) {
      int points = 10 + stillfirechoirAllyCount * 2;
      addScore(points, enemy->pos.x, enemy->pos.y);
      particle(enemy->pos.x, enemy->pos.y, 9, 1.5, CGLP_PI_2, CGLP_PI * 2);
      play(COIN);
      enemy->isAlive = false;
      continue;
    }
    if (hit2.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
    enemy->isAlive = enemy->pos.y <= 105;
  }

  stillfirechoirBeamTicks = fmax(0, stillfirechoirBeamTicks - 1);
  stillfirechoirBeamAfterglowTicks = fmax(0, stillfirechoirBeamAfterglowTicks - 1);
  stillfirechoirLockPulseTicks = fmax(0, stillfirechoirLockPulseTicks - 1);
}

void addGameStillfirechoir() {
  addGame(stillfirechoirTitle, stillfirechoirDescription, stillfirechoirCharacters,
          stillfirechoirCharactersCount, &stillfirechoirOptions, false, &stillfirechoirUpdate);
}
