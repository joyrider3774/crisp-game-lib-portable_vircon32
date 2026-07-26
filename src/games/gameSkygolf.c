#include "../cglp.h"

int* skygolfTitle = "SKY GOLF";
int* skygolfDescription = "[Hold]\n Adjust power\n[Release]\n Shoot";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] skygolfCharacters = {{
    "      ",
    " ll   ",
    "llll  ",
    "llll  ",
    " ll   ",
    "      ",
}};
int skygolfCharactersCount = 1;

// Upstream sets isShowingScore:false; this port has no such option, so the score UI always shows.
Options skygolfOptions = {150, 100, 1, false};

enum SkygolfState {
  SKYGOLF_STATE_TITLE,
  SKYGOLF_STATE_IN_GAME,
  SKYGOLF_STATE_GO_TO_NEXT_HOLE,
  SKYGOLF_STATE_GIVE_UP,
  SKYGOLF_STATE_HOLE_OUT
};
SkygolfState skygolfState;

enum SkygolfBallState {
  SKYGOLF_BALL_SHOT,
  SKYGOLF_BALL_POWER,
  SKYGOLF_BALL_FLY
};

struct SkygolfBall {
  Vector pos;
  Vector prevPos;
  Vector vel;
  float angle;
  float angleVel;
  float power;
  float basePower;
  float prevBasePower;
  SkygolfBallState state;
};
SkygolfBall skygolfBall;

enum SkygolfGroundType {
  SKYGOLF_GROUND_FAIRWAY,
  SKYGOLF_GROUND_SAND,
  SKYGOLF_GROUND_WATER,
  SKYGOLF_GROUND_TREE,
  SKYGOLF_GROUND_FLAG
};

// Argument to skygolfAddGround() - distinct from SkygolfGroundType, since
// "hole" resolves into a flag/fairway ground and never becomes a stored ground type itself.
enum SkygolfAddGroundType {
  SKYGOLF_ADD_GROUND_FAIRWAY,
  SKYGOLF_ADD_GROUND_SAND,
  SKYGOLF_ADD_GROUND_WATER,
  SKYGOLF_ADD_GROUND_TREE,
  SKYGOLF_ADD_GROUND_HOLE
};

struct SkygolfGround {
  SkygolfGroundType type;
  float height; // only meaningful when type == SKYGOLF_GROUND_TREE
};

#define SKYGOLF_MAX_GROUND_COUNT 25
struct SkygolfPlatform {
  Vector pos;
  SkygolfGround[SKYGOLF_MAX_GROUND_COUNT] grounds;
  int groundsCount;
};

#define SKYGOLF_MAX_PLATFORM_COUNT 3
SkygolfPlatform[SKYGOLF_MAX_PLATFORM_COUNT] skygolfPlatforms;
int skygolfPlatformCount;

struct SkygolfButton {
  Vector pos;
  Vector size;
  int* text;
};
SkygolfButton skygolfEasyButton;
SkygolfButton skygolfMediumButton;
SkygolfButton skygolfHardButton;

int skygolfBallCount;
int skygolfHoleCount;
int skygolfCourseDifficulty;
int skygolfInstructionTicks;
int skygolfHoleStartingTicks;
int skygolfCourseTime;
int skygolfGoToNextHoleTicks;
int skygolfGiveUpTicks;
int skygolfHoleOutTicks;

#define SKYGOLF_DIFFICULTY_COUNT 3
#define SKYGOLF_MAX_HOLE_COUNT 9
int[SKYGOLF_DIFFICULTY_COUNT][SKYGOLF_MAX_HOLE_COUNT] skygolfHoleSeeds = {
    {71, 45, 9, 0, 0, 0, 0, 0, 0},
    {49, 7, 98, 31, 54, 99, 0, 0, 0},
    {15, 4, 67, 5, 90, 53, 79, 85, 78},
};
int[SKYGOLF_DIFFICULTY_COUNT] skygolfHoleCounts = {3, 6, 9};

