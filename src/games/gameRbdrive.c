#include "../cglp.h"

int* rbdriveTitle = "RB DRIVE";
int* rbdriveDescription = "[Tap]\n Change Lane";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rbdriveCharacters = {
    {
        "rrrrr ",
        " rrrrr",
        " rrrrr",
        "rrrrr ",
    },
    {
        "llll  ",
        "  llll",
        "  llll",
        "llll  ",
    },
    {
        " llll ",
        "lwwwwl",
        " llll ",
    },
};
int rbdriveCharactersCount = 3;

Options rbdriveOptions = {100, 100, 10, false};

struct RbdriveCar {
  int angle;
  int prevAngle;
  float phase;
  int color;
  bool isAlive;
};
#define RBDRIVE_MAX_CAR_COUNT 32
RbdriveCar[RBDRIVE_MAX_CAR_COUNT] rbdriveCars;
int rbdriveCarIndex;

float rbdriveNextCarTicks;
int rbdriveNextColor;
int rbdriveNextColorCount;
float[4] rbdriveLaneValues;
int rbdriveLaneAngle;
// Fixed direction table (right/down/left/up), one per lane. JS keeps this as
// a module-level constant array outside update(); set once at tick 0 here
// instead since Vircon32 globals can't take a full array initializer list
// tied to another include-order-sensitive global.
Vector[4] rbdriveAngleOffsets;

void rbdriveDrawLanes() {
  Collision scratch;
  color = LIGHT_RED;
  rect(41, 0, 19, rbdriveLaneValues[3] * 42, &scratch);
  rect(41, 100, 19, -rbdriveLaneValues[1] * 41, &scratch);
  color = LIGHT_BLACK;
  rect(100, 41, -rbdriveLaneValues[0] * 41, 19, &scratch);
  rect(0, 41, rbdriveLaneValues[2] * 42, 19, &scratch);
  color = BLACK;
  rect(40, 0, 1, 40, &scratch);
  rect(40, 60, 1, 40, &scratch);
  rect(60, 0, 1, 40, &scratch);
  rect(60, 60, 1, 40, &scratch);
  rect(0, 40, 41, 1, &scratch);
  rect(60, 40, 40, 1, &scratch);
  rect(0, 60, 40, 1, &scratch);
  rect(60, 60, 40, 1, &scratch);
}

