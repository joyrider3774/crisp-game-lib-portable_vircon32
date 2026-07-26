#include "../cglp.h"

int* islashTitle = "I SLASH";
int* islashDescription = "[Hold]\n Slash";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] islashCharacters = {
    {
        "  ll  ",
        " ll   ",
        "llll  ",
        " l  l ",
        "l l   ",
    },
    {
        "   ll ",
        " ll   ",
        "  l   ",
        " l ll ",
        "l     ",
    },
    {
        "   ll ",
        " ll   ",
        "  l   ",
        " l l  ",
        "  l l ",
    },
};
int islashCharactersCount = 3;

Options islashOptions = {200, 50, 7, false};

#define ISLASH_STATE_WAIT 0
#define ISLASH_STATE_SLASH 1

struct IslashEnemy {
  float x;
  float ticks;
};
// Enemies are only ever removed by slashing (index 0); a player that never
// slashes lets them pile up indefinitely (same latent behavior as upstream
// JS's unbounded array), so this is sized generously rather than tightly.
#define ISLASH_MAX_ENEMY_COUNT 256
IslashEnemy[ISLASH_MAX_ENEMY_COUNT] islashEnemies;
int islashEnemyCount;
float islashNextEnemyDist;

void islashRemoveEnemy(int index) {
  memcpy(&islashEnemies[index], &islashEnemies[index + 1],
         (islashEnemyCount - 1 - index) * sizeof(islashEnemies[0]));
  islashEnemyCount--;
}

struct IslashPlayer {
  float x;
  float vx;
  float slashDist;
  float ticks;
  float hitX;
  int state;
};
IslashPlayer islashPlayer;

bool islashIsSlashReady;
float islashFloorX;
int islashMultiplier;
#define ISLASH_PLAYER_Y 37

void islashHitEnemy(int ei) {
  play(SELECT);
  islashPlayer.vx = (islashPlayer.x - islashEnemies[ei].x + 9) * 0.25;
  islashPlayer.x = islashEnemies[ei].x - 6;
  islashPlayer.hitX = islashEnemies[ei].x;
  color = BLACK;
  particle(islashPlayer.hitX, ISLASH_PLAYER_Y, 9, islashPlayer.vx, CGLP_PI, 0.1);
  islashPlayer.state = ISLASH_STATE_WAIT;
  if (islashMultiplier > 1) {
    islashMultiplier--;
  }
}

void islashStartSlash() {
  islashPlayer.x += islashPlayer.slashDist;
  bool isHitting = false;
  if (islashEnemyCount > 1 && islashEnemies[1].x < islashPlayer.x) {
    islashHitEnemy(1);
    isHitting = true;
  } else {
    islashPlayer.state = ISLASH_STATE_SLASH;
    islashPlayer.ticks = 9 / sqrt(difficulty);
  }
  if (islashEnemies[0].x < islashPlayer.x) {
    color = RED;
    particle(islashEnemies[0].x, ISLASH_PLAYER_Y, 16, 1, 0, CGLP_PI * 2);
    if (!isHitting) {
      play(EXPLOSION);
      addScore(islashMultiplier, islashEnemies[0].x, ISLASH_PLAYER_Y);
      islashMultiplier++;
    }
    islashRemoveEnemy(0);
  } else {
    islashPlayer.state = ISLASH_STATE_WAIT;
  }
}

void islashUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - enemy hits and
  // slashes are direct position comparisons (see islashHitEnemy()/
  // islashStartSlash() above), so the engine's own O(n^2) hitbox scan (see
  // checkHitBox() in cglp.c) is pure waste here. Restored automatically
  // when the next real game starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    islashEnemyCount = 0;
    islashNextEnemyDist = 0;
    islashPlayer.x = 10;
    islashPlayer.vx = 0;
    islashPlayer.slashDist = 0;
    islashPlayer.ticks = 0;
    islashPlayer.hitX = 0;
    islashPlayer.state = ISLASH_STATE_WAIT;
    islashIsSlashReady = false;
    islashFloorX = 100;
    islashMultiplier = 1;
  }
  float scr = (islashPlayer.x - 30) * 0.05;
  color = LIGHT_BLACK;
  rect(0, 40, 200, 10, &scratch);
  islashFloorX = cgl_wrap(islashFloorX - scr, 0, 200);
  color = WHITE;
  rect(islashFloorX, 40, 1, 10, &scratch);
  float waitBonus;
  if (islashPlayer.state == ISLASH_STATE_WAIT) {
    waitBonus = sqrt(difficulty);
  } else {
    waitBonus = 0;
  }
  islashNextEnemyDist -= scr + waitBonus;
  if (islashEnemyCount == 0) {
    islashNextEnemyDist = 0;
  }
  if (islashNextEnemyDist <= 0) {
    float d = rnd(15, 30);
    int c = rndi(3, 10);
    float x = 203;
    TIMES(c, i) {
      if (islashEnemyCount < ISLASH_MAX_ENEMY_COUNT) {
        islashEnemies[islashEnemyCount].x = x;
        islashEnemies[islashEnemyCount].ticks = 0;
        islashEnemyCount++;
      }
      x += d;
    }
    islashNextEnemyDist = d * c + rnd(0, 30);
  }
  if (input.isJustReleased) {
    islashIsSlashReady = true;
  }
  bool isSlashing = islashIsSlashReady && input.isPressed;
  islashPlayer.x -= scr + islashPlayer.vx;
  if (islashPlayer.x < 0) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  islashPlayer.vx *= 0.9;
  if (isSlashing && islashPlayer.vx < 1 && islashPlayer.state == ISLASH_STATE_WAIT) {
    islashPlayer.slashDist = (islashEnemies[0].x - islashPlayer.x) * 2;
    islashStartSlash();
  }
  if (islashPlayer.state == ISLASH_STATE_WAIT) {
    if (islashEnemies[0].x < islashPlayer.x) {
      islashHitEnemy(0);
    }
  } else {
    color = RED;
    rect(islashPlayer.x, ISLASH_PLAYER_Y, -islashPlayer.slashDist, 1, &scratch);
    islashPlayer.ticks--;
    if (islashPlayer.ticks < 0) {
      if (isSlashing) {
        islashStartSlash();
      } else {
        islashPlayer.state = ISLASH_STATE_WAIT;
      }
    }
  }
  color = BLACK;
  if (islashPlayer.vx > 1) {
    islashPlayer.hitX -= scr;
    rect(islashPlayer.x, ISLASH_PLAYER_Y, islashPlayer.hitX - islashPlayer.x, 1, &scratch);
  }
  character("a", islashPlayer.x, ISLASH_PLAYER_Y, &scratch);
  for (int i = 0; i < islashEnemyCount; i++) {
    IslashEnemy* e = &islashEnemies[i];
    e->x -= scr;
    if (islashPlayer.state == ISLASH_STATE_WAIT) {
      e->x -= sqrt(difficulty);
      e->ticks += sqrt(difficulty);
    }
    int[2] ec;
    ec[0] = 'b' + (int)floor(e->ticks / 15) % 2;
    ec[1] = 0;
    character(ec, e->x, ISLASH_PLAYER_Y, &scratch);
  }
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(islashMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameIslash() {
  addGame(islashTitle, islashDescription, islashCharacters,
          islashCharactersCount, &islashOptions, false, &islashUpdate);
}
