#include "../cglp.h"

int* tectonictetherTitle = "TECTONIC TETHER";
int* tectonictetherDescription = "[Hold]\n Charge & Retract\n[Release]\n Extend";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tectonictetherCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int tectonictetherCharactersCount = 0;

Options tectonictetherOptions = {100, 100, 53, false};

#define TECTONICTETHER_TETHER_SPEED 0.7
#define TECTONICTETHER_BASE_ANGULAR_VELOCITY 0.01
#define TECTONICTETHER_MAX_ANGULAR_VELOCITY 0.05
#define TECTONICTETHER_MAX_CHARGE_POWER 5
#define TECTONICTETHER_MAX_ABSORPTION_POWER 30
#define TECTONICTETHER_DANGER_ABSORPTION_THRESHOLD 10
#define TECTONICTETHER_EMPTY_ABSORPTION_THRESHOLD 0
#define TECTONICTETHER_ABSORPTION_DECAY_RATE 0.05
#define TECTONICTETHER_ABSORPTION_RECOVERY_RATE 1.5
#define TECTONICTETHER_ASTEROID_COUNT 5

struct TectonictetherPlayerShard {
  float angle;
  float angularVelocity;
  float tetherLength;
  Vector pos;
};
TectonictetherPlayerShard tectonictetherPlayerShard;

struct TectonictetherGeodeFragment {
  Vector pos;
  float growthTime;
  bool isCollectible;
  Vector vel;
  bool isAlive;
};
// Geode fragments never expire upstream either (unbounded growth); sized generously but worth a second look.
#define TECTONICTETHER_MAX_GEODE_COUNT 1024
TectonictetherGeodeFragment[TECTONICTETHER_MAX_GEODE_COUNT] tectonictetherGeodeFragments;
int tectonictetherGeodeFragmentIndex;

struct TectonictetherAsteroid {
  Vector pos;
  float size;
  Vector vel;
};
TectonictetherAsteroid[TECTONICTETHER_ASTEROID_COUNT] tectonictetherAsteroids;

float tectonictetherChargePower;
float tectonictetherExtensionBoost;
float tectonictetherAbsorptionPower;

