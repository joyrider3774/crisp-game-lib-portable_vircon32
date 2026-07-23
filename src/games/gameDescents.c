#include "../cglp.h"

char* descentsTitle = "DESCENT S";
char* descentsDescription = "[Hold] Thrust up";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] descentsCharacters = {
    {
        "l cc  ",
        "lllll ",
        "llllll",
        "      ",
        "      ",
        "      ",
    },
    {
        "  ll  ",
        "  ll  ",
        "  ll  ",
        "llllll",
        " llll ",
        "  ll  ",
    },
    {
        "ll    ",
        "  l   ",
        " l    ",
        "lll   ",
        "      ",
        "      ",
    }
};

int descentsCharactersCount = 3;

Options descentsOptions = {100, 100, 1, false};

struct DescentsShip {
  Vector pos;
  Vector vel;
  int up;
  int down;
  int nextUp;
  int nextDown;
};

struct DescentsDeck {
  Vector pos;
  float width;
  bool isAlive;
};

#define DESCENT_MAX_DECK_COUNT 10
DescentsDeck[DESCENT_MAX_DECK_COUNT] descentsDecks;
int descentsDeckCount;
DescentsShip descentsShip;

void descentsShiftDecksDown(int removedIndex) {
  memcpy(&descentsDecks[removedIndex], &descentsDecks[removedIndex + 1],
         (descentsDeckCount - 1 - removedIndex) * sizeof(descentsDecks[0]));
  descentsDecks[descentsDeckCount - 1].isAlive = false;
  descentsDeckCount--;
}

void descentsUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&descentsShip.pos, 9, 9);
    vectorSet(&descentsShip.vel, 0.2, 0.2);
    descentsShip.up = -2;
    descentsShip.down = 2;
    descentsShip.nextUp = -3;
    descentsShip.nextDown = 3;

    INIT_UNALIVED_ARRAY_FAST(descentsDecks);
    descentsDeckCount = 0;

    vectorSet(&descentsDecks[0].pos, 30, 50);
    descentsDecks[0].width = 40;
    descentsDecks[0].isAlive = true;
    descentsDeckCount++;

    vectorSet(&descentsDecks[1].pos, 50, 75);
    descentsDecks[1].width = 35;
    descentsDecks[1].isAlive = true;
    descentsDeckCount++;

    vectorSet(&descentsDecks[2].pos, 70, 99);
    descentsDecks[2].width = 30;
    descentsDecks[2].isAlive = true;
    descentsDeckCount++;
  }

  float velStep;
  if (input.isPressed) {
    velStep = descentsShip.up * 0.005;
  } else {
    velStep = descentsShip.down * 0.005;
  }
  descentsShip.vel.y += velStep;

  if (input.isJustPressed) {
    play(COIN);
  }

  if (input.isJustReleased) {
    play(LASER);
  }

  descentsShip.pos.y += descentsShip.vel.y;

  if (descentsShip.vel.y > 1) {
    color = RED;
  } else if (descentsShip.vel.y > 0.6) {
    color = YELLOW;
  } else {
    color = BLUE;
  }
  rect(1, 20, 3, descentsShip.vel.y * 20, &scratch);

  color = RED;
  rect(0, 40, 5, 1, &scratch);

  float scrY = 0;
  if (descentsShip.pos.y > 19) {
    scrY = (19 - descentsShip.pos.y) * 0.2;
  } else if (descentsShip.pos.y < 9) {
    scrY = (9 - descentsShip.pos.y) * 0.2;
  }
  descentsShip.pos.y += scrY;

  for (int i = 0; i < descentsDeckCount; i++) {
    descentsDecks[i].pos.x -= descentsShip.vel.x;
    descentsDecks[i].pos.y += scrY;

    if (i == 0) {
      color = BLACK;
    } else {
      color = LIGHT_BLACK;
    }
    rect(descentsDecks[i].pos.x, descentsDecks[i].pos.y, descentsDecks[i].width, 3, &scratch);

    if (i == 0) {
      color = YELLOW;
      rect(0, descentsDecks[i].pos.y + 3, descentsDecks[i].pos.x + descentsDecks[i].width, 2, &scratch);
      rect(descentsDecks[i].pos.x + descentsDecks[i].width, 0, 1, descentsDecks[i].pos.y + 5, &scratch);
    }
  }

  color = BLACK;
  Collision collision;
  character("a", descentsShip.pos.x, descentsShip.pos.y, &collision);

  if (collision.isColliding.rect[BLACK]) {
    if (descentsShip.vel.y > 1) {
      play(EXPLOSION);
      gameOver();
    } else {
      if (descentsDeckCount > 0) {
        play(POWER_UP);
        addScore(floor(descentsDecks[0].pos.x + descentsDecks[0].width - descentsShip.pos.x + 1), descentsShip.pos.x, descentsShip.pos.y);

        descentsShiftDecksDown(0);

        if (descentsDeckCount > 0) {
          DescentsDeck* lastDeck = &descentsDecks[descentsDeckCount - 1];

          vectorSet(&descentsDecks[descentsDeckCount].pos,
                  lastDeck->pos.x + lastDeck->width * 0.7 + rnd(0, 30),
                  lastDeck->pos.y + rnd(20, 40));
          descentsDecks[descentsDeckCount].width = rnd(25, 50);
          descentsDecks[descentsDeckCount].isAlive = true;
          descentsDeckCount++;
        } else {
          vectorSet(&descentsDecks[0].pos, descentsShip.pos.x + 50, descentsShip.pos.y + 30);
          descentsDecks[0].width = rnd(25, 50);
          descentsDecks[0].isAlive = true;
          descentsDeckCount = 1;
        }

        descentsShip.up = descentsShip.nextUp;
        descentsShip.down = descentsShip.nextDown;
        descentsShip.nextUp = floor(-difficulty * 4 + rnd(-difficulty * 3, difficulty * 3));
        descentsShip.nextDown = floor(difficulty * 4 + rnd(-difficulty * 2, difficulty * 2));
        vectorSet(&descentsShip.vel, difficulty * 0.2, 0);
      }
    }
  }

  if (collision.isColliding.rect[YELLOW] || descentsShip.pos.y < 0) {
    play(EXPLOSION);
    gameOver();
  }

  // Draw UI elements
  int[16] buffer;

  // Thrust up indicator
  if (input.isPressed) {
    color = YELLOW;
  } else {
    color = CYAN;
  }
  characterOptions.isMirrorY = true;
  character("b", 60, 10, &scratch);
  characterOptions.isMirrorY = false;

  color = BLACK;
  strcpy(buffer, intToChar(-descentsShip.up));
  strcat(buffer, "m/s");
  int upOffset;
  if (-descentsShip.up > 9) {
    upOffset = 6;
  } else {
    upOffset = 0;
  }
  text(buffer, 76 - upOffset, 10, &scratch);
  character("c", 97, 9, &scratch);

  // Thrust down indicator
  if (!input.isPressed) {
    color = YELLOW;
  } else {
    color = CYAN;
  }
  character("b", 60, 17, &scratch);

  color = BLACK;
  strcpy(buffer, intToChar(descentsShip.down));
  strcat(buffer, "m/s");
  int downOffset;
  if (descentsShip.down > 9) {
    downOffset = 6;
  } else {
    downOffset = 0;
  }
  text(buffer, 76 - downOffset, 17, &scratch);
  character("c", 97, 16, &scratch);

  // Next indicators
  text("NEXT", 70, 24, &scratch);

  color = CYAN;
  characterOptions.isMirrorY = true;
  character("b", 60, 31, &scratch);
  characterOptions.isMirrorY = false;

  color = BLACK;
  strcpy(buffer, intToChar((int)fabs((float)descentsShip.nextUp)));
  strcat(buffer, "m/s");
  int nextUpOffset;
  if (-descentsShip.nextUp > 9) {
    nextUpOffset = 6;
  } else {
    nextUpOffset = 0;
  }
  text(buffer, 76 - nextUpOffset, 31, &scratch);
  character("c", 97, 30, &scratch);

  color = CYAN;
  character("b", 60, 38, &scratch);

  color = BLACK;
  strcpy(buffer, intToChar(descentsShip.nextDown));
  strcat(buffer, "m/s");
  int nextDownOffset;
  if (descentsShip.nextDown > 9) {
    nextDownOffset = 6;
  } else {
    nextDownOffset = 0;
  }
  text(buffer, 76 - nextDownOffset, 38, &scratch);
  character("c", 97, 37, &scratch);
}

void addGameDescents() {
  addGame(descentsTitle, descentsDescription, descentsCharacters, descentsCharactersCount,
          &descentsOptions, false, &descentsUpdate);
}