// Upstream's own from-scratch Random class is the same xorshift128 generator random.h
// already implements, so this reuses that API directly instead of duplicating it.
Random skygolfRandom;

void skygolfInitTitle();
void skygolfUpdateTitle();
void skygolfInitInGame(int difficultyLevel);
void skygolfUpdateInGame();
void skygolfDrawBallAndTime();
void skygolfGoToNextHole();
void skygolfInitBall();
void skygolfUpdateShotState();
void skygolfUpdatePowerState();
void skygolfUpdateFlyState();
void skygolfBackToPrevBallPos();
void skygolfInitBallShotState();
void skygolfDrawHole();
int skygolfGroundColor(SkygolfGroundType type);
void skygolfDrawGround(float x, float y, float w, SkygolfGroundType type);
void skygolfDrawTree(float x, float y, float h);
void skygolfDrawFlag(float x, float y);
void skygolfInitGoToNextHole();
void skygolfUpdateGoToNextHole();
void skygolfInitGiveUp();
void skygolfUpdateGiveUp();
void skygolfInitHoleOut();
void skygolfUpdateHoleOut();
void skygolfCreateHole(int seed);
void skygolfAddPlatform(Vector* pos, int width, bool hasHole, bool hasTeeing);
void skygolfAddGround(SkygolfGround* grounds, int groundsCount, SkygolfAddGroundType type);
void skygolfDrawTime(float timeVal, float x, float y);
void skygolfGetPaddedNumber(int* out, int v, int digit);
void skygolfDrawButton(SkygolfButton* b);
bool skygolfIsButtonClicked(SkygolfButton* b);

void skygolfDrawButton(SkygolfButton* b) {
  Collision scratch;
  color = WHITE;
  rect(b->pos.x, b->pos.y, b->size.x, b->size.y, &scratch);
  color = BLACK;
  text(b->text, b->pos.x + 2, b->pos.y + 1, &scratch);
}

bool skygolfIsButtonClicked(SkygolfButton* b) {
  return input.isJustPressed && input.pos.x >= b->pos.x &&
         input.pos.x <= b->pos.x + b->size.x && input.pos.y >= b->pos.y &&
         input.pos.y <= b->pos.y + b->size.y;
}

void skygolfGetPaddedNumber(int* out, int v, int digit) {
  int[16] buf;
  strcpy(buf, "0000");
  strcat(buf, intToChar(v));
  int len = strlen(buf);
  int start = len - digit;
  int i = 0;
  while (i < digit) {
    out[i] = buf[start + i];
    i++;
  }
  out[i] = 0;
}

void skygolfDrawTime(float timeVal, float x, float y) {
  Collision scratch;
  int t = (int)floor(timeVal * 100 / 50);
  if (t >= 10 * 60 * 100) {
    t = 10 * 60 * 100 - 1;
  }
  int minutes = t / 6000;
  int seconds = (t % 6000) / 100;
  int hundredths = t % 100;
  int[8] minutesText;
  int[8] secondsText;
  int[8] hundredthsText;
  skygolfGetPaddedNumber(minutesText, minutes, 1);
  skygolfGetPaddedNumber(secondsText, seconds, 2);
  skygolfGetPaddedNumber(hundredthsText, hundredths, 2);
  int[24] timeText;
  strcpy(timeText, minutesText);
  strcat(timeText, "'");
  strcat(timeText, secondsText);
  strcat(timeText, "\"");
  strcat(timeText, hundredthsText);
  text(timeText, x, y, &scratch);
}

int skygolfGroundColor(SkygolfGroundType type) {
  if (type == SKYGOLF_GROUND_SAND) {
    return YELLOW;
  } else if (type == SKYGOLF_GROUND_WATER) {
    return BLUE;
  } else if (type == SKYGOLF_GROUND_FLAG) {
    return WHITE;
  }
  return GREEN; // fairway or tree
}

void skygolfDrawGround(float x, float y, float w, SkygolfGroundType type) {
  Collision scratch;
  color = skygolfGroundColor(type);
  rect(x, y, w, -3, &scratch);
}

