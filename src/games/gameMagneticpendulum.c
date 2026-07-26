#include "../cglp.h"

int* magneticpendulumTitle = "MAGNETIC PENDULUM";
int* magneticpendulumDescription = "[Hold]\n Shorten rope\nCollect falling magnets";

// Sprites trimmed from upstream's 8/7 rows to this engine's fixed 6-row grid.
int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] magneticpendulumCharacters = {
    {
        " rrrr ",
        "rRlllr",
        "rlRRRr",
        "rlllRr",
        "rRRRlr",
        " rrrr ",
    },
    {
        " bbbb ",
        "blBBlb",
        "bllBlb",
        "blBllb",
        "blBBlb",
        " bbbb ",
    },
};
int magneticpendulumCharactersCount = 2;

Options magneticpendulumOptions = {100, 100, 1, false};

#define MAGNETICPENDULUM_ROPE_MAX_LENGTH 66
#define MAGNETICPENDULUM_ROPE_MIN_LENGTH 10
#define MAGNETICPENDULUM_ROPE_CHANGE_SPEED 1.0
#define MAGNETICPENDULUM_PENDULUM_RADIUS 3
#define MAGNETICPENDULUM_ANCHOR_X 50
#define MAGNETICPENDULUM_ANCHOR_Y 5
#define MAGNETICPENDULUM_MAGNET_RADIUS 4
#define MAGNETICPENDULUM_MAGNET_FALL_SPEED_BASE 0.3
#define MAGNETICPENDULUM_MAGNET_SPAWN_INTERVAL_BASE 90
#define MAGNETICPENDULUM_MAGNETIC_FORCE 0.15
#define MAGNETICPENDULUM_MAGNETIC_RANGE 30
#define MAGNETICPENDULUM_GRAVITY_BASE 0.1
#define MAGNETICPENDULUM_DAMPING 0.99
#define MAGNETICPENDULUM_ANGLE_VELOCITY_INIT_BASE 0.02
#define MAGNETICPENDULUM_DIFFICULTY_SPEED_MULT 0.5
#define MAGNETICPENDULUM_DIFFICULTY_INTERVAL_MULT 0.3
#define MAGNETICPENDULUM_DIFFICULTY_GRAVITY_MULT 0.3
#define MAGNETICPENDULUM_DIFFICULTY_SWING_MULT 0.2
#define MAGNETICPENDULUM_FLOOR_RISE_AMOUNT 10
#define MAGNETICPENDULUM_FLOOR_LOWER_SPEED 0.01
#define MAGNETICPENDULUM_FLOOR_TRANSITION_SPEED 0.5
#define MAGNETICPENDULUM_FLOOR_COLLECT_BONUS 2
#define MAGNETICPENDULUM_MAX_MULTIPLIER 9
#define MAGNETICPENDULUM_BASE_SCORE 1

struct MagneticpendulumPendulum {
  Vector pos;
  Vector velocity;
};
MagneticpendulumPendulum magneticpendulumPendulum;

struct MagneticpendulumMagnet {
  Vector pos;
  Vector velocity;
  bool isAlive;
};
// Spawn interval and fall speed both scale with difficulty, keeping
// concurrent count low (~3-4 in practice) - generous headroom applied.
#define MAGNETICPENDULUM_MAX_MAGNET_COUNT 32
MagneticpendulumMagnet[MAGNETICPENDULUM_MAX_MAGNET_COUNT] magneticpendulumMagnets;
int magneticpendulumMagnetIndex;

float magneticpendulumRopeLength;
float magneticpendulumAngle;
float magneticpendulumAngularVelocity;
float magneticpendulumMagnetSpawnTimer;
float magneticpendulumFloorHeight;
float magneticpendulumTargetFloorHeight;
int magneticpendulumMultiplier;