void tectonictetherUpdate() {
  Collision scratch;
  if (!ticks) {
    // Vircon32 port note: "sss.setQuantize(16)" is a custom sound
    // sequencer call with no equivalent in this engine - dropped.
    tectonictetherPlayerShard.angle = 0;
    tectonictetherPlayerShard.angularVelocity = TECTONICTETHER_BASE_ANGULAR_VELOCITY;
    tectonictetherPlayerShard.tetherLength = 35;
    vectorSet(&tectonictetherPlayerShard.pos, 50, 10);

    INIT_UNALIVED_ARRAY_FAST(tectonictetherGeodeFragments);
    tectonictetherGeodeFragmentIndex = 0;
    TIMES(7, gi) {
      ASSIGN_ARRAY_ITEM(tectonictetherGeodeFragments, tectonictetherGeodeFragmentIndex,
                        TectonictetherGeodeFragment, ng);
      vectorSet(&ng->pos, rnd(10, 90), rnd(10, 90));
      ng->growthTime = 60;
      ng->isCollectible = true;
      vectorSet(&ng->vel, 0, 0);
      ng->isAlive = true;
      tectonictetherGeodeFragmentIndex =
          cgl_wrap(tectonictetherGeodeFragmentIndex + 1, 0, TECTONICTETHER_MAX_GEODE_COUNT);
    }

    TIMES(TECTONICTETHER_ASTEROID_COUNT, asi) {
      vectorSet(&tectonictetherAsteroids[asi].pos, rnd(20, 80), rnd(20, 80));
      tectonictetherAsteroids[asi].size = rnd(8, 12);
      vectorSet(&tectonictetherAsteroids[asi].vel, 0, 0);
    }

    tectonictetherChargePower = 0;
    tectonictetherExtensionBoost = 0;
    tectonictetherAbsorptionPower = TECTONICTETHER_MAX_ABSORPTION_POWER;
  }

  // --- Player Logic ---
  if (input.isPressed) {
    // Shorten tether
    tectonictetherPlayerShard.tetherLength -= TECTONICTETHER_TETHER_SPEED;
    tectonictetherPlayerShard.angularVelocity += 0.001;
    // Build charge only when tether is short
    if (tectonictetherPlayerShard.tetherLength <= 10) {
      tectonictetherChargePower = fmin(tectonictetherChargePower + 0.1, TECTONICTETHER_MAX_CHARGE_POWER);
    }
  } else {
    // Extend tether
    if (input.isJustReleased) {
      // Unleash the charged boost
      tectonictetherExtensionBoost = tectonictetherChargePower;
      tectonictetherChargePower = 0;
      if (tectonictetherExtensionBoost > 1) {
        play(LASER);
      }
    }
    tectonictetherPlayerShard.tetherLength += TECTONICTETHER_TETHER_SPEED + tectonictetherExtensionBoost;
    tectonictetherPlayerShard.angularVelocity -= 0.0005;
    // Decay the boost
    tectonictetherExtensionBoost *= 0.92;
  }

  tectonictetherPlayerShard.angularVelocity = clamp(tectonictetherPlayerShard.angularVelocity,
                                                     TECTONICTETHER_BASE_ANGULAR_VELOCITY,
                                                     TECTONICTETHER_MAX_ANGULAR_VELOCITY);

  tectonictetherPlayerShard.angle += tectonictetherPlayerShard.angularVelocity;
  vectorSet(&tectonictetherPlayerShard.pos, 50, 50);
  addWithAngle(&tectonictetherPlayerShard.pos, tectonictetherPlayerShard.angle,
               tectonictetherPlayerShard.tetherLength);

  // --- Absorption Power Logic ---
  tectonictetherAbsorptionPower -= TECTONICTETHER_ABSORPTION_DECAY_RATE * difficulty;

  if (tectonictetherAbsorptionPower <= TECTONICTETHER_EMPTY_ABSORPTION_THRESHOLD) {
    play(EXPLOSION);
    // Vircon32 port note: this particle() call has no explicit color() of
    // its own upstream (it fires before any drawing this frame) - red
    // (matching the explosion) is used explicitly here instead.
    color = RED;
    particle(tectonictetherPlayerShard.pos.x, tectonictetherPlayerShard.pos.y, 20, 5, 0, CGLP_PI * 2);
    gameOver();
  }

  // Wall collision for player
  if (tectonictetherPlayerShard.pos.x < 3) {
    tectonictetherPlayerShard.pos.x = 3;
  }
  if (tectonictetherPlayerShard.pos.x > 97) {
    tectonictetherPlayerShard.pos.x = 97;
  }
  if (tectonictetherPlayerShard.pos.y < 3) {
    tectonictetherPlayerShard.pos.y = 3;
  }
  if (tectonictetherPlayerShard.pos.y > 97) {
    tectonictetherPlayerShard.pos.y = 97;
  }
  tectonictetherPlayerShard.tetherLength = distanceTo(&tectonictetherPlayerShard.pos, 50, 50);
  tectonictetherPlayerShard.tetherLength = clamp(tectonictetherPlayerShard.tetherLength, 10, 999);

  // --- Drawing ---
  color = LIGHT_BLACK;
  rect(0, 0, 100, 2, &scratch);
  rect(0, 98, 100, 2, &scratch);
  rect(0, 0, 2, 100, &scratch);
  rect(98, 0, 2, 100, &scratch);

  color = BLACK;
  thickness = 1;
  line(50, 50, tectonictetherPlayerShard.pos.x, tectonictetherPlayerShard.pos.y, &scratch);

  // Draw charge power as yellow line overlapping the tether
  if (tectonictetherChargePower > 0) {
    color = YELLOW;
    thickness = 1 + tectonictetherChargePower * 0.7;
    line(50, 50, tectonictetherPlayerShard.pos.x, tectonictetherPlayerShard.pos.y, &scratch);
  }

  // Draw absorption power as yellow rectangle overlapping player
  color = YELLOW;
  float absorptionSize = 6 + tectonictetherAbsorptionPower * 0.25;
  rect(tectonictetherPlayerShard.pos.x - absorptionSize / 2, tectonictetherPlayerShard.pos.y - absorptionSize / 2,
       absorptionSize, absorptionSize, &scratch);

  // Apply danger effects when absorption power is low
  float playerPosX = tectonictetherPlayerShard.pos.x;
  float playerPosY = tectonictetherPlayerShard.pos.y;
  if (tectonictetherAbsorptionPower <= TECTONICTETHER_DANGER_ABSORPTION_THRESHOLD) {
    float shakeIntensity = (TECTONICTETHER_DANGER_ABSORPTION_THRESHOLD - tectonictetherAbsorptionPower) * 0.2;
    playerPosX += rnd(-shakeIntensity, shakeIntensity);
    playerPosY += rnd(-shakeIntensity, shakeIntensity);

    if (ticks % 10 < 3) {
      color = RED;
      if (ticks % 10 == 0) {
        play(CLICK);
      }
    } else {
      color = CYAN;
    }
  } else {
    color = CYAN;
  }

  Collision playerCollision;
  box(playerPosX, playerPosY, 6, 6, &playerCollision);

  color = GREEN;
  FOR_EACH(tectonictetherGeodeFragments, gfi) {
    ASSIGN_ARRAY_ITEM(tectonictetherGeodeFragments, gfi, TectonictetherGeodeFragment, g);
    SKIP_IS_NOT_ALIVE(g);
    if (g->growthTime < 60) {
      g->growthTime++;
      if (g->growthTime >= 60) {
        g->isCollectible = true;
        vectorSet(&g->vel, 0, 0);
      }
    }

    if (!g->isCollectible) {
      vectorAdd(&g->pos, g->vel.x, g->vel.y);
      vectorMul(&g->vel, 0.96);
    }

    float growthRatio = g->growthTime / 60;
    float currentSize = 1 + 4 * growthRatio;

    float attractionRange = 15 + tectonictetherAbsorptionPower;
    if (g->isCollectible &&
        distanceTo(&g->pos, tectonictetherPlayerShard.pos.x, tectonictetherPlayerShard.pos.y) <
            attractionRange) {
      float attractionStrength = 0.3 + tectonictetherAbsorptionPower * 0.05;
      float toPlayerX = tectonictetherPlayerShard.pos.x - g->pos.x;
      float toPlayerY = tectonictetherPlayerShard.pos.y - g->pos.y;
      float toPlayerLen = sqrt(toPlayerX * toPlayerX + toPlayerY * toPlayerY);
      // Vircon32 port note: real division by zero hard-traps the CPU here
      // (unlike JS) - guard before normalizing toward the player.
      if (toPlayerLen > 0) {
        vectorAdd(&g->pos, toPlayerX / toPlayerLen * attractionStrength,
                  toPlayerY / toPlayerLen * attractionStrength);
      }
    }

    g->pos.x = clamp(g->pos.x, 5, 95);
    g->pos.y = clamp(g->pos.y, 5, 95);

    Collision geodeCollision;
    box(g->pos.x, g->pos.y, currentSize, currentSize, &geodeCollision);

    if (g->isCollectible && geodeCollision.isColliding.rect[CYAN]) {
      play(COIN);
      // Vircon32 port note: bare particle() call upstream inherits
      // whatever color the frame last set - still "green" from this
      // loop's own color() above, made explicit here for clarity.
      color = GREEN;
      particle(g->pos.x, g->pos.y, 16, 1, 0, CGLP_PI * 2);
      tectonictetherAbsorptionPower =
          fmin(tectonictetherAbsorptionPower + TECTONICTETHER_ABSORPTION_RECOVERY_RATE,
               TECTONICTETHER_MAX_ABSORPTION_POWER);
      addScore(1, g->pos.x, g->pos.y);
      g->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  TIMES(TECTONICTETHER_ASTEROID_COUNT, ai) {
    TectonictetherAsteroid* a = &tectonictetherAsteroids[ai];
    vectorAdd(&a->pos, a->vel.x, a->vel.y);
    vectorMul(&a->vel, 0.95);

    Collision asteroidCollision;
    box(a->pos.x, a->pos.y, a->size, a->size, &asteroidCollision);

    // Player collision
    if (asteroidCollision.isColliding.rect[CYAN]) {
      float playerDist = distanceTo(&tectonictetherPlayerShard.pos, 50, 50);
      float asteroidDist = distanceTo(&a->pos, 50, 50);

      float collisionStrength =
          tectonictetherPlayerShard.angularVelocity * (tectonictetherPlayerShard.tetherLength / 20);

      play(HIT);

      if (playerDist < asteroidDist) {
        tectonictetherPlayerShard.tetherLength -= 5;
      } else {
        tectonictetherPlayerShard.tetherLength += 5;
      }

      float knockbackX = a->pos.x - tectonictetherPlayerShard.pos.x;
      float knockbackY = a->pos.y - tectonictetherPlayerShard.pos.y;
      float knockbackLen = sqrt(knockbackX * knockbackX + knockbackY * knockbackY);
      float knockbackUnitX = 0;
      float knockbackUnitY = 0;
      if (knockbackLen > 0) {
        knockbackUnitX = knockbackX / knockbackLen;
        knockbackUnitY = knockbackY / knockbackLen;
      }
      vectorAdd(&a->vel, knockbackUnitX * 0.8, knockbackUnitY * 0.8);

      // Spawn geodes based on collision strength
      int numGeodes = (int)(collisionStrength * 20 * difficulty);

      // Calculate base direction (opposite to knockback)
      float baseDirX = -knockbackUnitX;
      float baseDirY = -knockbackUnitY;
      float baseAngle = cgl_atan2(baseDirY, baseDirX);

      int kg;
      for (kg = 0; kg < numGeodes; kg++) {
        float spreadAngle = rnd(-CGLP_PI / 3, CGLP_PI / 3);
        float finalAngle = baseAngle + spreadAngle;

        float speed = rnd(1.5, 3.5);
        float ivx = cos(finalAngle) * speed * 0.3;
        float ivy = sin(finalAngle) * speed * 0.3;

        float spawnX = clamp(a->pos.x + rnd(-3, 3), 10, 90);
        float spawnY = clamp(a->pos.y + rnd(-3, 3), 10, 90);

        ASSIGN_ARRAY_ITEM(tectonictetherGeodeFragments, tectonictetherGeodeFragmentIndex,
                          TectonictetherGeodeFragment, ngA);
        vectorSet(&ngA->pos, spawnX, spawnY);
        ngA->growthTime = 0;
        ngA->isCollectible = false;
        vectorSet(&ngA->vel, ivx, ivy);
        ngA->isAlive = true;
        tectonictetherGeodeFragmentIndex =
            cgl_wrap(tectonictetherGeodeFragmentIndex + 1, 0, TECTONICTETHER_MAX_GEODE_COUNT);
      }
    }

    // Wall collision (destruction)
    if (asteroidCollision.isColliding.rect[LIGHT_BLACK]) {
      play(EXPLOSION);
      // Vircon32 port note: bare particle(), same reasoning as the geode
      // collect case above (still "black" from this loop's own color()).
      color = BLACK;
      particle(a->pos.x, a->pos.y, 16, 1, 0, CGLP_PI * 2);
      addScore(10, a->pos.x, a->pos.y);

      int numGeodes2 = (int)((a->size / 2 + 1) * difficulty);
      int kg2;
      for (kg2 = 0; kg2 < numGeodes2; kg2++) {
        float angle2 = rnd(0, CGLP_PI * 2);
        float speed2 = rnd(1.0, 1.5);
        float ivx2 = cos(angle2) * speed2;
        float ivy2 = sin(angle2) * speed2;

        float spawnX2 = clamp(a->pos.x + rnd(-8, 8), 10, 90);
        float spawnY2 = clamp(a->pos.y + rnd(-8, 8), 10, 90);

        ASSIGN_ARRAY_ITEM(tectonictetherGeodeFragments, tectonictetherGeodeFragmentIndex,
                          TectonictetherGeodeFragment, ngB);
        vectorSet(&ngB->pos, spawnX2, spawnY2);
        ngB->growthTime = 0;
        ngB->isCollectible = false;
        vectorSet(&ngB->vel, ivx2, ivy2);
        ngB->isAlive = true;
        tectonictetherGeodeFragmentIndex =
            cgl_wrap(tectonictetherGeodeFragmentIndex + 1, 0, TECTONICTETHER_MAX_GEODE_COUNT);
      }

      vectorSet(&a->pos, rnd(20, 80), rnd(20, 80));
      vectorSet(&a->vel, 0, 0);
    }

    // Asteroid vs Asteroid collision
    int aj;
    for (aj = ai + 1; aj < TECTONICTETHER_ASTEROID_COUNT; aj++) {
      TectonictetherAsteroid* b = &tectonictetherAsteroids[aj];
      float dist = distanceTo(&a->pos, b->pos.x, b->pos.y);
      float minDist = (a->size + b->size) / 2;
      if (dist < minDist) {
        play(CLICK);
        float overlap = minDist - dist;
        float pushX = a->pos.x - b->pos.x;
        float pushY = a->pos.y - b->pos.y;
        float pushLen = sqrt(pushX * pushX + pushY * pushY);
        if (pushLen > 0) {
          float pushNormX = pushX / pushLen;
          float pushNormY = pushY / pushLen;
          vectorAdd(&a->pos, pushNormX * (overlap / 2), pushNormY * (overlap / 2));
          vectorAdd(&b->pos, -pushNormX * (overlap / 2), -pushNormY * (overlap / 2));
          float knockX = pushNormX * 0.4;
          float knockY = pushNormY * 0.4;
          vectorAdd(&a->vel, knockX, knockY);
          vectorAdd(&b->vel, -knockX, -knockY);
        }
      }
    }
  }
}

void addGameTectonictether() {
  addGame(tectonictetherTitle, tectonictetherDescription, tectonictetherCharacters,
          tectonictetherCharactersCount, &tectonictetherOptions, false, &tectonictetherUpdate);
}