void skygolfDrawTree(float x, float y, float h) {
  Collision scratch;
  float h2 = floor(h / 2);
  color = RED;
  rect(x + 1, y, 3, -h2, &scratch);
  color = GREEN;
  rect(x, y - h2, 5, -h2, &scratch);
}

void skygolfDrawFlag(float x, float y) {
  Collision scratch;
  color = LIGHT_YELLOW;
  rect(x + 1, y, 2, -10, &scratch);
  color = LIGHT_RED;
  rect(x + 3, y - 6, 5, -4, &scratch);
}

void skygolfDrawHole() {
  Collision scratch;
  TIMES(skygolfPlatformCount, i) {
    SkygolfPlatform* p = &skygolfPlatforms[i];
    color = RED;
    rect(p->pos.x, p->pos.y, p->groundsCount * 6, -2, &scratch);
    SkygolfGroundType pgt = p->grounds[0].type;
    float x = p->pos.x;
    float bx = p->pos.x;
    TIMES(p->groundsCount, gi) {
      SkygolfGround* g = &p->grounds[gi];
      if (g->type != pgt) {
        skygolfDrawGround(bx, p->pos.y - 2, x - bx, pgt);
        bx = x;
        pgt = g->type;
      }
      if (g->type == SKYGOLF_GROUND_TREE) {
        skygolfDrawTree(x, p->pos.y - 5, g->height);
      } else if (g->type == SKYGOLF_GROUND_FLAG) {
        skygolfDrawFlag(x, p->pos.y - 5);
      }
      x += 6;
    }
    skygolfDrawGround(bx, p->pos.y - 2, x - bx, pgt);
  }
}

void skygolfAddGround(SkygolfGround* grounds, int groundsCount, SkygolfAddGroundType type) {
  int w;
  int x;
  if (type == SKYGOLF_ADD_GROUND_HOLE) {
    w = getIntRandom(&skygolfRandom, 3, 6);
    x = groundsCount - w - getIntRandom(&skygolfRandom, 0, 3);
  } else {
    w = getIntRandom(&skygolfRandom, 3, groundsCount / 2);
    x = getIntRandom(&skygolfRandom, 0, groundsCount - w);
  }
  int bh = getIntRandom(&skygolfRandom, 10, 20);
  TIMES(w, i) {
    if (type == SKYGOLF_ADD_GROUND_HOLE) {
      if (x + i == (int)floor(x + w / 2.0)) {
        grounds[x + i].type = SKYGOLF_GROUND_FLAG;
      } else {
        grounds[x + i].type = SKYGOLF_GROUND_FAIRWAY;
      }
    } else if (type == SKYGOLF_ADD_GROUND_TREE) {
      int height = (int)floor(bh + getIntRandom(&skygolfRandom, -5, 6));
      grounds[x + i].type = SKYGOLF_GROUND_TREE;
      grounds[x + i].height = height;
    } else if (type == SKYGOLF_ADD_GROUND_FAIRWAY) {
      grounds[x + i].type = SKYGOLF_GROUND_FAIRWAY;
    } else if (type == SKYGOLF_ADD_GROUND_SAND) {
      grounds[x + i].type = SKYGOLF_GROUND_SAND;
    } else if (type == SKYGOLF_ADD_GROUND_WATER) {
      grounds[x + i].type = SKYGOLF_GROUND_WATER;
    }
  }
}

