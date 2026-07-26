#include "../cglp.h"

int* ricochetpinsTitle = "RICOCHET PINS";
int* ricochetpinsDescription = "[Tap] Shoot pins\nand recoil";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] ricochetpinsCharacters = {
    {
        "  cc  ",
        " c cc ",
        "cc  cc",
        " c cc ",
        "  cc  ",
    },
    {
        "r rr r",
        " rrrr ",
        "rrrrrr",
        " rrrr ",
        "r rr r",
    },
    {
        "  yy  ",
        " yggy ",
        "ygggyy",
        " yggy ",
        "  yy  ",
    },
};
int ricochetpinsCharactersCount = 3;

Options ricochetpinsOptions = {100, 100, 1, true};

#define RICOCHETPINS_PLAYER_INITIAL_SPEED 0.5
#define RICOCHETPINS_PLAYER_FRICTION 0.98
#define RICOCHETPINS_PLAYER_BASE_SCALE 1.0
#define RICOCHETPINS_RECOIL_FORCE 2.0
#define RICOCHETPINS_PIN_SCALE 1.0
#define RICOCHETPINS_INITIAL_PIN_COUNT 5
#define RICOCHETPINS_PIN_COUNT_INCREMENT 1
#define RICOCHETPINS_MAX_HEAT 100
#define RICOCHETPINS_HEAT_PER_SHOT 25
#define RICOCHETPINS_COOLDOWN_RATE 1.0
#define RICOCHETPINS_OVERHEAT_RECOVERY 30
#define RICOCHETPINS_HEAT_SCALE_FACTOR 0.008
#define RICOCHETPINS_MOVING_PIN_RATIO 0.4
#define RICOCHETPINS_PIN_BASE_MOVE_SPEED 0.3
#define RICOCHETPINS_PIN_SPEED_INCREASE_PER_DIFFICULTY 0.05

struct RicochetpinsPlayer {
  Vector pos;
  Vector vel;
  float heat;
  bool isOverheated;
};
RicochetpinsPlayer ricochetpinsPlayer;

struct RicochetpinsPin {
  Vector pos;
  bool isMoving;
  bool isHorizontal;
  Vector vel;
  bool isAlive;
};
// Pin count grows by 1 every time a full wave is cleared, with no upstream
// cap - clamped here to this array's capacity (see the clamp in the "wave
// cleared" branch below) so a very long, skilled session can't overflow it.
#define RICOCHETPINS_MAX_PIN_COUNT 64
RicochetpinsPin[RICOCHETPINS_MAX_PIN_COUNT] ricochetpinsPins;

struct RicochetpinsItem {
  Vector pos;
  bool exists;
};
RicochetpinsItem ricochetpinsItem;

int ricochetpinsCurrentPinCount;
int ricochetpinsMultiplier;
Vector ricochetpinsShotTarget;
bool ricochetpinsShotTargetSet;
int ricochetpinsShotTimer;

// Vircon32 port note: vector.h has no normalize() - divide by length here.
void ricochetpinsNormalize(Vector* v) {
  float len = vectorLength(v);
  if (len > 0) {
    vectorMul(v, 1.0 / len);
  }
}