void rbdriveUpdate() {
  Collision scratch;
  // Never reads a Collision result - car/lane logic is decided by angle/index math.
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(rbdriveCars);
    rbdriveCarIndex = 0;
    TIMES(4, i) {
      rbdriveLaneValues[i] = 0.7;
    }
    rbdriveNextCarTicks = 0;
    rbdriveNextColorCount = 0;
    rbdriveNextColor = rndi(0, 2);
    rbdriveLaneAngle = 3;
    vectorSet(&rbdriveAngleOffsets[0], 1, 0);
    vectorSet(&rbdriveAngleOffsets[1], 0, 1);
    vectorSet(&rbdriveAngleOffsets[2], -1, 0);
    vectorSet(&rbdriveAngleOffsets[3], 0, -1);
  }
  float sd = sqrt(difficulty);
  rbdriveDrawLanes();
  if (input.isJustPressed) {
    // Vircon32 port note: upstream passes a per-call sound seed
    // ({seed:3}/{seed:1}/{seed:2}) to vary these effects' tone from the
    // game's other sounds; play() here has no per-call seed parameter, so
    // every call just uses the shared sound RNG state (cosmetic only).
    play(SELECT);
    rbdriveLaneAngle = cgl_wrap(rbdriveLaneAngle + 1, 0, 4);
  }
  Vector* ao = &rbdriveAngleOffsets[rbdriveLaneAngle];
  int lpo;
  if (rbdriveLaneAngle == 1 || rbdriveLaneAngle == 2) {
    lpo = 51;
  } else {
    lpo = 50;
  }
  Vector lp;
  vectorSet(&lp, ao->x, ao->y);
  vectorMul(&lp, 10 + fmod((float)ticks / 5, 7));
  vectorAdd(&lp, lpo, lpo);
  color = BLACK;
  TIMES(6, i) {
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = rbdriveLaneAngle;
    character("c", lp.x, lp.y, &scratch);
    vectorAdd(&lp, ao->x * 7, ao->y * 7);
  }

  rbdriveNextCarTicks -= sd;
  if (rbdriveNextCarTicks < 0) {
    play(CLICK);
    rbdriveNextColorCount--;
    if (rbdriveNextColorCount < 0) {
      rbdriveNextColor = cgl_wrap(rbdriveNextColor + 1, 0, 2);
      rbdriveNextColorCount = rndi(1, 4);
    }
    ASSIGN_ARRAY_ITEM(rbdriveCars, rbdriveCarIndex, RbdriveCar, nc);
    nc->angle = rndi(0, 4);
    nc->prevAngle = 0;
    nc->phase = 0;
    nc->color = rbdriveNextColor;
    nc->isAlive = true;
    rbdriveCarIndex = cgl_wrap(rbdriveCarIndex + 1, 0, RBDRIVE_MAX_CAR_COUNT);
    rbdriveNextCarTicks += rnd(80, 99);
  }

  FOR_EACH(rbdriveCars, ci) {
    ASSIGN_ARRAY_ITEM(rbdriveCars, ci, RbdriveCar, c);
    SKIP_IS_NOT_ALIVE(c);
    Vector* cao = &rbdriveAngleOffsets[c->angle];
    Vector* caoo = &rbdriveAngleOffsets[(int)cgl_wrap(c->angle + 1, 0, 4)];
    Vector cp;
    if (c->phase < 1) {
      vectorSet(&cp, cao->x, cao->y);
      vectorMul(&cp, (1 - c->phase) * 40 + 10);
      vectorAdd(&cp, 50 + caoo->x * 5, 50 + caoo->y * 5);
    } else if (c->phase < 2) {
      Vector* pao = &rbdriveAngleOffsets[c->prevAngle];
      Vector* paoo = &rbdriveAngleOffsets[(int)cgl_wrap(c->prevAngle + 1, 0, 4)];
      vectorSet(&cp, pao->x, pao->y);
      vectorMul(&cp, 10);
      vectorAdd(&cp, paoo->x * 5, paoo->y * 5);
      Vector cpo;
      vectorSet(&cpo, cao->x, cao->y);
      vectorMul(&cpo, 10);
      vectorAdd(&cpo, -(caoo->x * 5), -(caoo->y * 5));
      vectorAdd(&cpo, -cp.x, -cp.y);
      vectorMul(&cpo, c->phase - 1);
      vectorAdd(&cp, cpo.x, cpo.y);
      vectorAdd(&cp, 50, 50);
    } else {
      vectorSet(&cp, cao->x, cao->y);
      vectorMul(&cp, (c->phase - 2) * 40 + 10);
      vectorAdd(&cp, 50 - caoo->x * 5, 50 - caoo->y * 5);
    }
    int[2] carChar;
    carChar[0] = 'a' + c->color;
    carChar[1] = 0;
    int rot = c->angle;
    if (c->phase < 1) {
      rot += 2;
    }
    if (rot >= 4) {
      rot -= 4;
    }
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = rot;
    character(carChar, cp.x, cp.y, &scratch);
    float pp = c->phase;
    float phaseAdd;
    if (c->phase > 1 && c->phase < 2) {
      phaseAdd = 0.05;
    } else {
      phaseAdd = 0.01;
    }
    c->phase += sd * phaseAdd;
    if (pp < 1 && c->phase >= 1) {
      // Vircon32 port note: dropped {seed:1} per-call sound seed - see the
      // note above input.isJustPressed.
      play(HIT);
      c->prevAngle = c->angle;
      if (c->angle != rbdriveLaneAngle) {
        c->angle = rbdriveLaneAngle;
      } else {
        c->angle = cgl_wrap(c->angle + 2, 0, 4);
      }
    }
    if (c->phase > 3) {
      if (c->angle % 2 == c->color) {
        play(EXPLOSION);
        rbdriveLaneValues[c->angle] *= 0.66;
      } else {
        // Vircon32 port note: dropped {seed:2} per-call sound seed - see
        // the note above input.isJustPressed.
        play(POWER_UP);
        rbdriveLaneValues[c->angle] += 0.3;
        if (rbdriveLaneValues[c->angle] > 1) {
          rbdriveLaneValues[c->angle] = 1;
        }
        addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      }
      c->isAlive = false;
      continue;
    }
  }

  TIMES(4, i2) {
    rbdriveLaneValues[i2] -= sd * 0.0006;
    if (rbdriveLaneValues[i2] < 0.01) {
      play(RANDOM);
      gameOver();
    }
  }
}

void addGameRbdrive() {
  addGame(rbdriveTitle, rbdriveDescription, rbdriveCharacters, rbdriveCharactersCount,
          &rbdriveOptions, false, &rbdriveUpdate);
}
