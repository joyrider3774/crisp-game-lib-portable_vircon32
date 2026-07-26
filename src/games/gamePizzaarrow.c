#include "../cglp.h"

int* pizzaarrowTitle = "PIZZA ARROW";
int* pizzaarrowDescription = "[Hold]\n Pull\n[Release]\n Release";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] pizzaarrowCharacters = {{
    " r   r",
    "rrllll",
    " r   r",
}};
int pizzaarrowCharactersCount = 1;

Options pizzaarrowOptions = {100, 100, 6, false};

struct PizzaarrowPizza {
  float from;
  float to;
  float angle;
  float angleVel;
  float y;
};
PizzaarrowPizza pizzaarrowPizza;

struct PizzaarrowPart {
  float from;
  float to;
  float angle;
  Vector pos;
};
PizzaarrowPart pizzaarrowPart;
bool pizzaarrowPartActive;

struct PizzaarrowArrow {
  float x;
  float vx;
};
PizzaarrowArrow pizzaarrowArrow;
bool pizzaarrowArrowActive;

int pizzaarrowArrowCount;
int pizzaarrowNextArrowCount;
float pizzaarrowNextPizzaTicks;
float pizzaarrowGameSpeed;
int pizzaarrowMultiplier;

void pizzaarrowUpdate() {
  Collision scratch;
  if (!ticks) {
    pizzaarrowArrowActive = false;
    pizzaarrowArrowCount = 1;
    pizzaarrowNextArrowCount = 1;
    pizzaarrowNextPizzaTicks = 16;
    pizzaarrowGameSpeed = 1;
    pizzaarrowMultiplier = 1;
    pizzaarrowPartActive = false;
  }
  pizzaarrowNextPizzaTicks--;
  if (pizzaarrowNextPizzaTicks >= 0) {
    if (pizzaarrowNextPizzaTicks <= 15) {
      if (pizzaarrowNextPizzaTicks == 15) {
        pizzaarrowPizza.from = CGLP_PI / 4;
        pizzaarrowPizza.to = (CGLP_PI / 4) * 7;
        pizzaarrowPizza.angle = rnd(0, CGLP_PI / 2);
        pizzaarrowPizza.angleVel = 0.2;
        pizzaarrowPizza.y = 0;
      }
      pizzaarrowPizza.y = 50 - pizzaarrowNextPizzaTicks * 5;
      pizzaarrowMultiplier = 1;
    } else {
      pizzaarrowPizza.y = (30 - pizzaarrowNextPizzaTicks) * 5 + 50;
    }
  }
  color = YELLOW;
  Vector c;
  vectorSet(&c, 30, pizzaarrowPizza.y);
  float f = pizzaarrowPizza.from + pizzaarrowPizza.angle;
  float t = pizzaarrowPizza.to + pizzaarrowPizza.angle;
  thickness = 4;
  arc(c.x, c.y, 20, f, t, &scratch);
  Vector lf, lt;
  vectorSet(&lf, c.x, c.y);
  addWithAngle(&lf, f, 5);
  vectorSet(&lt, c.x, c.y);
  addWithAngle(&lt, f, 20);
  thickness = 4;
  line(lf.x, lf.y, lt.x, lt.y, &scratch);
  vectorSet(&lf, c.x, c.y);
  addWithAngle(&lf, t, 5);
  vectorSet(&lt, c.x, c.y);
  addWithAngle(&lt, t, 20);
  thickness = 4;
  line(lf.x, lf.y, lt.x, lt.y, &scratch);
  color = RED;
  thickness = 4;
  arc(30, pizzaarrowPizza.y, 5, f, t - CGLP_PI * 2, &scratch);
  pizzaarrowPizza.angle += pizzaarrowPizza.angleVel * pizzaarrowGameSpeed;
  if (pizzaarrowNextPizzaTicks < 0 && !pizzaarrowArrowActive && input.isPressed) {
    play(SELECT);
    pizzaarrowArrow.x = 80;
    pizzaarrowArrow.vx = 1;
    pizzaarrowArrowActive = true;
    pizzaarrowArrowCount--;
  }
  if (pizzaarrowNextPizzaTicks < 0) {
    color = BLACK;
    TIMES(pizzaarrowArrowCount, i) { character("a", 95, 40 - i * 3, &scratch); }
    int[16] multText;
    strcpy(multText, "x");
    strcat(multText, intToChar(pizzaarrowMultiplier));
    text(multText, 3, 10, &scratch);
  }
  if (pizzaarrowArrowActive) {
    if (input.isPressed) {
      pizzaarrowGameSpeed += (0.05 - pizzaarrowGameSpeed) * 0.1;
    }
    if (input.isJustReleased || pizzaarrowArrow.x > 90) {
      play(LASER);
      pizzaarrowArrow.vx = -5;
    }
    if (pizzaarrowArrow.vx < 0) {
      pizzaarrowGameSpeed += (1 - pizzaarrowGameSpeed) * 0.2;
    }
    if (pizzaarrowArrow.x > 70) {
      color = LIGHT_BLACK;
      thickness = 2;
      line(80, 30, pizzaarrowArrow.x + 4, 51, &scratch);
      thickness = 2;
      line(80, 70, pizzaarrowArrow.x + 4, 51, &scratch);
    }
    pizzaarrowArrow.x += pizzaarrowArrow.vx * pizzaarrowGameSpeed;
    color = BLACK;
    Collision ac;
    character("a", pizzaarrowArrow.x, 50, &ac);
    if (ac.isColliding.rect[YELLOW]) {
      play(HIT);
      float a = cgl_wrap(-pizzaarrowPizza.angle, 0, CGLP_PI * 2);
      if (a > pizzaarrowPizza.from && a < pizzaarrowPizza.to) {
        float sa;
        if (a - pizzaarrowPizza.from > pizzaarrowPizza.to - a) {
          sa = pizzaarrowPizza.to - a;
          pizzaarrowPart.from = a;
          pizzaarrowPart.to = pizzaarrowPizza.to;
          pizzaarrowPart.angle = pizzaarrowPizza.angle;
          vectorSet(&pizzaarrowPart.pos, 30, 50);
          pizzaarrowPizza.to = a;
        } else {
          sa = a - pizzaarrowPizza.from;
          pizzaarrowPart.from = pizzaarrowPizza.from;
          pizzaarrowPart.to = a;
          pizzaarrowPart.angle = pizzaarrowPizza.angle;
          vectorSet(&pizzaarrowPart.pos, 30, 50);
          pizzaarrowPizza.from = a;
        }
        pizzaarrowPartActive = true;
        play(COIN);
        addScore(ceil(sa * 100 * pizzaarrowMultiplier), 40, 50);
        pizzaarrowMultiplier++;
      }
      pizzaarrowArrowActive = false;
      if (pizzaarrowArrowCount == 0) {
        play(POWER_UP);
        pizzaarrowNextArrowCount++;
        pizzaarrowArrowCount = pizzaarrowNextArrowCount;
        pizzaarrowNextPizzaTicks = 30;
      }
    } else if (ac.isColliding.rect[RED]) {
      play(EXPLOSION);
      pizzaarrowArrowActive = false;
      gameOver();
    }
  }
  if (pizzaarrowPartActive) {
    vectorAdd(&pizzaarrowPart.pos, -5, 3);
    color = LIGHT_YELLOW;
    Vector pc;
    vectorSet(&pc, pizzaarrowPart.pos.x, pizzaarrowPart.pos.y);
    float pf = pizzaarrowPart.from + pizzaarrowPart.angle;
    float pt = pizzaarrowPart.to + pizzaarrowPart.angle;
    thickness = 4;
    arc(pc.x, pc.y, 20, pf, pt, &scratch);
    Vector plf, plt;
    vectorSet(&plf, pc.x, pc.y);
    addWithAngle(&plf, pf, 5);
    vectorSet(&plt, pc.x, pc.y);
    addWithAngle(&plt, pf, 20);
    thickness = 4;
    line(plf.x, plf.y, plt.x, plt.y, &scratch);
    vectorSet(&plf, pc.x, pc.y);
    addWithAngle(&plf, pt, 5);
    vectorSet(&plt, pc.x, pc.y);
    addWithAngle(&plt, pt, 20);
    thickness = 4;
    line(plf.x, plf.y, plt.x, plt.y, &scratch);
    if (pizzaarrowPart.pos.x < -20) {
      pizzaarrowPartActive = false;
    }
  }
}

void addGamePizzaarrow() {
  addGame(pizzaarrowTitle, pizzaarrowDescription, pizzaarrowCharacters,
          pizzaarrowCharactersCount, &pizzaarrowOptions, false,
          &pizzaarrowUpdate);
}
