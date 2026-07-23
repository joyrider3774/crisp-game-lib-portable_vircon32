#include "../cglp.h"

char* fracaveTitle = "FRACAVE";
char* fracaveDescription = "[Hold]\n Accelerate";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] fracaveCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int fracaveCharactersCount = 1;

Options fracaveOptions = {100, 150, 4, false};

struct FracavePlayer {
  Vector pos;
  float angle;
  float speed;
  float noReflectionDistance;
};
FracavePlayer fracavePlayer;

struct Wall {
  float x1;
  float y1;
  float x2;
  float y2;
  bool isAlive;
};
#define FRACAVE_MAX_WALL_COUNT 100
Wall[FRACAVE_MAX_WALL_COUNT] fracaveWalls;
int fracaveWallIndex;

float fracaveWallSpeed;
float fracaveWallInterval;

void fracaveGenerateFractalWall(float x1, float y1, float x2, float y2, int depth) {
  if (depth == 0) {
    fracaveWalls[fracaveWallIndex].x1 = x1;
    fracaveWalls[fracaveWallIndex].y1 = y1;
    fracaveWalls[fracaveWallIndex].x2 = x2;
    fracaveWalls[fracaveWallIndex].y2 = y2;
    fracaveWalls[fracaveWallIndex].isAlive = true;
    fracaveWallIndex = cgl_wrap(fracaveWallIndex + 1, 0, FRACAVE_MAX_WALL_COUNT);
    return;
  }
  float midX = (x1 + x2) / 2;
  float midY = (y1 + y2) / 2;
  float offsetX = rnd(-10, 10);
  float offsetY = rnd(-5, 5);
  fracaveGenerateFractalWall(x1, y1, midX + offsetX, midY + offsetY, depth - 1);
  fracaveGenerateFractalWall(midX + offsetX, midY + offsetY, x2, y2, depth - 1);
}

void fracaveUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&fracavePlayer.pos, 40, 75);
    fracavePlayer.angle = 0;
    fracavePlayer.speed = 1;
    fracavePlayer.noReflectionDistance = 0;
    INIT_UNALIVED_ARRAY_FAST(fracaveWalls);
    fracaveWallIndex = 0;
    fracaveGenerateFractalWall(20, 0, 20, 150, 3);
    fracaveGenerateFractalWall(80, 0, 80, 150, 3);
    fracaveWallSpeed = 1;
    fracaveWallInterval = 150;
  }
  fracaveWallInterval += fracaveWallSpeed;
  if (fracaveWallInterval >= 150) {
    fracaveWallInterval = 0;
    fracaveGenerateFractalWall(20, -150, 20, 0, 3);
    fracaveGenerateFractalWall(80, -150, 80, 0, 3);
  }
  if (input.isJustPressed) {
    play(CLICK);
  }
  if (input.isPressed) {
    fracavePlayer.speed += (2 - fracavePlayer.speed) * 0.3;
  } else {
    fracavePlayer.speed += (0.1 - fracavePlayer.speed) * 0.3;
  }
  addWithAngle(&fracavePlayer.pos, fracavePlayer.angle, fracavePlayer.speed * difficulty);
  fracavePlayer.noReflectionDistance -= fracavePlayer.speed * difficulty;
  FOR_EACH(fracaveWalls, i) {
    ASSIGN_ARRAY_ITEM(fracaveWalls, i, Wall, wall);
    SKIP_IS_NOT_ALIVE(wall);
    wall->y1 += fracaveWallSpeed;
    wall->y2 += fracaveWallSpeed;
    wall->isAlive = wall->y1 < 170;
  }
  color = CYAN;
  bar(fracavePlayer.pos.x, fracavePlayer.pos.y, 7, fracavePlayer.angle, &scratch);
  color = BLACK;
  FOR_EACH(fracaveWalls, i) {
    ASSIGN_ARRAY_ITEM(fracaveWalls, i, Wall, wall);
    SKIP_IS_NOT_ALIVE(wall);
    line(wall->x1, wall->y1, wall->x2, wall->y2, &scratch);
    if (scratch.isColliding.rect[CYAN] && fracavePlayer.noReflectionDistance < 0) {
      Vector wallVector;
      wallVector.x = wall->x2 - wall->x1;
      wallVector.y = wall->y2 - wall->y1;
      Vector wallNormal;
      wallNormal.x = -wallVector.y;
      wallNormal.y = wallVector.x;
      float wallNormalLength = vectorLength(&wallNormal);
      if (wallNormalLength > 0) {
        vectorMul(&wallNormal, 1.0 / wallNormalLength);
      } else {
        // Degenerate (zero-length) wall segment - no defined normal to
        // reflect off. Vircon32 hard-traps on the division this would
        // otherwise be (unlike most platforms, which would silently
        // produce Infinity/NaN and carry on), so fall back to an
        // arbitrary unit vector instead of leaving wallNormal at (0,0),
        // which the reflection math below assumes is unit-length.
        vectorSet(&wallNormal, 0, 1);
      }
      Vector playerVector;
      playerVector.x = cos(fracavePlayer.angle);
      playerVector.y = sin(fracavePlayer.angle);
      float dot = playerVector.x * wallNormal.x + playerVector.y * wallNormal.y;
      Vector reflectVector;
      reflectVector.x = playerVector.x - 2 * dot * wallNormal.x;
      reflectVector.y = playerVector.y - 2 * dot * wallNormal.y;
      fracavePlayer.angle = cgl_atan2(reflectVector.y, reflectVector.x);
      fracavePlayer.speed = 0.2;
      addScore(1, fracavePlayer.pos.x, fracavePlayer.pos.y);
      addWithAngle(&fracavePlayer.pos, fracavePlayer.angle, 7);
      play(HIT);
      fracavePlayer.noReflectionDistance = 9;
    }
  }
  if (fracavePlayer.pos.x < 0 || fracavePlayer.pos.x > 100 || fracavePlayer.pos.y < 0 ||
      fracavePlayer.pos.y > 150) {
    play(EXPLOSION);
    gameOver();
  }
  fracaveWallSpeed = difficulty;
}

void addGameFracave() {
  addGame(fracaveTitle, fracaveDescription, fracaveCharacters,
          fracaveCharactersCount, &fracaveOptions, false, &fracaveUpdate);
}
