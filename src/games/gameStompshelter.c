#include "../cglp.h"

int* stompshelterTitle = "STOMPSHELTER";
int* stompshelterDescription = "[Tap]\n Double jump\n[Hold]\n Slide";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] stompshelterCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int stompshelterCharactersCount = 1;

Options stompshelterOptions = {100, 100, 0, false};

struct StompshelterPlayer {
  float x;
  float y;
  float vx;
  float vy;
  bool grounded;
  float scaleX;
  float scaleY;
};
StompshelterPlayer stompshelterPlayer;

struct StompshelterPlatform {
  float x;
  float y;
  float w;
  float hp;
  bool isAlive;
};
#define STOMPSHELTER_MAX_PLATFORM_COUNT 40
StompshelterPlatform[STOMPSHELTER_MAX_PLATFORM_COUNT] stompshelterPlatforms;
int stompshelterPlatformIndex;

struct StompshelterEnemy {
  float x;
  float y;
  int dir;
  float py;
  bool isAlive;
};
#define STOMPSHELTER_MAX_ENEMY_COUNT 64
StompshelterEnemy[STOMPSHELTER_MAX_ENEMY_COUNT] stompshelterEnemies;
int stompshelterEnemyIndex;

struct StompshelterDebris {
  float x;
  float y;
  float speed;
  bool isAlive;
};
// Sized well past the ~110 estimated at difficulty 30; also guarded against a modulo-by-zero trap past difficulty 70.
#define STOMPSHELTER_MAX_DEBRIS_COUNT 256
StompshelterDebris[STOMPSHELTER_MAX_DEBRIS_COUNT] stompshelterDebris;
int stompshelterDebrisIndex;

struct StompshelterTrail {
  float x;
  float y;
  int life;
  bool isAlive;
};
#define STOMPSHELTER_MAX_TRAIL_COUNT 32
StompshelterTrail[STOMPSHELTER_MAX_TRAIL_COUNT] stompshelterTrails;
int stompshelterTrailIndex;

float stompshelterNextEnemyDist;
float stompshelterLavaY;
int stompshelterCombo;
int stompshelterJumpCount;
bool stompshelterWasGrounded;

