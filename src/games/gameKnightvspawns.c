#include "../cglp.h"

int* knightvspawnsTitle = "KNIGHT VS. PAWNS";
int* knightvspawnsDescription = "[Tap]\nMove knight";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] knightvspawnsCharacters = {
    {
        "  ll  ",
        " llll ",
        "lllll ",
        "  lll ",
        "      ",
        " lllll",
    },
    {
        "  ll  ",
        " llll ",
        "llllll",
        "llllll",
        " llll ",
        "  ll  ",
    },
    {
        "  ll  ",
        "  ll  ",
        "      ",
        " llll ",
        "  ll  ",
        "llllll",
    },
};
int knightvspawnsCharactersCount = 3;

Options knightvspawnsOptions = {100, 100, 2, false};

#define KNIGHTVSPAWNS_BOARD_SIZE 8
#define KNIGHTVSPAWNS_SQUARE_SIZE 10

struct KnightvspawnsKnight {
  Vector pos;
};
KnightvspawnsKnight knightvspawnsKnight;

struct KnightvspawnsGhostKnight {
  Vector pos;
  int moveIndex;
};
KnightvspawnsGhostKnight knightvspawnsGhostKnight;

struct KnightvspawnsPawn {
  Vector pos;
  bool isAlive;
};
#define KNIGHTVSPAWNS_MAX_PAWN_COUNT 16
KnightvspawnsPawn[KNIGHTVSPAWNS_MAX_PAWN_COUNT] knightvspawnsPawns;
int knightvspawnsPawnIndex;

float knightvspawnsMoveInterval;
float knightvspawnsPawnInterval;
float knightvspawnsSpawnInterval;

Vector[8] knightvspawnsValidMoves = {
    {1, -2}, {2, -1}, {2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2},
};

bool knightvspawnsIsValidMove(float x, float y) {
  return x >= 0 && x < KNIGHTVSPAWNS_BOARD_SIZE && y >= 0 && y < KNIGHTVSPAWNS_BOARD_SIZE;
}

void knightvspawnsMoveGhostKnight() {
  Vector newPos;
  do {
    newPos.x = knightvspawnsKnight.pos.x + knightvspawnsValidMoves[knightvspawnsGhostKnight.moveIndex].x;
    newPos.y = knightvspawnsKnight.pos.y + knightvspawnsValidMoves[knightvspawnsGhostKnight.moveIndex].y;
    knightvspawnsGhostKnight.moveIndex = (knightvspawnsGhostKnight.moveIndex + 1) % 8;
  } while (!knightvspawnsIsValidMove(newPos.x, newPos.y));
  knightvspawnsGhostKnight.pos = newPos;
}

void knightvspawnsUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - the knight,
  // ghost knight and pawns all move on an 8x8 grid and are checked via
  // direct x/y equality (see "p->pos.x == knightvspawnsKnight.pos.x"
  // below), so the engine's own O(n^2) hitbox scan (see checkHitBox() in
  // cglp.c) is pure waste here. Restored automatically when the next
  // real game starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    vectorSet(&knightvspawnsKnight.pos, 3, 7);
    vectorSet(&knightvspawnsGhostKnight.pos, 5, 6);
    knightvspawnsGhostKnight.moveIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(knightvspawnsPawns);
    knightvspawnsPawnIndex = 0;
    knightvspawnsMoveInterval = 0;
    knightvspawnsPawnInterval = 0;
    knightvspawnsSpawnInterval = 0;
  }
  TIMES(KNIGHTVSPAWNS_BOARD_SIZE, i) {
    TIMES(KNIGHTVSPAWNS_BOARD_SIZE, j) {
      if ((i + j) % 2 == 0) {
        color = LIGHT_BLACK;
      } else {
        color = WHITE;
      }
      rect(i * KNIGHTVSPAWNS_SQUARE_SIZE + 10, j * KNIGHTVSPAWNS_SQUARE_SIZE + 10,
           KNIGHTVSPAWNS_SQUARE_SIZE, KNIGHTVSPAWNS_SQUARE_SIZE, &scratch);
    }
  }
  knightvspawnsMoveInterval--;
  if (knightvspawnsMoveInterval <= 0) {
    knightvspawnsMoveGhostKnight();
    knightvspawnsMoveInterval = 30 / difficulty;
  }
  color = BLUE;
  character("b", knightvspawnsGhostKnight.pos.x * KNIGHTVSPAWNS_SQUARE_SIZE + 15,
            knightvspawnsGhostKnight.pos.y * KNIGHTVSPAWNS_SQUARE_SIZE + 15, &scratch);
  if (input.isJustPressed) {
    play(LASER);
    knightvspawnsKnight.pos = knightvspawnsGhostKnight.pos;
    knightvspawnsMoveGhostKnight();
    addScore(KNIGHTVSPAWNS_BOARD_SIZE - knightvspawnsKnight.pos.y,
             knightvspawnsKnight.pos.x * KNIGHTVSPAWNS_SQUARE_SIZE + 15,
             knightvspawnsKnight.pos.y * KNIGHTVSPAWNS_SQUARE_SIZE + 15);
  }
  color = BLUE;
  character("a", knightvspawnsKnight.pos.x * KNIGHTVSPAWNS_SQUARE_SIZE + 15,
            knightvspawnsKnight.pos.y * KNIGHTVSPAWNS_SQUARE_SIZE + 15, &scratch);
  knightvspawnsPawnInterval--;
  if (knightvspawnsPawnInterval <= 0) {
    FOR_EACH(knightvspawnsPawns, i) {
      ASSIGN_ARRAY_ITEM(knightvspawnsPawns, i, KnightvspawnsPawn, p);
      SKIP_IS_NOT_ALIVE(p);
      p->pos.y++;
    }
    knightvspawnsPawnInterval = 60 / difficulty;
  }
  knightvspawnsSpawnInterval--;
  if (knightvspawnsSpawnInterval <= 0) {
    ASSIGN_ARRAY_ITEM(knightvspawnsPawns, knightvspawnsPawnIndex, KnightvspawnsPawn, np);
    vectorSet(&np->pos, floor(rnd(0, KNIGHTVSPAWNS_BOARD_SIZE)), 0);
    np->isAlive = true;
    knightvspawnsPawnIndex = cgl_wrap(knightvspawnsPawnIndex + 1, 0, KNIGHTVSPAWNS_MAX_PAWN_COUNT);
    knightvspawnsSpawnInterval = 120 / difficulty;
  }
  color = RED;
  FOR_EACH(knightvspawnsPawns, i) {
    ASSIGN_ARRAY_ITEM(knightvspawnsPawns, i, KnightvspawnsPawn, p);
    SKIP_IS_NOT_ALIVE(p);
    character("c", p->pos.x * KNIGHTVSPAWNS_SQUARE_SIZE + 15,
              p->pos.y * KNIGHTVSPAWNS_SQUARE_SIZE + 15, &scratch);
  }
  FOR_EACH(knightvspawnsPawns, i) {
    ASSIGN_ARRAY_ITEM(knightvspawnsPawns, i, KnightvspawnsPawn, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->pos.x == knightvspawnsKnight.pos.x && p->pos.y == knightvspawnsKnight.pos.y) {
      play(EXPLOSION);
      gameOver();
    }
  }
  FOR_EACH(knightvspawnsPawns, i) {
    ASSIGN_ARRAY_ITEM(knightvspawnsPawns, i, KnightvspawnsPawn, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->pos.y >= KNIGHTVSPAWNS_BOARD_SIZE) {
      p->isAlive = false;
    }
  }
}

void addGameKnightvspawns() {
  addGame(knightvspawnsTitle, knightvspawnsDescription, knightvspawnsCharacters,
          knightvspawnsCharactersCount, &knightvspawnsOptions, false,
          &knightvspawnsUpdate);
}
