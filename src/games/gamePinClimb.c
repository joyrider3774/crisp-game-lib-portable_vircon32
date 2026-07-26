// =============================================================================
// Vircon32 port of gamePinClimb.c
// =============================================================================
// This is the reference example for porting the other 41 games. Changes
// from the original, all following the rules in cglp.h's header comment:
//  - every file-scope name gets a "pinclimb" prefix (no more "static" to
//    keep names private - see cglp.h's port-notes for why).
//  - "typedef struct {...} Pin;" -> "struct Pin {...};".
//  - "Options options = { .viewSizeX = 100, ... };" (designated init) ->
//    positional init in declared field order (viewSizeX, viewSizeY,
//    soundSeed, isDarkColor).
//  - bar(...)/box(...) used to return "Collision" by value and could be
//    used inline (in an if-condition, or with the result simply
//    discarded); now they take a Collision* out-pointer as their last
//    argument, so every call site becomes "declare a Collision, then
//    call" - see pinclimbUpdate() below for both the "discard the
//    result" case (bar) and the "use the result" case (box).
//  - ceilf -> ceil (only one float type in this dialect).
// =============================================================================

#include "../cglp.h"

int* pinclimbTitle = "PIN CLIMB";
int* pinclimbDescription = "[Hold] Stretch";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pinclimbCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int pinclimbCharactersCount = 1;

Options pinclimbOptions = {100, 100, 1, false};

struct Pin {
  Vector pos;
  bool isAlive;
};
struct Cord {
  float angle;
  float length;
  Pin* pin;
};
Cord pinclimbCord;
#define PIN_CLIMB_MAX_PIN_COUNT 32
Pin[PIN_CLIMB_MAX_PIN_COUNT] pinclimbPins;
int pinclimbPinIndex;
float pinclimbNextPinDist;
float pinclimbCordLength = 7;

void pinclimbAddPin(float x, float y) {
  ASSIGN_ARRAY_ITEM(pinclimbPins, pinclimbPinIndex, Pin, p);
  p->pos.x = x;
  p->pos.y = y;
  p->isAlive = true;
  pinclimbPinIndex = cgl_wrap(pinclimbPinIndex + 1, 0, PIN_CLIMB_MAX_PIN_COUNT);
}

void pinclimbUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(pinclimbPins);
    pinclimbPinIndex = 0;
    pinclimbAddPin(50, 5);
    pinclimbNextPinDist = 5;
    pinclimbCord.angle = 0;
    pinclimbCord.length = pinclimbCordLength;
    pinclimbCord.pin = &pinclimbPins[0];
    barCenterPosRatio = 0;
  }
  float scr = difficulty * 0.02;
  if (pinclimbCord.pin->pos.y < 80) {
    scr += (80 - pinclimbCord.pin->pos.y) * 0.1;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    pinclimbCord.length += difficulty;
  } else {
    pinclimbCord.length += (pinclimbCordLength - pinclimbCord.length) * 0.1;
  }
  pinclimbCord.angle += difficulty * 0.05;
  bar(pinclimbCord.pin->pos.x, pinclimbCord.pin->pos.y, pinclimbCord.length,
      pinclimbCord.angle, &scratch);
  if (pinclimbCord.pin->pos.y > 98) {
    play(EXPLOSION);
    gameOver();
  }
  Pin* nextPin = NULL;
  FOR_EACH(pinclimbPins, i) {
    ASSIGN_ARRAY_ITEM(pinclimbPins, i, Pin, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.y += scr;
    box(p->pos.x, p->pos.y, 3, 3, &scratch);
    if (scratch.isColliding.rect[BLACK] && p != pinclimbCord.pin) {
      nextPin = p;
    }
    p->isAlive = p->pos.y <= 102;
  }
  if (nextPin != NULL) {
    play(POWER_UP);
    addScore(ceil(distanceTo(&pinclimbCord.pin->pos, nextPin->pos.x, nextPin->pos.y)),
             nextPin->pos.x, nextPin->pos.y);
    pinclimbCord.pin = nextPin;
    pinclimbCord.length = pinclimbCordLength;
  }
  pinclimbNextPinDist -= scr;
  while (pinclimbNextPinDist < 0) {
    pinclimbAddPin(rnd(10, 90), -2 - pinclimbNextPinDist);
    pinclimbNextPinDist += rnd(5, 15);
  }
}

void addGamePinClimb() {
  addGame(pinclimbTitle, pinclimbDescription, pinclimbCharacters,
          pinclimbCharactersCount, &pinclimbOptions, false, &pinclimbUpdate);
}
