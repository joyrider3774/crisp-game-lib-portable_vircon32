#include "../cglp.h"

int* geoeraseTitle = "GEOERASE";
int* geoeraseDescription = "[Hold]\n Mark\n[Release]\n Erase";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] geoeraseCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int geoeraseCharactersCount = 0;

Options geoeraseOptions = {100, 100, 7, false};

struct GeoerasePlayer {
  float x;
  float y;
};
GeoerasePlayer geoerasePlayer;

struct GeoeraseEnemy {
  float x;
  float y;
  float vx;
  float vy;
  bool isSquare;
  bool marked;
  float scaleY;
  float rotation;
  bool isAlive;
};
// Sized well above the ~15-17 concurrent enemies estimated at peak (difficulty 3-4).
#define GEOERASE_MAX_ENEMY_COUNT 64
GeoeraseEnemy[GEOERASE_MAX_ENEMY_COUNT] geoeraseEnemies;
int geoeraseEnemyIndex;

struct GeoeraseTrail {
  float x;
  float y;
  int life;
  int maxLife;
  float size;
  bool isSquare;
  bool isAlive;
};
// Sized generously above ~3-4 trails per enemy times up to ~17 concurrent enemies.
#define GEOERASE_MAX_TRAIL_COUNT 128
GeoeraseTrail[GEOERASE_MAX_TRAIL_COUNT] geoeraseTrails;
int geoeraseTrailIndex;

float geoeraseLaserAngle;
float geoeraseLaserSpeed;
float geoeraseSpawnTimer;
int geoeraseCombo;

