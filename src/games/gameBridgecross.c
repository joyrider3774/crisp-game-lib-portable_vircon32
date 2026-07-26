#include "../cglp.h"

int* bridgecrossTitle = "BRIDGE CROSS";
int* bridgecrossDescription = "[Hold] Jump";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bridgecrossCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int bridgecrossCharactersCount = 0;

Options bridgecrossOptions = {150, 50, 3, false};

struct BridgecrossBridge {
  Vector pos;
  float length;
};
#define BRIDGECROSS_MAX_BRIDGE_COUNT 16
BridgecrossBridge[BRIDGECROSS_MAX_BRIDGE_COUNT] bridgecrossBridges;
int bridgecrossBridgeHead;
int bridgecrossBridgeCount;

struct BridgecrossBrigade {
  Vector pos;
  float length;
  float vy;
  bool isJumping;
  bool isFalling;
  bool isOnBridge;
};
BridgecrossBrigade bridgecrossBrigade;
float bridgecrossNextBridgeTicks;
int bridgecrossMultiplier;
float bridgecrossNextMultiplierDist;

void bridgecrossUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - whether the
  // brigade is standing on a bridge is a direct position-overlap check
  // (see the "brigade.pos.x + brigade.length >= b->pos.x" comparison
  // below), so the engine's own O(n^2) hitbox scan (see checkHitBox() in
  // cglp.c) is pure waste here. Restored automatically when the next real
  // game starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    vectorSet(&bridgecrossBrigade.pos, 10, 37);
    bridgecrossBrigade.length = 30;
    bridgecrossBrigade.vy = 0;
    bridgecrossBrigade.isJumping = false;
    bridgecrossBrigade.isFalling = false;
    bridgecrossBrigade.isOnBridge = false;
    bridgecrossBridgeHead = 0;
    bridgecrossBridgeCount = 0;
    bridgecrossNextBridgeTicks = 0;
    bridgecrossMultiplier = 1;
    bridgecrossNextMultiplierDist = 0;
  }
  color = LIGHT_BLUE;
  rect(0, 0, 150, 50, &scratch);
  color = GREEN;
  rect(0, 40, 150, 10, &scratch);
  if (bridgecrossNextBridgeTicks <= 0) {
    float length = rnd(20, 40);
    int idx = (bridgecrossBridgeHead + bridgecrossBridgeCount) % BRIDGECROSS_MAX_BRIDGE_COUNT;
    vectorSet(&bridgecrossBridges[idx].pos, 153, 35);
    bridgecrossBridges[idx].length = length;
    bridgecrossBridgeCount++;
    bridgecrossNextBridgeTicks = rnd(50, 100);
  }
  bridgecrossNextBridgeTicks -= difficulty;
  TIMES(bridgecrossBridgeCount, k) {
    int idx = (bridgecrossBridgeHead + k) % BRIDGECROSS_MAX_BRIDGE_COUNT;
    BridgecrossBridge* b = &bridgecrossBridges[idx];
    b->pos.x -= difficulty;
    color = YELLOW;
    rect(b->pos.x, b->pos.y, b->length, 5, &scratch);
    color = BLUE;
    rect(b->pos.x, b->pos.y + 5, b->length, 10, &scratch);
  }
  bool isOnBridge = false;
  TIMES(bridgecrossBridgeCount, k) {
    int idx = (bridgecrossBridgeHead + k) % BRIDGECROSS_MAX_BRIDGE_COUNT;
    BridgecrossBridge* b = &bridgecrossBridges[idx];
    if (bridgecrossBrigade.pos.x + bridgecrossBrigade.length >= b->pos.x &&
        bridgecrossBrigade.pos.x <= b->pos.x + b->length) {
      isOnBridge = true;
    }
  }
  while (bridgecrossBridgeCount > 0) {
    BridgecrossBridge* b = &bridgecrossBridges[bridgecrossBridgeHead];
    if (!(b->pos.x + b->length > 0)) {
      bridgecrossBridgeHead = (bridgecrossBridgeHead + 1) % BRIDGECROSS_MAX_BRIDGE_COUNT;
      bridgecrossBridgeCount--;
    } else {
      break;
    }
  }
  if (bridgecrossBrigade.pos.y < 35 && !bridgecrossBrigade.isFalling && !isOnBridge) {
    bridgecrossBrigade.isFalling = true;
    bridgecrossBrigade.isOnBridge = false;
  }
  if (bridgecrossBrigade.isJumping || bridgecrossBrigade.isFalling) {
    bridgecrossBrigade.pos.y += bridgecrossBrigade.vy;
    float vyAdd;
    if (input.isPressed) {
      vyAdd = 0.1;
    } else {
      vyAdd = 0.2;
    }
    bridgecrossBrigade.vy += vyAdd * difficulty;
    if (isOnBridge && bridgecrossBrigade.pos.y > 32 && bridgecrossBrigade.vy > 0) {
      bridgecrossBrigade.isJumping = false;
      bridgecrossBrigade.isFalling = false;
      bridgecrossBrigade.isOnBridge = true;
      bridgecrossBrigade.pos.y = 32;
      bridgecrossBrigade.vy = 0;
    }
    if (!isOnBridge && bridgecrossBrigade.pos.y > 37 && bridgecrossBrigade.vy > 0) {
      bridgecrossBrigade.isJumping = false;
      bridgecrossBrigade.isFalling = false;
      bridgecrossBrigade.pos.y = 37;
      bridgecrossBrigade.vy = 0;
    }
  }
  if (!bridgecrossBrigade.isJumping && input.isJustPressed) {
    play(JUMP);
    bridgecrossBrigade.isJumping = true;
    bridgecrossBrigade.isOnBridge = false;
    bridgecrossBrigade.vy = -2 * sqrt(difficulty);
    bridgecrossMultiplier = ceil(bridgecrossMultiplier / 2.0);
  }
  if (bridgecrossBrigade.isOnBridge) {
    bridgecrossNextMultiplierDist -= difficulty;
    if (bridgecrossNextMultiplierDist < 0) {
      play(COIN);
      addScore(bridgecrossMultiplier,
               bridgecrossBrigade.pos.x + clamp(bridgecrossMultiplier * 3, 0, 50),
               bridgecrossBrigade.pos.y);
      bridgecrossNextMultiplierDist = 10;
      bridgecrossMultiplier++;
    }
  }
  if (!bridgecrossBrigade.isJumping && bridgecrossBrigade.pos.y > 32 && isOnBridge) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLACK;
  rect(bridgecrossBrigade.pos.x, bridgecrossBrigade.pos.y, bridgecrossBrigade.length, 3,
       &scratch);
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(bridgecrossMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameBridgecross() {
  addGame(bridgecrossTitle, bridgecrossDescription, bridgecrossCharacters,
          bridgecrossCharactersCount, &bridgecrossOptions, false,
          &bridgecrossUpdate);
}
