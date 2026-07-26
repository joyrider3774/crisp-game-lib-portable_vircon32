#include "../cglp.h"

int* windowwasherTitle = "WINDOW WASHER";
int* windowwasherDescription = "[Hold] Ascend\n[Release] Descend\nClean windows!";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] windowwasherCharacters = {{
    "  ll  ",
    " l  l ",
    "llllll",
    " l  l ",
    "      ",
    "      ",
}};
int windowwasherCharactersCount = 1;

Options windowwasherOptions = {100, 100, 1, false};

struct WindowwasherPlatform {
  Vector pos;
  float vx;
  float vy;
  float width;
  float height;
};
WindowwasherPlatform windowwasherPlatform;

struct WindowwasherWindow {
  Vector pos;
  float width;
  float height;
  bool isCleaned;
};
#define WINDOWWASHER_WINDOW_COUNT 10
WindowwasherWindow[WINDOWWASHER_WINDOW_COUNT] windowwasherWindows;

struct WindowwasherObstacle {
  Vector pos;
  float vx;
  bool isBird;
  bool isAlive;
};
#define WINDOWWASHER_MAX_OBSTACLE_COUNT 32
WindowwasherObstacle[WINDOWWASHER_MAX_OBSTACLE_COUNT] windowwasherObstacles;
int windowwasherObstacleIndex;

float windowwasherNextObstacleDist;
float windowwasherScreenScrollSpeed;
int windowwasherMultiplier;

void windowwasherDrawPlatform() {
  Collision scratch;
  color = LIGHT_BLACK;
  rect(windowwasherPlatform.pos.x - windowwasherPlatform.width / 2, 0, 1, windowwasherPlatform.pos.y, &scratch);
  rect(windowwasherPlatform.pos.x + windowwasherPlatform.width / 2 - 1, 0, 1, windowwasherPlatform.pos.y, &scratch);
  color = BLUE;
  box(windowwasherPlatform.pos.x, windowwasherPlatform.pos.y, windowwasherPlatform.width, windowwasherPlatform.height, &scratch);
}

void windowwasherUpdateBackground() {
  Collision scratch;
  color = LIGHT_CYAN;
  rect(0, 0, 3, 100, &scratch);
  rect(97, 0, 3, 100, &scratch);
}

void windowwasherUpdatePlatform() {
  if (input.isJustPressed) {
    play(LASER);
  }
  if (input.isPressed) {
    windowwasherPlatform.vy = -1.5;
  } else {
    windowwasherPlatform.vy = 1;
  }
  windowwasherPlatform.pos.x += windowwasherPlatform.vx;
  if (windowwasherPlatform.pos.x > 90 || windowwasherPlatform.pos.x < 10) {
    play(CLICK);
    windowwasherPlatform.vx *= -1;
  }
  windowwasherPlatform.pos.y += windowwasherPlatform.vy;
  windowwasherPlatform.pos.y = clamp(windowwasherPlatform.pos.y, 5, 95);
  windowwasherDrawPlatform();
}

void windowwasherEndGame() {
  play(EXPLOSION);
  gameOver();
}

void windowwasherUpdateWindows() {
  Collision scratch;
  TIMES(WINDOWWASHER_WINDOW_COUNT, wi) {
    WindowwasherWindow* w = &windowwasherWindows[wi];
    w->pos.y -= windowwasherScreenScrollSpeed;
    if (w->pos.y < -30) {
      w->pos.y += 330;
      if (w->isCleaned) {
        w->isCleaned = false;
      } else {
        play(HIT);
        windowwasherMultiplier--;
        if (windowwasherMultiplier < 1) {
          windowwasherMultiplier = 1;
        }
      }
    }
    if (w->isCleaned) {
      color = YELLOW;
    } else {
      color = CYAN;
    }
    box(w->pos.x, w->pos.y, w->width, w->height, &scratch);
    if (scratch.isColliding.rect[BLUE] && !w->isCleaned) {
      play(POWER_UP);
      w->isCleaned = true;
      addScore(windowwasherMultiplier, w->pos.x, w->pos.y);
      windowwasherMultiplier++;
    }
  }
}

