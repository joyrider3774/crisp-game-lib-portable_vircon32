#include "../cglp.h"

int* molenTitle = "MOLE N";
int* molenDescription = "[Tap]\n Whack a number";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] molenCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int molenCharactersCount = 0;

Options molenOptions = {100, 100, 3, false};

#define MOLEN_NUMBER_COUNT 9
int[MOLEN_NUMBER_COUNT][MOLEN_NUMBER_COUNT] molenNumbers;
int[MOLEN_NUMBER_COUNT][MOLEN_NUMBER_COUNT] molenCurrentNumbers;
float molenNumbersY;
float molenPenaltyY;
int molenTargetNumber;
int molenNextTargetNumberCount;

struct MolenMole {
  Vector pos;
  int value;
  float removeTicks;
  float score;
  bool isAlive;
};
#define MOLEN_MAX_MOLE_COUNT 32
MolenMole[MOLEN_MAX_MOLE_COUNT] molenMoles;
int molenMoleIndex;
float molenNextMoleDist;
float molenMoleMoveTicks;
Vector molenHitPos;
float molenHitTicks;

Vector[4] molenOffsets;

int molenGetRandomNumber() {
  int hi = (int)clamp((sqrt(difficulty) - 1) * 5 + 2.5, 1, 10);
  return rndi(1, hi);
}

bool molenExistsMole(Vector* p) {
  if (!(p->x >= 0 && p->x < MOLEN_NUMBER_COUNT && p->y >= 0 && p->y < MOLEN_NUMBER_COUNT)) {
    return true;
  }
  bool exists = false;
  FOR_EACH(molenMoles, i) {
    ASSIGN_ARRAY_ITEM(molenMoles, i, MolenMole, m);
    SKIP_IS_NOT_ALIVE(m);
    if (m->pos.x == p->x && m->pos.y == p->y) {
      exists = true;
    }
  }
  return exists;
}

void molenUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - the tapped
  // cell is computed directly from input.pos via grid math, and mole
  // occupancy/movement is direct molenExistsMole() grid lookup, so the
  // engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure
  // waste here. Restored automatically when the next real game starts,
  // via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    TIMES(MOLEN_NUMBER_COUNT, x) {
      TIMES(MOLEN_NUMBER_COUNT, y) {
        molenNumbers[x][y] = rndi(0, 10);
        molenCurrentNumbers[x][y] = 0;
      }
    }
    molenNumbersY = 0;
    molenPenaltyY = 0;
    molenTargetNumber = 1;
    molenNextTargetNumberCount = 0;
    INIT_UNALIVED_ARRAY_FAST(molenMoles);
    molenMoleIndex = 0;
    molenNextMoleDist = 0;
    molenMoleMoveTicks = 0;
    vectorSet(&molenHitPos, 0, 0);
    molenHitTicks = 0;
    vectorSet(&molenOffsets[0], 1, 0);
    vectorSet(&molenOffsets[1], 0, 1);
    vectorSet(&molenOffsets[2], -1, 0);
    vectorSet(&molenOffsets[3], 0, -1);
  }
  float scr = sqrt(difficulty) * 0.02 + molenPenaltyY;
  molenPenaltyY *= 0.9;
  molenNumbersY += scr;
  if (molenNumbersY > 0) {
    TIMES(MOLEN_NUMBER_COUNT, y) {
      TIMES(MOLEN_NUMBER_COUNT, x) {
        if (y < MOLEN_NUMBER_COUNT - 1) {
          molenNumbers[x][MOLEN_NUMBER_COUNT - y - 1] = molenNumbers[x][MOLEN_NUMBER_COUNT - y - 2];
        } else {
          molenNumbers[x][0] = rndi(0, 10);
        }
      }
    }
    FOR_EACH(molenMoles, i) {
      ASSIGN_ARRAY_ITEM(molenMoles, i, MolenMole, m);
      SKIP_IS_NOT_ALIVE(m);
      m->pos.y++;
    }
    molenHitPos.y++;
    molenNumbersY -= 11;
  }
  if (input.isJustPressed) {
    play(LASER);
    Vector p;
    vectorSet(&p, floor((input.pos.x - 1) / 11), floor((input.pos.y - molenNumbersY) / 11));
    molenHitPos = p;
    molenHitTicks = 40;
    bool pInRect = p.x >= 0 && p.x < MOLEN_NUMBER_COUNT && p.y >= 0 && p.y < MOLEN_NUMBER_COUNT;
    if (pInRect) {
      bool isTargetRemoved = false;
      FOR_EACH(molenMoles, i) {
        ASSIGN_ARRAY_ITEM(molenMoles, i, MolenMole, m);
        SKIP_IS_NOT_ALIVE(m);
        if (m->removeTicks == 0 && m->pos.x == p.x && m->pos.y == p.y) {
          if (m->value == molenTargetNumber) {
            isTargetRemoved = true;
          } else {
            m->removeTicks = 60;
            m->score = -m->value;
            molenPenaltyY += 1;
          }
        }
      }
      if (isTargetRemoved) {
        molenTargetNumber = molenGetRandomNumber();
        FOR_EACH(molenMoles, i) {
          ASSIGN_ARRAY_ITEM(molenMoles, i, MolenMole, m);
          SKIP_IS_NOT_ALIVE(m);
          m->removeTicks = 60;
          m->score = m->value;
        }
      }
      molenPenaltyY += 1;
    }
  }
  COUNT_IS_ALIVE(molenMoles, aliveMoleCount);
  if (aliveMoleCount == 0) {
    molenNextMoleDist = 0;
  }
  molenNextMoleDist -= scr;
  if (molenNextMoleDist < 0) {
    Vector pos;
    vectorSet(&pos, 0, 0);
    TIMES(9, i) {
      pos.x = rndi(0, MOLEN_NUMBER_COUNT);
      if (!molenExistsMole(&pos)) {
        molenNextTargetNumberCount--;
        int value;
        if (molenNextTargetNumberCount <= 0) {
          value = molenTargetNumber;
          molenNextTargetNumberCount = rndi(4, 7);
        } else {
          value = molenGetRandomNumber();
        }
        ASSIGN_ARRAY_ITEM(molenMoles, molenMoleIndex, MolenMole, nm);
        nm->pos = pos;
        nm->value = value;
        nm->removeTicks = 0;
        nm->score = 0;
        nm->isAlive = true;
        molenMoleIndex = cgl_wrap(molenMoleIndex + 1, 0, MOLEN_MAX_MOLE_COUNT);
        break;
      }
    }
    molenNextMoleDist += rnd(9, 12);
  }
  molenMoleMoveTicks -= sqrt(sqrt(difficulty));
  if (molenMoleMoveTicks < 0) {
    FOR_EACH(molenMoles, i) {
      ASSIGN_ARRAY_ITEM(molenMoles, i, MolenMole, m);
      SKIP_IS_NOT_ALIVE(m);
      if (m->removeTicks > 0) {
        continue;
      }
      int w = rndi(0, 4);
      Vector p;
      TIMES(4, k) {
        Vector o = molenOffsets[w];
        vectorSet(&p, m->pos.x + o.x, m->pos.y + o.y);
        if (p.y > 0 && !molenExistsMole(&p)) {
          play(HIT);
          m->pos = p;
          break;
        }
        w = (int)cgl_wrap(w + 1, 0, 4);
      }
    }
    molenMoleMoveTicks += 99;
  }
  FOR_EACH(molenMoles, i) {
    ASSIGN_ARRAY_ITEM(molenMoles, i, MolenMole, m);
    SKIP_IS_NOT_ALIVE(m);
    if (m->removeTicks > 0) {
      m->removeTicks -= sqrt(difficulty);
      if (m->removeTicks <= 0) {
        color = RED;
        float px = m->pos.x * 11 + 6;
        float py = m->pos.y * 11 + 6 + molenNumbersY;
        if (m->score != 0) {
          if (m->score > 0) {
            play(POWER_UP);
          } else {
            play(COIN);
          }
          particle(px, py, 9, 2, 0, CGLP_PI * 2);
          addScore(m->score, px, py);
        }
        m->isAlive = false;
        continue;
      }
    } else if (m->pos.y >= MOLEN_NUMBER_COUNT - 1) {
      m->removeTicks = 60;
      if (m->value == molenTargetNumber) {
        play(EXPLOSION);
        FOR_EACH(molenMoles, k) {
          ASSIGN_ARRAY_ITEM(molenMoles, k, MolenMole, mk);
          SKIP_IS_NOT_ALIVE(mk);
          mk->removeTicks = 60;
        }
        color = LIGHT_RED;
        rect(m->pos.x * 11 + 1, m->pos.y * 11 + 1 + molenNumbersY, 11, 11, &scratch);
        gameOver();
      }
    }
  }
  TIMES(MOLEN_NUMBER_COUNT, y) {
    TIMES(MOLEN_NUMBER_COUNT, x) { molenCurrentNumbers[x][y] = molenNumbers[x][y]; }
  }
  FOR_EACH(molenMoles, i) {
    ASSIGN_ARRAY_ITEM(molenMoles, i, MolenMole, m);
    SKIP_IS_NOT_ALIVE(m);
    int n;
    if (m->removeTicks > 0) {
      n = m->value + 10;
    } else {
      n = (int)cgl_wrap(molenCurrentNumbers[(int)m->pos.x][(int)m->pos.y] + m->value, 0, 10);
    }
    molenCurrentNumbers[(int)m->pos.x][(int)m->pos.y] = n;
  }
  // The JS version draws each digit at 2x scale via a text `scale` option
  // this port doesn't support; drawn at normal size instead (visual only).
  TIMES(MOLEN_NUMBER_COUNT, y) {
    TIMES(MOLEN_NUMBER_COUNT, x) {
      int n = molenCurrentNumbers[x][y];
      if (n >= 10) {
        n -= 10;
        color = RED;
      } else {
        if (molenHitTicks > 0 && x == (int)molenHitPos.x && y == (int)molenHitPos.y) {
          color = GREEN;
        } else {
          color = BLUE;
        }
      }
      text(intToChar(n), x * 11 + 6, y * 11 + 6 + molenNumbersY, &scratch);
    }
  }
  molenHitTicks -= sqrt(difficulty);
  color = WHITE;
  rect(0, 0, 100, 7, &scratch);
  rect(0, 93, 100, 7, &scratch);
  color = BLACK;
  text("TARGET =", 3, 96, &scratch);
  color = RED;
  text(intToChar(molenTargetNumber), 3 + 6 * 9, 96, &scratch);
}

void addGameMolen() {
  addGame(molenTitle, molenDescription, molenCharacters, molenCharactersCount,
          &molenOptions, true, &molenUpdate);
}
