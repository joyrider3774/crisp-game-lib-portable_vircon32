#include "../cglp.h"

int* pumppressTitle = "PUMP PRESS";
int* pumppressDescription = "[Tap]  Shot\n[Hold] Speed up";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pumppressCharacters = {
    {
        " lll  ",
        " lccl ",
        "rrrrrr",
        "rrrrrr",
        " llll ",
        " lll  ",
    },
    {
        "    r ",
        "llllrr",
        "    r ",
    },
    {
        "  ll  ",
        " ll ll",
        "yyyyy ",
        "yyyyy ",
        " ll ll",
        "  ll  ",
    },
};
int pumppressCharactersCount = 3;

Options pumppressOptions = {200, 50, 7, false};

struct PumppressEnemy {
  float x;
  float vx;
  float size;
  bool isPressed;
  Vector pressedOfs;
  bool isAlive;
};
#define PUMPPRESS_MAX_ENEMY_COUNT 64
PumppressEnemy[PUMPPRESS_MAX_ENEMY_COUNT] pumppressEnemies;
int pumppressEnemyIndex;
float pumppressNextEnemyDist;

bool pumppressHasRocket;
float pumppressRocketX;
bool pumppressIsRocketGoing;
float pumppressNextRocketDist;
float pumppressShipX;
float pumppressShipSpeed;
bool pumppressHasShot;
float pumppressShotX;
float pumppressShotRange;
int pumppressPressedCount;
float pumppressScr;

void pumppressUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(pumppressEnemies);
    pumppressEnemyIndex = 0;
    pumppressNextEnemyDist = 0;
    pumppressHasRocket = false;
    pumppressRocketX = 0;
    pumppressIsRocketGoing = false;
    pumppressNextRocketDist = 30;
    pumppressShipX = 100;
    pumppressShipSpeed = 1;
    pumppressHasShot = false;
    pumppressShotX = 0;
    pumppressShotRange = 0;
    pumppressPressedCount = 0;
    pumppressScr = 0;
  }
  color = LIGHT_PURPLE;
  rect(0, 20, 200, 1, &scratch);
  rect(0, 29, 200, 1, &scratch);
  color = BLACK;
  pumppressShipX += difficulty * pumppressShipSpeed;
  float speedTarget;
  if (input.isPressed) {
    speedTarget = 1;
  } else {
    speedTarget = 0.5;
  }
  pumppressShipSpeed += (speedTarget - pumppressShipSpeed) * 0.1;
  if (input.isPressed) {
    particle(pumppressShipX, 25, 1, 2, CGLP_PI, 0.3);
  }
  pumppressScr = difficulty * 0.1;
  if (pumppressShipX > 100) {
    pumppressScr += (pumppressShipX - 100) * 0.1;
  }
  pumppressShipX -= pumppressScr;
  character("a", pumppressShipX, 25, &scratch);
  if (!pumppressHasShot && input.isJustPressed) {
    play(LASER);
    pumppressHasShot = true;
    pumppressShotX = pumppressShipX + 6;
    pumppressShotRange = 60;
  }
  if (pumppressHasShot) {
    float s = difficulty * 3;
    pumppressShotX += s - pumppressScr;
    pumppressShotRange -= s;
    character("b", pumppressShotX, 25, &scratch);
    if (pumppressShotRange < 0) {
      pumppressHasShot = false;
    }
  }
  pumppressNextEnemyDist -= pumppressScr;
  if (pumppressNextEnemyDist < 0) {
    ASSIGN_ARRAY_ITEM(pumppressEnemies, pumppressEnemyIndex, PumppressEnemy, ne);
    ne->x = 205;
    ne->vx = rnd(0.5, 0.4 + 0.1 * sqrt(difficulty)) * difficulty;
    ne->size = 0;
    ne->isPressed = false;
    vectorSet(&ne->pressedOfs, rnd(0, 2) * RNDPM() - 4, rnd(0, 2) * RNDPM());
    ne->isAlive = true;
    pumppressEnemyIndex = cgl_wrap(pumppressEnemyIndex + 1, 0, PUMPPRESS_MAX_ENEMY_COUNT);
    pumppressNextEnemyDist = rnd(40 - sqrt(difficulty) * 10, 40) / sqrt(difficulty) + 3;
  }
  if (pumppressHasRocket) {
    pumppressRocketX -= pumppressScr;
    if (pumppressIsRocketGoing) {
      particle(pumppressRocketX, 25, 1, difficulty, 0, 0.4);
      pumppressRocketX -= difficulty;
    }
    Collision rc;
    character("c", pumppressRocketX, 25, &rc);
    if (!pumppressIsRocketGoing && rc.isColliding.character['b']) {
      pumppressIsRocketGoing = true;
      pumppressHasShot = false;
    }
    if (pumppressRocketX < 5) {
      if (pumppressPressedCount > 0) {
        play(EXPLOSION);
        addScore(((1 + pumppressPressedCount) * pumppressPressedCount) / 2.0, 20, 20);
        particle(pumppressRocketX, 25, pumppressPressedCount * 5,
                 sqrt(pumppressPressedCount) * 0.5 + 2, 0, CGLP_PI * 2);
      }
      pumppressHasRocket = false;
      pumppressPressedCount = 0;
    }
  } else {
    pumppressNextRocketDist -= pumppressScr;
    if (pumppressNextRocketDist < 0) {
      pumppressHasRocket = true;
      pumppressRocketX = 205;
      pumppressIsRocketGoing = false;
      pumppressNextRocketDist = 100;
    }
  }
  FOR_EACH(pumppressEnemies, i) {
    ASSIGN_ARRAY_ITEM(pumppressEnemies, i, PumppressEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float y = 24;
    if (!e->isPressed) {
      if (e->size > 0) {
        e->size -= difficulty * 0.02;
        if (e->size < 0) {
          e->size = 0;
        }
      } else {
        float dir;
        if (e->x < pumppressShipX) {
          dir = 1;
        } else {
          dir = -1;
        }
        e->x += e->vx * dir;
      }
      e->x -= pumppressScr;
    } else {
      if (!pumppressHasRocket) {
        e->isAlive = false;
        continue;
      }
      e->x = pumppressRocketX + e->pressedOfs.x;
      y += e->pressedOfs.y;
    }
    float s = ceil(e->size) * 2 + 2;
    color = RED;
    rect(e->x - s, y - s, s * 2, 2, &scratch);
    rect(e->x - s, y + s, s * 2, 2, &scratch);
    rect(e->x - s, y - s, 2, s * 2, &scratch);
    rect(e->x + s - 2, y - s, 2, s * 2, &scratch);
    color = YELLOW;
    Collision cc;
    rect(e->x - s, y, s * 2, 2, &cc);
    if (!e->isPressed) {
      if (cc.isColliding.character['b']) {
        pumppressHasShot = false;
        e->size = ceil(e->size) + 1;
        if (e->size > 3) {
          play(EXPLOSION);
          color = RED;
          particle(e->x, 25, 16, 1, 0, CGLP_PI * 2);
          e->isAlive = false;
          continue;
        } else {
          play(HIT);
        }
      }
      if (pumppressIsRocketGoing && cc.isColliding.character['c']) {
        e->isPressed = true;
        pumppressPressedCount++;
      }
      if (e->size == 0 && cc.isColliding.character['a']) {
        play(POWER_UP);
        gameOver();
      }
      if (e->x < -5) {
        e->isAlive = false;
        continue;
      }
    }
  }
}

void addGamePumppress() {
  addGame(pumppressTitle, pumppressDescription, pumppressCharacters,
          pumppressCharactersCount, &pumppressOptions, false, &pumppressUpdate);
}
