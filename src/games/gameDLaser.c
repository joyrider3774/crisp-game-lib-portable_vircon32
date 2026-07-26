#include "../cglp.h"

char* dlaserTitle = "D LASER";
char* dlaserDescription = "[Tap]  Turn\n[Hold] Stop";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] dlaserCharacters = {{
    " ll   ",
    "ll    ",
    " l    ",
    " ll   ",
    "l     ",
    "      ",
}, {
    " ll   ",
    "ll    ",
    " l    ",
    "ll    ",
    "  l   ",
    "      ",
}, {
    "l l l ",
    " lll  ",
    "  l   ",
    " l l  ",
    "l   l ",
    "      ",
}};
int dlaserCharactersCount = 3;

Options dlaserOptions = {100, 100, 2, false};

struct DlaserLaser {
  Vector pos;
  bool isHit;
  bool isAlive;
};

struct DlaserShield {
  Vector pos;
  float tx;
  bool isAlive;
};

struct DlaserPlayer {
  Vector pos;
  int vx;
  float ticks;
  float dist;
};

#define DLASER_MAX_LASER_COUNT 10
#define DLASER_MAX_SHIELD_COUNT 32

DlaserLaser[DLASER_MAX_LASER_COUNT] dlaserLasers;
float dlaserLaserCount;
float dlaserLcv;
float dlaserLcVv;
DlaserShield[DLASER_MAX_SHIELD_COUNT] dlaserShields;
int dlaserShieldIndex;
float dlaserNextShieldDist;
DlaserPlayer dlaserPlayer;
float dlaserScrY;

void dlaserResetLaser() {
  INIT_UNALIVED_ARRAY_FAST(dlaserLasers);
  dlaserLaserCount = 0;
  dlaserLcv *= rnd(0, 1);
  dlaserLcVv *= rnd(0, 1);
  dlaserPlayer.dist = 0;
}

void dlaserUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(dlaserLasers);
    dlaserLaserCount = 0;
    dlaserLcv = 0;
    dlaserLcVv = 0;
    INIT_UNALIVED_ARRAY_FAST(dlaserShields);
    dlaserShieldIndex = 0;
    dlaserNextShieldDist = 0;
    dlaserScrY = 80;
    vectorSet(&dlaserPlayer.pos, 50, 90);
    dlaserPlayer.vx = 1;
    dlaserPlayer.ticks = 0;
    dlaserPlayer.dist = 0;
  }

  float scr = sqrt(difficulty) * 0.05;
  if (dlaserPlayer.pos.y < dlaserScrY) {
    scr += (dlaserScrY - dlaserPlayer.pos.y) * 0.1;
  }
  dlaserScrY += (80 - dlaserScrY) * 0.001;

  dlaserNextShieldDist -= scr;
  if (dlaserNextShieldDist < 0) {
    play(HIT);
    float tx = rndi(0, 10) * 10 + 5;
    ASSIGN_ARRAY_ITEM(dlaserShields, dlaserShieldIndex, DlaserShield, s);
    float startX;
    if (tx < 50) {
      startX = 0;
    } else {
      startX = 99;
    }
    vectorSet(&s->pos, startX, 15);
    s->tx = tx;
    s->isAlive = true;
    dlaserShieldIndex = cgl_wrap(dlaserShieldIndex + 1, 0, DLASER_MAX_SHIELD_COUNT);
    dlaserNextShieldDist += rnd(9, 36);
  }

  FOR_EACH(dlaserShields, i) {
    ASSIGN_ARRAY_ITEM(dlaserShields, i, DlaserShield, s);
    SKIP_IS_NOT_ALIVE(s);

    s->pos.y += scr;

    if (fabs(s->pos.x - s->tx) < 1) {
      s->pos.x = s->tx;
      color = BLUE;
    } else {
      s->pos.x += (s->tx - s->pos.x) * 0.2;
      color = LIGHT_BLUE;
    }

    box(s->pos.x, s->pos.y, 8, 4, &scratch);
    color = BLACK;
    character("c", s->pos.x, s->pos.y + 4, &scratch);

    s->isAlive = s->pos.y <= 99;
  }

  COUNT_IS_ALIVE(dlaserLasers, activeLasers);
  if (dlaserLaserCount > 10) {
    dlaserLaserCount += sqrt(difficulty) * 9;

    if (activeLasers == 0) {
      play(POWER_UP);
      for (int i = 0; i < DLASER_MAX_LASER_COUNT; i++) {
        dlaserLasers[i].pos.x = i * 10;
        dlaserLasers[i].pos.y = 10;
        dlaserLasers[i].isHit = false;
        dlaserLasers[i].isAlive = true;
      }
    }

    play(LASER);

    if (dlaserLaserCount > 199) {
      play(COIN);
      addScore(
        ceil(dlaserPlayer.dist * sqrt(dlaserPlayer.dist) * sqrt(difficulty) * 0.1),
        dlaserPlayer.pos.x, dlaserPlayer.pos.y
      );
      dlaserResetLaser();
    }
  } else {
    dlaserLcVv += rnd(-sqrt(difficulty), sqrt(difficulty)) * 0.0001;
    dlaserLcv += dlaserLcVv;

    if ((dlaserLcv < 0 && dlaserLcVv < 0) || (dlaserLcv > sqrt(difficulty) * 0.2 && dlaserLcVv > 0)) {
      dlaserLcVv = -dlaserLcVv;
      dlaserLcv *= rnd(0.2, 1.5);
    }

    dlaserLaserCount += dlaserLcv;
  }

  color = LIGHT_RED;
  rect(0, 9, 100, 1, &scratch);
  color = RED;
  float barH;
  if (dlaserLaserCount < 10) {
    barH = dlaserLaserCount;
  } else {
    barH = 10;
  }
  rect(0, 0, 100, barH, &scratch);

  FOR_EACH(dlaserLasers, i) {
    ASSIGN_ARRAY_ITEM(dlaserLasers, i, DlaserLaser, l);
    SKIP_IS_NOT_ALIVE(l);

    if (l->isHit) {
      l->pos.y += scr;
    } else {
      l->pos.y += sqrt(difficulty) * 9;

      // Check collision with shields
      color = TRANSPARENT;
      Collision collision;
      rect(l->pos.x, 10, 10, l->pos.y - 10, &collision);

      if (collision.isColliding.rect[BLUE]) {
        l->isHit = true;
        // Back up laser to just before collision
        for (int j = 0; j < 99; j++) {
          l->pos.y--;
          color = TRANSPARENT;
          rect(l->pos.x, 10, 10, l->pos.y - 10, &collision);
          if (!collision.isColliding.rect[BLUE]) {
            break;
          }
        }
      }
    }

    color = RED;
    rect(l->pos.x, 10, 10, l->pos.y - 10, &scratch);
  }

  if (input.isJustPressed) {
    play(SELECT);
    dlaserPlayer.vx *= -1;
  }

  dlaserPlayer.pos.y += scr;

  if (!input.isPressed) {
    dlaserPlayer.pos.x += dlaserPlayer.vx * sqrt(difficulty);

    if ((dlaserPlayer.pos.x < 0 && dlaserPlayer.vx < 0) || (dlaserPlayer.pos.x > 99 && dlaserPlayer.vx > 0)) {
      dlaserPlayer.vx *= -1;
    }

    dlaserPlayer.pos.x = clamp(dlaserPlayer.pos.x, 0, 99);
    dlaserPlayer.pos.y -= sqrt(difficulty) * 0.5;
    dlaserPlayer.dist += sqrt(difficulty) * 0.5;
    dlaserPlayer.ticks += sqrt(difficulty);
  }

  if (dlaserPlayer.pos.y > 99) {
    play(RANDOM);
    gameOver();
  }

  color = BLACK;
  characterOptions.isMirrorX = dlaserPlayer.vx < 0;
  int[2] pc;
  pc[0] = 'a' + (int)(dlaserPlayer.ticks / 20) % 2;
  pc[1] = 0;

  Collision playerCl;
  character(pc, dlaserPlayer.pos.x, dlaserPlayer.pos.y, &playerCl);
  if (playerCl.isColliding.rect[RED]) {
    particle(dlaserPlayer.pos.x, dlaserPlayer.pos.y, 30, 3, 0, M_PI * 2);
    dlaserScrY += 7;
    dlaserPlayer.pos.y += 7;
    play(EXPLOSION);
    dlaserResetLaser();
  }
  characterOptions.isMirrorX = false;
}

void addGameDLaser() {
  addGame(dlaserTitle, dlaserDescription, dlaserCharacters, dlaserCharactersCount,
          &dlaserOptions, false, &dlaserUpdate);
}
