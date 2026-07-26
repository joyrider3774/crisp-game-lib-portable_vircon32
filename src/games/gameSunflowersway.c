#include "../cglp.h"

int* sunflowerswayTitle = "SUNFLOWER SWAY";
int* sunflowerswayDescription = "[Hold] Sunny";

int[5][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] sunflowerswayCharacters = {
    {
        " yyyy ",
        "ylllly",
        "ylllly",
        "ylllly",
        "ylllly",
        " yyyy ",
    },
    {
        "  l   ",
        " lll  ",
        "l lll ",
        "l lll ",
        " lll  ",
    },
    {
        "l ll l",
        " l  l ",
        "l ll l",
        "llllll",
        " llll ",
        "l ll l",
    },
    {
        "   ll ",
        "  lll ",
        " llll ",
        "lllll ",
        "  ll  ",
        " ll   ",
    },
    {
        " llll ",
        "llllll",
        "llllll",
        "llllll",
        "llllll",
        " llll ",
    },
};
int sunflowerswayCharactersCount = 5;

Options sunflowerswayOptions = {100, 100, 1, false};

struct SunflowerswaySunflower {
  Vector pos;
  float height;
  float angle;
  float swayVelocity;
};
SunflowerswaySunflower sunflowerswaySunflower;
// The JS version stores these as sunflower.leafPositions (a fixed 3-value
// array); pulled out to its own global since it never changes and doesn't
// need to live inside the struct.
float[3] sunflowerswayLeafPositions;

struct SunflowerswayDewdrop {
  Vector pos;
  float speed;
  bool isAlive;
};
#define SUNFLOWERSWAY_MAX_DEWDROP_COUNT 32
SunflowerswayDewdrop[SUNFLOWERSWAY_MAX_DEWDROP_COUNT] sunflowerswayDewdrops;
int sunflowerswayDewdropIndex;

struct SunflowerswayInsect {
  Vector pos;
  float speed;
  bool isAlive;
};
#define SUNFLOWERSWAY_MAX_INSECT_COUNT 32
SunflowerswayInsect[SUNFLOWERSWAY_MAX_INSECT_COUNT] sunflowerswayInsects;
int sunflowerswayInsectIndex;

float sunflowerswaySunX;
float sunflowerswayNextDewdropTicks;
float sunflowerswayNextInsectTicks;

void sunflowerswayDrawSun(float x) {
  Collision scratch;
  color = YELLOW;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  character("e", x, 10, &scratch);
  TIMES(7, i) {
    float a = ticks * 0.05 + i * CGLP_PI * 2 / 7;
    float l = fabs(sin(i + ticks * 0.05) * 5) + 10;
    Vector p1;
    vectorSet(&p1, x, 10);
    addWithAngle(&p1, a, 7);
    Vector p2;
    vectorSet(&p2, x, 10);
    addWithAngle(&p2, a, l);
    line(p1.x, p1.y, p2.x, p2.y, &scratch);
  }
}

void sunflowerswayUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&sunflowerswaySunflower.pos, 30, 95);
    sunflowerswaySunflower.height = 40;
    sunflowerswaySunflower.angle = 0;
    sunflowerswaySunflower.swayVelocity = 0.001;
    sunflowerswayLeafPositions[0] = 0.2;
    sunflowerswayLeafPositions[1] = 0.4;
    sunflowerswayLeafPositions[2] = 0.6;
    INIT_UNALIVED_ARRAY_FAST(sunflowerswayDewdrops);
    sunflowerswayDewdropIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(sunflowerswayInsects);
    sunflowerswayInsectIndex = 0;
    sunflowerswaySunX = 120;
    sunflowerswayNextDewdropTicks = 30;
    sunflowerswayNextInsectTicks = 60;
  }
  color = YELLOW;
  rect(0, 90, 100, 10, &scratch);
  if (input.isPressed) {
    sunflowerswaySunflower.swayVelocity += 0.005;
    sunflowerswaySunX += (80 - sunflowerswaySunX) * 0.1;
  } else {
    sunflowerswaySunX += (120 - sunflowerswaySunX) * 0.1;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  sunflowerswaySunflower.swayVelocity *= 0.97;
  sunflowerswaySunflower.angle += sunflowerswaySunflower.swayVelocity;
  sunflowerswaySunflower.swayVelocity -= sunflowerswaySunflower.angle * 0.01;
  if (sunflowerswaySunX < 110) {
    sunflowerswayDrawSun(sunflowerswaySunX);
  }
  sunflowerswaySunflower.height -= 0.045 * difficulty;
  if (sunflowerswaySunflower.height > 70) {
    sunflowerswaySunflower.height = 70;
  }
  if (sunflowerswaySunflower.height < 9) {
    play(EXPLOSION);
    gameOver();
  }
  color = GREEN;
  Vector stemTop;
  vectorSet(&stemTop, sunflowerswaySunflower.pos.x, sunflowerswaySunflower.pos.y);
  addWithAngle(&stemTop, sunflowerswaySunflower.angle - CGLP_PI_2, sunflowerswaySunflower.height);
  line(sunflowerswaySunflower.pos.x, sunflowerswaySunflower.pos.y, stemTop.x, stemTop.y, &scratch);
  TIMES(3, i) {
    Vector leafPoint;
    vectorSet(&leafPoint, sunflowerswaySunflower.pos.x, sunflowerswaySunflower.pos.y);
    addWithAngle(&leafPoint, sunflowerswaySunflower.angle - CGLP_PI_2,
                 sunflowerswaySunflower.height * sunflowerswayLeafPositions[i]);
    int ox;
    if (i % 2 == 0) {
      ox = 1;
    } else {
      ox = -1;
    }
    characterOptions.isMirrorX = ox < 0;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    character("d", leafPoint.x + ox * 3, leafPoint.y, &scratch);
  }
  color = BLACK;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  // Vircon32 port note: the JS version draws the sunflower head at 2x scale
  // via a char `scale` option this port doesn't support; drawn at normal
  // size instead (visual only, see gameMolen.c/gameAttackchain.c for
  // precedent).
  character("a", stemTop.x, stemTop.y, &scratch);

  sunflowerswayNextDewdropTicks--;
  if (sunflowerswayNextDewdropTicks <= 0) {
    ASSIGN_ARRAY_ITEM(sunflowerswayDewdrops, sunflowerswayDewdropIndex, SunflowerswayDewdrop, nd);
    vectorSet(&nd->pos, rnd(0, 99), -3);
    nd->speed = rnd(0.5, 1) * difficulty;
    nd->isAlive = true;
    sunflowerswayDewdropIndex = cgl_wrap(sunflowerswayDewdropIndex + 1, 0, SUNFLOWERSWAY_MAX_DEWDROP_COUNT);
    sunflowerswayNextDewdropTicks = rnd(30, 40) / difficulty;
  }
  FOR_EACH(sunflowerswayDewdrops, di) {
    ASSIGN_ARRAY_ITEM(sunflowerswayDewdrops, di, SunflowerswayDewdrop, d);
    SKIP_IS_NOT_ALIVE(d);
    d->pos.y += d->speed;
    color = LIGHT_BLUE;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    character("b", d->pos.x, d->pos.y, &scratch);
    if (scratch.isColliding.character['a']) {
      play(COIN);
      addScore(sunflowerswaySunflower.height, stemTop.x, stemTop.y);
      sunflowerswaySunflower.height += 7;
      d->isAlive = false;
      continue;
    }
    if (d->pos.y > 90) {
      d->isAlive = false;
      continue;
    }
  }

  sunflowerswayNextInsectTicks--;
  if (sunflowerswayNextInsectTicks <= 0) {
    ASSIGN_ARRAY_ITEM(sunflowerswayInsects, sunflowerswayInsectIndex, SunflowerswayInsect, ni);
    if (rnd(0, 1) < 0.25) {
      vectorSet(&ni->pos, rnd(0, 20), -3);
    } else {
      vectorSet(&ni->pos, rnd(40, 99), -3);
    }
    ni->speed = rnd(0.3, 0.8) * difficulty;
    ni->isAlive = true;
    sunflowerswayInsectIndex = cgl_wrap(sunflowerswayInsectIndex + 1, 0, SUNFLOWERSWAY_MAX_INSECT_COUNT);
    sunflowerswayNextInsectTicks = rnd(60, 70) / difficulty;
  }
  FOR_EACH(sunflowerswayInsects, ii) {
    ASSIGN_ARRAY_ITEM(sunflowerswayInsects, ii, SunflowerswayInsect, ins);
    SKIP_IS_NOT_ALIVE(ins);
    ins->pos.y += ins->speed;
    color = RED;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    character("c", ins->pos.x, ins->pos.y, &scratch);
    if (scratch.isColliding.character['a']) {
      play(HIT);
      sunflowerswaySunflower.height -= 15;
      ins->isAlive = false;
      continue;
    }
    if (ins->pos.y > 90) {
      ins->isAlive = false;
      continue;
    }
  }
}

void addGameSunflowersway() {
  addGame(sunflowerswayTitle, sunflowerswayDescription, sunflowerswayCharacters,
          sunflowerswayCharactersCount, &sunflowerswayOptions, false, &sunflowerswayUpdate);
}
