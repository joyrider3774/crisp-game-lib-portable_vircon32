#include "../cglp.h"

int* gravitybubbleTitle = "GRAVITY BUBBLE";
int* gravitybubbleDescription = "[Hold & Release]\n Place\n anti-gravity bubbles";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] gravitybubbleCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int gravitybubbleCharactersCount = 0;

Options gravitybubbleOptions = {100, 100, 101, false};

struct GravitybubblePlayer {
  Vector pos;
  Vector vel;
  float size;
};
GravitybubblePlayer gravitybubblePlayer;

struct GravitybubbleBubble {
  Vector pos;
  float size;
  int life;
  bool isAlive;
};
// Each bubble lives a fixed 180 ticks (3s); a press+release cycle can be as
// short as ~5-7 ticks under realistic rapid tapping, so concurrent count can
// reach ~25-36 - raised above the old 32 cap, which sat right at that edge.
#define GRAVITYBUBBLE_MAX_BUBBLE_COUNT 64
GravitybubbleBubble[GRAVITYBUBBLE_MAX_BUBBLE_COUNT] gravitybubbleBubbles;
int gravitybubbleBubbleIndex;

struct GravitybubbleEnemy {
  Vector pos;
  Vector vel;
  float size;
  bool inBubble;
  bool isAlive;
};
// Sized generously above the ~2-3 concurrent estimate, extra headroom for bubble-slowed enemies.
#define GRAVITYBUBBLE_MAX_ENEMY_COUNT 64
GravitybubbleEnemy[GRAVITYBUBBLE_MAX_ENEMY_COUNT] gravitybubbleEnemies;
int gravitybubbleEnemyIndex;

struct GravitybubbleWall {
  Vector pos;
  float width;
  float height;
  bool isAlive;
};
#define GRAVITYBUBBLE_MAX_WALL_COUNT 32
GravitybubbleWall[GRAVITYBUBBLE_MAX_WALL_COUNT] gravitybubbleWalls;
int gravitybubbleWallIndex;

float gravitybubbleGameHeight;
float gravitybubbleNextWallHeight;
float gravitybubbleNextEnemySpawn;
float gravitybubbleLastHeightScore;
bool gravitybubbleShowingPreview;
int gravitybubbleHoldTime;
Vector gravitybubbleSavedPreviewPos;
float gravitybubbleSavedPreviewSize;
bool gravitybubblePlayerInBubble;

