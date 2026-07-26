#include "../cglp.h"

int* gravitydriftTitle = "GRAVITY DRIFT";
int* gravitydriftDescription = "[Hold] Thrust up";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] gravitydriftCharacters = {{
    " llll ",
    "l ll l",
    "l ll l",
    "llllll",
    " l  l ",
    "ll  ll",
}};
int gravitydriftCharactersCount = 1;

Options gravitydriftOptions = {100, 100, 2, false};

struct GravitydriftPlayer {
  float x;
  float y;
  float vy;
};
GravitydriftPlayer gravitydriftPlayer;

struct GravitydriftAsteroid {
  float x;
  float y;
  float size;
  bool isAlive;
};
// Concurrent count stays ~self-similar (both spawn interval and lifetime
// scale as 1/spd), roughly 4-5 in practice - generous headroom applied.
#define GRAVITYDRIFT_MAX_ASTEROID_COUNT 32
GravitydriftAsteroid[GRAVITYDRIFT_MAX_ASTEROID_COUNT] gravitydriftAsteroids;
int gravitydriftAsteroidIndex;

struct GravitydriftStar {
  float x;
  float y;
  bool isAlive;
};
#define GRAVITYDRIFT_MAX_STAR_COUNT 8
GravitydriftStar[GRAVITYDRIFT_MAX_STAR_COUNT] gravitydriftStars;
int gravitydriftStarIndex;

bool gravitydriftReversed;
int gravitydriftMultiplier;

void gravitydriftUpdate() {
  Collision scratch;
  if (!ticks) {
    gravitydriftPlayer.x = 20;
    gravitydriftPlayer.y = 50;
    gravitydriftPlayer.vy = 0;
    INIT_UNALIVED_ARRAY_FAST(gravitydriftAsteroids);
    gravitydriftAsteroidIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(gravitydriftStars);
    gravitydriftStarIndex = 0;
    gravitydriftReversed = false;
    gravitydriftMultiplier = 1;
  }

  float spd = 0.5 + difficulty * 0.5;

  float targetX;
  if (gravitydriftReversed) {
    targetX = 80;
  } else {
    targetX = 20;
  }
  gravitydriftPlayer.x += (targetX - gravitydriftPlayer.x) * 0.05;

  if (input.isPressed) {
    gravitydriftPlayer.vy -= 0.25;
    color = CYAN;
    particle(gravitydriftPlayer.x, gravitydriftPlayer.y + 2, 1, 1, CGLP_PI / 2, 0.5);
  }
  if (input.isJustPressed) {
    play(CLICK);
  }
  gravitydriftPlayer.vy += 0.12;
  gravitydriftPlayer.vy *= 0.95;
  gravitydriftPlayer.y += gravitydriftPlayer.vy;

  if (gravitydriftPlayer.y < 0 || gravitydriftPlayer.y > 100) {
    play(EXPLOSION);
    gameOver();
  }

  if (ticks % (int)ceil(25 / spd) == 0) {
    float targetY = gravitydriftPlayer.y + rnd(-9, 9);
    targetY = clamp(targetY, 0, 100);
    ASSIGN_ARRAY_ITEM(gravitydriftAsteroids, gravitydriftAsteroidIndex, GravitydriftAsteroid, na);
    if (gravitydriftReversed) {
      na->x = -5;
    } else {
      na->x = 105;
    }
    na->y = targetY;
    na->size = rnd(5, 10);
    na->isAlive = true;
    gravitydriftAsteroidIndex = cgl_wrap(gravitydriftAsteroidIndex + 1, 0, GRAVITYDRIFT_MAX_ASTEROID_COUNT);
  }

  if (ticks % 120 == 0) {
    ASSIGN_ARRAY_ITEM(gravitydriftStars, gravitydriftStarIndex, GravitydriftStar, ns);
    if (gravitydriftReversed) {
      ns->x = -5;
    } else {
      ns->x = 105;
    }
    ns->y = rnd(20, 80);
    ns->isAlive = true;
    gravitydriftStarIndex = cgl_wrap(gravitydriftStarIndex + 1, 0, GRAVITYDRIFT_MAX_STAR_COUNT);
  }

  color = CYAN;
  character("a", gravitydriftPlayer.x, gravitydriftPlayer.y, &scratch);

  color = YELLOW;
  FOR_EACH(gravitydriftStars, si) {
    ASSIGN_ARRAY_ITEM(gravitydriftStars, si, GravitydriftStar, s);
    SKIP_IS_NOT_ALIVE(s);
    if (gravitydriftReversed) {
      s->x += spd * 0.5;
    } else {
      s->x -= spd * 0.5;
    }
    box(s->x, s->y, 4, 4, &scratch);
    if (scratch.isColliding.character['a']) {
      play(POWER_UP);
      gravitydriftReversed = !gravitydriftReversed;
      gravitydriftMultiplier++;
      s->isAlive = false;
      continue;
    }
    if (!(s->x > -10 && s->x < 110)) {
      s->isAlive = false;
    }
  }

  if (gravitydriftReversed) {
    color = BLUE;
  } else {
    color = RED;
  }
  FOR_EACH(gravitydriftAsteroids, ai) {
    ASSIGN_ARRAY_ITEM(gravitydriftAsteroids, ai, GravitydriftAsteroid, a);
    SKIP_IS_NOT_ALIVE(a);
    if (gravitydriftReversed) {
      a->x += spd;
    } else {
      a->x -= spd;
    }
    box(a->x, a->y, a->size, a->size, &scratch);
    if (scratch.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
    if (!(a->x > -10 && a->x < 110)) {
      a->isAlive = false;
    }
  }

  addScore(gravitydriftMultiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);

  color = BLACK;
  int[16] gravitydriftMultText;
  strcpy(gravitydriftMultText, "x");
  strcat(gravitydriftMultText, intToChar(gravitydriftMultiplier));
  // Vircon32 port note: JS drew this with an unsupported isSmallText option;
  // drawn at normal text size instead (see gameSlimestretcher.c precedent).
  text(gravitydriftMultText, 3, 9, &scratch);
}

void addGameGravitydrift() {
  addGame(gravitydriftTitle, gravitydriftDescription, gravitydriftCharacters,
          gravitydriftCharactersCount, &gravitydriftOptions, false, &gravitydriftUpdate);
}