void stompshelterUpdate() {
  Collision scratch;
  if (!ticks) {
    stompshelterPlayer.x = 50;
    stompshelterPlayer.y = 70;
    stompshelterPlayer.vx = 1;
    stompshelterPlayer.vy = 0;
    stompshelterPlayer.grounded = false;
    stompshelterPlayer.scaleX = 1;
    stompshelterPlayer.scaleY = 1;
    INIT_UNALIVED_ARRAY_FAST(stompshelterPlatforms);
    stompshelterPlatformIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(stompshelterEnemies);
    stompshelterEnemyIndex = 0;
    stompshelterNextEnemyDist = 100;
    INIT_UNALIVED_ARRAY_FAST(stompshelterDebris);
    stompshelterDebrisIndex = 0;
    stompshelterLavaY = 102;
    stompshelterCombo = 0;
    stompshelterJumpCount = 0;
    INIT_UNALIVED_ARRAY_FAST(stompshelterTrails);
    stompshelterTrailIndex = 0;
    stompshelterWasGrounded = false;
    TIMES(6, initPlI) {
      ASSIGN_ARRAY_ITEM(stompshelterPlatforms, stompshelterPlatformIndex, StompshelterPlatform, np);
      np->x = rnd(20, 60);
      np->y = 15 + initPlI * 15;
      np->w = rnd(25, 38);
      np->hp = 120;
      np->isAlive = true;
      stompshelterPlatformIndex = cgl_wrap(stompshelterPlatformIndex + 1, 0, STOMPSHELTER_MAX_PLATFORM_COUNT);
    }
  }

  stompshelterLavaY -= 0.025;

  float topY = 999;
  FOR_EACH(stompshelterPlatforms, tpI) {
    ASSIGN_ARRAY_ITEM(stompshelterPlatforms, tpI, StompshelterPlatform, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->y < topY) {
      topY = p->y;
    }
  }
  if (topY > 5) {
    ASSIGN_ARRAY_ITEM(stompshelterPlatforms, stompshelterPlatformIndex, StompshelterPlatform, np2);
    np2->x = rnd(10, 55);
    np2->y = topY - rnd(13, 17);
    np2->w = rnd(25, 38);
    np2->hp = 120;
    np2->isAlive = true;
    stompshelterPlatformIndex = cgl_wrap(stompshelterPlatformIndex + 1, 0, STOMPSHELTER_MAX_PLATFORM_COUNT);
  }

  if (stompshelterNextEnemyDist < 0) {
    int vpCount = 0;
    FOR_EACH(stompshelterPlatforms, vpI) {
      ASSIGN_ARRAY_ITEM(stompshelterPlatforms, vpI, StompshelterPlatform, p);
      SKIP_IS_NOT_ALIVE(p);
      if (p->y < 20 && p->hp > 60) {
        vpCount++;
      }
    }
    if (vpCount > 0) {
      int pick = (int)floor(rnd(0, vpCount));
      int seen = 0;
      StompshelterPlatform* chosen = NULL;
      FOR_EACH(stompshelterPlatforms, vpI2) {
        ASSIGN_ARRAY_ITEM(stompshelterPlatforms, vpI2, StompshelterPlatform, p);
        SKIP_IS_NOT_ALIVE(p);
        if (p->y < 20 && p->hp > 60) {
          if (seen == pick) {
            chosen = p;
          }
          seen++;
        }
      }
      if (chosen != NULL) {
        ASSIGN_ARRAY_ITEM(stompshelterEnemies, stompshelterEnemyIndex, StompshelterEnemy, ne);
        ne->x = chosen->x + chosen->w / 2;
        ne->y = chosen->y - 5;
        ne->dir = rndi(0, 2) * 2 - 1;
        ne->py = chosen->y;
        ne->isAlive = true;
        stompshelterEnemyIndex = cgl_wrap(stompshelterEnemyIndex + 1, 0, STOMPSHELTER_MAX_ENEMY_COUNT);
        stompshelterNextEnemyDist = rnd(100, 200) / sqrt(difficulty);
      }
    }
  }

  int debrisInterval = (int)floor(70.0 / difficulty);
  if (debrisInterval < 1) {
    debrisInterval = 1;
  }
  if (ticks % debrisInterval == 0) {
    ASSIGN_ARRAY_ITEM(stompshelterDebris, stompshelterDebrisIndex, StompshelterDebris, nd);
    nd->x = rnd(8, 92);
    nd->y = -4;
    nd->speed = 0.5;
    nd->isAlive = true;
    stompshelterDebrisIndex = cgl_wrap(stompshelterDebrisIndex + 1, 0, STOMPSHELTER_MAX_DEBRIS_COUNT);
  }

  if (input.isJustPressed && (stompshelterPlayer.grounded || stompshelterJumpCount < 2)) {
    stompshelterPlayer.vy = -2.6;
    stompshelterPlayer.grounded = false;
    stompshelterJumpCount++;
    play(JUMP);
    stompshelterPlayer.scaleX = 0.7;
    stompshelterPlayer.scaleY = 1.4;
    color = CYAN;
    particle(stompshelterPlayer.x, stompshelterPlayer.y + 3, 5, 1, -CGLP_PI_2, CGLP_PI_4);
  }

  stompshelterPlayer.vy += 0.1;
  stompshelterPlayer.y += stompshelterPlayer.vy;

  float driftFactor;
  if (input.isPressed) {
    driftFactor = 1;
  } else {
    driftFactor = 0.1;
  }
  stompshelterPlayer.x += stompshelterPlayer.vx * driftFactor;
  if ((stompshelterPlayer.x > 90 && stompshelterPlayer.vx > 0) ||
      (stompshelterPlayer.x < 10 && stompshelterPlayer.vx < 0)) {
    stompshelterPlayer.vx *= -1;
  }

  stompshelterPlayer.grounded = false;
  FOR_EACH(stompshelterPlatforms, pcI) {
    ASSIGN_ARRAY_ITEM(stompshelterPlatforms, pcI, StompshelterPlatform, p);
    SKIP_IS_NOT_ALIVE(p);
    if (stompshelterPlayer.vy > 0 && p->hp > 0 &&
        stompshelterPlayer.x > p->x - 3 && stompshelterPlayer.x < p->x + p->w + 3 &&
        stompshelterPlayer.y > p->y - 8 && stompshelterPlayer.y < p->y + 2) {
      stompshelterPlayer.y = p->y - 4;
      stompshelterPlayer.vy = 0;
      stompshelterPlayer.grounded = true;
      stompshelterJumpCount = 0;
      stompshelterCombo = 0;
      p->hp -= 1;
      if (!stompshelterWasGrounded) {
        stompshelterPlayer.scaleX = 1.4;
        stompshelterPlayer.scaleY = 0.6;
        color = CYAN;
        particle(stompshelterPlayer.x, stompshelterPlayer.y + 3, 4, 0.8, CGLP_PI_2, CGLP_PI / 3);
      }
    }
  }
  stompshelterWasGrounded = stompshelterPlayer.grounded;

  if (stompshelterPlayer.y < 60) {
    float scroll = 60 - stompshelterPlayer.y;
    stompshelterPlayer.y = 60;
    stompshelterLavaY += scroll;
    stompshelterNextEnemyDist -= scroll;
    FOR_EACH(stompshelterPlatforms, spI) {
      ASSIGN_ARRAY_ITEM(stompshelterPlatforms, spI, StompshelterPlatform, p);
      SKIP_IS_NOT_ALIVE(p);
      p->y += scroll;
    }
    FOR_EACH(stompshelterEnemies, seI) {
      ASSIGN_ARRAY_ITEM(stompshelterEnemies, seI, StompshelterEnemy, e);
      SKIP_IS_NOT_ALIVE(e);
      e->y += scroll;
      e->py += scroll;
    }
    FOR_EACH(stompshelterDebris, sdI) {
      ASSIGN_ARRAY_ITEM(stompshelterDebris, sdI, StompshelterDebris, d);
      SKIP_IS_NOT_ALIVE(d);
      d->y += scroll;
    }
    FOR_EACH(stompshelterTrails, stI) {
      ASSIGN_ARRAY_ITEM(stompshelterTrails, stI, StompshelterTrail, t);
      SKIP_IS_NOT_ALIVE(t);
      t->y += scroll;
    }
    addScore(floor(scroll), SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  }

  FOR_EACH(stompshelterPlatforms, rpI) {
    ASSIGN_ARRAY_ITEM(stompshelterPlatforms, rpI, StompshelterPlatform, p);
    SKIP_IS_NOT_ALIVE(p);
    if (!(p->y < 110 && p->hp > 0)) {
      p->isAlive = false;
      continue;
    }
  }
  FOR_EACH(stompshelterEnemies, reI) {
    ASSIGN_ARRAY_ITEM(stompshelterEnemies, reI, StompshelterEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!(e->y < 105)) {
      e->isAlive = false;
      continue;
    }
  }
  FOR_EACH(stompshelterDebris, rdI) {
    ASSIGN_ARRAY_ITEM(stompshelterDebris, rdI, StompshelterDebris, d);
    SKIP_IS_NOT_ALIVE(d);
    if (!(d->y < 105)) {
      d->isAlive = false;
      continue;
    }
  }

  FOR_EACH(stompshelterEnemies, emI) {
    ASSIGN_ARRAY_ITEM(stompshelterEnemies, emI, StompshelterEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x += e->dir * 0.35;
    FOR_EACH(stompshelterPlatforms, epI) {
      ASSIGN_ARRAY_ITEM(stompshelterPlatforms, epI, StompshelterPlatform, p);
      SKIP_IS_NOT_ALIVE(p);
      if (p->hp > 0 && fabs(e->py - p->y) < 2) {
        if (e->x <= p->x + 2) {
          e->dir = 1;
        }
        if (e->x >= p->x + p->w - 2) {
          e->dir = -1;
        }
      }
    }
  }

  FOR_EACH(stompshelterDebris, dfI) {
    ASSIGN_ARRAY_ITEM(stompshelterDebris, dfI, StompshelterDebris, d);
    SKIP_IS_NOT_ALIVE(d);
    d->y += d->speed;
  }

  stompshelterPlayer.scaleX += (1 - stompshelterPlayer.scaleX) * 0.15;
  stompshelterPlayer.scaleY += (1 - stompshelterPlayer.scaleY) * 0.15;

  if (stompshelterPlayer.grounded && fabs(stompshelterPlayer.scaleX - 1) < 0.05) {
    float breath = sin(ticks * 0.1) * 0.05;
    stompshelterPlayer.scaleX = 1 + breath;
    stompshelterPlayer.scaleY = 1 - breath;
  }

  if (fabs(stompshelterPlayer.vy) > 1 || fabs(stompshelterPlayer.vx) > 0.5) {
    ASSIGN_ARRAY_ITEM(stompshelterTrails, stompshelterTrailIndex, StompshelterTrail, nt);
    nt->x = stompshelterPlayer.x;
    nt->y = stompshelterPlayer.y;
    nt->life = 8;
    nt->isAlive = true;
    stompshelterTrailIndex = cgl_wrap(stompshelterTrailIndex + 1, 0, STOMPSHELTER_MAX_TRAIL_COUNT);
  }
  FOR_EACH(stompshelterTrails, tlI) {
    ASSIGN_ARRAY_ITEM(stompshelterTrails, tlI, StompshelterTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life--;
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
  }

  FOR_EACH(stompshelterTrails, dtI) {
    ASSIGN_ARRAY_ITEM(stompshelterTrails, dtI, StompshelterTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    color = LIGHT_CYAN;
    float alpha = (float)t->life / 8;
    box(t->x, t->y, 4 * alpha, 4 * alpha, &scratch);
  }

  color = CYAN;
  float tilt = stompshelterPlayer.vx * 0.15;
  float pw = 6 * stompshelterPlayer.scaleX;
  float ph = 6 * stompshelterPlayer.scaleY;
  thickness = pw;
  bar(stompshelterPlayer.x, stompshelterPlayer.y, ph / 2, -CGLP_PI_2 + tilt, &scratch);

  color = WHITE;
  float eyeOffX;
  if (stompshelterPlayer.vx > 0) {
    eyeOffX = 1;
  } else {
    eyeOffX = -1;
  }
  float eyeOffY;
  if (stompshelterPlayer.vy > 0) {
    eyeOffY = 1;
  } else if (stompshelterPlayer.vy < -1) {
    eyeOffY = -1;
  } else {
    eyeOffY = 0;
  }
  box(stompshelterPlayer.x - 1.2, stompshelterPlayer.y - 1, 2, 2, &scratch);
  box(stompshelterPlayer.x + 1.2, stompshelterPlayer.y - 1, 2, 2, &scratch);
  color = BLACK;
  box(stompshelterPlayer.x - 1.2 + eyeOffX * 0.4, stompshelterPlayer.y - 1 + eyeOffY * 0.3, 1, 1, &scratch);
  box(stompshelterPlayer.x + 1.2 + eyeOffX * 0.4, stompshelterPlayer.y - 1 + eyeOffY * 0.3, 1, 1, &scratch);

  if (stompshelterLavaY > 100) {
    stompshelterLavaY += (100 - stompshelterLavaY) * 0.5;
  }
  color = YELLOW;
  rect(0, stompshelterLavaY, 100, 120 - stompshelterLavaY, &scratch);
  if (stompshelterPlayer.y > stompshelterLavaY - 3) {
    play(EXPLOSION);
    gameOver();
  }

  FOR_EACH(stompshelterPlatforms, dpI) {
    ASSIGN_ARRAY_ITEM(stompshelterPlatforms, dpI, StompshelterPlatform, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->hp > 80) {
      color = BLACK;
    } else if (p->hp > 40) {
      color = BLUE;
    } else {
      color = LIGHT_RED;
    }
    rect(p->x, p->y, p->w, 4, &scratch);
  }

  FOR_EACH(stompshelterEnemies, deI) {
    ASSIGN_ARRAY_ITEM(stompshelterEnemies, deI, StompshelterEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    color = RED;
    Collision c;
    box(e->x, e->y, 7, 7, &c);

    float eTilt = e->dir * 0.2;
    thickness = 7;
    bar(e->x, e->y, 3.5, -CGLP_PI_2 + eTilt, &scratch);

    color = WHITE;
    float lookX;
    if (stompshelterPlayer.x > e->x) {
      lookX = 1;
    } else {
      lookX = -1;
    }
    float lookY;
    if (stompshelterPlayer.y > e->y) {
      lookY = 1;
    } else if (stompshelterPlayer.y < e->y - 5) {
      lookY = -1;
    } else {
      lookY = 0;
    }
    box(e->x - 1.5, e->y - 1, 2.5, 2.5, &scratch);
    box(e->x + 1.5, e->y - 1, 2.5, 2.5, &scratch);
    color = BLACK;
    box(e->x - 1.5 + lookX * 0.5, e->y - 1 + lookY * 0.4, 1.2, 1.2, &scratch);
    box(e->x + 1.5 + lookX * 0.5, e->y - 1 + lookY * 0.4, 1.2, 1.2, &scratch);

    if (c.isColliding.rect[CYAN]) {
      if (stompshelterPlayer.vy > 0.5 && stompshelterPlayer.y < e->y - 2) {
        stompshelterPlayer.vy = -2.2;
        stompshelterCombo++;
        addScore(8 + stompshelterCombo * 4, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        play(POWER_UP);
        color = RED;
        particle(e->x, e->y, 10, 2, 0, CGLP_PI * 2);
        e->isAlive = false;
        continue;
      } else {
        play(HIT);
        gameOver();
      }
    }
  }

  FOR_EACH(stompshelterDebris, ddI) {
    ASSIGN_ARRAY_ITEM(stompshelterDebris, ddI, StompshelterDebris, d);
    SKIP_IS_NOT_ALIVE(d);
    color = LIGHT_PURPLE;
    box(d->x, d->y - 3, 2, 2, &scratch);
    box(d->x, d->y - 6, 1, 1, &scratch);

    bool blocked = false;
    FOR_EACH(stompshelterPlatforms, bpI) {
      ASSIGN_ARRAY_ITEM(stompshelterPlatforms, bpI, StompshelterPlatform, p);
      SKIP_IS_NOT_ALIVE(p);
      if (p->hp > 0 && d->x > p->x && d->x < p->x + p->w && d->y > p->y - 2 && d->y < p->y + 5) {
        blocked = true;
        p->hp -= 15;
      }
    }
    if (blocked) {
      color = PURPLE;
      particle(d->x, d->y, 6, 1, 0, CGLP_PI * 2);
      d->isAlive = false;
      continue;
    }
    color = PURPLE;
    Collision dc;
    box(d->x, d->y, 4, 4, &dc);
    if (dc.isColliding.rect[CYAN]) {
      play(HIT);
      gameOver();
    }
  }
}

void addGameStompshelter() {
  addGame(stompshelterTitle, stompshelterDescription, stompshelterCharacters,
          stompshelterCharactersCount, &stompshelterOptions, false, &stompshelterUpdate);
}
