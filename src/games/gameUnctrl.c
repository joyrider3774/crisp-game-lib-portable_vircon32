#include "../cglp.h"

int* unctrlTitle = "UNCTRL";
int* unctrlDescription = "[Tap]  Fire\n[Hold] Go up";

// The original sprites are 7px wide ("l ll ll" etc.); this engine's
// CHARACTER_WIDTH is fixed at 6, so the first/last rows are truncated by
// one column (dropping the trailing tread pixel) to fit.
int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] unctrlCharacters = {
    {
        "l ll l",
        " lll  ",
        "l  lll",
        "l  lll",
        " lll  ",
        "l ll l",
    },
    {
        " ll ll",
        " lll  ",
        "l  lll",
        "l  lll",
        " lll  ",
        " ll ll",
    },
    {
        "ll ll ",
        " lll  ",
        "l  lll",
        "l  lll",
        " lll  ",
        "ll ll ",
    },
};
int unctrlCharactersCount = 3;

Options unctrlOptions = {150, 100, 15, true};

struct UnctrlTank {
  Vector pos;
  float bulletAngle;
  float fireTicks;
  float animTicks;
  bool isAlive;
};
#define UNCTRL_MAX_TANK_COUNT 32
UnctrlTank[UNCTRL_MAX_TANK_COUNT] unctrlTanks;
int unctrlTankIndex;
float unctrlNextTankTicks;

