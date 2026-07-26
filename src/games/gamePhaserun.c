#include "../cglp.h"

int* phaserunTitle = "PHASERUN";
int* phaserunDescription = "[Tap] Phase\n[Hold] Grow";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] phaserunCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int phaserunCharactersCount = 0;

Options phaserunOptions = {100, 100, 0, false};

struct PhaserunPlayer {
  Vector pos;
  float size;
  float targetSize;
  float scaleX;
  float scaleY;
};
PhaserunPlayer phaserunPlayer;

struct PhaserunObstacle {
  Vector pos;
  float rot;
  bool isAlive;
};
#define PHASERUN_MAX_OBSTACLE_COUNT 16
PhaserunObstacle[PHASERUN_MAX_OBSTACLE_COUNT] phaserunObstacles;
int phaserunObstacleIndex;

struct PhaserunCoin {
  Vector pos;
  float rot;
  float bobPhase;
  bool isAlive;
};
#define PHASERUN_MAX_COIN_COUNT 16
PhaserunCoin[PHASERUN_MAX_COIN_COUNT] phaserunCoins;
int phaserunCoinIndex;

struct PhaserunTrail {
  Vector pos;
  float size;
  int life;
  bool isAlive;
};
#define PHASERUN_MAX_TRAIL_COUNT 32
PhaserunTrail[PHASERUN_MAX_TRAIL_COUNT] phaserunTrails;
int phaserunTrailIndex;

float phaserunSpawnTimer;
float phaserunCoinTimer;
bool phaserunIsSolid;
float phaserunPhaseAnimTimer;
float phaserunBreathPhase;

void phaserunUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&phaserunPlayer.pos, 20, 50);
    phaserunPlayer.size = 8;
    phaserunPlayer.targetSize = 8;
    phaserunPlayer.scaleX = 1;
    phaserunPlayer.scaleY = 1;
    INIT_UNALIVED_ARRAY_FAST(phaserunObstacles);
    phaserunObstacleIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(phaserunCoins);
    phaserunCoinIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(phaserunTrails);
    phaserunTrailIndex = 0;
    phaserunSpawnTimer = 60;
    phaserunCoinTimer = 0;
    phaserunIsSolid = true;
    phaserunPhaseAnimTimer = 0;
    phaserunBreathPhase = 0;
  }

  // Breathing animation
  phaserunBreathPhase += 0.08;
  float breathScale = 1 + sin(phaserunBreathPhase) * 0.03;

  // Input - toggle between solid and ghost phase
  if (input.isJustPressed) {
    phaserunIsSolid = !phaserunIsSolid;
    play(SELECT);
    phaserunPlayer.scaleX = 1.3;
    phaserunPlayer.scaleY = 0.7;
    phaserunPhaseAnimTimer = 10;
    if (phaserunIsSolid) {
      color = BLUE;
    } else {
      color = LIGHT_BLUE;
    }
    particle(phaserunPlayer.pos.x, phaserunPlayer.pos.y, 12, 2, 0, CGLP_PI * 2);
  }

  // Animate squash/stretch back to normal
  if (phaserunPhaseAnimTimer > 0) {
    phaserunPhaseAnimTimer--;
    phaserunPlayer.scaleX += (1 - phaserunPlayer.scaleX) * 0.3;
    phaserunPlayer.scaleY += (1 - phaserunPlayer.scaleY) * 0.3;
  } else {
    phaserunPlayer.scaleX = breathScale;
    phaserunPlayer.scaleY = 2 - breathScale;
  }

  // Spawn obstacles
  phaserunSpawnTimer--;
  if (phaserunSpawnTimer <= 0) {
    ASSIGN_ARRAY_ITEM(phaserunObstacles, phaserunObstacleIndex, PhaserunObstacle, no);
    vectorSet(&no->pos, 105, 50);
    no->rot = 0;
    no->isAlive = true;
    phaserunObstacleIndex = cgl_wrap(phaserunObstacleIndex + 1, 0, PHASERUN_MAX_OBSTACLE_COUNT);
    phaserunSpawnTimer = floor(rnd(88, 111) / sqrt(difficulty));
  }

  // Spawn coins
  phaserunCoinTimer--;
  if (phaserunCoinTimer <= 0) {
    ASSIGN_ARRAY_ITEM(phaserunCoins, phaserunCoinIndex, PhaserunCoin, ncn);
    vectorSet(&ncn->pos, 105, 50);
    ncn->rot = 0;
    ncn->bobPhase = rnd(0, CGLP_PI * 2);
    ncn->isAlive = true;
    phaserunCoinIndex = cgl_wrap(phaserunCoinIndex + 1, 0, PHASERUN_MAX_COIN_COUNT);
    phaserunCoinTimer = floor(rnd(11, 77) / difficulty);
  }

  float speed = 1.0 * difficulty;

  if (input.isPressed) {
    phaserunPlayer.targetSize += 3 * sqrt(difficulty);
  } else {
    phaserunPlayer.targetSize += (8 - phaserunPlayer.targetSize) * 0.5;
  }
  phaserunPlayer.size += (phaserunPlayer.targetSize - phaserunPlayer.size) * 0.3;

  // Add trails for obstacles
  FOR_EACH(phaserunTrails, ti) {
    ASSIGN_ARRAY_ITEM(phaserunTrails, ti, PhaserunTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life--;
    color = LIGHT_RED;
    float alpha = (float)t->life / 10;
    box(t->pos.x, t->pos.y, t->size * alpha, t->size * alpha, &scratch);
    t->isAlive = t->life > 0;
  }

  // Draw player FIRST for collision detection
  float drawSizeX = phaserunPlayer.size * phaserunPlayer.scaleX;
  float drawSizeY = phaserunPlayer.size * phaserunPlayer.scaleY;
  color = CYAN;
  box(phaserunPlayer.pos.x, phaserunPlayer.pos.y, drawSizeX, drawSizeY, &scratch);

  // Find nearest threat for eye direction
  bool nearestFound = false;
  float nearestDist = 999;
  float nearestX = 0;
  float nearestY = 0;
  FOR_EACH(phaserunObstacles, oi) {
    ASSIGN_ARRAY_ITEM(phaserunObstacles, oi, PhaserunObstacle, obs);
    SKIP_IS_NOT_ALIVE(obs);
    float d = obs->pos.x - phaserunPlayer.pos.x;
    if (d > 0 && d < nearestDist) {
      nearestDist = d;
      nearestX = obs->pos.x;
      nearestY = obs->pos.y;
      nearestFound = true;
    }
  }
  FOR_EACH(phaserunCoins, ci) {
    ASSIGN_ARRAY_ITEM(phaserunCoins, ci, PhaserunCoin, coin);
    SKIP_IS_NOT_ALIVE(coin);
    float d2 = coin->pos.x - phaserunPlayer.pos.x;
    if (d2 > 0 && d2 < nearestDist) {
      nearestDist = d2;
      nearestX = coin->pos.x;
      nearestY = coin->pos.y;
      nearestFound = true;
    }
  }

  // Draw coins and check collision
  color = YELLOW;
  FOR_EACH(phaserunCoins, ci2) {
    ASSIGN_ARRAY_ITEM(phaserunCoins, ci2, PhaserunCoin, coin2);
    SKIP_IS_NOT_ALIVE(coin2);
    coin2->pos.x -= speed;
    coin2->rot += 0.1;
    coin2->bobPhase += 0.15;

    float bobY = 50 + sin(coin2->bobPhase) * 3;
    coin2->pos.y = bobY;

    float coinSize = 6;
    thickness = 2;
    bar(coin2->pos.x, coin2->pos.y, coinSize, coin2->rot, &scratch);
    thickness = 2;
    bar(coin2->pos.x, coin2->pos.y, coinSize, coin2->rot + CGLP_PI_2, &scratch);

    Collision collision;
    box(coin2->pos.x, coin2->pos.y, coinSize, coinSize, &collision);

    if (phaserunIsSolid && collision.isColliding.rect[CYAN]) {
      addScore(ceil(phaserunPlayer.size), coin2->pos.x, coin2->pos.y);
      play(COIN);
      color = YELLOW;
      particle(coin2->pos.x, coin2->pos.y, 8, 2, 0, CGLP_PI * 2);
      coin2->isAlive = false;
      continue;
    }
    coin2->isAlive = coin2->pos.x >= -10;
  }

  // Draw obstacles and check collision
  color = RED;
  FOR_EACH(phaserunObstacles, oi2) {
    ASSIGN_ARRAY_ITEM(phaserunObstacles, oi2, PhaserunObstacle, obs2);
    SKIP_IS_NOT_ALIVE(obs2);
    if (ticks % 3 == 0) {
      ASSIGN_ARRAY_ITEM(phaserunTrails, phaserunTrailIndex, PhaserunTrail, nt);
      vectorSet(&nt->pos, obs2->pos.x, obs2->pos.y);
      nt->size = 8;
      nt->life = 10;
      nt->isAlive = true;
      phaserunTrailIndex = cgl_wrap(phaserunTrailIndex + 1, 0, PHASERUN_MAX_TRAIL_COUNT);
    }

    obs2->pos.x -= speed;
    obs2->rot -= 0.15 * speed;

    float obsSize = 10;
    thickness = 3;
    bar(obs2->pos.x, obs2->pos.y, obsSize, obs2->rot, &scratch);
    thickness = 3;
    bar(obs2->pos.x, obs2->pos.y, obsSize, obs2->rot + CGLP_PI_2, &scratch);

    Collision collision2;
    box(obs2->pos.x, obs2->pos.y, obsSize, obsSize, &collision2);

    if (phaserunIsSolid && collision2.isColliding.rect[CYAN]) {
      play(HIT);
      color = RED;
      particle(obs2->pos.x, obs2->pos.y, 20, 3, 0, CGLP_PI * 2);
      gameOver();
    }
    obs2->isAlive = obs2->pos.x >= -10;
  }

  // Draw phase indicator and eyes on top of player
  float eyeOffsetX = 0;
  float eyeOffsetY = 0;
  if (nearestFound) {
    Vector dir;
    vectorSet(&dir, nearestX - phaserunPlayer.pos.x, nearestY - phaserunPlayer.pos.y);
    float len = vectorLength(&dir);
    if (len > 0) {
      vectorMul(&dir, 1.0 / len);
    }
    eyeOffsetX = dir.x * 1.5;
    eyeOffsetY = dir.y * 1.5;
  }

  if (phaserunIsSolid) {
    color = BLUE;
    box(phaserunPlayer.pos.x, phaserunPlayer.pos.y, 5 * phaserunPlayer.scaleX, 5 * phaserunPlayer.scaleY,
        &scratch);
    // Eyes - whites
    color = WHITE;
    float eyeSpacing = 2 * phaserunPlayer.scaleX;
    box(phaserunPlayer.pos.x - eyeSpacing, phaserunPlayer.pos.y - 1, 2, 3, &scratch);
    box(phaserunPlayer.pos.x + eyeSpacing, phaserunPlayer.pos.y - 1, 2, 3, &scratch);
    // Eyes - pupils (look toward threat)
    color = BLACK;
    box(phaserunPlayer.pos.x - eyeSpacing + eyeOffsetX * 0.3, phaserunPlayer.pos.y - 1 + eyeOffsetY * 0.3, 1,
        2, &scratch);
    box(phaserunPlayer.pos.x + eyeSpacing + eyeOffsetX * 0.3, phaserunPlayer.pos.y - 1 + eyeOffsetY * 0.3, 1,
        2, &scratch);
  } else {
    color = LIGHT_BLUE;
    box(phaserunPlayer.pos.x, phaserunPlayer.pos.y, 4 * phaserunPlayer.scaleX, 4 * phaserunPlayer.scaleY,
        &scratch);
    // Ghost eyes - more ethereal
    color = WHITE;
    float eyeSpacing2 = 2 * phaserunPlayer.scaleX;
    // Blink occasionally in ghost mode
    if ((ticks / 30) % 4 != 0) {
      box(phaserunPlayer.pos.x - eyeSpacing2, phaserunPlayer.pos.y - 1, 2, 2, &scratch);
      box(phaserunPlayer.pos.x + eyeSpacing2, phaserunPlayer.pos.y - 1, 2, 2, &scratch);
      color = LIGHT_PURPLE;
      box(phaserunPlayer.pos.x - eyeSpacing2 + eyeOffsetX * 0.3, phaserunPlayer.pos.y - 1 + eyeOffsetY * 0.3,
          1, 1, &scratch);
      box(phaserunPlayer.pos.x + eyeSpacing2 + eyeOffsetX * 0.3, phaserunPlayer.pos.y - 1 + eyeOffsetY * 0.3,
          1, 1, &scratch);
    }
  }

  // UI
  color = BLACK;
  if (phaserunIsSolid) {
    text("SOLID", 3, 10, &scratch);
  } else {
    text("GHOST", 3, 10, &scratch);
  }
}

void addGamePhaserun() {
  addGame(phaserunTitle, phaserunDescription, phaserunCharacters, phaserunCharactersCount,
          &phaserunOptions, false, &phaserunUpdate);
}
