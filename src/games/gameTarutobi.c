#include "../cglp.h"

int* tarutobiTitle = "TARUTOBI";
int* tarutobiDescription = "[Slide] Move";

// Vircon32 port note: upstream's raw template-literal strings each carry a
// trailing indentation-only line (whatever whitespace preceded the closing
// backtick in the source) as one extra "row" beyond the 6 real pixel rows -
// harmless upstream (that renderer just reads however many rows are given),
// but this port's character grid is a fixed CHARACTER_WIDTH(6)-row array,
// so that trailing formatting artifact is dropped here, keeping only the 6
// rows that actually draw pixels (verified row-by-row against the source
// strings, including the shorter, non-full-width rows - those are
// intentional in the original art, not a transcription mistake).
int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tarutobiCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l",
        " l  l",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
        "  ",
    },
    {
        "  ll",
        " llll",
        "l    l",
        "l    l",
        " l  l",
        "  ll",
    },
};
int tarutobiCharactersCount = 3;

// Vircon32 port note: this game reads input.pos.x directly (the original
// control is "slide anywhere on screen to steer") - the engine only drives
// input.pos from the d-pad-emulated cursor when a game is registered with
// usesMouse = true (see main()'s loop in portVircon32.c); otherwise
// input.pos never moves and the slide control would be dead. So, unlike
// most of this porting batch, this game IS registered with usesMouse =
// true below, even though it wasn't called out in the original task
// description as one of the mouse-driven games - worth double-checking
// during integration.
Options tarutobiOptions = {120, 60, 0, false};

struct TarutobiObstacle {
  Vector pos;
  Vector vel;
  float t;
  bool isScoreAdded;
  bool isAlive;
};
#define TARUTOBI_MAX_OBSTACLE_COUNT 32
TarutobiObstacle[TARUTOBI_MAX_OBSTACLE_COUNT] tarutobiObstacles;
int tarutobiObstacleIndex;

Vector tarutobiP;
Vector tarutobiV;
bool tarutobiIsJumping;
int tarutobiAddedScore;

void tarutobiUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&tarutobiP, 30, 30);
    vectorSet(&tarutobiV, 0, 0);
    tarutobiIsJumping = true;
    tarutobiAddedScore = 1;
    INIT_UNALIVED_ARRAY_FAST(tarutobiObstacles);
    tarutobiObstacleIndex = 0;
  }
  if (rnd(0, 1) < 0.01 * difficulty) {
    ASSIGN_ARRAY_ITEM(tarutobiObstacles, tarutobiObstacleIndex, TarutobiObstacle, no);
    vectorSet(&no->pos, 123, 47);
    vectorSet(&no->vel, -rnd(0.4, 0.8 * difficulty), 0);
    no->t = 0;
    no->isScoreAdded = false;
    no->isAlive = true;
    tarutobiObstacleIndex = cgl_wrap(tarutobiObstacleIndex + 1, 0, TARUTOBI_MAX_OBSTACLE_COUNT);
  }
  FOR_EACH(tarutobiObstacles, oi) {
    ASSIGN_ARRAY_ITEM(tarutobiObstacles, oi, TarutobiObstacle, t);
    SKIP_IS_NOT_ALIVE(t);
    vectorAdd(&t->pos, t->vel.x, t->vel.y);
    // Vircon32 port note: upstream computes this rotation as a raw,
    // unbounded floor(t.t / 10) - the original (canvas-based) renderer
    // treats rotation as a continuous angle, where any integer multiple of
    // a quarter-turn wraps around to the same visual result regardless of
    // sign or magnitude. This engine's characterOptions.rotation only
    // special-cases the values 1/2/3 (anything else, including negative
    // values or values >= 4, renders unrotated - see setColorGrid() in
    // cglp.c), so the value is normalized into 0-3 here to reproduce the
    // same visual cycling that other already-ported games get for free
    // from upstream's own explicit "% 4" (compare gameMrider.c) -
    // tarutobi's original source just never wrote that "% 4" itself, so
    // without this normalization the obstacle would render unrotated for
    // almost its entire lifetime (t.t goes negative from frame 2 onward,
    // since obstacles move left).
    int rot = (int)floor(t->t / 10) % 4;
    if (rot < 0) {
      rot += 4;
    }
    characterOptions.rotation = rot;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    if (tarutobiIsJumping) {
      text("*", t->pos.x, t->pos.y, &scratch);
    } else {
      character("c", t->pos.x, t->pos.y, &scratch);
    }
    characterOptions.rotation = 0;
    t->t += t->vel.x;
    if (!t->isScoreAdded && t->pos.x < tarutobiP.x) {
      play(COIN);
      addScore(tarutobiAddedScore, t->pos.x, t->pos.y);
      tarutobiAddedScore++;
      t->isScoreAdded = true;
    }
    if (t->pos.x <= 0) {
      t->isAlive = false;
      continue;
    }
  }
  rect(0, 50, 120, 9, &scratch);
  if (tarutobiIsJumping) {
    tarutobiV.y += 0.1;
    if (tarutobiP.y > 47) {
      tarutobiIsJumping = false;
      tarutobiP.y = 47;
      tarutobiV.y = 0;
    }
  } else {
    vectorMul(&tarutobiV, 0.95);
    tarutobiAddedScore = 1;
  }
  if (tarutobiP.x < 0 || tarutobiP.x > 120) {
    tarutobiP.x = clamp(tarutobiP.x, 0, 120);
    tarutobiV.x = 0;
  }
  float steerFactor;
  if (tarutobiIsJumping) {
    steerFactor = 0.004;
  } else {
    steerFactor = 0.01;
  }
  tarutobiV.x += clamp(input.pos.x - tarutobiP.x, -7, 7) * steerFactor;
  vectorAdd(&tarutobiP, tarutobiV.x, tarutobiV.y);
  characterOptions.rotation = 0;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  int[2] playerChar;
  playerChar[0] = 'a' + ((int)(ticks / 30) % 2);
  playerChar[1] = 0;
  Collision c;
  character(playerChar, tarutobiP.x, tarutobiP.y, &c);
  if (c.isColliding.character['c']) {
    play(JUMP);
    tarutobiIsJumping = true;
    tarutobiP.y = 42;
    tarutobiV.y = -1.2 - fabs(tarutobiV.x);
  }
  if (c.isColliding.text['*']) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameTarutobi() {
  addGame(tarutobiTitle, tarutobiDescription, tarutobiCharacters,
          tarutobiCharactersCount, &tarutobiOptions, true, &tarutobiUpdate);
}