struct UnctrlBullet {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define UNCTRL_MAX_BULLET_COUNT 32
UnctrlBullet[UNCTRL_MAX_BULLET_COUNT] unctrlBullets;
int unctrlBulletIndex;

#define UNCTRL_SHOT_READY 0
#define UNCTRL_SHOT_FIRED 1
struct UnctrlShot {
  Vector pos;
  Vector vel;
  int state;
};
UnctrlShot unctrlShot;

float unctrlAnimTicks;
int unctrlMultiplier;

#define UNCTRL_GROUND_COUNT 20
Vector[UNCTRL_GROUND_COUNT] unctrlGrounds;

void unctrlSetNextShot() {
  unctrlShot.pos.x = 8;
  unctrlShot.pos.y = 50;
  vectorSet(&unctrlShot.vel, 0.5, 0.2);
  unctrlShot.state = UNCTRL_SHOT_READY;
  unctrlMultiplier = 1;
}

void unctrlUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(unctrlTanks);
    unctrlTankIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(unctrlBullets);
    unctrlBulletIndex = 0;
    unctrlNextTankTicks = 0;
    vectorSet(&unctrlShot.pos, 0, 0);
    vectorSet(&unctrlShot.vel, 0, 0);
    unctrlShot.state = UNCTRL_SHOT_READY;
    unctrlAnimTicks = 0;
    unctrlSetNextShot();
    TIMES(UNCTRL_GROUND_COUNT, i) {
      vectorSet(&unctrlGrounds[i], rnd(0, 150), rnd(0, 100));
    }
  }
  color = LIGHT_PURPLE;
  TIMES(UNCTRL_GROUND_COUNT, i) {
    box(unctrlGrounds[i].x, unctrlGrounds[i].y, 5, 2, &scratch);
    unctrlGrounds[i].x = cgl_wrap(unctrlGrounds[i].x - 0.1 * sqrt(difficulty), -3, 153);
  }
  if (unctrlShot.state == UNCTRL_SHOT_READY) {
    color = LIGHT_BLUE;
  } else {
    color = BLUE;
  }
  if (unctrlShot.state == UNCTRL_SHOT_READY) {
    if (input.isJustPressed) {
      play(SELECT);
      unctrlShot.state = UNCTRL_SHOT_FIRED;
    }
  } else {
    if (input.isJustPressed) {
      play(HIT);
    }
    if (input.isJustReleased) {
      play(LASER);
    }
    if (input.isPressed) {
      unctrlShot.vel.y += -0.1;
    } else {
      unctrlShot.vel.y += 0.1;
    }
    vectorAdd(&unctrlShot.pos, unctrlShot.vel.x, unctrlShot.vel.y);
    float shotAngle = vectorAngle(&unctrlShot.vel);
    particle(unctrlShot.pos.x, unctrlShot.pos.y, 1, 1, shotAngle + CGLP_PI, 1);
    bool shotInRect = unctrlShot.pos.x >= 0 && unctrlShot.pos.x < 150 &&
                       unctrlShot.pos.y >= 0 && unctrlShot.pos.y < 100;
    if (!shotInRect) {
      unctrlSetNextShot();
    }
  }
  thickness = 4;
  barCenterPosRatio = -0.5;
  bar(unctrlShot.pos.x, unctrlShot.pos.y, 6, vectorAngle(&unctrlShot.vel), &scratch);
  COUNT_IS_ALIVE(unctrlTanks, aliveTankCount);
  if (aliveTankCount == 0) {
    unctrlNextTankTicks = 0;
  }
  unctrlNextTankTicks--;
  if (unctrlNextTankTicks < 0) {
    ASSIGN_ARRAY_ITEM(unctrlTanks, unctrlTankIndex, UnctrlTank, nt);
    vectorSet(&nt->pos, 153, rnd(20, 40) * RNDPM() + 50);
    nt->bulletAngle = rnd(0, CGLP_PI * 0.3) * RNDPM() + CGLP_PI;
    nt->fireTicks = rnd(0, 60);
    nt->animTicks = 0;
    nt->isAlive = true;
    unctrlTankIndex = cgl_wrap(unctrlTankIndex + 1, 0, UNCTRL_MAX_TANK_COUNT);
    unctrlNextTankTicks = 200 / sqrt(difficulty);
  }
  FOR_EACH(unctrlTanks, i) {
    ASSIGN_ARRAY_ITEM(unctrlTanks, i, UnctrlTank, t);
    SKIP_IS_NOT_ALIVE(t);
    float animMul;
    if (t->pos.x < 70) {
      animMul = 4;
    } else {
      animMul = 1;
    }
    t->animTicks += sqrt(difficulty) * animMul;
    float moveMul;
    if (t->pos.x < 70) {
      moveMul = 0.4;
    } else {
      moveMul = 0.1;
    }
    t->pos.x -= sqrt(difficulty) * moveMul;
    color = RED;
    int[2] tc;
    tc[0] = 'a' + (int)floor(t->animTicks / 20) % 3;
    tc[1] = 0;
    characterOptions.isMirrorX = true;
    Collision tcoll;
    character(tc, t->pos.x, t->pos.y, &tcoll);
    characterOptions.isMirrorX = false;
    if (tcoll.isColliding.rect[BLUE]) {
      play(POWER_UP);
      particle(t->pos.x, t->pos.y, 19, 3, 0, CGLP_PI * 2);
      addScore(unctrlMultiplier * 10, t->pos.x, t->pos.y);
      unctrlSetNextShot();
      t->isAlive = false;
      continue;
    }
    t->fireTicks -= difficulty;
    if (t->pos.x > 70) {
      if (t->fireTicks < 0) {
        ASSIGN_ARRAY_ITEM(unctrlBullets, unctrlBulletIndex, UnctrlBullet, nb);
        vectorSet(&nb->pos, t->pos.x - 3, t->pos.y);
        vectorSet(&nb->vel, 0, 0);
        addWithAngle(&nb->vel, t->bulletAngle, 0.3 * difficulty);
        nb->isAlive = true;
        unctrlBulletIndex = cgl_wrap(unctrlBulletIndex + 1, 0, UNCTRL_MAX_BULLET_COUNT);
        t->fireTicks = 60;
        t->bulletAngle = rnd(0, CGLP_PI * 0.3) * RNDPM() + CGLP_PI;
      } else {
        color = LIGHT_RED;
        thickness = 2;
        barCenterPosRatio = -0.5;
        bar(t->pos.x - 3, t->pos.y, 4, t->bulletAngle, &scratch);
      }
    }
    t->isAlive = t->pos.x >= -3;
  }
  color = RED;
  FOR_EACH(unctrlBullets, i) {
    ASSIGN_ARRAY_ITEM(unctrlBullets, i, UnctrlBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    thickness = 2;
    barCenterPosRatio = -0.5;
    Collision bc;
    bar(b->pos.x, b->pos.y, 4, vectorAngle(&b->vel), &bc);
    if (bc.isColliding.rect[BLUE]) {
      play(COIN);
      addScore(unctrlMultiplier, b->pos.x, b->pos.y);
      particle(b->pos.x, b->pos.y, 9, 1, 0, CGLP_PI * 2);
      if (unctrlMultiplier < 64) {
        unctrlMultiplier *= 2;
      }
      b->isAlive = false;
      continue;
    }
    bool bInRect = b->pos.x >= 0 && b->pos.x < 150 && b->pos.y >= 0 && b->pos.y < 100;
    b->isAlive = bInRect;
  }
  color = BLUE;
  unctrlAnimTicks += difficulty;
  int[2] pc;
  pc[0] = 'a' + (int)floor(unctrlAnimTicks / 20) % 3;
  pc[1] = 0;
  Collision pcoll;
  character(pc, 10, 50, &pcoll);
  if (pcoll.isColliding.rect[RED]) {
    play(RANDOM);
    gameOver();
  }
}

void addGameUnctrl() {
  addGame(unctrlTitle, unctrlDescription, unctrlCharacters,
          unctrlCharactersCount, &unctrlOptions, false, &unctrlUpdate);
}
