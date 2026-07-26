#include "../cglp.h"

int* screenTitle = "SCREEN";
int* screenDescription = "[Tap]\n Fire";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] screenCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
};
int screenCharactersCount = 4;

Options screenOptions = {100, 100, 500, false};

struct ScreenEnemy {
  Vector pos;
  float vy;
  bool isAlive;
};
#define SCREEN_MAX_ENEMY_COUNT 32
ScreenEnemy[SCREEN_MAX_ENEMY_COUNT] screenEnemies;
int screenEnemyIndex;
float screenNextEnemyTicks;

struct ScreenPlayer {
  Vector pos;
  float tx;
  float angle;
  int shotCount;
};
ScreenPlayer screenPlayer;

struct ScreenShot {
  Vector pos;
  float angle;
  float speed;
  bool isAlive;
};
#define SCREEN_MAX_SHOT_COUNT 16
ScreenShot[SCREEN_MAX_SHOT_COUNT] screenShots;
int screenShotIndex;
int screenMultiplier;

void screenUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(screenEnemies);
    screenEnemyIndex = 0;
    screenNextEnemyTicks = 0;
    vectorSet(&screenPlayer.pos, 55, 95);
    screenPlayer.tx = 55;
    screenPlayer.angle = 0;
    screenPlayer.shotCount = 3;
    INIT_UNALIVED_ARRAY_FAST(screenShots);
    screenShotIndex = 0;
    screenMultiplier = 1;
  }
  screenPlayer.angle += 0.01 * difficulty;
  if (screenPlayer.angle >= CGLP_PI / 2) {
    screenPlayer.angle -= CGLP_PI / 2;
  }
  float sa;
  if (screenPlayer.pos.x > 50) {
    sa = -screenPlayer.angle;
  } else {
    sa = CGLP_PI + screenPlayer.angle;
  }
  if (screenPlayer.shotCount > 0 && fabs(screenPlayer.pos.x - screenPlayer.tx) < 1 &&
      input.isJustPressed) {
    play(COIN);
    ASSIGN_ARRAY_ITEM(screenShots, screenShotIndex, ScreenShot, ns);
    ns->pos = screenPlayer.pos;
    ns->angle = sa;
    ns->speed = difficulty;
    ns->isAlive = true;
    screenShotIndex = cgl_wrap(screenShotIndex + 1, 0, SCREEN_MAX_SHOT_COUNT);
    screenPlayer.shotCount--;
  }
  if (screenPlayer.shotCount <= 0) {
    if (screenPlayer.tx == 55) {
      screenPlayer.tx = 45;
    } else {
      screenPlayer.tx = 55;
    }
  }
  float pp = screenPlayer.pos.x;
  screenPlayer.pos.x += (screenPlayer.tx - screenPlayer.pos.x) * 0.2;
  if (fabs(screenPlayer.pos.x - screenPlayer.tx) < 1) {
    screenPlayer.pos.x = screenPlayer.tx;
  }
  if ((pp - 50) * (screenPlayer.pos.x - 50) <= 0) {
    play(SELECT);
    screenPlayer.shotCount = 3;
  }
  color = LIGHT_BLACK;
  thickness = 2;
  barCenterPosRatio = 0;
  bar(screenPlayer.pos.x, screenPlayer.pos.y, 99, sa, &scratch);
  color = BLACK;
  int[2] pc;
  pc[0] = 'a' + (int)floor(ticks / 20) % 2;
  pc[1] = 0;
  characterOptions.isMirrorX = screenPlayer.tx < 50;
  character(pc, screenPlayer.pos.x, screenPlayer.pos.y, &scratch);
  characterOptions.isMirrorX = false;
  color = BLUE;
  float bx;
  if (screenPlayer.pos.x < 50) {
    bx = 39;
  } else {
    bx = 61;
  }
  TIMES(screenPlayer.shotCount, i) {
    box(bx, 97, 4, 2, &scratch);
    if (screenPlayer.pos.x < 50) {
      bx -= 5;
    } else {
      bx += 5;
    }
  }
  FOR_EACH(screenShots, i) {
    ASSIGN_ARRAY_ITEM(screenShots, i, ScreenShot, s);
    SKIP_IS_NOT_ALIVE(s);
    addWithAngle(&s->pos, s->angle, s->speed);
    thickness = 3;
    barCenterPosRatio = 0.5;
    bar(s->pos.x, s->pos.y, 2, s->angle, &scratch);
    bool inRect = s->pos.x >= -5 && s->pos.x < 105 && s->pos.y >= -5 && s->pos.y < 105;
    if (!inRect) {
      if (screenMultiplier > 1) {
        screenMultiplier--;
      }
      s->isAlive = false;
      continue;
    }
  }
  screenNextEnemyTicks--;
  if (screenNextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(screenEnemies, screenEnemyIndex, ScreenEnemy, ne);
    vectorSet(&ne->pos, 50 + rnd(15, 40) * RNDPM(), -3);
    ne->vy = rnd(1, difficulty) * 0.1;
    ne->isAlive = true;
    screenEnemyIndex = cgl_wrap(screenEnemyIndex + 1, 0, SCREEN_MAX_ENEMY_COUNT);
    screenNextEnemyTicks = rnd(60, 90) / difficulty;
  }
  color = BLACK;
  rect(49, 0, 2, 100, &scratch);
  color = RED;
  bool isEnd = false;
  FOR_EACH(screenEnemies, i) {
    ASSIGN_ARRAY_ITEM(screenEnemies, i, ScreenEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.y += e->vy;
    int[2] ec;
    ec[0] = 'c' + (int)floor(fabs(ticks) / 20) % 2;
    ec[1] = 0;
    characterOptions.isMirrorX = e->pos.x >= 50;
    Collision ecoll;
    character(ec, e->pos.x, e->pos.y, &ecoll);
    characterOptions.isMirrorX = false;
    if (ecoll.isColliding.rect[BLUE]) {
      play(POWER_UP);
      addScore(screenMultiplier, e->pos.x, e->pos.y);
      screenMultiplier++;
      particle(e->pos.x, e->pos.y, 16, 1, 0, CGLP_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (e->pos.y > 99) {
      play(EXPLOSION);
      text("X", e->pos.x, 96, &scratch);
      gameOver();
      isEnd = true;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(screenShots, i) {
    ASSIGN_ARRAY_ITEM(screenShots, i, ScreenShot, s);
    SKIP_IS_NOT_ALIVE(s);
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision sc;
    bar(s->pos.x, s->pos.y, 2, s->angle, &sc);
    if (sc.isColliding.character['c'] || sc.isColliding.character['d']) {
      s->isAlive = false;
    }
  }
  if (!isEnd) {
    color = LIGHT_BLACK;
    float rx;
    if (screenPlayer.pos.x < 50) {
      rx = 51;
    } else {
      rx = 0;
    }
    rect(rx, 0, 49, 100, &scratch);
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "+");
  strcat(multText, intToChar(screenMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameScreen() {
  addGame(screenTitle, screenDescription, screenCharacters,
          screenCharactersCount, &screenOptions, false, &screenUpdate);
}
