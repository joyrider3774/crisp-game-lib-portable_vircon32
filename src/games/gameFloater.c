#include "../cglp.h"

int* floaterTitle = "FLOATER";
int* floaterDescription = "[Tap] Jump\n[Hold] Fly";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] floaterCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int floaterCharactersCount = 0;

Options floaterOptions = {100, 100, 1, false};

struct FloaterFloater {
  Vector cp;
  Vector p;
  float a;
  float r;
  float v;
  bool isValid;
  bool isAlive;
};
#define FLOATER_MAX_FLOATER_COUNT 32
FloaterFloater[FLOATER_MAX_FLOATER_COUNT] floaterFloaters;
int floaterFloaterIndex;
float floaterAddDist;
float floaterFy;

struct FloaterPlayer {
  Vector pos;
  Vector vel;
  bool hasOn;
  int onIndex;
};
FloaterPlayer floaterPlayer;

void floaterUpdate() {
  Collision scratch;
  if (!ticks) {
    floaterFy = 50;
    INIT_UNALIVED_ARRAY_FAST(floaterFloaters);
    floaterFloaterIndex = 0;
    vectorSet(&floaterPlayer.pos, 105, 10);
    vectorSet(&floaterPlayer.vel, 0, 0);
    floaterPlayer.hasOn = false;
    floaterAddDist = 0;
  }
  if (floaterAddDist <= 0) {
    float r = rnd(10, 20);
    if (floaterFy < 30 + r) {
      floaterFy = 30 + r + (30 + r - floaterFy);
    } else if (floaterFy > 80 - r) {
      floaterFy = 80 - r - (floaterFy - (80 - r));
    }
    ASSIGN_ARRAY_ITEM(floaterFloaters, floaterFloaterIndex, FloaterFloater, nf);
    vectorSet(&nf->cp, 105, floaterFy);
    vectorSet(&nf->p, 0, 0);
    nf->a = rnd(0, CGLP_PI * 2);
    nf->r = r;
    nf->v = rnd(0.05, 0.1) * difficulty;
    nf->isValid = true;
    nf->isAlive = true;
    floaterFloaterIndex = cgl_wrap(floaterFloaterIndex + 1, 0, FLOATER_MAX_FLOATER_COUNT);
    floaterFy += rnd(0, 20) * RNDPM();
    floaterAddDist += rnd(20, 40);
  }
  float sc = difficulty * 0.1;
  if (floaterPlayer.pos.x > 30) {
    sc += (floaterPlayer.pos.x - 30) * 0.05;
  }
  floaterAddDist -= sc;
  color = LIGHT_BLACK;
  rect(0, 0, 99, 5, &scratch);
  FOR_EACH(floaterFloaters, i) {
    ASSIGN_ARRAY_ITEM(floaterFloaters, i, FloaterFloater, f);
    SKIP_IS_NOT_ALIVE(f);
    f->cp.x -= sc;
    if (f->cp.x < -10) {
      f->isAlive = false;
      continue;
    }
    if (f->isValid) {
      color = BLUE;
    } else {
      color = LIGHT_BLUE;
    }
    vectorSet(&f->p, f->cp.x, f->cp.y + sin(f->a) * f->r);
    box(f->p.x, f->p.y, 15, 5, &scratch);
    f->a += f->v;
  }
  floaterPlayer.pos.x -= sc;
  addScore(sc, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  if (floaterPlayer.hasOn) {
    floaterPlayer.pos.y = floaterFloaters[floaterPlayer.onIndex].p.y - 6;
  } else {
    if (input.isPressed) {
      floaterPlayer.vel.y += 0.05;
    } else {
      floaterPlayer.vel.y += 0.2;
    }
    vectorAdd(&floaterPlayer.pos, floaterPlayer.vel.x, floaterPlayer.vel.y);
  }
  color = GREEN;
  box(floaterPlayer.pos.x, floaterPlayer.pos.y, 7, 7, &scratch);
  if (floaterPlayer.hasOn) {
    if (input.isJustPressed) {
      FloaterFloater* on = &floaterFloaters[floaterPlayer.onIndex];
      floaterPlayer.vel.x = 1;
      floaterPlayer.vel.y = cos(on->a) * on->v * 20 - 1;
      on->isValid = false;
      floaterPlayer.hasOn = false;
      play(JUMP);
    }
  } else {
    if (scratch.isColliding.rect[LIGHT_BLACK] && floaterPlayer.vel.y < 0) {
      floaterPlayer.vel.y *= -0.25;
    }
    if (scratch.isColliding.rect[BLUE]) {
      play(LASER);
      FOR_EACH(floaterFloaters, i) {
        ASSIGN_ARRAY_ITEM(floaterFloaters, i, FloaterFloater, f);
        SKIP_IS_NOT_ALIVE(f);
        if (fabs(f->p.x - floaterPlayer.pos.x) < 12) {
          floaterPlayer.hasOn = true;
          floaterPlayer.onIndex = i;
        }
      }
    }
  }
  if (floaterPlayer.pos.y > 99 || floaterPlayer.pos.x < 0) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameFloater() {
  addGame(floaterTitle, floaterDescription, floaterCharacters,
          floaterCharactersCount, &floaterOptions, false, &floaterUpdate);
}
