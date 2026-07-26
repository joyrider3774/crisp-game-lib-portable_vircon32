#include "../cglp.h"

int* pressboundTitle = "PRESSBOUND";
int* pressboundDescription = "[Hold]\n Narrow\n[Release]\n Release";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pressboundCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int pressboundCharactersCount = 0;

Options pressboundOptions = {100, 100, 0, false};

struct PressboundBall {
  Vector pos;
  Vector vel;
  float scaleX;
  float scaleY;
  float rot;
};
#define PRESSBOUND_MAX_BALL_COUNT 5
PressboundBall[PRESSBOUND_MAX_BALL_COUNT] pressboundBalls;
int pressboundBallCount;

struct PressboundTrail {
  float x;
  float y;
  int life;
  float size;
  bool isAlive;
};
#define PRESSBOUND_MAX_TRAIL_COUNT 64
PressboundTrail[PRESSBOUND_MAX_TRAIL_COUNT] pressboundTrails;
int pressboundTrailIndex;

float pressboundWallPress;
float pressboundCharge;
bool pressboundFrozen;

void pressboundUpdate() {
  Collision scratch;
  if (!ticks) {
    pressboundBallCount = 1;
    vectorSet(&pressboundBalls[0].pos, rnd(10, 90), rnd(10, 90));
    vectorSet(&pressboundBalls[0].vel, rnd(0.5, 1) * RNDPM(), rnd(0.5, 1) * RNDPM());
    pressboundBalls[0].scaleX = 1;
    pressboundBalls[0].scaleY = 1;
    pressboundBalls[0].rot = 0;
    INIT_UNALIVED_ARRAY_FAST(pressboundTrails);
    pressboundTrailIndex = 0;
    pressboundWallPress = 5;
    pressboundCharge = 0;
    pressboundFrozen = false;
  }

  float pressRate;
  if (input.isPressed) {
    pressRate = 0.2;
  } else {
    pressRate = 0.05;
  }
  pressboundWallPress += pressRate * sqrt(difficulty);

  if (input.isJustPressed) {
    play(SELECT);
    pressboundFrozen = true;
    pressboundCharge = 0;
    pressboundWallPress += 2;
  }

  float pb = pressboundCharge * 0.1;
  int sc = (int)floor(pb * pb * pressboundBallCount * pressboundBallCount);
  if (input.isPressed && pressboundFrozen) {
    pressboundCharge += 1;
    int[16] scoreText;
    strcpy(scoreText, intToChar(sc));
    color = BLACK;
    text(scoreText, 50 - strlen(scoreText) * 6 + 6, 30, &scratch);
  }
  if (input.isJustReleased && pressboundFrozen) {
    if (sc > 0) {
      addScore(sc, 50, 50);
      play(JUMP);
    }
    TIMES(12, pi2) {
      float angle = ((float)pi2 / 12) * CGLP_PI * 2;
      float px = 50 + cos(angle) * 30;
      float py = 50 + sin(angle) * 30;
      color = YELLOW;
      particle(px, py, 8, 2, angle, 0.3);
    }
    pressboundWallPress = 5;
    pressboundCharge = 0;
    pressboundFrozen = false;
    INIT_UNALIVED_ARRAY_FAST(pressboundTrails);
    pressboundTrailIndex = 0;
    float speed = sqrt(difficulty);
    pressboundBallCount = rndi(2, 5);
    TIMES(pressboundBallCount, bi) {
      vectorSet(&pressboundBalls[bi].pos, rnd(5, 95), rnd(5, 95));
      vectorSet(&pressboundBalls[bi].vel, rnd(0.5, 1) * RNDPM() * speed, rnd(0.5, 1) * RNDPM() * speed);
      pressboundBalls[bi].scaleX = 1;
      pressboundBalls[bi].scaleY = 1;
      pressboundBalls[bi].rot = 0;
    }
  }

  float maxWall = 43;
  if (pressboundWallPress >= maxWall && !pressboundFrozen) {
    play(EXPLOSION);
    gameOver();
  }

  if (!pressboundFrozen) {
    TIMES(pressboundBallCount, bi2) {
      PressboundBall* b = &pressboundBalls[bi2];
      float sspd = sqrt(b->vel.x * b->vel.x + b->vel.y * b->vel.y);
      if (sspd > 0.8) {
        ASSIGN_ARRAY_ITEM(pressboundTrails, pressboundTrailIndex, PressboundTrail, nt);
        nt->x = b->pos.x;
        nt->y = b->pos.y;
        nt->life = 8;
        nt->size = 4;
        nt->isAlive = true;
        pressboundTrailIndex = cgl_wrap(pressboundTrailIndex + 1, 0, PRESSBOUND_MAX_TRAIL_COUNT);
      }

      b->pos.x += b->vel.x;
      b->pos.y += b->vel.y;

      float rotDir;
      if (b->vel.x > 0) {
        rotDir = 1;
      } else {
        rotDir = -1;
      }
      b->rot += sspd * 0.1 * rotDir;

      b->scaleX += (1 - b->scaleX) * 0.15;
      b->scaleY += (1 - b->scaleY) * 0.15;

      float margin = 1;
      float minB = pressboundWallPress + margin;
      float maxB = 100 - pressboundWallPress - margin;

      if (b->pos.x < minB) {
        b->pos.x = minB;
        b->vel.x = fabs(b->vel.x);
        b->scaleX = 0.6;
        b->scaleY = 1.4;
        color = WHITE;
        particle(b->pos.x, b->pos.y, 3, 1, 0, 0.5);
        play(HIT);
      }
      if (b->pos.x > maxB) {
        b->pos.x = maxB;
        b->vel.x = -fabs(b->vel.x);
        b->scaleX = 0.6;
        b->scaleY = 1.4;
        color = WHITE;
        particle(b->pos.x, b->pos.y, 3, 1, CGLP_PI, 0.5);
        play(HIT);
      }
      if (b->pos.y < minB) {
        b->pos.y = minB;
        b->vel.y = fabs(b->vel.y);
        b->scaleX = 1.4;
        b->scaleY = 0.6;
        color = WHITE;
        particle(b->pos.x, b->pos.y, 3, 1, -CGLP_PI / 2, 0.5);
        play(HIT);
      }
      if (b->pos.y > maxB) {
        b->pos.y = maxB;
        b->vel.y = -fabs(b->vel.y);
        b->scaleX = 1.4;
        b->scaleY = 0.6;
        color = WHITE;
        particle(b->pos.x, b->pos.y, 3, 1, CGLP_PI / 2, 0.5);
        play(HIT);
      }
    }
  } else {
    TIMES(pressboundBallCount, bi3) {
      float breathe = sin(ticks * 0.2) * 0.1;
      pressboundBalls[bi3].scaleX = 1 + breathe;
      pressboundBalls[bi3].scaleY = 1 - breathe;
    }
  }

  color = LIGHT_CYAN;
  FOR_EACH(pressboundTrails, tri) {
    ASSIGN_ARRAY_ITEM(pressboundTrails, tri, PressboundTrail, tr);
    SKIP_IS_NOT_ALIVE(tr);
    tr->life -= 1;
    if (tr->life > 0) {
      float alpha = (float)tr->life / 8;
      box(tr->x, tr->y, tr->size * alpha, tr->size * alpha, &scratch);
    } else {
      tr->isAlive = false;
    }
  }

  int wallColor;
  if (pressboundWallPress > 35 && !pressboundFrozen) {
    if ((int)floor(ticks / 15) % 2 == 0) {
      wallColor = LIGHT_RED;
    } else {
      wallColor = LIGHT_PURPLE;
    }
  } else {
    wallColor = LIGHT_PURPLE;
  }
  color = wallColor;
  float wp = pressboundWallPress;
  rect(0, 0, wp, 100, &scratch);
  rect(100, 0, -wp, 100, &scratch);
  rect(0, 0, 100, wp, &scratch);
  rect(0, 100, 100, -wp, &scratch);

  TIMES(pressboundBallCount, bi4) {
    PressboundBall* b = &pressboundBalls[bi4];
    float crushMargin;
    if (pressboundFrozen) {
      crushMargin = 6;
    } else {
      crushMargin = 4;
    }
    if (b->pos.x < pressboundWallPress + crushMargin ||
        b->pos.x > 100 - pressboundWallPress - crushMargin ||
        b->pos.y < pressboundWallPress + crushMargin ||
        b->pos.y > 100 - pressboundWallPress - crushMargin) {
      if (ticks % 3 == 0) {
        color = CYAN;
        particle(b->pos.x, b->pos.y, 3, 1, 0, CGLP_PI * 2);
      }
    }

    if (pressboundFrozen) {
      if ((int)floor(ticks / 3) % 2 == 0) {
        color = BLUE;
      } else {
        color = LIGHT_BLUE;
      }
    } else {
      color = CYAN;
    }

    float w = 5 * b->scaleX;
    float h = 5 * b->scaleY;
    box(b->pos.x, b->pos.y, w, h, &scratch);

    float lineLen = 3;
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(b->pos.x, b->pos.y, lineLen, b->rot, &scratch);
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(b->pos.x, b->pos.y, lineLen, b->rot + CGLP_PI / 2, &scratch);

    float eyeOffsetX;
    if (b->vel.x > 0) {
      eyeOffsetX = 1;
    } else {
      eyeOffsetX = -1;
    }
    float eyeOffsetY;
    if (b->vel.y > 0) {
      eyeOffsetY = 0.5;
    } else {
      eyeOffsetY = -0.5;
    }

    color = WHITE;
    box(b->pos.x - 1.5, b->pos.y - 1, 2, 2, &scratch);
    box(b->pos.x + 1.5, b->pos.y - 1, 2, 2, &scratch);

    color = BLACK;
    box(b->pos.x - 1.5 + eyeOffsetX * 0.5, b->pos.y - 1 + eyeOffsetY, 1, 1, &scratch);
    box(b->pos.x + 1.5 + eyeOffsetX * 0.5, b->pos.y - 1 + eyeOffsetY, 1, 1, &scratch);

    color = TRANSPARENT;
    Collision c;
    box(b->pos.x, b->pos.y, 1, 1, &c);
    if (pressboundFrozen && c.isColliding.rect[LIGHT_PURPLE]) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void addGamePressbound() {
  addGame(pressboundTitle, pressboundDescription, pressboundCharacters,
          pressboundCharactersCount, &pressboundOptions, false, &pressboundUpdate);
}