void gravitybubbleUpdate() {
  Collision scratch;
  // Never reads a Collision result - all hits are decided by distance/overlap math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&gravitybubblePlayer.pos, 50, 50);
    vectorSet(&gravitybubblePlayer.vel, 1, 0);
    gravitybubblePlayer.size = 6;
    INIT_UNALIVED_ARRAY_FAST(gravitybubbleBubbles);
    gravitybubbleBubbleIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(gravitybubbleEnemies);
    gravitybubbleEnemyIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(gravitybubbleWalls);
    gravitybubbleWallIndex = 0;
    gravitybubbleGameHeight = 0;
    gravitybubbleNextWallHeight = 150;
    gravitybubbleNextEnemySpawn = 60;
    gravitybubbleLastHeightScore = 0;
    gravitybubbleShowingPreview = false;
    gravitybubbleHoldTime = 0;
    vectorSet(&gravitybubbleSavedPreviewPos, 0, 0);
    gravitybubbleSavedPreviewSize = 8;
    gravitybubblePlayerInBubble = false;
  }

  // === Player Physics ===
  gravitybubblePlayer.vel.y += 0.025;
  gravitybubblePlayer.vel.y *= 0.995;

  gravitybubblePlayer.pos.x += gravitybubblePlayer.vel.x;
  if (gravitybubblePlayer.pos.x > 95 || gravitybubblePlayer.pos.x < 5) {
    gravitybubblePlayer.vel.x *= -1;
  }

  gravitybubblePlayer.pos.y += gravitybubblePlayer.vel.y;

  // === Scroll Processing ===
  if (gravitybubblePlayer.pos.y < 60) {
    float scrollOffset = (60 - gravitybubblePlayer.pos.y) * 0.5;

    gravitybubblePlayer.pos.y += scrollOffset;

    FOR_EACH(gravitybubbleBubbles, sbi) {
      ASSIGN_ARRAY_ITEM(gravitybubbleBubbles, sbi, GravitybubbleBubble, b);
      SKIP_IS_NOT_ALIVE(b);
      b->pos.y += scrollOffset;
    }
    FOR_EACH(gravitybubbleEnemies, sei) {
      ASSIGN_ARRAY_ITEM(gravitybubbleEnemies, sei, GravitybubbleEnemy, e);
      SKIP_IS_NOT_ALIVE(e);
      e->pos.y += scrollOffset;
    }
    FOR_EACH(gravitybubbleWalls, swi) {
      ASSIGN_ARRAY_ITEM(gravitybubbleWalls, swi, GravitybubbleWall, w);
      SKIP_IS_NOT_ALIVE(w);
      w->pos.y += scrollOffset;
    }

    gravitybubbleGameHeight += scrollOffset;
  }

  // === Bubble System ===
  if (input.isPressed) {
    gravitybubbleShowingPreview = true;
    gravitybubbleHoldTime++;

    int previewFrames = gravitybubbleHoldTime;

    float vpx = gravitybubblePlayer.pos.x;
    float vpy = gravitybubblePlayer.pos.y;
    float vvx = gravitybubblePlayer.vel.x;
    float vvy = gravitybubblePlayer.vel.y;

    TIMES(previewFrames, simI) {
      vpx += vvx;
      vpy += vvy;
      if (vpx > 95 || vpx < 5) {
        vvx *= -1;
      }
    }

    vectorSet(&gravitybubbleSavedPreviewPos, vpx, vpy);

    gravitybubbleSavedPreviewSize = 8 + fmin(25, gravitybubbleHoldTime * 0.2);

    color = CYAN;
    thickness = 2;
    arc(gravitybubbleSavedPreviewPos.x, gravitybubbleSavedPreviewPos.y, gravitybubbleSavedPreviewSize,
        0, CGLP_PI * 2, &scratch);
  }

  if (input.isJustReleased && gravitybubbleShowingPreview) {
    ASSIGN_ARRAY_ITEM(gravitybubbleBubbles, gravitybubbleBubbleIndex, GravitybubbleBubble, nb);
    vectorSet(&nb->pos, gravitybubbleSavedPreviewPos.x, gravitybubbleSavedPreviewPos.y);
    nb->size = gravitybubbleSavedPreviewSize;
    nb->life = 180;
    nb->isAlive = true;
    gravitybubbleBubbleIndex = cgl_wrap(gravitybubbleBubbleIndex + 1, 0, GRAVITYBUBBLE_MAX_BUBBLE_COUNT);

    play(POWER_UP);
    particle(gravitybubbleSavedPreviewPos.x, gravitybubbleSavedPreviewPos.y,
             floor(gravitybubbleSavedPreviewSize / 3), 1.5, 0, CGLP_PI * 2);
  }

  if (!input.isPressed) {
    gravitybubbleShowingPreview = false;
    gravitybubbleHoldTime = 0;
  }

  // === Altitude Calculation ===
  float currentHeight = gravitybubbleGameHeight + fmax(0, 100 - gravitybubblePlayer.pos.y);

  // === Enemy Generation System ===
  int difficultyLevel = (int)floor(currentHeight / 100) + 1;

  if (ticks >= gravitybubbleNextEnemySpawn) {
    float enemySpeed = rnd(0.2, 0.25 + sqrt(difficultyLevel) * 0.08);
    int levelHalf = (int)floor((float)difficultyLevel / 2.0);
    int sizeExtra = min(3, levelHalf);
    float enemySize = 7 + sizeExtra;

    ASSIGN_ARRAY_ITEM(gravitybubbleEnemies, gravitybubbleEnemyIndex, GravitybubbleEnemy, ne);
    vectorSet(&ne->pos, rnd(15, 85), -5);
    vectorSet(&ne->vel, rnd(-0.6, 0.6), enemySpeed);
    ne->size = enemySize;
    ne->inBubble = false;
    ne->isAlive = true;
    gravitybubbleEnemyIndex = cgl_wrap(gravitybubbleEnemyIndex + 1, 0, GRAVITYBUBBLE_MAX_ENEMY_COUNT);

    int baseInterval = max(45, 120 - difficultyLevel * 10);
    float variation = baseInterval * 0.3;
    gravitybubbleNextEnemySpawn = ticks + baseInterval + rnd(-variation, variation);
  }

  // === Wall Generation System ===
  if (difficultyLevel > 2 && currentHeight >= gravitybubbleNextWallHeight) {
    float wallLength = rnd(25, 50);
    float wallX = rnd(0, 100 - wallLength);

    ASSIGN_ARRAY_ITEM(gravitybubbleWalls, gravitybubbleWallIndex, GravitybubbleWall, nw);
    vectorSet(&nw->pos, wallX + wallLength / 2, -5);
    nw->width = wallLength;
    nw->height = 4;
    nw->isAlive = true;
    gravitybubbleWallIndex = cgl_wrap(gravitybubbleWallIndex + 1, 0, GRAVITYBUBBLE_MAX_WALL_COUNT);

    gravitybubbleNextWallHeight = currentHeight + rnd(70, 150);
  }

  // === Bubble Processing ===
  bool wasInBubble = gravitybubblePlayerInBubble;
  gravitybubblePlayerInBubble = false;

  FOR_EACH(gravitybubbleBubbles, bi) {
    ASSIGN_ARRAY_ITEM(gravitybubbleBubbles, bi, GravitybubbleBubble, b);
    SKIP_IS_NOT_ALIVE(b);
    b->life--;

    float shrinkPhase = max(0, 150 - b->life);
    float currentBubbleSize = b->size * (1 - shrinkPhase / 150);

    float distanceToPlayer = distanceTo(&gravitybubblePlayer.pos, b->pos.x, b->pos.y);
    if (distanceToPlayer < currentBubbleSize + 9) {
      gravitybubblePlayer.vel.y -= 0.05;
      gravitybubblePlayerInBubble = true;
    }

    FOR_EACH(gravitybubbleEnemies, bei) {
      ASSIGN_ARRAY_ITEM(gravitybubbleEnemies, bei, GravitybubbleEnemy, e);
      SKIP_IS_NOT_ALIVE(e);
      float distanceToEnemy = distanceTo(&e->pos, b->pos.x, b->pos.y);
      if (distanceToEnemy < currentBubbleSize + e->size / 2) {
        e->vel.y -= 0.08;
        e->inBubble = true;
      } else {
        e->inBubble = false;
      }
    }

    color = CYAN;
    thickness = 1;
    arc(b->pos.x, b->pos.y, currentBubbleSize, 0, CGLP_PI * 2, &scratch);

    if (b->life <= 0) {
      b->isAlive = false;
      continue;
    }
  }

  if (!wasInBubble && gravitybubblePlayerInBubble) {
    particle(gravitybubblePlayer.pos.x, gravitybubblePlayer.pos.y, 5, 1, 0, CGLP_PI * 2);
  } else if (wasInBubble && !gravitybubblePlayerInBubble) {
    play(SELECT);
  }

  // === Enemy Processing and Drawing ===
  FOR_EACH(gravitybubbleEnemies, ei) {
    ASSIGN_ARRAY_ITEM(gravitybubbleEnemies, ei, GravitybubbleEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!e->inBubble) {
      e->vel.y += 0.02;
    }

    vectorAdd(&e->pos, e->vel.x, e->vel.y);

    if (e->pos.x > 95 || e->pos.x < 5) {
      e->vel.x *= -1;
    }

    FOR_EACH(gravitybubbleWalls, ewi) {
      ASSIGN_ARRAY_ITEM(gravitybubbleWalls, ewi, GravitybubbleWall, w);
      SKIP_IS_NOT_ALIVE(w);
      float enemyLeft = e->pos.x - e->size / 2;
      float enemyRight = e->pos.x + e->size / 2;
      float wallLeft = w->pos.x - w->width / 2;
      float wallRight = w->pos.x + w->width / 2;

      bool xOverlap = enemyRight > wallLeft && enemyLeft < wallRight;
      bool yOverlap = fabs(e->pos.y - w->pos.y) < (e->size + w->height) / 2;

      if (xOverlap && yOverlap) {
        e->vel.y *= -0.6;
        if (e->pos.y < w->pos.y) {
          e->pos.y = w->pos.y - (w->height + e->size) / 2 - 1;
        } else {
          e->pos.y = w->pos.y + (w->height + e->size) / 2 + 1;
        }
      }
    }

    if (e->inBubble) {
      color = YELLOW;
    } else {
      color = RED;
    }
    box(e->pos.x, e->pos.y, e->size, e->size, &scratch);

    float distanceToPlayerE = distanceTo(&gravitybubblePlayer.pos, e->pos.x, e->pos.y);
    if (distanceToPlayerE < (gravitybubblePlayer.size + e->size) / 2 + 2) {
      float impulse = e->vel.y * 1.5;
      gravitybubblePlayer.vel.y += impulse;

      play(HIT);
      particle(e->pos.x, e->pos.y, 25, 2, 0, CGLP_PI * 2);
      addScore(100, e->pos.x, e->pos.y);

      e->isAlive = false;
      continue;
    }

    if (e->pos.y > 110) {
      e->isAlive = false;
      continue;
    }
  }

  // === Wall Processing and Drawing ===
  FOR_EACH(gravitybubbleWalls, wdi) {
    ASSIGN_ARRAY_ITEM(gravitybubbleWalls, wdi, GravitybubbleWall, w);
    SKIP_IS_NOT_ALIVE(w);
    color = GREEN;
    box(w->pos.x, w->pos.y, w->width, w->height, &scratch);

    float playerLeft = gravitybubblePlayer.pos.x - gravitybubblePlayer.size / 2;
    float playerRight = gravitybubblePlayer.pos.x + gravitybubblePlayer.size / 2;
    float wallLeft = w->pos.x - w->width / 2;
    float wallRight = w->pos.x + w->width / 2;

    bool xOverlap = playerRight > wallLeft && playerLeft < wallRight;
    bool yOverlap = fabs(gravitybubblePlayer.pos.y - w->pos.y) < (gravitybubblePlayer.size + w->height) / 2;

    if (xOverlap && yOverlap) {
      gravitybubblePlayer.vel.y *= -0.8;
      play(HIT);
      particle(w->pos.x, w->pos.y, 5, 1.5, 0, CGLP_PI * 2);

      if (gravitybubblePlayer.pos.y < w->pos.y) {
        gravitybubblePlayer.pos.y = w->pos.y - (w->height + gravitybubblePlayer.size) / 2 - 1;
      } else {
        gravitybubblePlayer.pos.y = w->pos.y + (w->height + gravitybubblePlayer.size) / 2 + 1;
      }
    }

    if (w->pos.y > 110) {
      w->isAlive = false;
      continue;
    }
  }

  // === Player Drawing ===
  if (gravitybubblePlayerInBubble) {
    color = CYAN;
  } else {
    color = BLUE;
  }
  box(gravitybubblePlayer.pos.x, gravitybubblePlayer.pos.y, gravitybubblePlayer.size, gravitybubblePlayer.size,
      &scratch);

  // === Score Update ===
  float currentHeightScore = floor(currentHeight);
  if (currentHeightScore > gravitybubbleLastHeightScore) {
    addScore(currentHeightScore - gravitybubbleLastHeightScore, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    gravitybubbleLastHeightScore = currentHeightScore;
  }

  if (gravitybubblePlayer.pos.y > 105) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameGravitybubble() {
  addGame(gravitybubbleTitle, gravitybubbleDescription, gravitybubbleCharacters,
          gravitybubbleCharactersCount, &gravitybubbleOptions, false, &gravitybubbleUpdate);
}