void skygolfAddPlatform(Vector* pos, int width, bool hasHole, bool hasTeeing) {
  ASSIGN_ARRAY_ITEM(skygolfPlatforms, skygolfPlatformCount, SkygolfPlatform, plat);
  vectorSet(&plat->pos, pos->x, pos->y);
  plat->groundsCount = width;
  TIMES(width, i) {
    plat->grounds[i].type = SKYGOLF_GROUND_FAIRWAY;
    plat->grounds[i].height = 0;
  }
  skygolfAddGround(plat->grounds, width, SKYGOLF_ADD_GROUND_TREE);
  if (getRandom(&skygolfRandom, 0, 1) < 0.7) {
    skygolfAddGround(plat->grounds, width, SKYGOLF_ADD_GROUND_FAIRWAY);
  }
  if (getRandom(&skygolfRandom, 0, 1) < 0.8) {
    skygolfAddGround(plat->grounds, width, SKYGOLF_ADD_GROUND_SAND);
  }
  if (getRandom(&skygolfRandom, 0, 1) < 0.5) {
    skygolfAddGround(plat->grounds, width, SKYGOLF_ADD_GROUND_WATER);
  }
  if (hasHole) {
    skygolfAddGround(plat->grounds, width, SKYGOLF_ADD_GROUND_HOLE);
  }
  if (hasTeeing) {
    TIMES(5, i) {
      plat->grounds[i].type = SKYGOLF_GROUND_FAIRWAY;
      plat->grounds[i].height = 0;
    }
  }
  skygolfPlatformCount++;
}

void skygolfCreateHole(int seed) {
  setRandomSeed(&skygolfRandom, seed);
  skygolfPlatformCount = 0;
  int pc = getIntRandom(&skygolfRandom, 1, 3);
  int y = 90;
  int w = 25;
  if (getRandom(&skygolfRandom, 0, 1) < 0.5) {
    w = getIntRandom(&skygolfRandom, 12, 20);
    if (getRandom(&skygolfRandom, 0, 1) < 0.5) {
      y = getIntRandom(&skygolfRandom, 30, 70);
    }
  }
  skygolfBall.pos.y = y - 7;
  Vector pos0;
  vectorSet(&pos0, 0, y);
  skygolfAddPlatform(&pos0, w, false, true);
  TIMES(pc, i) {
    int pw = getIntRandom(&skygolfRandom, 9, 20);
    // Split into separate statements (unlike upstream's inline vec(getInt(),getInt())) since C's
    // argument evaluation order is unspecified and this must draw x before y to match the RNG stream.
    int platX = getIntRandom(&skygolfRandom, 0, 150 - pw * 6);
    int platY = getIntRandom(&skygolfRandom, 30, 70);
    Vector posN;
    vectorSet(&posN, platX, platY);
    bool isLast = i == pc - 1;
    skygolfAddPlatform(&posN, pw, isLast, false);
  }
}

void skygolfInitBall() {
  vectorSet(&skygolfBall.pos, 5, 83);
  vectorSet(&skygolfBall.prevPos, 0, 0);
  vectorSet(&skygolfBall.vel, 0, 0);
  skygolfBall.angle = 0;
  skygolfBall.angleVel = -1;
  skygolfBall.power = 0;
  skygolfBall.basePower = 1;
  skygolfBall.prevBasePower = 1;
  skygolfBall.state = SKYGOLF_BALL_SHOT;
}

void skygolfInitTitle() {
  skygolfState = SKYGOLF_STATE_TITLE;
  vectorSet(&skygolfEasyButton.pos, 15, 45);
  vectorSet(&skygolfEasyButton.size, 50, 7);
  skygolfEasyButton.text = "Easy";
  vectorSet(&skygolfMediumButton.pos, 15, 55);
  vectorSet(&skygolfMediumButton.size, 50, 7);
  skygolfMediumButton.text = "Medium";
  vectorSet(&skygolfHardButton.pos, 15, 65);
  vectorSet(&skygolfHardButton.size, 50, 7);
  skygolfHardButton.text = "Hard";
  skygolfInitBall();
  skygolfCreateHole(103);
}

void skygolfUpdateTitle() {
  Collision scratch;
  skygolfDrawHole();
  color = BLACK;
  text("SKY GOLF", 9, 38, &scratch);
  skygolfDrawButton(&skygolfEasyButton);
  text("3 holes", 75, 48, &scratch);
  skygolfDrawButton(&skygolfMediumButton);
  text("6 holes", 75, 58, &scratch);
  skygolfDrawButton(&skygolfHardButton);
  text("9 holes", 75, 68, &scratch);
  text("Click button to start", 20, 79, &scratch);
  if (skygolfIsButtonClicked(&skygolfEasyButton)) {
    skygolfInitInGame(0);
  } else if (skygolfIsButtonClicked(&skygolfMediumButton)) {
    skygolfInitInGame(1);
  } else if (skygolfIsButtonClicked(&skygolfHardButton)) {
    skygolfInitInGame(2);
  }
}