void ricochetpinsSpawnPins() {
  INIT_UNALIVED_ARRAY_FAST(ricochetpinsPins);
  for (int i = 0; i < ricochetpinsCurrentPinCount; i++) {
    int attempts = 0;
    bool validPos = false;
    float px = 0;
    float py = 0;
    while (!validPos && attempts < 50) {
      px = rnd(15, 85);
      py = rnd(15, 85);
      float distToPlayer = distanceTo(&ricochetpinsPlayer.pos, px, py);
      if (distToPlayer > 30) {
        validPos = true;
      }
      attempts++;
    }

    bool isMoving = rnd(0, 1) < RICOCHETPINS_MOVING_PIN_RATIO;
    bool isHorizontal = rnd(0, 1) < 0.5;
    float currentSpeed = RICOCHETPINS_PIN_BASE_MOVE_SPEED +
                          difficulty * RICOCHETPINS_PIN_SPEED_INCREASE_PER_DIFFICULTY;
    Vector velocity;
    if (isMoving) {
      if (isHorizontal) {
        float sx;
        if (rnd(0, 1) < 0.5) {
          sx = -currentSpeed;
        } else {
          sx = currentSpeed;
        }
        vectorSet(&velocity, sx, 0);
      } else {
        float sy;
        if (rnd(0, 1) < 0.5) {
          sy = -currentSpeed;
        } else {
          sy = currentSpeed;
        }
        vectorSet(&velocity, 0, sy);
      }
    } else {
      vectorSet(&velocity, 0, 0);
    }

    RicochetpinsPin* newPin = &ricochetpinsPins[i];
    vectorSet(&newPin->pos, px, py);
    newPin->isMoving = isMoving;
    newPin->isHorizontal = isHorizontal;
    newPin->vel = velocity;
    newPin->isAlive = true;
  }

  int itemAttempts = 0;
  bool validItemPos = false;
  float ix = 0;
  float iy = 0;
  while (!validItemPos && itemAttempts < 50) {
    ix = rnd(15, 85);
    iy = rnd(15, 85);
    float distToPlayer = distanceTo(&ricochetpinsPlayer.pos, ix, iy);
    bool farFromPins = true;
    for (int i = 0; i < ricochetpinsCurrentPinCount; i++) {
      if (distanceTo(&ricochetpinsPins[i].pos, ix, iy) < 15) {
        farFromPins = false;
      }
    }
    if (distToPlayer > 20 && farFromPins) {
      validItemPos = true;
    }
    itemAttempts++;
  }
  vectorSet(&ricochetpinsItem.pos, ix, iy);
  ricochetpinsItem.exists = true;
}

