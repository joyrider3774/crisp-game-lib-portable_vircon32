#include "../cglp.h"

char* timbertestTitle = "TIMBER TEST";
char* timbertestDescription = "[Tap] Cut a log";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] timbertestCharacters = {
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    },
};
int timbertestCharactersCount = 1;

Options timbertestOptions = {100, 100, 18, false};

struct TimbertestTimber {
  float x;
  float width;
};
TimbertestTimber timbertestTimber;
int timbertestCutCount;
int timbertestCutIndex;
struct Piece {
  Vector size;
  Vector pos;
  Vector targetPos;
  bool isAlive;
};
#define TIMBER_TEST_MAX_PIECE_COUNT 64
Piece[TIMBER_TEST_MAX_PIECE_COUNT] timbertestPieces;
int timbertestPieceIndex;
struct TimbertestSaw {
  float x;
  float vx;
};
TimbertestSaw timbertestSaw;
int timbertestScoreCountTicks;
int timbertestScoreCountIndex;
int timbertestTurnScore;
int timbertestTurnIndex;
bool timbertestIsShowingIndicator;

void timbertestNextTimber();

void timbertestUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - piece
  // position matching uses distanceTo() instead (see the dist check
  // below), so the engine's own O(n^2) hitbox scan (see checkHitBox() in
  // cglp.c) is pure waste here. Restored automatically when the next
  // real game starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    timbertestTurnIndex = 0;
    timbertestPieceIndex = 0;
    timbertestNextTimber();
  }
  color = RED;
  rect(timbertestTimber.x, 20, timbertestTimber.width, 10, &scratch);
  timbertestSaw.x += timbertestSaw.vx;
  color = BLACK;
  float sawHeight;
  if (input.isJustPressed) {
    sawHeight = 30;
  } else {
    sawHeight = 7;
  }
  rect(timbertestSaw.x - 1, 10, 3, sawHeight, &scratch);
  int[4] cutText;
  cutText[0] = '1';
  cutText[1] = '/';
  cutText[2] = '1';
  cutText[3] = 0;
  cutText[2] = '0' + timbertestCutCount;
  text(cutText, 5, 35, &scratch);
  if (timbertestScoreCountTicks == 0 && timbertestSaw.x >= timbertestTimber.x + timbertestTimber.width) {
    ASSIGN_ARRAY_ITEM(timbertestPieces, timbertestPieceIndex, Piece, p);
    vectorSet(&p->size, timbertestTimber.width, 10);
    vectorSet(&p->pos, timbertestTimber.x + p->size.x / 2, 20 + 10 / 2);
    vectorSet(&p->targetPos, 50, 40 + timbertestCutIndex * 15);
    p->isAlive = true;
    timbertestPieceIndex = cgl_wrap(timbertestPieceIndex + 1, 0, TIMBER_TEST_MAX_PIECE_COUNT);
    timbertestTimber.width = 0;
    timbertestCutIndex++;
    timbertestScoreCountTicks = 1;
  }
  if (timbertestScoreCountTicks == 0 && input.isJustPressed) {
    timbertestIsShowingIndicator = false;
    float cw = timbertestSaw.x - timbertestTimber.x;
    if (cw > 0) {
      play(SELECT);
      ASSIGN_ARRAY_ITEM(timbertestPieces, timbertestPieceIndex, Piece, p);
      vectorSet(&p->size, cw, 10);
      vectorSet(&p->pos, timbertestTimber.x + p->size.x / 2, 20 + 10 / 2);
      vectorSet(&p->targetPos, 50, 40 + timbertestCutIndex * 15);
      p->isAlive = true;
      timbertestPieceIndex = cgl_wrap(timbertestPieceIndex + 1, 0, TIMBER_TEST_MAX_PIECE_COUNT);
      timbertestTimber.x = timbertestSaw.x;
      timbertestTimber.width -= cw;
      timbertestCutIndex++;
    }
  }
  FOR_EACH(timbertestPieces, i) {
    ASSIGN_ARRAY_ITEM(timbertestPieces, i, Piece, p);
    SKIP_IS_NOT_ALIVE(p);
    if (distanceTo(&p->pos, p->targetPos.x, p->targetPos.y) < 1) {
      vectorSet(&p->pos, p->targetPos.x, p->targetPos.y);
    } else {
      Vector diff;
      vectorSet(&diff, p->targetPos.x, p->targetPos.y);
      vectorAdd(&diff, -p->pos.x, -p->pos.y);
      vectorMul(&diff, 0.1);
      vectorAdd(&p->pos, diff.x, diff.y);
    }
    color = RED;
    box(p->pos.x, p->pos.y, p->size.x, p->size.y, &scratch);
  }
  if (timbertestScoreCountTicks > 0) {
    COUNT_IS_ALIVE(timbertestPieces, pc);
    timbertestScoreCountTicks += difficulty * (pc + 1) * 0.5;
    int c = clamp(floor(timbertestScoreCountTicks / 20.0), 0, fmax(timbertestCutCount, pc));
    TIMES(c, i) {
      color = BLACK;
      float y = 40 + i * 15;
      if (i == 0) {
        if (timbertestTurnScore < 0) {
          color = RED;
        }
        text(intToChar(timbertestTurnScore), 80, y, &scratch);
      } else {
        float pw1;
        if (i - 1 < pc) {
          pw1 = timbertestPieces[i - 1].size.x;
        } else {
          pw1 = 0;
        }
        float pw2;
        if (i < pc) {
          pw2 = timbertestPieces[i].size.x;
        } else {
          pw2 = 0;
        }
        int p;
        if ((pw1 == 0 && pw2 == 0) || i > timbertestCutCount) {
          p = 100;
        } else {
          p = floor(fabs(pw1 - pw2) / (pw1 + pw2) * 300);
        }
        text("-", 74, y, &scratch);
        text(intToChar(p), 80, y, &scratch);
        if (i == timbertestScoreCountIndex) {
          play(HIT);
          timbertestTurnScore -= p;
          timbertestScoreCountIndex++;
        }
      }
    }
  }
  if (timbertestSaw.x > 160) {
    if (timbertestTurnScore < 0) {
      play(EXPLOSION);
      gameOver();
    } else {
      score += timbertestTurnScore;
      timbertestNextTimber();
    }
  }
  color = BLACK;
  if (timbertestTurnIndex <= 3 && timbertestIsShowingIndicator) {
    TIMES(timbertestCutCount - 1, i) {
      int[2] indicatorText = "^";
      text(indicatorText, timbertestTimber.x + (timbertestTimber.width / timbertestCutCount) * (i + 1), 35, &scratch);
    }
    text("Cut here!", 32, 38, &scratch);
  }
}

void timbertestNextTimber() {
  play(POWER_UP);
  float tw = rnd(40, 80);
  timbertestTimber.x = (100 - tw) / 2 + rnd(0, (100 - tw) / 3);
  timbertestTimber.width = tw;
  timbertestCutCount = rndi(2, 5);
  timbertestTurnScore = (timbertestCutCount - 1) * 100;
  timbertestCutIndex = 0;
  INIT_UNALIVED_ARRAY_FAST(timbertestPieces);
  timbertestSaw.x = -30;
  timbertestSaw.vx = (difficulty / sqrt(timbertestCutCount)) * 2;
  timbertestScoreCountTicks = 0;
  timbertestScoreCountIndex = 1;
  timbertestTurnIndex++;
  timbertestIsShowingIndicator = true;
}

void addGameTimberTest() {
  addGame(timbertestTitle, timbertestDescription, timbertestCharacters,
          timbertestCharactersCount, &timbertestOptions, false, &timbertestUpdate);
}