void magneticpendulumUpdate() {
  Collision scratch;
  if (!ticks) {
    magneticpendulumRopeLength = MAGNETICPENDULUM_ROPE_MAX_LENGTH;
    magneticpendulumAngle = 0;
    magneticpendulumAngularVelocity =
        MAGNETICPENDULUM_ANGLE_VELOCITY_INIT_BASE * (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_SWING_MULT);
    vectorSet(&magneticpendulumPendulum.pos, MAGNETICPENDULUM_ANCHOR_X,
              MAGNETICPENDULUM_ANCHOR_Y + magneticpendulumRopeLength);
    vectorSet(&magneticpendulumPendulum.velocity, 0, 0);
    INIT_UNALIVED_ARRAY_FAST(magneticpendulumMagnets);
    magneticpendulumMagnetIndex = 0;
    magneticpendulumMagnetSpawnTimer =
        MAGNETICPENDULUM_MAGNET_SPAWN_INTERVAL_BASE / (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_INTERVAL_MULT);
    magneticpendulumFloorHeight = 100;
    magneticpendulumTargetFloorHeight = 100;
    magneticpendulumMultiplier = 1;
  }

  magneticpendulumTargetFloorHeight = fmin(100, magneticpendulumTargetFloorHeight + MAGNETICPENDULUM_FLOOR_LOWER_SPEED);

  float heightDiff = magneticpendulumTargetFloorHeight - magneticpendulumFloorHeight;
  magneticpendulumFloorHeight += heightDiff * MAGNETICPENDULUM_FLOOR_TRANSITION_SPEED;

  if (input.isPressed) {
    magneticpendulumRopeLength = fmax(MAGNETICPENDULUM_ROPE_MIN_LENGTH,
                                       magneticpendulumRopeLength - MAGNETICPENDULUM_ROPE_CHANGE_SPEED);
  } else {
    magneticpendulumRopeLength = fmin(MAGNETICPENDULUM_ROPE_MAX_LENGTH,
                                       magneticpendulumRopeLength + MAGNETICPENDULUM_ROPE_CHANGE_SPEED);
  }

  magneticpendulumMagnetSpawnTimer--;
  if (magneticpendulumMagnetSpawnTimer <= 0) {
    float spawnX = rnd(15, 85);
    float magnetSpeed =
        MAGNETICPENDULUM_MAGNET_FALL_SPEED_BASE * (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_SPEED_MULT);
    ASSIGN_ARRAY_ITEM(magneticpendulumMagnets, magneticpendulumMagnetIndex, MagneticpendulumMagnet, nm);
    vectorSet(&nm->pos, spawnX, 0);
    vectorSet(&nm->velocity, 0, magnetSpeed);
    nm->isAlive = true;
    magneticpendulumMagnetIndex = cgl_wrap(magneticpendulumMagnetIndex + 1, 0, MAGNETICPENDULUM_MAX_MAGNET_COUNT);
    magneticpendulumMagnetSpawnTimer =
        MAGNETICPENDULUM_MAGNET_SPAWN_INTERVAL_BASE / (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_INTERVAL_MULT);
  }

  color = LIGHT_BLACK;
  box(MAGNETICPENDULUM_ANCHOR_X, MAGNETICPENDULUM_ANCHOR_Y, 4, 2, &scratch);

  color = BLACK;
  int[16] magneticpendulumMultText;
  strcpy(magneticpendulumMultText, "x");
  strcat(magneticpendulumMultText, intToChar(magneticpendulumMultiplier));
  // Vircon32 port note: JS drew this with an unsupported isSmallText option;
  // drawn at normal text size instead (see gameSlimestretcher.c precedent).
  text(magneticpendulumMultText, 3, 9, &scratch);

  if (magneticpendulumFloorHeight < 100) {
    color = BLACK;
    rect(0, magneticpendulumFloorHeight, 100, 100 - magneticpendulumFloorHeight, &scratch);
  }

  float totalMagneticForceX = 0;
  float totalMagneticForceY = 0;

  FOR_EACH(magneticpendulumMagnets, mfi) {
    ASSIGN_ARRAY_ITEM(magneticpendulumMagnets, mfi, MagneticpendulumMagnet, mf);
    SKIP_IS_NOT_ALIVE(mf);
    float dx = mf->pos.x - magneticpendulumPendulum.pos.x;
    float dy = mf->pos.y - magneticpendulumPendulum.pos.y;
    float dist = sqrt(dx * dx + dy * dy);

    if (dist < MAGNETICPENDULUM_MAGNETIC_RANGE && dist > 0) {
      float force = MAGNETICPENDULUM_MAGNETIC_FORCE * (1 - dist / MAGNETICPENDULUM_MAGNETIC_RANGE);
      totalMagneticForceX += (dx / dist) * force;
      totalMagneticForceY += (dy / dist) * force;
    }
  }

  float tangentialForceX = cos(magneticpendulumAngle);
  float tangentialForceY = -sin(magneticpendulumAngle);
  float tangentialMagneticForce =
      totalMagneticForceX * tangentialForceX + totalMagneticForceY * tangentialForceY;

  float currentGravity = MAGNETICPENDULUM_GRAVITY_BASE * (1 + difficulty * MAGNETICPENDULUM_DIFFICULTY_GRAVITY_MULT);
  float angularAcceleration =
      -(currentGravity * sin(magneticpendulumAngle)) / magneticpendulumRopeLength +
      tangentialMagneticForce / magneticpendulumRopeLength;

  magneticpendulumAngularVelocity += angularAcceleration;
  magneticpendulumAngularVelocity *= MAGNETICPENDULUM_DAMPING;
  magneticpendulumAngle += magneticpendulumAngularVelocity;

  magneticpendulumPendulum.pos.x = MAGNETICPENDULUM_ANCHOR_X + sin(magneticpendulumAngle) * magneticpendulumRopeLength;
  magneticpendulumPendulum.pos.y = MAGNETICPENDULUM_ANCHOR_Y + cos(magneticpendulumAngle) * magneticpendulumRopeLength;

  color = LIGHT_RED;
  FOR_EACH(magneticpendulumMagnets, mli) {
    ASSIGN_ARRAY_ITEM(magneticpendulumMagnets, mli, MagneticpendulumMagnet, ml);
    SKIP_IS_NOT_ALIVE(ml);
    float dx2 = ml->pos.x - magneticpendulumPendulum.pos.x;
    float dy2 = ml->pos.y - magneticpendulumPendulum.pos.y;
    float dist2 = sqrt(dx2 * dx2 + dy2 * dy2);

    if (dist2 < MAGNETICPENDULUM_MAGNETIC_RANGE && dist2 > 0) {
      thickness = 1;
      line(magneticpendulumPendulum.pos.x, magneticpendulumPendulum.pos.y, ml->pos.x, ml->pos.y, &scratch);
    }
  }

  color = LIGHT_BLUE;
  thickness = 2;
  line(MAGNETICPENDULUM_ANCHOR_X, MAGNETICPENDULUM_ANCHOR_Y, magneticpendulumPendulum.pos.x,
       magneticpendulumPendulum.pos.y, &scratch);

  color = BLACK;
  character("b", magneticpendulumPendulum.pos.x, magneticpendulumPendulum.pos.y, &scratch);

  if (magneticpendulumPendulum.pos.y + MAGNETICPENDULUM_PENDULUM_RADIUS > magneticpendulumFloorHeight) {
    play(EXPLOSION);
    gameOver();
  }

  color = BLACK;
  FOR_EACH(magneticpendulumMagnets, mi) {
    ASSIGN_ARRAY_ITEM(magneticpendulumMagnets, mi, MagneticpendulumMagnet, m);
    SKIP_IS_NOT_ALIVE(m);
    vectorAdd(&m->pos, m->velocity.x, m->velocity.y);

    Collision magnetCol;
    character("a", m->pos.x, m->pos.y, &magnetCol);

    if (magnetCol.isColliding.character['b']) {
      play(COIN);
      particle(m->pos.x, m->pos.y, 10, 2, 0, CGLP_PI * 2);
      addScore(MAGNETICPENDULUM_BASE_SCORE * magneticpendulumMultiplier, m->pos.x, m->pos.y);
      magneticpendulumMultiplier = min(MAGNETICPENDULUM_MAX_MULTIPLIER, magneticpendulumMultiplier + 1);
      magneticpendulumTargetFloorHeight = fmin(100, magneticpendulumTargetFloorHeight + MAGNETICPENDULUM_FLOOR_COLLECT_BONUS);
      m->isAlive = false;
      continue;
    }

    if (m->pos.y + MAGNETICPENDULUM_MAGNET_RADIUS > magneticpendulumFloorHeight) {
      play(HIT);
      magneticpendulumTargetFloorHeight = fmax(0, magneticpendulumTargetFloorHeight - MAGNETICPENDULUM_FLOOR_RISE_AMOUNT);
      magneticpendulumMultiplier = 1;
      m->isAlive = false;
      continue;
    }
  }
}

void addGameMagneticpendulum() {
  addGame(magneticpendulumTitle, magneticpendulumDescription, magneticpendulumCharacters,
          magneticpendulumCharactersCount, &magneticpendulumOptions, false, &magneticpendulumUpdate);
}