void windowwasherUpdateObstacles() {
  Collision scratch;
  FOR_EACH(windowwasherObstacles, oi) {
    ASSIGN_ARRAY_ITEM(windowwasherObstacles, oi, WindowwasherObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.y -= windowwasherScreenScrollSpeed;
    if (o->isBird) {
      o->pos.x += o->vx;
      if (o->pos.x > 95 || o->pos.x < 5) {
        o->vx *= -1;
      }
    }
    color = RED;
    if (o->isBird) {
      characterOptions.isMirrorX = !(o->vx > 0);
      characterOptions.isMirrorY = false;
      characterOptions.rotation = 0;
      character("a", o->pos.x, o->pos.y, &scratch);
      if (scratch.isColliding.rect[BLUE]) {
        windowwasherEndGame();
      }
    } else {
      box(o->pos.x, o->pos.y, 10, 14, &scratch);
      if (scratch.isColliding.rect[BLUE]) {
        windowwasherEndGame();
      }
    }
    if (o->pos.y < -10) {
      o->isAlive = false;
      continue;
    }
  }
  windowwasherNextObstacleDist -= windowwasherScreenScrollSpeed;
  if (windowwasherNextObstacleDist < 0) {
    bool isBird = rnd(0, 1) < 0.5;
    float px = rnd(15, 85);
    float py = 110;
    if (!isBird) {
      color = TRANSPARENT;
      box(px, py, 14, 18, &scratch);
      if (scratch.isColliding.rect[CYAN]) {
        isBird = true;
      }
    }
    ASSIGN_ARRAY_ITEM(windowwasherObstacles, windowwasherObstacleIndex, WindowwasherObstacle, no);
    vectorSet(&no->pos, px, py);
    if (isBird) {
      if (rnd(0, 1) < 0.5) {
        no->vx = 0.5;
      } else {
        no->vx = -0.5;
      }
    } else {
      no->vx = 0;
    }
    no->isBird = isBird;
    no->isAlive = true;
    windowwasherObstacleIndex = cgl_wrap(windowwasherObstacleIndex + 1, 0, WINDOWWASHER_MAX_OBSTACLE_COUNT);
    windowwasherNextObstacleDist += rnd(40, 50);
  }
}

void windowwasherUpdateScroll() {
  windowwasherScreenScrollSpeed = 0.5 + difficulty * 0.1;
}

void windowwasherInitGame() {
  vectorSet(&windowwasherPlatform.pos, 50, 10);
  windowwasherPlatform.vx = 1;
  windowwasherPlatform.vy = 1;
  windowwasherPlatform.width = 20;
  windowwasherPlatform.height = 5;
  TIMES(WINDOWWASHER_WINDOW_COUNT, i) {
    WindowwasherWindow* w = &windowwasherWindows[i];
    vectorSet(&w->pos, rnd(15, 85), 20 + i * 30);
    w->width = 15;
    w->height = 20;
    w->isCleaned = false;
  }
  INIT_UNALIVED_ARRAY_FAST(windowwasherObstacles);
  windowwasherObstacleIndex = 0;
  windowwasherNextObstacleDist = 0;
  windowwasherScreenScrollSpeed = 0.5;
  windowwasherMultiplier = 1;
}

void windowwasherUpdate() {
  Collision scratch;
  if (!ticks) {
    windowwasherInitGame();
  }
  windowwasherUpdateBackground();
  windowwasherUpdatePlatform();
  windowwasherUpdateWindows();
  windowwasherUpdateObstacles();
  windowwasherDrawPlatform();
  windowwasherUpdateScroll();
  color = BLACK;
  int[16] windowwasherMultText;
  strcpy(windowwasherMultText, "x");
  strcat(windowwasherMultText, intToChar(windowwasherMultiplier));
  text(windowwasherMultText, 2, 10, &scratch);
}

void addGameWindowwasher() {
  addGame(windowwasherTitle, windowwasherDescription, windowwasherCharacters,
          windowwasherCharactersCount, &windowwasherOptions, false, &windowwasherUpdate);
}
