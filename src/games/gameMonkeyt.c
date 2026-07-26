#include "../cglp.h"

int* monkeytTitle = "MONKEY T";
int* monkeytDescription = "[Hold]\n  Compress\n[Release]\n  Launch";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] monkeytCharacters = {
    {
        "l  ll ",
        "l l  l",
        "l  ll ",
        " lll l",
        "lll  l",
        "      ",
    },
    {
        "  lll ",
        "   l  ",
        "  l l ",
        "  l l ",
        " l l l",
        "l l l ",
    },
    {
        "llll  ",
        " llll ",
        "  llll",
        " lll  ",
        "ll  l ",
        "      ",
    },
};
int monkeytCharactersCount = 3;

Options monkeytOptions = {100, 100, 1, false};

#define MONKEYT_MAX_COLLECTIBLE_COUNT 8

struct MonkeytCollectible {
  Vector pos;
  bool isAlive;
};
MonkeytCollectible[MONKEYT_MAX_COLLECTIBLE_COUNT] monkeytCollectibles;

struct MonkeytHazard {
  Vector pos;
  float speed;
};
MonkeytHazard[3] monkeytHazards;

Vector monkeytMonkeyPosition;
Vector monkeytMonkeyVelocity;
int monkeytMonkeyCollectingCount;
float monkeytTrampolineCompression;
float monkeytTrampolineLaunchPower;
int monkeytMultiplier;

void monkeytAddCollectables() {
  TIMES(5, i) {
    vectorSet(&monkeytCollectibles[i].pos, rnd(30, 70), rnd(20, 70));
    monkeytCollectibles[i].isAlive = true;
  }
  for (int i = 5; i < MONKEYT_MAX_COLLECTIBLE_COUNT; i++) {
    monkeytCollectibles[i].isAlive = false;
  }
}

void monkeytUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&monkeytMonkeyPosition, 50, 90);
    vectorSet(&monkeytMonkeyVelocity, 0.3, 0);
    monkeytMonkeyCollectingCount = 0;
    monkeytTrampolineCompression = 0;
    monkeytTrampolineLaunchPower = 0;
    monkeytAddCollectables();
    TIMES(3, i) {
      vectorSet(&monkeytHazards[i].pos, rnd(10, 90), rnd(10, 70));
      monkeytHazards[i].speed = 0.5 + i * 0.1;
    }
    monkeytMultiplier = 1;
  }
  if (input.isJustPressed) {
    monkeytTrampolineLaunchPower = 0;
    monkeytTrampolineCompression = 0;
  }
  if (input.isPressed) {
    float c2 = monkeytTrampolineCompression + 1;
    if (c2 > 30) {
      c2 = 30;
    }
    monkeytTrampolineCompression = c2;
  }
  if (input.isJustReleased) {
    monkeytTrampolineLaunchPower = monkeytTrampolineCompression;
    monkeytTrampolineCompression = 0;
    monkeytMonkeyCollectingCount = 0;
  }
  float trampolineHeight =
      90 + monkeytTrampolineCompression * 0.25 + monkeytTrampolineLaunchPower * 0.25;
  color = BLACK;
  thickness = 2;
  line(25, trampolineHeight, 75, trampolineHeight, &scratch);
  monkeytMonkeyVelocity.y += 0.1;
  vectorAdd(&monkeytMonkeyPosition, monkeytMonkeyVelocity.x * difficulty, monkeytMonkeyVelocity.y);
  if ((monkeytMonkeyPosition.x < 25 && monkeytMonkeyVelocity.x < 0) ||
      (monkeytMonkeyPosition.x > 75 && monkeytMonkeyVelocity.x > 0)) {
    monkeytMonkeyVelocity.x = -monkeytMonkeyVelocity.x;
  }
  color = RED;
  if (monkeytMonkeyVelocity.x > 0) {
    characterOptions.isMirrorX = false;
  } else {
    characterOptions.isMirrorX = true;
  }
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  character("a", monkeytMonkeyPosition.x, monkeytMonkeyPosition.y, &scratch);
  if (scratch.isColliding.rect[BLACK] && monkeytMonkeyVelocity.y > 0) {
    monkeytMonkeyPosition.y = trampolineHeight - 5;
    monkeytMonkeyVelocity.y = -monkeytMonkeyVelocity.y * 0.2 - monkeytTrampolineLaunchPower * 0.13;
    if (monkeytTrampolineLaunchPower > 1) {
      play(JUMP);
      monkeytTrampolineLaunchPower = 0;
    }
  }
  if (monkeytMonkeyPosition.y > 99) {
    monkeytMonkeyPosition.y = 80;
  }
  color = YELLOW;
  FOR_EACH(monkeytCollectibles, i) {
    ASSIGN_ARRAY_ITEM(monkeytCollectibles, i, MonkeytCollectible, c);
    SKIP_IS_NOT_ALIVE(c);
    character("b", c->pos.x, c->pos.y, &scratch);
    if (scratch.isColliding.character['a']) {
      play(COIN);
      monkeytMultiplier += monkeytMonkeyCollectingCount;
      monkeytMonkeyCollectingCount++;
      addScore(monkeytMultiplier, monkeytMonkeyPosition.x, monkeytMonkeyPosition.y);
      particle(c->pos.x, c->pos.y, 9, 1, 0, CGLP_PI * 2);
      c->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(monkeytCollectibles, aliveCollectibleCount);
  if (aliveCollectibleCount == 0) {
    monkeytAddCollectables();
  }
  color = BLACK;
  TIMES(3, i) {
    MonkeytHazard* h = &monkeytHazards[i];
    float mul;
    if ((h->pos.x < 10 && h->speed > 0) || (h->pos.x > 90 && h->speed < 0)) {
      mul = 0.5;
    } else {
      mul = 1;
    }
    h->pos.x += h->speed * mul;
    if ((h->pos.x < -10 && h->speed < 0) || (h->pos.x > 110 && h->speed > 0)) {
      h->pos.y = rnd(10, 70);
      h->speed = -h->speed;
    }
    if (h->speed > 0) {
      characterOptions.isMirrorX = false;
    } else {
      characterOptions.isMirrorX = true;
    }
    character("c", h->pos.x, h->pos.y, &scratch);
    if (scratch.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
  }
  characterOptions.isMirrorX = false;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(monkeytMultiplier));
  text(multText, 3, 10, &scratch);
}

void addGameMonkeyt() {
  addGame(monkeytTitle, monkeytDescription, monkeytCharacters,
          monkeytCharactersCount, &monkeytOptions, false, &monkeytUpdate);
}