void skygolfInitBallShotState() {
  skygolfBall.state = SKYGOLF_BALL_SHOT;
  skygolfBall.power = 0.1;
  // Upstream starts a seeded procedural BGM here; this port has no such generator.
}

void skygolfGoToNextHole() {
  skygolfState = SKYGOLF_STATE_IN_GAME;
  skygolfInitBall();
  skygolfCreateHole(skygolfHoleSeeds[skygolfCourseDifficulty][skygolfHoleCount]);
  vectorSet(&skygolfBall.prevPos, skygolfBall.pos.x, skygolfBall.pos.y);
  skygolfHoleStartingTicks = 120;
  skygolfHoleCount++;
  skygolfBallCount += 5;
  skygolfInitBallShotState();
}

void skygolfInitInGame(int difficultyLevel) {
  skygolfCourseDifficulty = difficultyLevel;
  skygolfBallCount = 0;
  skygolfHoleCount = 0;
  skygolfCourseTime = 0;
  skygolfGoToNextHole();
}

void skygolfBackToPrevBallPos() {
  play(EXPLOSION);
  vectorSet(&skygolfBall.pos, skygolfBall.prevPos.x, skygolfBall.prevPos.y);
  skygolfBall.basePower = skygolfBall.prevBasePower;
}

void skygolfInitHoleOut() {
  skygolfState = SKYGOLF_STATE_HOLE_OUT;
  skygolfHoleOutTicks = 0;
  // Upstream plays a seeded victory jingle here; omitted (no procedural BGM in this port).
}

void skygolfInitGoToNextHole() {
  if (skygolfHoleCount == skygolfHoleCounts[skygolfCourseDifficulty]) {
    skygolfInitHoleOut();
    return;
  }
  // Upstream plays a seeded transition jingle here; omitted (no procedural BGM in this port).
  skygolfState = SKYGOLF_STATE_GO_TO_NEXT_HOLE;
  skygolfGoToNextHoleTicks = 0;
}

void skygolfInitGiveUp() {
  skygolfState = SKYGOLF_STATE_GIVE_UP;
  skygolfGiveUpTicks = 0;
}

void skygolfUpdateShotState() {
  Collision scratch;
  skygolfBall.angle += skygolfBall.angleVel * 0.05;
  if ((skygolfBall.angle < -CGLP_PI && skygolfBall.angleVel < 0) ||
      (skygolfBall.angle > 0 && skygolfBall.angleVel > 0)) {
    skygolfBall.angleVel = -skygolfBall.angleVel;
    skygolfBall.angle += skygolfBall.angleVel * 0.05 * 2;
  }
  color = LIGHT_BLACK;
  Vector tip;
  vectorSet(&tip, skygolfBall.pos.x, skygolfBall.pos.y);
  addWithAngle(&tip, skygolfBall.angle, 9);
  thickness = 2;
  line(skygolfBall.pos.x, skygolfBall.pos.y, tip.x, tip.y, &scratch);
  if (input.isJustPressed) {
    // Upstream stops the procedural BGM here; omitted (no procedural BGM in this port).
    play(SELECT);
    skygolfBall.state = SKYGOLF_BALL_POWER;
  }
}

void skygolfUpdatePowerState() {
  Collision scratch;
  skygolfBall.power += 0.2;
  color = LIGHT_BLACK;
  Vector tip;
  vectorSet(&tip, skygolfBall.pos.x, skygolfBall.pos.y);
  addWithAngle(&tip, skygolfBall.angle, skygolfBall.power);
  thickness = 2;
  line(skygolfBall.pos.x, skygolfBall.pos.y, tip.x, tip.y, &scratch);
  if (skygolfBall.power > 9 || input.isJustReleased) {
    play(LASER);
    vectorSet(&skygolfBall.vel, 0, 0);
    addWithAngle(&skygolfBall.vel, skygolfBall.angle, skygolfBall.power * 0.5 * skygolfBall.basePower);
    skygolfBall.state = SKYGOLF_BALL_FLY;
    skygolfBallCount--;
  }
}

