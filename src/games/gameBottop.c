#include "../cglp.h"

int* bottopTitle = "BOTTOP";
int* bottopDescription = "[Tap]\n Jump\n[Hold]\n Fly";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bottopCharacters = {
    {
        "  ll  ",
        "  l   ",
        " llll ",
        "l l   ",
        " l  l ",
        "l  l  ",
    },
    {
        "  ll  ",
        "  l   ",
        " lll  ",
        "  l   ",
        "  ll  ",
        " ll   ",
    },
    {
        "  ll  ",
        "l l  l",
        " llll ",
        "  l   ",
        " l lll",
        "l     ",
    },
    {
        "ll  ll",
        " llll ",
        "llllll",
        "llllll",
        "  ll  ",
        "  ll  ",
    },
};
int bottopCharactersCount = 4;

Options bottopOptions = {100, 100, 2, false};

float bottopY;
float bottopVy;
bool bottopIsJumping;

struct BottopSpike {
  Vector pos;
  bool isAlive;
};
#define BOTTOP_MAX_SPIKE_COUNT 32
BottopSpike[BOTTOP_MAX_SPIKE_COUNT] bottopSpikes;
int bottopSpikeIndex;
float bottopSpikeAddDist;
float bottopScrolling;

void bottopUpdate() {
  Collision scratch;
  if (!ticks) {
    bottopY = 0;
    bottopVy = 0;
    bottopIsJumping = false;
    INIT_UNALIVED_ARRAY_FAST(bottopSpikes);
    bottopSpikeIndex = 0;
    bottopSpikeAddDist = 0;
    bottopScrolling = 1;
  }
  bottopScrolling = difficulty;
  score += bottopScrolling / 10;
  bottopSpikeAddDist -= bottopScrolling;
  if (bottopSpikeAddDist < 0) {
    float sy;
    if (rnd(0, 1) < 0.33) {
      if (rnd(0, 1) < 0.5) {
        sy = 8;
      } else {
        sy = 92;
      }
    } else {
      sy = rnd(8, 92);
    }
    ASSIGN_ARRAY_ITEM(bottopSpikes, bottopSpikeIndex, BottopSpike, ns);
    vectorSet(&ns->pos, 103, sy);
    ns->isAlive = true;
    bottopSpikeIndex = cgl_wrap(bottopSpikeIndex + 1, 0, BOTTOP_MAX_SPIKE_COUNT);
    bottopSpikeAddDist += rnd(30, 60);
  }
  color = RED;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  FOR_EACH(bottopSpikes, i) {
    ASSIGN_ARRAY_ITEM(bottopSpikes, i, BottopSpike, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x -= bottopScrolling;
    characterOptions.rotation = (int)(ticks / 10) % 4;
    character("d", s->pos.x, s->pos.y, &scratch);
    if (s->pos.x <= -3) {
      s->isAlive = false;
    }
  }
  characterOptions.rotation = 0;
  if (!bottopIsJumping && input.isPressed) {
    play(POWER_UP);
    bottopIsJumping = true;
    bottopVy = 3;
  }
  if (bottopIsJumping) {
    if (input.isPressed) {
      bottopVy -= 0.1;
    } else {
      bottopVy -= 0.3;
    }
    bottopY += bottopVy;
    if (bottopY < 0) {
      bottopY = 0;
      bottopIsJumping = false;
    }
  }
  color = BLACK;
  int[2] c;
  if (bottopIsJumping) {
    c[0] = 'c';
  } else {
    c[0] = 'a' + ((int)(ticks / 15) % 2);
  }
  c[1] = 0;
  characterOptions.isMirrorY = true;
  Collision c1;
  character(c, 9, 8 + bottopY, &c1);
  bool hit1 = c1.isColliding.character['d'];
  bool hit2 = false;
  if (!hit1) {
    characterOptions.isMirrorY = false;
    Collision c2;
    character(c, 9, 92 - bottopY, &c2);
    hit2 = c2.isColliding.character['d'];
  }
  characterOptions.isMirrorY = false;
  if (hit1 || hit2) {
    play(EXPLOSION);
    gameOver();
  }
  color = LIGHT_BLUE;
  rect(0, 0, 99, 5, &scratch);
  rect(0, 95, 99, 5, &scratch);
}

void addGameBottop() {
  addGame(bottopTitle, bottopDescription, bottopCharacters,
          bottopCharactersCount, &bottopOptions, false, &bottopUpdate);
}
