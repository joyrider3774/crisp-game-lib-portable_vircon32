#include "../cglp.h"

int* twinjumpersTitle = "TWIN JUMPERS";
int* twinjumpersDescription = "[Tap] Jump";

// Vircon32 port note: character 'b' upstream is only 4 rows tall ("\n
// gggg\ngggggg\ngggggg\ngg  gg\n") - the remaining 2 rows of the fixed
// CHARACTER_HEIGHT+1-tall array are simply omitted from the initializer and
// come out zero-filled (a blank row), matching how gameAccelb.c's own
// 2-row-tall characters are declared.
int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] twinjumpersCharacters = {
    {
        " bb   ",
        " bb   ",
        "bbbb  ",
        "bbbb  ",
        "b  b  ",
        "b  b  ",
    },
    {
        " gggg ",
        "gggggg",
        "gggggg",
        "gg  gg",
    },
};
int twinjumpersCharactersCount = 2;

Options twinjumpersOptions = {100, 100, 9, false};

struct TwinjumpersJumper {
  Vector pos;
  float vx;
  float vy;
  float onPlatformTicks;
};
// Vircon32 port note: upstream always has exactly two jumpers (never added
// to or removed), so this is a plain fixed 2-element array rather than a
// ring-buffered/isAlive-tracked one.
TwinjumpersJumper[2] twinjumpersJumpers;
float twinjumpersJumpPower;

struct TwinjumpersPlatform {
  Vector pos;
  float width;
  bool isAlive;
};
#define TWINJUMPERS_MAX_PLATFORM_COUNT 32
TwinjumpersPlatform[TWINJUMPERS_MAX_PLATFORM_COUNT] twinjumpersPlatforms;
int twinjumpersPlatformIndex;
float twinjumpersScrollSpeed;
float twinjumpersNextPlatformDistance;

void twinjumpersAddPlatform(float x, float y, float width) {
  ASSIGN_ARRAY_ITEM(twinjumpersPlatforms, twinjumpersPlatformIndex, TwinjumpersPlatform, p);
  vectorSet(&p->pos, x, y);
  p->width = width;
  p->isAlive = true;
  twinjumpersPlatformIndex = cgl_wrap(twinjumpersPlatformIndex + 1, 0, TWINJUMPERS_MAX_PLATFORM_COUNT);
}

void twinjumpersUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&twinjumpersJumpers[0].pos, 20, 70);
    twinjumpersJumpers[0].vx = 1;
    twinjumpersJumpers[0].vy = 0;
    twinjumpersJumpers[0].onPlatformTicks = 0;
    vectorSet(&twinjumpersJumpers[1].pos, 80, 70);
    twinjumpersJumpers[1].vx = 1;
    twinjumpersJumpers[1].vy = 0;
    twinjumpersJumpers[1].onPlatformTicks = 0;
    twinjumpersJumpPower = 1;
    INIT_UNALIVED_ARRAY_FAST(twinjumpersPlatforms);
    twinjumpersPlatformIndex = 0;
    twinjumpersAddPlatform(50, 80, 100);
    twinjumpersScrollSpeed = 0.1;
    twinjumpersNextPlatformDistance = 0;
    TIMES(3, i) {
      twinjumpersAddPlatform(rnd(0, 40), 70 - i * 30, rnd(20, 50));
      twinjumpersAddPlatform(rnd(60, 100), 70 - i * 30, rnd(20, 50));
    }
  }

  twinjumpersScrollSpeed = 0.1 * sqrt(difficulty);
  float my = fmax(twinjumpersJumpers[0].pos.y, twinjumpersJumpers[1].pos.y);
  if (my < 60) {
    twinjumpersScrollSpeed += (60 - my) * 0.05;
  }
  addScore(twinjumpersScrollSpeed, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);

  if (input.isJustPressed) {
    TIMES(2, i) {
      if (twinjumpersJumpers[i].onPlatformTicks > 0) {
        play(JUMP);
        if (i == 0) {
          twinjumpersJumpers[i].vy = -3 * twinjumpersJumpPower;
        } else {
          twinjumpersJumpers[i].vy = -2.5 * twinjumpersJumpPower;
        }
      }
    }
    twinjumpersJumpPower *= 0.8;
  } else {
    twinjumpersJumpPower = clamp(twinjumpersJumpPower + 0.01, 0, 1);
  }

  TIMES(2, i) {
    twinjumpersJumpers[i].pos.y += twinjumpersJumpers[i].vy;
    if (input.isPressed) {
      twinjumpersJumpers[i].vy += 0.1;
    } else {
      twinjumpersJumpers[i].vy += 0.2;
    }
    twinjumpersJumpers[i].pos.x += twinjumpersJumpers[i].vx * 0.6;
    twinjumpersJumpers[i].onPlatformTicks--;
    if ((twinjumpersJumpers[i].pos.x < 3 && twinjumpersJumpers[i].vx < 0) ||
        (twinjumpersJumpers[i].pos.x > 97 && twinjumpersJumpers[i].vx > 0)) {
      twinjumpersJumpers[i].vx = -twinjumpersJumpers[i].vx;
    }
    if (twinjumpersJumpers[i].pos.y < 0 && twinjumpersJumpers[i].vy < 0) {
      twinjumpersJumpers[i].pos.y = 0;
      twinjumpersJumpers[i].vy *= -0.5;
    }
  }

  FOR_EACH(twinjumpersPlatforms, pIdx) {
    ASSIGN_ARRAY_ITEM(twinjumpersPlatforms, pIdx, TwinjumpersPlatform, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.y += twinjumpersScrollSpeed;
    p->isAlive = p->pos.y <= 109;
  }

  twinjumpersNextPlatformDistance -= twinjumpersScrollSpeed;
  if (twinjumpersNextPlatformDistance <= 0) {
    twinjumpersAddPlatform(rnd(0, 50), -rnd(0, 20) - 5, rnd(30, 50));
    twinjumpersAddPlatform(rnd(50, 100), -rnd(0, 20) - 5, rnd(30, 50));
    twinjumpersNextPlatformDistance = rnd(20, 30);
  }

  color = BLACK;
  character("a", twinjumpersJumpers[0].pos.x, twinjumpersJumpers[0].pos.y, &scratch);
  character("b", twinjumpersJumpers[1].pos.x, twinjumpersJumpers[1].pos.y, &scratch);

  color = YELLOW;
  FOR_EACH(twinjumpersPlatforms, pi2) {
    ASSIGN_ARRAY_ITEM(twinjumpersPlatforms, pi2, TwinjumpersPlatform, p2);
    SKIP_IS_NOT_ALIVE(p2);
    box(p2->pos.x, p2->pos.y, p2->width, 4, &scratch);
    if (scratch.isColliding.character['a'] && twinjumpersJumpers[0].vy > 0) {
      if (twinjumpersJumpers[0].vy > 2) {
        play(CLICK);
      }
      twinjumpersJumpers[0].pos.y = p2->pos.y - 4;
      twinjumpersJumpers[0].vy = 0;
      twinjumpersJumpers[0].onPlatformTicks = 9;
    }
    // Vircon32 port note: upstream's own check here literally reads
    // "jumpers[0].vy > 2" inside the *second* jumper's branch too (not
    // jumpers[1].vy) - kept verbatim to match the original game's behavior
    // exactly, not fixed as a "typo", since this only gates which sound
    // effect plays and isn't something safe to silently change.
    if (scratch.isColliding.character['b'] && twinjumpersJumpers[1].vy > 0) {
      if (twinjumpersJumpers[0].vy > 2) {
        play(CLICK);
      }
      twinjumpersJumpers[1].pos.y = p2->pos.y - 3;
      twinjumpersJumpers[1].vy = 0;
      twinjumpersJumpers[1].onPlatformTicks = 9;
    }
  }

  bool anyFallen = false;
  TIMES(2, i) {
    if (twinjumpersJumpers[i].pos.y > 102) {
      anyFallen = true;
    }
  }
  if (anyFallen) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameTwinjumpers() {
  addGame(twinjumpersTitle, twinjumpersDescription, twinjumpersCharacters,
          twinjumpersCharactersCount, &twinjumpersOptions, false, &twinjumpersUpdate);
}