void skygolfUpdateFlyState() {
  Vector p;
  Collision ch;
  Collision cv;
  color = TRANSPARENT;
  vectorSet(&p, skygolfBall.pos.x, skygolfBall.pos.y);
  vectorAdd(&p, skygolfBall.vel.x, 0);
  character("a", p.x, p.y, &ch);
  if (ch.isColliding.rect[RED] || ch.isColliding.rect[GREEN] ||
      ch.isColliding.rect[YELLOW] || ch.isColliding.rect[BLUE] ||
      (skygolfBall.vel.x < 0 && skygolfBall.pos.x < 2) ||
      (skygolfBall.vel.x > 0 && skygolfBall.pos.x > 148)) {
    skygolfBall.vel.x *= -0.8;
    skygolfBall.vel.y *= 0.8;
  }
  vectorSet(&p, skygolfBall.pos.x, skygolfBall.pos.y);
  vectorAdd(&p, 0, skygolfBall.vel.y);
  character("a", p.x, p.y, &cv);
  if (cv.isColliding.rect[RED] || cv.isColliding.rect[GREEN] ||
      cv.isColliding.rect[YELLOW] || cv.isColliding.rect[BLUE]) {
    float vr = 0.8;
    if (skygolfBall.vel.y > 0 && cv.isColliding.rect[BLUE]) {
      play(CLICK);
      vr = 0.4;
      color = BLUE;
      particle(skygolfBall.pos.x, skygolfBall.pos.y + 2, 3, 1, -CGLP_PI / 2, CGLP_PI / 4);
    } else if (skygolfBall.vel.y > 0 && cv.isColliding.rect[YELLOW]) {
      play(CLICK);
      vr = 0.5;
      color = YELLOW;
      particle(skygolfBall.pos.x, skygolfBall.pos.y + 2, 3, 1, -CGLP_PI / 2, CGLP_PI / 4);
    } else {
      play(HIT);
    }
    skygolfBall.vel.y *= -vr;
    skygolfBall.vel.x *= vr;
    if (skygolfBall.vel.y < 0 && vectorLength(&skygolfBall.vel) < 0.5) {
      if (cv.isColliding.rect[WHITE]) {
        skygolfInitGoToNextHole();
        return;
      } else if (skygolfBallCount <= 0) {
        skygolfInitGiveUp();
        return;
      }
      skygolfInitBallShotState();
      if (cv.isColliding.rect[YELLOW]) {
        skygolfBall.basePower = 0.5;
      } else {
        skygolfBall.basePower = 1;
      }
      if (cv.isColliding.rect[BLUE]) {
        color = BLUE;
        particle(skygolfBall.pos.x, skygolfBall.pos.y + 2, 9, 0.5, -CGLP_PI / 2, CGLP_PI / 2);
        skygolfBackToPrevBallPos();
      }
      vectorSet(&skygolfBall.prevPos, skygolfBall.pos.x, skygolfBall.pos.y);
      skygolfBall.prevBasePower = skygolfBall.basePower;
      return;
    }
  }
  vectorAdd(&skygolfBall.pos, skygolfBall.vel.x, skygolfBall.vel.y);
  vectorMul(&skygolfBall.vel, 0.98);
  skygolfBall.vel.y += 0.1;
  if (skygolfBall.pos.y > 110) {
    if (skygolfBallCount <= 0) {
      skygolfInitGiveUp();
      return;
    }
    skygolfBackToPrevBallPos();
    skygolfInitBallShotState();
  }
}