void ricochetpinsUpdate() {
  Collision scratch;
  // Never reads a Collision result - pin hits are decided by distance math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&ricochetpinsPlayer.pos, 50, 50);
    vectorSet(&ricochetpinsPlayer.vel, RICOCHETPINS_PLAYER_INITIAL_SPEED, 0);
    ricochetpinsPlayer.heat = 0;
    ricochetpinsPlayer.isOverheated = false;
    ricochetpinsCurrentPinCount = RICOCHETPINS_INITIAL_PIN_COUNT;
    ricochetpinsMultiplier = 1;
    ricochetpinsItem.exists = false;
    ricochetpinsSpawnPins();
    ricochetpinsShotTargetSet = false;
    ricochetpinsShotTimer = 0;
  }

  vectorAdd(&ricochetpinsPlayer.pos, ricochetpinsPlayer.vel.x, ricochetpinsPlayer.vel.y);
  vectorMul(&ricochetpinsPlayer.vel, RICOCHETPINS_PLAYER_FRICTION);

  if (ricochetpinsPlayer.heat > 0) {
    ricochetpinsPlayer.heat -= RICOCHETPINS_COOLDOWN_RATE;
    if (ricochetpinsPlayer.heat < 0) {
      ricochetpinsPlayer.heat = 0;
    }
  }

  if (ricochetpinsPlayer.isOverheated && ricochetpinsPlayer.heat <= RICOCHETPINS_OVERHEAT_RECOVERY) {
    ricochetpinsPlayer.isOverheated = false;
  }

  float currentPlayerScale = RICOCHETPINS_PLAYER_BASE_SCALE + ricochetpinsPlayer.heat * RICOCHETPINS_HEAT_SCALE_FACTOR;
  float currentPlayerRadius = 3 * currentPlayerScale;

  if (ricochetpinsPlayer.pos.x < currentPlayerRadius || ricochetpinsPlayer.pos.x > 100 - currentPlayerRadius) {
    ricochetpinsPlayer.vel.x *= -1;
    ricochetpinsPlayer.pos.x = clamp(ricochetpinsPlayer.pos.x, currentPlayerRadius, 100 - currentPlayerRadius);
    play(SELECT);
  }
  if (ricochetpinsPlayer.pos.y < currentPlayerRadius || ricochetpinsPlayer.pos.y > 100 - currentPlayerRadius) {
    ricochetpinsPlayer.vel.y *= -1;
    ricochetpinsPlayer.pos.y = clamp(ricochetpinsPlayer.pos.y, currentPlayerRadius, 100 - currentPlayerRadius);
    play(SELECT);
  }

  FOR_EACH(ricochetpinsPins, pinIdx) {
    ASSIGN_ARRAY_ITEM(ricochetpinsPins, pinIdx, RicochetpinsPin, pin);
    SKIP_IS_NOT_ALIVE(pin);
    if (pin->isMoving) {
      vectorAdd(&pin->pos, pin->vel.x, pin->vel.y);
      float pinRadius = 3 * RICOCHETPINS_PIN_SCALE;
      if (pin->isHorizontal) {
        if (pin->pos.x < pinRadius || pin->pos.x > 100 - pinRadius) {
          pin->vel.x *= -1;
          pin->pos.x = clamp(pin->pos.x, pinRadius, 100 - pinRadius);
        }
      } else {
        if (pin->pos.y < pinRadius || pin->pos.y > 100 - pinRadius) {
          pin->vel.y *= -1;
          pin->pos.y = clamp(pin->pos.y, pinRadius, 100 - pinRadius);
        }
      }
    }

    float pinRadius2 = 3 * RICOCHETPINS_PIN_SCALE;
    float dist = distanceTo(&ricochetpinsPlayer.pos, pin->pos.x, pin->pos.y);
    float collisionDist = currentPlayerRadius + pinRadius2;
    if (dist < collisionDist) {
      play(EXPLOSION);
      gameOver();
    }

    if (dist < collisionDist + 10 && dist >= collisionDist) {
      color = YELLOW;
      if (ticks % 10 < 5) {
        thickness = 2;
        arc(pin->pos.x, pin->pos.y, 12, 0, CGLP_PI * 2, &scratch);
      }
    }

    if (pin->isMoving) {
      color = PURPLE;
    } else {
      color = RED;
    }
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    // Vircon32 port note: character() has no scale option (PIN_SCALE is
    // always 1.0 here anyway, so this is a no-op visually).
    character("b", pin->pos.x, pin->pos.y, &scratch);

    if (pin->isMoving) {
      color = LIGHT_PURPLE;
      thickness = 1;
      if (pin->isHorizontal) {
        line(pin->pos.x - 8, pin->pos.y, pin->pos.x + 8, pin->pos.y, &scratch);
      } else {
        line(pin->pos.x, pin->pos.y - 8, pin->pos.x, pin->pos.y + 8, &scratch);
      }
    }
  }

  if (ricochetpinsItem.exists) {
    float itemDist = distanceTo(&ricochetpinsPlayer.pos, ricochetpinsItem.pos.x, ricochetpinsItem.pos.y);
    if (itemDist < 7) {
      play(COIN);
      particle(ricochetpinsItem.pos.x, ricochetpinsItem.pos.y, 15, 3, 0, CGLP_PI * 2);
      addScore(10 * ricochetpinsMultiplier, ricochetpinsPlayer.pos.x, ricochetpinsPlayer.pos.y);
      ricochetpinsItem.exists = false;
      ricochetpinsMultiplier++;
    } else {
      color = BLACK;
      characterOptions.isMirrorX = false;
      characterOptions.isMirrorY = false;
      characterOptions.rotation = 0;
      // Vircon32 port note: the JS pulses this item's scale over time -
      // character() has no scale option, drawn at normal size (visual
      // only, doesn't affect the fixed-radius collection check below).
      character("c", ricochetpinsItem.pos.x, ricochetpinsItem.pos.y, &scratch);
    }
  }

  RicochetpinsPin* nearestPin = NULL;
  float nearestDist = 999999;
  FOR_EACH(ricochetpinsPins, npi) {
    ASSIGN_ARRAY_ITEM(ricochetpinsPins, npi, RicochetpinsPin, np);
    SKIP_IS_NOT_ALIVE(np);
    float d = distanceTo(&ricochetpinsPlayer.pos, np->pos.x, np->pos.y);
    if (d < nearestDist) {
      nearestDist = d;
      nearestPin = np;
    }
  }

  if (nearestPin != NULL) {
    color = LIGHT_BLACK;
    thickness = 1;
    line(ricochetpinsPlayer.pos.x, ricochetpinsPlayer.pos.y, nearestPin->pos.x, nearestPin->pos.y, &scratch);
  }

  if (ricochetpinsPlayer.isOverheated) {
    color = RED;
    if (ticks % 5 == 0) {
      particle(ricochetpinsPlayer.pos.x, ricochetpinsPlayer.pos.y, 3, 1, 0, CGLP_PI * 2);
    }
  } else if (ricochetpinsPlayer.heat > 75) {
    color = YELLOW;
  } else if (ricochetpinsPlayer.heat > 50) {
    color = LIGHT_CYAN;
  } else {
    color = CYAN;
  }
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  // No scale option on character(); drawn at normal size (collision radius still scales via currentPlayerRadius).
  character("a", ricochetpinsPlayer.pos.x, ricochetpinsPlayer.pos.y, &scratch);

  if (ricochetpinsShotTimer > 0) {
    if (ricochetpinsShotTargetSet) {
      float intensity = ricochetpinsShotTimer / 5.0;
      color = WHITE;
      thickness = 5 * intensity;
      line(ricochetpinsPlayer.pos.x, ricochetpinsPlayer.pos.y, ricochetpinsShotTarget.x, ricochetpinsShotTarget.y, &scratch);
      color = YELLOW;
      thickness = 3 * intensity;
      line(ricochetpinsPlayer.pos.x, ricochetpinsPlayer.pos.y, ricochetpinsShotTarget.x, ricochetpinsShotTarget.y, &scratch);
      if (ricochetpinsShotTimer >= 5) {
        color = BLACK;
        box(ricochetpinsPlayer.pos.x, ricochetpinsPlayer.pos.y, 5 * intensity, 5 * intensity, &scratch);
      }
    }
    ricochetpinsShotTimer--;
  }

  if (input.isJustPressed && nearestPin != NULL && !ricochetpinsPlayer.isOverheated) {
    RicochetpinsPin* targetPin = nearestPin;
    play(HIT);
    vectorSet(&ricochetpinsShotTarget, targetPin->pos.x, targetPin->pos.y);
    ricochetpinsShotTargetSet = true;
    ricochetpinsShotTimer = 9;

    particle(ricochetpinsPlayer.pos.x, ricochetpinsPlayer.pos.y, 5, 2, 0, CGLP_PI * 2);

    Vector beamDir;
    vectorSet(&beamDir, targetPin->pos.x - ricochetpinsPlayer.pos.x, targetPin->pos.y - ricochetpinsPlayer.pos.y);
    TIMES(5, bi) {
      float t = (bi + 1) / 6.0;
      float particlePosX = ricochetpinsPlayer.pos.x + beamDir.x * t;
      float particlePosY = ricochetpinsPlayer.pos.y + beamDir.y * t;
      particle(particlePosX, particlePosY, 3, 1, 0, CGLP_PI * 2);
    }

    particle(targetPin->pos.x, targetPin->pos.y, 20, 5, 0, CGLP_PI * 2);
    targetPin->isAlive = false;

    Vector recoilDir;
    vectorSet(&recoilDir, ricochetpinsPlayer.pos.x - targetPin->pos.x, ricochetpinsPlayer.pos.y - targetPin->pos.y);
    ricochetpinsNormalize(&recoilDir);
    vectorAdd(&ricochetpinsPlayer.vel, recoilDir.x * RICOCHETPINS_RECOIL_FORCE, recoilDir.y * RICOCHETPINS_RECOIL_FORCE);

    addScore(ricochetpinsMultiplier, targetPin->pos.x, targetPin->pos.y);

    ricochetpinsPlayer.heat += RICOCHETPINS_HEAT_PER_SHOT;
    if (ricochetpinsPlayer.heat >= RICOCHETPINS_MAX_HEAT) {
      ricochetpinsPlayer.heat = RICOCHETPINS_MAX_HEAT;
      ricochetpinsPlayer.isOverheated = true;
      play(CLICK);
    }

    bool anyPinAlive = false;
    FOR_EACH(ricochetpinsPins, api) {
      ASSIGN_ARRAY_ITEM(ricochetpinsPins, api, RicochetpinsPin, ap);
      SKIP_IS_NOT_ALIVE(ap);
      anyPinAlive = true;
    }
    if (!anyPinAlive) {
      play(POWER_UP);
      ricochetpinsCurrentPinCount += RICOCHETPINS_PIN_COUNT_INCREMENT;
      if (ricochetpinsCurrentPinCount > RICOCHETPINS_MAX_PIN_COUNT) {
        ricochetpinsCurrentPinCount = RICOCHETPINS_MAX_PIN_COUNT;
      }
      ricochetpinsSpawnPins();
      addScore(ricochetpinsCurrentPinCount * ricochetpinsMultiplier * 2, 50, 50);
    }
  }

  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(ricochetpinsMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameRicochetpins() {
  addGame(ricochetpinsTitle, ricochetpinsDescription, ricochetpinsCharacters,
          ricochetpinsCharactersCount, &ricochetpinsOptions, false, &ricochetpinsUpdate);
}
