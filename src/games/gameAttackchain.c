#include "../cglp.h"

int* attackchainTitle = "ATTACK CHAIN";
int* attackchainDescription = "[Tap]\n Select card";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] attackchainCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int attackchainCharactersCount = 0;

Options attackchainOptions = {100, 100, 2, false};

#define ATTACKCHAIN_CARD_COUNT 6

struct AttackchainCard {
  float v;
  int type;
  bool isValid;
};
AttackchainCard[ATTACKCHAIN_CARD_COUNT] attackchainCards;

float attackchainBarY;
float attackchainTotalDamage;
float attackchainTotalDamageTicks;
float attackchainDamage;
float attackchainDamageTicks;
int attackchainLastAttackType;
// Vircon32 port note: sized for the longest possible composed message -
// "AMAZING" (7) + " B_Return" (9) + null terminator = 17 words; 16 would
// silently overflow into the next global (attackchainMessageColor) the
// first time a player reaches >=200 total damage with a blue last
// attack, since this dialect has no bounds checking on strcpy/strcat.
int[20] attackchainMessage;
int attackchainMessageColor;
float attackchainMessageTicks;
int attackchainTurn;
int attackchainTotalTurn;
int attackchainMultiplier;

void attackchainInitCards() {
  play(POWER_UP);
  int greenCount = 2;
  TIMES(ATTACKCHAIN_CARD_COUNT, i) {
    int type;
    if (rnd(0, 1) < 0.2 && greenCount > 0) {
      type = GREEN;
      greenCount--;
    } else {
      if (rnd(0, 1) < 0.5) {
        type = RED;
      } else {
        type = BLUE;
      }
    }
    attackchainCards[i].v =
        floor(rndi(3, 7) * (4 + sqrt(clamp(attackchainMultiplier, 1, 20))));
    attackchainCards[i].type = type;
    attackchainCards[i].isValid = true;
  }
}

int attackchainCardCount() {
  int v = 0;
  TIMES(ATTACKCHAIN_CARD_COUNT, i) {
    if (attackchainCards[i].isValid) {
      v++;
    }
  }
  return v;
}

void attackchainSetMessage(int* m, int cl) {
  if (m[0] == 0) {
    return;
  }
  strcpy(attackchainMessage, m);
  attackchainMessageColor = cl;
  attackchainMessageTicks = 60;
}

void attackchainReturnCards(int count, bool isBestReturner) {
  if (count == 0) {
    return;
  }
  if (isBestReturner) {
    int bi = 0;
    float bv = 0;
    TIMES(ATTACKCHAIN_CARD_COUNT, i) {
      if (!attackchainCards[i].isValid && attackchainCards[i].v > bv) {
        bv = attackchainCards[i].v;
        bi = i;
      }
    }
    attackchainCards[bi].isValid = true;
    count--;
  }
  TIMES(count, k) {
    int[ATTACKCHAIN_CARD_COUNT] ivi;
    int iviCount = 0;
    TIMES(ATTACKCHAIN_CARD_COUNT, i) {
      if (!attackchainCards[i].isValid) {
        ivi[iviCount] = i;
        iviCount++;
      }
    }
    if (iviCount > 0) {
      attackchainCards[ivi[rndi(0, iviCount)]].isValid = true;
    }
  }
}

void attackchainUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - the tapped card
  // column is computed directly from input.pos via grid math (see "i =
  // floor(...)" below), so the engine's own O(n^2) hitbox scan (see
  // checkHitBox() in cglp.c) is pure waste here. Restored automatically
  // when the next real game starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    attackchainBarY = 0;
    attackchainTotalDamage = 0;
    attackchainTotalDamageTicks = 0;
    attackchainDamage = 0;
    attackchainDamageTicks = 0;
    attackchainLastAttackType = -1;
    attackchainMessage[0] = 0;
    attackchainMessageColor = BLACK;
    attackchainMessageTicks = 0;
    attackchainTurn = 0;
    attackchainTotalTurn = 0;
    attackchainMultiplier = 1;
    attackchainInitCards();
  }
  TIMES(ATTACKCHAIN_CARD_COUNT, i) {
    AttackchainCard* c = &attackchainCards[i];
    if (!c->isValid) {
      continue;
    }
    float x = (i * 80.0) / 5 + 10;
    color = c->type;
    box(x, 90, 6, 6, &scratch);
    color = BLACK;
    float offX;
    if (c->v > 99) {
      offX = x - 6;
    } else {
      offX = x - 3;
    }
    text(intToChar((int)c->v), offX, 82, &scratch);
  }
  color = BLACK;
  rect(0, 75, 100, 1, &scratch);
  color = RED;
  attackchainBarY += (difficulty + sqrt(attackchainMultiplier)) * 0.002;
  if (attackchainDamageTicks <= 0 && attackchainTotalDamageTicks <= 0 &&
      attackchainBarY >= 75) {
    play(EXPLOSION);
    attackchainBarY = 75;
    gameOver();
  }
  rect(0, attackchainBarY, 100, 1, &scratch);
  color = BLACK;
  if (attackchainDamageTicks <= 0 && attackchainTotalDamageTicks <= 0 &&
      input.isJustPressed) {
    int i = (int)floor(input.pos.x / (100.0 / ATTACKCHAIN_CARD_COUNT));
    if (i >= 0 && i < ATTACKCHAIN_CARD_COUNT && attackchainCards[i].isValid) {
      play(SELECT);
      attackchainBarY += difficulty + sqrt(attackchainMultiplier) * 0.05;
      AttackchainCard* c = &attackchainCards[i];
      c->isValid = false;
      if (attackchainTurn == 0 && c->type == RED) {
        attackchainSetMessage("First x1.25", RED);
        attackchainDamage = c->v * 1.25;
      } else if (c->type == GREEN) {
        if (attackchainTotalDamage > 99) {
          attackchainDamage = 0;
          attackchainSetMessage("Limit <=99", GREEN);
        } else if (attackchainTotalDamage + c->v > 99) {
          attackchainDamage = 99 - attackchainTotalDamage;
          attackchainSetMessage("Limit <=99", GREEN);
        } else {
          attackchainDamage = c->v;
        }
      } else {
        attackchainDamage = c->v;
      }
      attackchainDamage = floor(attackchainDamage);
      attackchainDamageTicks = 30;
      attackchainTurn++;
      attackchainLastAttackType = c->type;
      c->v = floor(c->v * 1.1 + 10);
    }
  }
  if (attackchainDamageTicks > 0) {
    attackchainDamageTicks -= difficulty;
    float dOfs;
    if (attackchainDamage > 99) {
      dOfs = 6;
    } else {
      dOfs = 3;
    }
    text(intToChar((int)attackchainDamage), 50 - dOfs,
         45 + attackchainDamageTicks / 2, &scratch);
    if (attackchainDamageTicks <= 0) {
      play(HIT);
      attackchainTotalDamage += attackchainDamage;
      if (attackchainTotalDamage >= 100 || attackchainCardCount() == 0) {
        attackchainTotalDamageTicks = 30;
      }
    }
  }
  float pctOfs;
  if (attackchainTotalDamage > 99) {
    pctOfs = 12;
  } else if (attackchainTotalDamage > 9) {
    pctOfs = 6;
  } else {
    pctOfs = 0;
  }
  float totalY;
  if (attackchainTotalDamageTicks > 0) {
    totalY = attackchainTotalDamageTicks;
  } else {
    totalY = 30;
  }
  // Vircon32 port note: the JS version draws these two digits/percent-sign
  // at 2x scale via a text `scale` option this port doesn't support; drawn
  // at normal size instead (visual only, see gameMolen.c for precedent).
  text(intToChar((int)attackchainTotalDamage), 50 - pctOfs, totalY, &scratch);
  text("%", 59 + pctOfs, totalY + 3, &scratch);
  if (attackchainTotalDamageTicks > 0) {
    attackchainTotalDamageTicks -= difficulty;
    if (attackchainTotalDamageTicks <= 0) {
      play(CLICK);
      addScore(attackchainTotalDamage * attackchainMultiplier, 50, 30);
      if (attackchainTotalDamage >= 115) {
        attackchainBarY -= (attackchainTotalDamage - 115) *
                            sqrt(attackchainTotalDamage - 115) /
                            sqrt(attackchainMultiplier);
      }
      if (attackchainBarY < 0) {
        attackchainBarY = 0;
      }
      int rc = 0;
      int[20] m;
      m[0] = 0;
      int cl = BLACK;
      if (attackchainTotalDamage >= 200) {
        strcpy(m, "AMAZING");
        rc = 3;
      } else if (attackchainTotalDamage >= 150) {
        strcpy(m, "BRAVO");
        rc = 2;
      } else if (attackchainTotalDamage >= 100) {
        strcpy(m, "COOL");
        rc = 1;
      }
      if (rc > 0 && attackchainLastAttackType == BLUE) {
        strcat(m, " B_Return");
        cl = BLUE;
      }
      attackchainSetMessage(m, cl);
      attackchainTurn = 0;
      attackchainTotalTurn++;
      attackchainTotalDamage = 0;
      attackchainReturnCards(rc, attackchainLastAttackType == BLUE);
      if (attackchainCardCount() == 0 || attackchainTotalTurn > 6) {
        attackchainTotalTurn = 0;
        attackchainMultiplier++;
        attackchainInitCards();
      }
    }
  }
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(attackchainMultiplier));
  text(multText, 3, 9, &scratch);
  if (attackchainMessageTicks > 0) {
    attackchainMessageTicks -= difficulty;
    color = attackchainMessageColor;
    text(attackchainMessage, 50 - strlen(attackchainMessage) * 3, 70, &scratch);
  }
}

void addGameAttackchain() {
  addGame(attackchainTitle, attackchainDescription, attackchainCharacters,
          attackchainCharactersCount, &attackchainOptions, true,
          &attackchainUpdate);
}