void skygolfDrawBallAndTime() {
  Collision scratch;
  color = BLACK;
  character("a", 3, 4, &scratch);
  int[16] ballText;
  strcpy(ballText, "x");
  strcat(ballText, intToChar(skygolfBallCount));
  text(ballText, 9, 3, &scratch);
  skygolfDrawTime((float)skygolfCourseTime, 110, 3);
}

void skygolfUpdateInGame() {
  Collision scratch;
  skygolfDrawHole();
  if (skygolfBall.state == SKYGOLF_BALL_SHOT && skygolfBall.basePower < 1) {
    color = YELLOW;
  } else {
    color = BLACK;
  }
  character("a", skygolfBall.pos.x, skygolfBall.pos.y, &scratch);
  if (skygolfBall.state == SKYGOLF_BALL_SHOT) {
    skygolfUpdateShotState();
  } else if (skygolfBall.state == SKYGOLF_BALL_POWER) {
    // Upstream stops the procedural BGM here; omitted (no procedural BGM in this port).
    skygolfUpdatePowerState();
  } else if (skygolfBall.state == SKYGOLF_BALL_FLY) {
    skygolfUpdateFlyState();
  }
  color = BLACK;
  if (skygolfInstructionTicks > 0) {
    skygolfInstructionTicks--;
    text("[Hold] to adjust power", 20, 60, &scratch);
    text("[Release] to shoot", 20, 68, &scratch);
  }
  if (skygolfHoleStartingTicks > 0) {
    skygolfHoleStartingTicks--;
    int[16] holeText;
    strcpy(holeText, "HOLE ");
    strcat(holeText, intToChar(skygolfHoleCount));
    text(holeText, 10, 95, &scratch);
  }
  skygolfCourseTime++;
  skygolfDrawBallAndTime();
}

void skygolfUpdateGoToNextHole() {
  Collision scratch;
  skygolfDrawHole();
  color = BLACK;
  text("GO TO NEXT HOLE", 30, 50, &scratch);
  skygolfDrawBallAndTime();
  skygolfGoToNextHoleTicks++;
  if (skygolfGoToNextHoleTicks > 150 || input.isJustPressed) {
    skygolfGoToNextHole();
  }
}

void skygolfUpdateGiveUp() {
  Collision scratch;
  skygolfDrawHole();
  color = BLACK;
  text("GIVE UP", 20, 50, &scratch);
  skygolfGiveUpTicks++;
  if (skygolfGiveUpTicks > 300 || input.isJustPressed) {
    skygolfInitTitle();
  }
}

void skygolfUpdateHoleOut() {
  Collision scratch;
  skygolfDrawHole();
  color = BLACK;
  text("HOLE OUT!", 70, 50, &scratch);
  skygolfDrawBallAndTime();
  skygolfHoleOutTicks++;
  if (skygolfHoleOutTicks > 600 || input.isJustPressed) {
    skygolfInitTitle();
  }
}

void skygolfUpdate() {
  if (!ticks) {
    // Upstream's document.title = "SKY GOLF" was a browser tab-title side effect;
    // skygolfTitle already carries the same string for this console's title screen.
    skygolfInstructionTicks = 200;
    skygolfInitTitle();
  }
  if (skygolfState == SKYGOLF_STATE_TITLE) {
    skygolfUpdateTitle();
  } else if (skygolfState == SKYGOLF_STATE_IN_GAME) {
    skygolfUpdateInGame();
  } else if (skygolfState == SKYGOLF_STATE_GO_TO_NEXT_HOLE) {
    skygolfUpdateGoToNextHole();
  } else if (skygolfState == SKYGOLF_STATE_GIVE_UP) {
    skygolfUpdateGiveUp();
  } else if (skygolfState == SKYGOLF_STATE_HOLE_OUT) {
    skygolfUpdateHoleOut();
  }
}

// usesMouse=true: only the title screen's difficulty buttons need input.pos; gameplay itself doesn't.
void addGameSkygolf() {
  addGame(skygolfTitle, skygolfDescription, skygolfCharacters,
          skygolfCharactersCount, &skygolfOptions, true, &skygolfUpdate);
}
