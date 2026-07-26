#include "../cglp.h"

int* blastbuoyTitle = "BLAST BUOY";
int* blastbuoyDescription = "[Hold] Sink\n[Release] Float";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] blastbuoyCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int blastbuoyCharactersCount = 0;

Options blastbuoyOptions = {100, 100, 12, false};

struct BlastbuoyPlayer {
  Vector pos;
  float vy;
  float vx;
};
BlastbuoyPlayer blastbuoyPlayer;

struct BlastbuoyEnemy {
  Vector pos;
  float dir;
  float spd;
  float fireT;
  bool isAlive;
};
// Concurrent count stays roughly constant across difficulty (spawn interval
// and enemy speed both scale with sqrt(difficulty)), estimated around 5
// typical / ~12 worst-case - sized generously above that.
#define BLASTBUOY_MAX_ENEMY_COUNT 64
BlastbuoyEnemy[BLASTBUOY_MAX_ENEMY_COUNT] blastbuoyEnemies;
int blastbuoyEnemyIndex;

struct BlastbuoyBullet {
  Vector pos;
  float vx;
  bool isAlive;
};
#define BLASTBUOY_MAX_BULLET_COUNT 48
BlastbuoyBullet[BLASTBUOY_MAX_BULLET_COUNT] blastbuoyBullets;
int blastbuoyBulletIndex;

int blastbuoyCombo;
int blastbuoyComboT;
int blastbuoyIframes;
float blastbuoyNextSubTicks;

struct BlastbuoyBlastFx {
  Vector pos;
  int t;
  int color;
};
BlastbuoyBlastFx blastbuoyBlastFx;

int blastbuoyHitstop;
float blastbuoyShakeMag;
int blastbuoyShakeT;

void blastbuoyUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&blastbuoyPlayer.pos, 30, 40);
    blastbuoyPlayer.vy = 0;
    blastbuoyPlayer.vx = 0;
    INIT_UNALIVED_ARRAY_FAST(blastbuoyEnemies);
    blastbuoyEnemyIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(blastbuoyBullets);
    blastbuoyBulletIndex = 0;
    blastbuoyCombo = 1;
    blastbuoyComboT = 0;
    blastbuoyIframes = 0;
    blastbuoyNextSubTicks = 0;
    vectorSet(&blastbuoyBlastFx.pos, 0, 0);
    blastbuoyBlastFx.t = 0;
    blastbuoyBlastFx.color = LIGHT_RED;
    blastbuoyHitstop = 0;
    blastbuoyShakeMag = 0;
    blastbuoyShakeT = 0;
  }

  bool frozen = blastbuoyHitstop > 0;
  if (frozen) {
    blastbuoyHitstop--;
  }

  if (!frozen) {
    if (input.isPressed) {
      blastbuoyPlayer.vy += 0.06;
    } else {
      blastbuoyPlayer.vy -= 0.05;
    }
    blastbuoyPlayer.vy = clamp(blastbuoyPlayer.vy, -1.1, 1.1);
    blastbuoyPlayer.vx *= 0.95;
    blastbuoyPlayer.pos.y += blastbuoyPlayer.vy;
    blastbuoyPlayer.pos.x += blastbuoyPlayer.vx;
    blastbuoyPlayer.pos.x = clamp(blastbuoyPlayer.pos.x, 5, 95);
    blastbuoyPlayer.pos.y = clamp(blastbuoyPlayer.pos.y, 13, 87);
    if (blastbuoyPlayer.pos.y <= 13 || blastbuoyPlayer.pos.y >= 87) {
      blastbuoyPlayer.vy = 0;
    }

    if (blastbuoyIframes > 0) {
      blastbuoyIframes--;
    }
    if (blastbuoyComboT > 0) {
      blastbuoyComboT--;
    } else {
      blastbuoyCombo = 1;
    }

    blastbuoyNextSubTicks--;
    if (blastbuoyNextSubTicks < 0) {
      float dir;
      if (rnd(0, 1) < 0.5) {
        dir = 1;
      } else {
        dir = -1;
      }
      float ex;
      if (dir > 0) {
        ex = -8;
      } else {
        ex = 108;
      }
      ASSIGN_ARRAY_ITEM(blastbuoyEnemies, blastbuoyEnemyIndex, BlastbuoyEnemy, ne);
      vectorSet(&ne->pos, ex, rnd(13, 87));
      ne->dir = dir;
      ne->spd = rnd(0.2, 0.45) * sqrt(difficulty);
      ne->fireT = rnd(60, 120);
      ne->isAlive = true;
      blastbuoyEnemyIndex = cgl_wrap(blastbuoyEnemyIndex + 1, 0, BLASTBUOY_MAX_ENEMY_COUNT);
      blastbuoyNextSubTicks = rnd(50, 90) / sqrt(difficulty);
    }
  }

  float shakeY = 0;
  if (blastbuoyShakeT > 0) {
    blastbuoyShakeT--;
    float m = blastbuoyShakeMag * (blastbuoyShakeT / 8.0);
    shakeY = rnd(-m, m);
  }

  color = LIGHT_BLUE;
  rect(0, 8 + shakeY, 100, 1, &scratch);
  color = LIGHT_BLACK;
  rect(0, 93 + shakeY, 100, 7, &scratch);

  if (blastbuoyBlastFx.t > 0) {
    blastbuoyBlastFx.t--;
    color = blastbuoyBlastFx.color;
    thickness = 2;
    arc(blastbuoyBlastFx.pos.x, blastbuoyBlastFx.pos.y + shakeY,
        25 * (1 - blastbuoyBlastFx.t / 14.0), 0, CGLP_PI * 2, &scratch);
  }

  if (blastbuoyIframes % 8 >= 4) {
    color = BLUE;
  } else {
    color = CYAN;
  }
  box(blastbuoyPlayer.pos.x, blastbuoyPlayer.pos.y, 6, 6, &scratch);

  bool blastAtSet = false;
  Vector blastAt;
  FOR_EACH(blastbuoyEnemies, ei) {
    ASSIGN_ARRAY_ITEM(blastbuoyEnemies, ei, BlastbuoyEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!frozen) {
      e->pos.x += e->dir * e->spd;
      e->fireT--;
      if (e->fireT < 0 && fabs(e->pos.y - blastbuoyPlayer.pos.y) < 10 &&
          e->pos.x > 0 && e->pos.x < 100) {
        if (distanceTo(&e->pos, blastbuoyPlayer.pos.x, blastbuoyPlayer.pos.y) > 36) {
          float d;
          if (blastbuoyPlayer.pos.x > e->pos.x) {
            d = 1;
          } else {
            d = -1;
          }
          ASSIGN_ARRAY_ITEM(blastbuoyBullets, blastbuoyBulletIndex, BlastbuoyBullet, nb);
          vectorSet(&nb->pos, e->pos.x + d * 6, e->pos.y);
          nb->vx = d * (0.8 + difficulty * 0.15);
          nb->isAlive = true;
          blastbuoyBulletIndex = cgl_wrap(blastbuoyBulletIndex + 1, 0, BLASTBUOY_MAX_BULLET_COUNT);
          play(LASER);
        }
        e->fireT = rnd(90, 150);
      }
    }
    color = GREEN;
    Collision ec;
    box(e->pos.x, e->pos.y, 9, 4, &ec);
    if (!blastAtSet && ec.isColliding.rect[CYAN]) {
      vectorSet(&blastAt, e->pos.x, e->pos.y);
      blastAtSet = true;
      e->isAlive = false;
      continue;
    }
    if (e->pos.x < -12 || e->pos.x > 112) {
      e->isAlive = false;
      continue;
    }
  }

  if (blastAtSet) {
    int kills = 1;
    FOR_EACH(blastbuoyEnemies, ki) {
      ASSIGN_ARRAY_ITEM(blastbuoyEnemies, ki, BlastbuoyEnemy, o);
      SKIP_IS_NOT_ALIVE(o);
      if (distanceTo(&o->pos, blastAt.x, blastAt.y) < 25) {
        kills++;
      }
    }
    int chainColor;
    if (kills >= 4) {
      chainColor = PURPLE;
    } else if (kills >= 2) {
      chainColor = RED;
    } else {
      chainColor = LIGHT_RED;
    }

    play(EXPLOSION);
    color = chainColor;
    particle(blastAt.x, blastAt.y, 30 + kills * 5, 4, 0, CGLP_PI * 2);
    vectorSet(&blastbuoyBlastFx.pos, blastAt.x, blastAt.y);
    blastbuoyBlastFx.t = 14;
    blastbuoyBlastFx.color = chainColor;

    int n = 0;
    FOR_EACH(blastbuoyEnemies, oi) {
      ASSIGN_ARRAY_ITEM(blastbuoyEnemies, oi, BlastbuoyEnemy, o2);
      SKIP_IS_NOT_ALIVE(o2);
      if (distanceTo(&o2->pos, blastAt.x, blastAt.y) < 25) {
        color = chainColor;
        particle(o2->pos.x, o2->pos.y, 12 + kills * 2, 3, 0, CGLP_PI * 2);
        n++;
        o2->isAlive = false;
        continue;
      }
    }
    FOR_EACH(blastbuoyBullets, bi) {
      ASSIGN_ARRAY_ITEM(blastbuoyBullets, bi, BlastbuoyBullet, b);
      SKIP_IS_NOT_ALIVE(b);
      if (distanceTo(&b->pos, blastAt.x, blastAt.y) < 25) {
        b->isAlive = false;
        continue;
      }
    }
    addScore((5 + n * 8) * blastbuoyCombo, blastAt.x, blastAt.y);
    blastbuoyCombo++;
    blastbuoyComboT = 120;
    float a = angleTo(&blastAt, blastbuoyPlayer.pos.x, blastbuoyPlayer.pos.y);
    blastbuoyPlayer.vx = cos(a) * 2.2;
    blastbuoyPlayer.vy = sin(a) * 2.2;
    blastbuoyIframes = 40;

    blastbuoyHitstop = (int)clamp(2 + kills, 2, 8);
    blastbuoyShakeMag = clamp(1.5 + kills, 1.5, 7);
    blastbuoyShakeT = 8;
  }

  FOR_EACH(blastbuoyBullets, bi2) {
    ASSIGN_ARRAY_ITEM(blastbuoyBullets, bi2, BlastbuoyBullet, b2);
    SKIP_IS_NOT_ALIVE(b2);
    if (!frozen) {
      b2->pos.x += b2->vx;
    }
    color = LIGHT_RED;
    box(b2->pos.x - b2->vx * 3, b2->pos.y, 3, 1.5, &scratch);
    box(b2->pos.x - b2->vx * 6, b2->pos.y, 2, 1, &scratch);
    color = RED;
    Collision bc;
    box(b2->pos.x, b2->pos.y, 5, 2, &bc);
    if (bc.isColliding.rect[CYAN]) {
      if (blastbuoyIframes > 0) {
        b2->isAlive = false;
        continue;
      }
      play(EXPLOSION);
      particle(blastbuoyPlayer.pos.x, blastbuoyPlayer.pos.y, 25, 3, 0, CGLP_PI * 2);
      gameOver();
    }
    if (b2->pos.x < -5 || b2->pos.x > 105) {
      b2->isAlive = false;
      continue;
    }
  }

  if (blastbuoyCombo > 1) {
    color = BLACK;
    int[16] comboText;
    strcpy(comboText, "x");
    strcat(comboText, intToChar(blastbuoyCombo));
    text(comboText, 3, 12, &scratch);
  }
}

void addGameBlastbuoy() {
  addGame(blastbuoyTitle, blastbuoyDescription, blastbuoyCharacters,
          blastbuoyCharactersCount, &blastbuoyOptions, false, &blastbuoyUpdate);
}