void geoeraseUpdate() {
  Collision scratch;
  if (!ticks) {
    geoerasePlayer.x = 50;
    geoerasePlayer.y = 50;
    INIT_UNALIVED_ARRAY_FAST(geoeraseEnemies);
    geoeraseEnemyIndex = 0;
    geoeraseLaserAngle = 0;
    geoeraseLaserSpeed = 0.04;
    geoeraseSpawnTimer = 0;
    geoeraseCombo = 1;
    INIT_UNALIVED_ARRAY_FAST(geoeraseTrails);
    geoeraseTrailIndex = 0;
  }

  float spawnInterval = fmax(20, 60 / difficulty);
  geoeraseSpawnTimer -= 1;
  if (geoeraseSpawnTimer <= 0) {
    geoeraseSpawnTimer = spawnInterval;
    int side = (int)floor(rnd(0, 4));
    float ex;
    float ey;
    if (side == 0) {
      ex = rnd(0, 100);
      ey = -5;
    } else if (side == 1) {
      ex = rnd(0, 100);
      ey = 105;
    } else if (side == 2) {
      ex = -5;
      ey = rnd(0, 100);
    } else {
      ex = 105;
      ey = rnd(0, 100);
    }
    bool isSquare = rnd(0, 1) < 0.3;
    float spawnSpeed;
    if (isSquare) {
      spawnSpeed = 0.4 * sqrt(difficulty);
    } else {
      spawnSpeed = 0.25 * sqrt(difficulty);
    }
    float angle = atan2(geoerasePlayer.y - ey, geoerasePlayer.x - ex);
    ASSIGN_ARRAY_ITEM(geoeraseEnemies, geoeraseEnemyIndex, GeoeraseEnemy, ne);
    ne->x = ex;
    ne->y = ey;
    ne->vx = cos(angle) * spawnSpeed;
    ne->vy = sin(angle) * spawnSpeed;
    ne->isSquare = isSquare;
    ne->marked = false;
    ne->scaleY = 1.3;
    ne->rotation = rnd(0, CGLP_PI * 2);
    ne->isAlive = true;
    geoeraseEnemyIndex = cgl_wrap(geoeraseEnemyIndex + 1, 0, GEOERASE_MAX_ENEMY_COUNT);
    color = LIGHT_CYAN;
    particle(ex, ey, 3, 1, angle + CGLP_PI, CGLP_PI / 2);
  }

  if (input.isPressed) {
    geoeraseLaserAngle += geoeraseLaserSpeed * sqrt(difficulty);
  } else {
    geoeraseLaserAngle += geoeraseLaserSpeed * 2 * sqrt(difficulty);
  }

  if (input.isJustReleased) {
    int eraseCount = 0;
    FOR_EACH(geoeraseEnemies, ei) {
      ASSIGN_ARRAY_ITEM(geoeraseEnemies, ei, GeoeraseEnemy, e);
      SKIP_IS_NOT_ALIVE(e);
      if (e->marked) {
        int pts;
        if (e->isSquare) {
          pts = 2;
        } else {
          pts = 1;
        }
        addScore(pts * geoeraseCombo, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        color = YELLOW;
        particle(e->x, e->y, 15, 2.5, 0, CGLP_PI);
        play(LASER);
        eraseCount++;
        e->isAlive = false;
        continue;
      }
    }
    if (eraseCount > 1) {
      geoeraseCombo = min(geoeraseCombo + eraseCount - 1, 16);
    } else {
      geoeraseCombo = max(1, geoeraseCombo - 1);
    }
    FOR_EACH(geoeraseEnemies, uei) {
      ASSIGN_ARRAY_ITEM(geoeraseEnemies, uei, GeoeraseEnemy, e2);
      SKIP_IS_NOT_ALIVE(e2);
      e2->marked = false;
    }
  }

  float breathScale = 1 + sin(ticks * 0.1) * 0.05;
  color = CYAN;
  box(geoerasePlayer.x, geoerasePlayer.y, 5 * breathScale, 5 * breathScale, &scratch);
  color = BLACK;
  float eyeOffsetX = cos(geoeraseLaserAngle) * 1.2;
  float eyeOffsetY = sin(geoeraseLaserAngle) * 1.2;
  box(geoerasePlayer.x - 1 + eyeOffsetX * 0.5, geoerasePlayer.y + eyeOffsetY * 0.5, 1, 1, &scratch);
  box(geoerasePlayer.x + 1 + eyeOffsetX * 0.5, geoerasePlayer.y + eyeOffsetY * 0.5, 1, 1, &scratch);

  color = LIGHT_YELLOW;
  float laserLen = 60;
  float lx = geoerasePlayer.x + cos(geoeraseLaserAngle) * laserLen;
  float ly = geoerasePlayer.y + sin(geoeraseLaserAngle) * laserLen;
  if (input.isPressed) {
    float prevAngle = geoeraseLaserAngle - geoeraseLaserSpeed * sqrt(difficulty) * 0.5;
    float plx = geoerasePlayer.x + cos(prevAngle) * laserLen * 0.9;
    float ply = geoerasePlayer.y + sin(prevAngle) * laserLen * 0.9;
    thickness = 1;
    line(geoerasePlayer.x, geoerasePlayer.y, plx, ply, &scratch);
  }
  color = YELLOW;
  if (input.isPressed) {
    thickness = 2;
  } else {
    thickness = 1;
  }
  line(geoerasePlayer.x, geoerasePlayer.y, lx, ly, &scratch);

  if (input.isPressed) {
    FOR_EACH(geoeraseEnemies, mi) {
      ASSIGN_ARRAY_ITEM(geoeraseEnemies, mi, GeoeraseEnemy, e);
      SKIP_IS_NOT_ALIVE(e);
      float dx = e->x - geoerasePlayer.x;
      float dy = e->y - geoerasePlayer.y;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist < laserLen && dist > 3) {
        // e is guaranteed away from the player here (dist > 3), so the
        // (0,0)-both-args atan2 hard-trap can't happen - cgl_atan2 used
        // anyway for consistency/safety.
        float enemyAngle = cgl_atan2(dy, dx);
        float angleDiff = fabs(enemyAngle - geoeraseLaserAngle);
        while (angleDiff > CGLP_PI) {
          angleDiff -= CGLP_PI * 2;
        }
        angleDiff = fabs(angleDiff);
        if (angleDiff < 0.2) {
          if (!e->marked) {
            play(CLICK);
            color = YELLOW;
            particle(e->x, e->y, 3, 0.5, 0, CGLP_PI);
          }
          e->marked = true;
          e->scaleY = 0.7;
        }
      }
    }
  }

  FOR_EACH(geoeraseTrails, ti) {
    ASSIGN_ARRAY_ITEM(geoeraseTrails, ti, GeoeraseTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life -= 1;
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
    float alpha = (float)t->life / t->maxLife;
    float size = t->size * alpha;
    if (t->isSquare) {
      color = LIGHT_PURPLE;
      box(t->x, t->y, size, size, &scratch);
    } else {
      color = LIGHT_RED;
      float s = size * 0.5;
      thickness = 1;
      line(t->x, t->y - s, t->x - s, t->y + s, &scratch);
      line(t->x - s, t->y + s, t->x + s, t->y + s, &scratch);
      line(t->x + s, t->y + s, t->x, t->y - s, &scratch);
    }
  }

  FOR_EACH(geoeraseEnemies, upi) {
    ASSIGN_ARRAY_ITEM(geoeraseEnemies, upi, GeoeraseEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float espeed = sqrt(e->vx * e->vx + e->vy * e->vy);
    if (espeed > 0.3 && ticks % 3 == 0) {
      ASSIGN_ARRAY_ITEM(geoeraseTrails, geoeraseTrailIndex, GeoeraseTrail, nt);
      nt->x = e->x;
      nt->y = e->y;
      nt->life = 10;
      nt->maxLife = 10;
      nt->size = 6;
      nt->isSquare = e->isSquare;
      nt->isAlive = true;
      geoeraseTrailIndex = cgl_wrap(geoeraseTrailIndex + 1, 0, GEOERASE_MAX_TRAIL_COUNT);
    }
    e->x += e->vx;
    e->y += e->vy;
    e->rotation += espeed * 0.15;
    e->scaleY += (1 - e->scaleY) * 0.1;
  }

  FOR_EACH(geoeraseEnemies, edi) {
    ASSIGN_ARRAY_ITEM(geoeraseEnemies, edi, GeoeraseEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (e->marked) {
      color = YELLOW;
    } else if (e->isSquare) {
      color = PURPLE;
    } else {
      color = RED;
    }
    bool hitPlayer = false;
    float scaleX = 2 - e->scaleY;
    if (e->isSquare) {
      float halfW = 3 * scaleX;
      float halfH = 3 * e->scaleY;
      float cosR = cos(e->rotation);
      float sinR = sin(e->rotation);
      float c0x = e->x + (-halfW) * cosR - (-halfH) * sinR;
      float c0y = e->y + (-halfW) * sinR + (-halfH) * cosR;
      float c1x = e->x + (halfW)*cosR - (-halfH) * sinR;
      float c1y = e->y + (halfW)*sinR + (-halfH) * cosR;
      float c2x = e->x + (halfW)*cosR - (halfH)*sinR;
      float c2y = e->y + (halfW)*sinR + (halfH)*cosR;
      float c3x = e->x + (-halfW) * cosR - (halfH)*sinR;
      float c3y = e->y + (-halfW) * sinR + (halfH)*cosR;
      thickness = 2;
      line(c0x, c0y, c1x, c1y, &scratch);
      line(c1x, c1y, c2x, c2y, &scratch);
      line(c2x, c2y, c3x, c3y, &scratch);
      line(c3x, c3y, c0x, c0y, &scratch);
      Collision hitCol;
      box(e->x, e->y, 6 * scaleX, 6 * e->scaleY, &hitCol);
      hitPlayer = hitCol.isColliding.rect[CYAN];

      if (e->marked) {
        color = RED;
      } else {
        color = BLACK;
      }
      float eyeAngle = cgl_atan2(geoerasePlayer.y - e->y, geoerasePlayer.x - e->x);
      float pupilX = cos(eyeAngle);
      float pupilY = sin(eyeAngle);
      box(e->x - 1.5 + pupilX * 0.5, e->y + pupilY * 0.5, 1, 1, &scratch);
      box(e->x + 1.5 + pupilX * 0.5, e->y + pupilY * 0.5, 1, 1, &scratch);
    } else {
      float hgt = 3 * e->scaleY;
      float wid = 3 * scaleX;
      thickness = 2;
      line(e->x, e->y - hgt, e->x - wid, e->y + hgt, &scratch);
      line(e->x - wid, e->y + hgt, e->x + wid, e->y + hgt, &scratch);
      line(e->x + wid, e->y + hgt, e->x, e->y - hgt, &scratch);
      Collision hitCol;
      box(e->x, e->y, 6 * scaleX, 6 * e->scaleY, &hitCol);
      hitPlayer = hitCol.isColliding.rect[CYAN];

      if (e->marked) {
        color = RED;
      } else {
        color = BLACK;
      }
      float eyeAngle = cgl_atan2(geoerasePlayer.y - e->y, geoerasePlayer.x - e->x);
      float pupilX = cos(eyeAngle) * 0.8;
      float pupilY = sin(eyeAngle) * 0.8;
      box(e->x + pupilX, e->y + pupilY, 1.5, 1.5, &scratch);
    }
    if (hitPlayer) {
      play(EXPLOSION);
      color = RED;
      particle(geoerasePlayer.x, geoerasePlayer.y, 20, 3, 0, CGLP_PI);
      gameOver();
    }
  }

  FOR_EACH(geoeraseEnemies, ofi) {
    ASSIGN_ARRAY_ITEM(geoeraseEnemies, ofi, GeoeraseEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!(e->x > -10 && e->x < 110 && e->y > -10 && e->y < 110)) {
      e->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  int[16] geoeraseComboText;
  strcpy(geoeraseComboText, "x");
  strcat(geoeraseComboText, intToChar(geoeraseCombo));
  text(geoeraseComboText, 3, 9, &scratch);
}

void addGameGeoerase() {
  addGame(geoeraseTitle, geoeraseDescription, geoeraseCharacters,
          geoeraseCharactersCount, &geoeraseOptions, false, &geoeraseUpdate);
}
