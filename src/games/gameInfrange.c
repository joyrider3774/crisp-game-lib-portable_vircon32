#include "../cglp.h"

int* infrangeTitle = "INF RANGE";
int* infrangeDescription = "[Tap]  Turn 90\n[Hold] Turn slow";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] infrangeCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int infrangeCharactersCount = 1;

Options infrangeOptions = {100, 100, 2, true};

struct InfrangeArrow {
  Vector pos;
  float angle;
  float angleVel;
  float speed;
  bool isAlive;
};
// Arrows only die on a mismatched-angle hit against the ship; a clean/avoided
// pass leaves them bouncing forever while spawn interval keeps shrinking with
// difficulty, so count grows roughly unbounded (~16 reached within a minute
// of ordinary play) - raise the cap well past that for long sessions.
#define INFRANGE_MAX_ARROW_COUNT 1024
InfrangeArrow[INFRANGE_MAX_ARROW_COUNT] infrangeArrows;
int infrangeArrowIndex;
float infrangeNextArrowTicks;

Vector infrangePos;
float infrangeAngle;
float infrangeSpeed;
float infrangeTapAngle;
float infrangeTapTicks;
Vector infrangeScr;
float infrangeMultiplier;

struct InfrangeLine {
  Vector pos;
  Vector size;
};
#define INFRANGE_LINE_COUNT 10
InfrangeLine[INFRANGE_LINE_COUNT] infrangeLines;

bool infrangeCheckCollision(InfrangeArrow* a) {
  if (fabs(cgl_wrap(infrangeAngle + CGLP_PI - a->angle, -CGLP_PI, CGLP_PI)) < 0.7) {
    a->angle += CGLP_PI;
    addWithAngle(&a->pos, a->angle, a->speed * 2);
    infrangeAngle += CGLP_PI;
    infrangeSpeed = 1;
    return true;
  } else {
    return false;
  }
}

void infrangeUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(infrangeArrows);
    infrangeArrowIndex = 0;
    infrangeNextArrowTicks = 200;
    vectorSet(&infrangePos, 50, 80);
    infrangeAngle = -CGLP_PI / 2;
    infrangeTapAngle = -CGLP_PI / 2;
    infrangeSpeed = 1;
    infrangeTapTicks = -99;
    vectorSet(&infrangeScr, 0, 0);
    infrangeMultiplier = 10.9;
    TIMES(5, i) {
      vectorSet(&infrangeLines[i].pos, 50, i * 20);
      vectorSet(&infrangeLines[i].size, 100, 1);
    }
    TIMES(5, i) {
      vectorSet(&infrangeLines[5 + i].pos, i * 20, 50);
      vectorSet(&infrangeLines[5 + i].size, 1, 100);
    }
  }
  addWithAngle(&infrangePos, infrangeAngle, infrangeSpeed);
  if (infrangePos.x < 40) {
    infrangeScr.x = (40 - infrangePos.x) * 0.2;
  } else if (infrangePos.x > 60) {
    infrangeScr.x = (60 - infrangePos.x) * 0.2;
  }
  if (infrangePos.y < 40) {
    infrangeScr.y = (40 - infrangePos.y) * 0.2;
  } else if (infrangePos.y > 60) {
    infrangeScr.y = (60 - infrangePos.y) * 0.2;
  }
  color = LIGHT_GREEN;
  TIMES(5, i) {
    InfrangeLine* l = &infrangeLines[i];
    l->pos.y = cgl_wrap(l->pos.y + infrangeScr.y, 0, 100);
    box(l->pos.x, l->pos.y, l->size.x, l->size.y, &scratch);
  }
  for (int i = 5; i < 10; i++) {
    InfrangeLine* l = &infrangeLines[i];
    l->pos.x = cgl_wrap(l->pos.x + infrangeScr.x, 0, 100);
    box(l->pos.x, l->pos.y, l->size.x, l->size.y, &scratch);
  }
  vectorAdd(&infrangePos, infrangeScr.x, infrangeScr.y);
  color = BLUE;
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(infrangePos.x, infrangePos.y, 5, infrangeAngle, &scratch);
  particle(infrangePos.x, infrangePos.y, 1, infrangeSpeed, infrangeAngle + CGLP_PI, 0.2);
  color = GREEN;
  Vector bp;
  vectorSet(&bp, infrangePos.x, infrangePos.y);
  addWithAngle(&bp, infrangeAngle, 4);
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(bp.x, bp.y, 3, infrangeAngle + CGLP_PI / 2, &scratch);
  if (input.isJustPressed) {
    play(LASER);
    infrangeTapAngle = infrangeAngle;
    infrangeTapTicks = ticks;
  }
  if (input.isPressed) {
    infrangeAngle += 0.02 * difficulty;
    infrangeSpeed += (1 - infrangeSpeed) * 0.1;
  } else {
    infrangeSpeed += (difficulty - infrangeSpeed) * 0.05;
  }
  if (input.isJustReleased) {
    if (ticks - infrangeTapTicks < 9) {
      play(SELECT);
      infrangeAngle = infrangeTapAngle + CGLP_PI / 2;
    }
  }
  FOR_EACH(infrangeArrows, i) {
    ASSIGN_ARRAY_ITEM(infrangeArrows, i, InfrangeArrow, a);
    SKIP_IS_NOT_ALIVE(a);
    a->angle += a->angleVel;
    addWithAngle(&a->pos, a->angle, a->speed);
    vectorAdd(&a->pos, infrangeScr.x, infrangeScr.y);
    a->pos.x = cgl_wrap(a->pos.x, -3, 103);
    a->pos.y = cgl_wrap(a->pos.y, -3, 103);
    color = RED;
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision ac;
    bar(a->pos.x, a->pos.y, 5, a->angle, &ac);
    if (ac.isColliding.rect[BLUE] || ac.isColliding.rect[GREEN]) {
      if (!infrangeCheckCollision(a)) {
        play(POWER_UP);
        particle(a->pos.x, a->pos.y, 9, (infrangeSpeed + a->speed) * 2, 0, CGLP_PI * 2);
        addScore(floor(infrangeMultiplier), a->pos.x, a->pos.y);
        infrangeMultiplier += 10;
        a->isAlive = false;
        continue;
      }
    }
    if (rnd(0, 1) < 0.2) {
      particle(a->pos.x, a->pos.y, 1, a->speed, a->angle + CGLP_PI, 0.1);
    }
    color = YELLOW;
    Vector abp;
    vectorSet(&abp, a->pos.x, a->pos.y);
    addWithAngle(&abp, a->angle, 4);
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision abc;
    bar(abp.x, abp.y, 2, a->angle + CGLP_PI / 2, &abc);
    if (abc.isColliding.rect[BLUE]) {
      if (!infrangeCheckCollision(a)) {
        play(RANDOM);
        gameOver();
      }
    }
  }
  COUNT_IS_ALIVE(infrangeArrows, aliveArrowCount);
  if (aliveArrowCount == 0) {
    infrangeNextArrowTicks = 0;
  }
  infrangeNextArrowTicks -= sqrt(difficulty);
  if (infrangeNextArrowTicks < 0) {
    Vector p;
    bool isApp = false;
    TIMES(9, k) {
      vectorSet(&p, rnd(10, 90), rnd(10, 90));
      if (fabs(cgl_wrap(infrangePos.x - p.x, -50, 50)) +
              fabs(cgl_wrap(infrangePos.y - p.y, -50, 50)) >
          36) {
        isApp = true;
        break;
      }
    }
    if (isApp) {
      play(HIT);
      ASSIGN_ARRAY_ITEM(infrangeArrows, infrangeArrowIndex, InfrangeArrow, na);
      na->pos = p;
      na->angle = rnd(0, CGLP_PI * 2);
      na->angleVel = rnd(0, sqrt(difficulty - 1) * 0.05) * RNDPM();
      na->speed = rnd(1, difficulty) / 2;
      na->isAlive = true;
      infrangeArrowIndex = cgl_wrap(infrangeArrowIndex + 1, 0, INFRANGE_MAX_ARROW_COUNT);
      infrangeNextArrowTicks = rnd(150, 250);
    }
  }
  infrangeMultiplier -= 0.02;
  if (infrangeMultiplier < 10.9) {
    infrangeMultiplier = 10.9;
  }
  color = BLACK;
  int[16] scoreText;
  strcpy(scoreText, "+");
  strcat(scoreText, intToChar((int)floor(infrangeMultiplier)));
  text(scoreText, 3, 97, &scratch);
}

void addGameInfrange() {
  addGame(infrangeTitle, infrangeDescription, infrangeCharacters,
          infrangeCharactersCount, &infrangeOptions, false, &infrangeUpdate);
}
