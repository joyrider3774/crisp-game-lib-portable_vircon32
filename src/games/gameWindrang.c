#include "../cglp.h"

int* windrangTitle = "WIND RANG";
int* windrangDescription = "[Tap]\n Toggle wind";

// Upstream's "b"/"e" glyphs only specify 5 rows; the 6th is added here as explicitly blank.
int[5][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] windrangCharacters = {
    {
        "llllll",
        "llllll",
        "llllll",
        "llllll",
        " l  l",
        " l  l",
    },
    {
        "llllll",
        "llllll",
        "llllll",
        "llllll",
        "ll  ll",
        "      ",
    },
    {
        "    ll",
        "    ll",
        "    ll",
        "    ll",
        "llllll",
        "llllll",
    },
    {
        "  lll",
        "ll l l",
        " llll",
        "  ll",
        " l  l",
        " l  l",
    },
    {
        "  lll",
        "ll l l",
        " llll",
        " l  l",
        "ll  ll",
        "      ",
    },
};
int windrangCharactersCount = 5;

Options windrangOptions = {100, 100, 2, false};

#define WINDRANG_WIND_FORCE 0.08
#define WINDRANG_WIND_LINE_SPEED 0.5
#define WINDRANG_WIND_LINE_ACCEL 0.05
#define WINDRANG_WIND_LINE_WIDTH 15
#define WINDRANG_WIND_LINE_HEIGHT 1
#define WINDRANG_BOOMERANG_SPEED 1.5
#define WINDRANG_BOOMERANG_INTERVAL 18
#define WINDRANG_ENEMY_SPAWN_INTERVAL 77
#define WINDRANG_ENEMY_SPEED 0.25
#define WINDRANG_CATCH_RADIUS 8
#define WINDRANG_PLAYER_Y 98

struct WindrangBoomerang {
  Vector pos;
  Vector vel;
  bool stateIsReturn;
  float angle;
  bool isAlive;
};
#define WINDRANG_MAX_BOOMERANG_COUNT 32
WindrangBoomerang[WINDRANG_MAX_BOOMERANG_COUNT] windrangBoomerangs;
int windrangBoomerangIndex;

struct WindrangEnemy {
  Vector pos;
  Vector vel;
  bool isAlive;
};
// Sized well above the ~15-25 typical estimate since speed and spawn rate don't fully cancel out.
#define WINDRANG_MAX_ENEMY_COUNT 128
WindrangEnemy[WINDRANG_MAX_ENEMY_COUNT] windrangEnemies;
int windrangEnemyIndex;

struct WindrangWindLine {
  Vector pos;
  Vector vel;
};
#define WINDRANG_WIND_LINE_COUNT 5
WindrangWindLine[WINDRANG_WIND_LINE_COUNT] windrangWindLines;

float windrangNextBoomerangTicks;
float windrangNextEnemyTicks;
bool windrangIsWindLeft;
int windrangMultiplier;
float windrangPlayerX;

void windrangUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(windrangBoomerangs);
    windrangBoomerangIndex = 0;
    windrangNextBoomerangTicks = WINDRANG_BOOMERANG_INTERVAL;
    INIT_UNALIVED_ARRAY_FAST(windrangEnemies);
    windrangEnemyIndex = 0;
    windrangNextEnemyTicks = WINDRANG_ENEMY_SPAWN_INTERVAL;
    windrangIsWindLeft = false;
    windrangMultiplier = 1;
    windrangPlayerX = 50;
    TIMES(WINDRANG_WIND_LINE_COUNT, wli) {
      WindrangWindLine* w = &windrangWindLines[wli];
      vectorSet(&w->pos, rnd(0, 100), 20 + wli * 15);
      vectorSet(&w->vel, WINDRANG_WIND_LINE_SPEED, rnd(-0.2, 0.2));
    }
  }

  if (input.isJustPressed) {
    windrangIsWindLeft = !windrangIsWindLeft;
    play(SELECT);
  }

  color = LIGHT_BLUE;
  TIMES(WINDRANG_WIND_LINE_COUNT, wli2) {
    WindrangWindLine* w = &windrangWindLines[wli2];
    if (windrangIsWindLeft) {
      w->vel.x -= WINDRANG_WIND_LINE_ACCEL;
    } else {
      w->vel.x += WINDRANG_WIND_LINE_ACCEL;
    }
    w->vel.y += rnd(-WINDRANG_WIND_LINE_ACCEL, WINDRANG_WIND_LINE_ACCEL) * 0.2;
    w->vel.x *= 0.98;
    w->vel.y *= 0.98;
    vectorAdd(&w->pos, w->vel.x, w->vel.y);
    if (w->pos.x < -WINDRANG_WIND_LINE_WIDTH) {
      w->pos.x = 100 + WINDRANG_WIND_LINE_WIDTH;
    } else if (w->pos.x > 100 + WINDRANG_WIND_LINE_WIDTH) {
      w->pos.x = -WINDRANG_WIND_LINE_WIDTH;
    }
    if (w->pos.y < 0) {
      w->pos.y = 100;
    } else if (w->pos.y > 100) {
      w->pos.y = 0;
    }
    box(w->pos.x, w->pos.y, WINDRANG_WIND_LINE_WIDTH, WINDRANG_WIND_LINE_HEIGHT, &scratch);
  }

  color = BLACK;
  int[2] playerChar;
  playerChar[0] = 'a' + ((int)(ticks / 15) % 2);
  playerChar[1] = 0;
  character(playerChar, windrangPlayerX, WINDRANG_PLAYER_Y, &scratch);

  windrangNextBoomerangTicks--;
  if (windrangNextBoomerangTicks <= 0) {
    ASSIGN_ARRAY_ITEM(windrangBoomerangs, windrangBoomerangIndex, WindrangBoomerang, nb);
    vectorSet(&nb->pos, windrangPlayerX, WINDRANG_PLAYER_Y);
    vectorSet(&nb->vel, 0, -WINDRANG_BOOMERANG_SPEED);
    nb->stateIsReturn = false;
    nb->angle = 0;
    nb->isAlive = true;
    windrangBoomerangIndex = cgl_wrap(windrangBoomerangIndex + 1, 0, WINDRANG_MAX_BOOMERANG_COUNT);
    windrangNextBoomerangTicks = WINDRANG_BOOMERANG_INTERVAL;
    play(CLICK);
  }

  FOR_EACH(windrangBoomerangs, bi) {
    ASSIGN_ARRAY_ITEM(windrangBoomerangs, bi, WindrangBoomerang, b);
    SKIP_IS_NOT_ALIVE(b);
    float windForce;
    if (windrangIsWindLeft) {
      windForce = -WINDRANG_WIND_FORCE;
    } else {
      windForce = WINDRANG_WIND_FORCE;
    }
    b->vel.x += windForce;
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    b->angle += 0.2;

    if (!b->stateIsReturn && b->pos.y < 10) {
      b->stateIsReturn = true;
      b->vel.y = WINDRANG_BOOMERANG_SPEED;
    }

    color = BLUE;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = (int)floor(fmod(b->angle / CGLP_PI_2, 4));
    int[2] boomChar;
    boomChar[0] = 'c';
    boomChar[1] = 0;
    character(boomChar, b->pos.x, b->pos.y, &scratch);

    if (b->stateIsReturn && b->pos.y > WINDRANG_PLAYER_Y - 5) {
      float dist = fabs(b->pos.x - windrangPlayerX);
      if (dist < WINDRANG_CATCH_RADIUS) {
        play(POWER_UP);
        windrangMultiplier++;
        addScore(10 * windrangMultiplier, b->pos.x, b->pos.y);
        particle(b->pos.x, b->pos.y, 10, 2, 0, CGLP_PI * 2);
        b->isAlive = false;
        continue;
      } else if (b->pos.y > 105) {
        b->isAlive = false;
        continue;
      }
    }

    if (b->pos.x < -5 || b->pos.x > 105 || b->pos.y > 105) {
      b->isAlive = false;
      continue;
    }
  }

  windrangNextEnemyTicks--;
  if (windrangNextEnemyTicks <= 0) {
    ASSIGN_ARRAY_ITEM(windrangEnemies, windrangEnemyIndex, WindrangEnemy, ne);
    vectorSet(&ne->pos, rnd(10, 90), 0);
    vectorSet(&ne->vel, 0, WINDRANG_ENEMY_SPEED * rnd(1, difficulty));
    ne->isAlive = true;
    windrangEnemyIndex = cgl_wrap(windrangEnemyIndex + 1, 0, WINDRANG_MAX_ENEMY_COUNT);
    windrangNextEnemyTicks = (WINDRANG_ENEMY_SPAWN_INTERVAL * rnd(0.5, 2)) / difficulty;
  }

  FOR_EACH(windrangEnemies, ei) {
    ASSIGN_ARRAY_ITEM(windrangEnemies, ei, WindrangEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float windForce2;
    if (windrangIsWindLeft) {
      windForce2 = -WINDRANG_WIND_FORCE * 0.25;
    } else {
      windForce2 = WINDRANG_WIND_FORCE * 0.25;
    }
    e->vel.x += windForce2;
    vectorAdd(&e->pos, e->vel.x, e->vel.y);

    if (e->pos.x < 0) {
      e->pos.x = 100;
    } else if (e->pos.x > 100) {
      e->pos.x = 0;
    }

    if (e->pos.y > 100) {
      play(EXPLOSION);
      gameOver();
    }

    color = RED;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    int[2] enemyChar;
    enemyChar[0] = 'd' + ((int)(ticks / 15) % 2);
    enemyChar[1] = 0;
    character(enemyChar, e->pos.x, e->pos.y, &scratch);

    if (scratch.isColliding.character['c']) {
      play(COIN);
      particle(e->pos.x, e->pos.y, 10, 3, 0, CGLP_PI * 2);
      addScore(1 * windrangMultiplier, e->pos.x, e->pos.y);
      e->isAlive = false;
      continue;
    }

    if (e->pos.y > WINDRANG_PLAYER_Y - 5 && fabs(e->pos.x - windrangPlayerX) < 5) {
      play(EXPLOSION);
      gameOver();
    }
  }

  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(windrangMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameWindrang() {
  addGame(windrangTitle, windrangDescription, windrangCharacters,
          windrangCharactersCount, &windrangOptions, false, &windrangUpdate);
}
