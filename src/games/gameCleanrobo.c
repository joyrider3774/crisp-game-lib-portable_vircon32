#include "../cglp.h"

int* cleanroboTitle = "CLEAN ROBO";
int* cleanroboDescription = "[Hold]\n Speed up";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] cleanroboCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int cleanroboCharactersCount = 0;

Options cleanroboOptions = {200, 100, 9, false};

#define CLEANROBO_ROBOT_RADIUS 10

struct CleanroboWall {
  Vector[3] poss;
  float dist;
  float angle;
};
CleanroboWall[2] cleanroboWalls;

struct CleanroboRobot {
  Vector pos;
  float angle;
  float vx;
  float speed;
};
CleanroboRobot cleanroboRobot;

Vector cleanroboGarbage;
bool cleanroboHasGarbage;
Vector cleanroboLeftGarbage;
bool cleanroboHasLeftGarbage;
float cleanroboMultiplier;

void cleanroboSetWall(int index, bool isChangingAngle) {
  CleanroboWall* w = &cleanroboWalls[index];
  float a;
  if (isChangingAngle) {
    a = rnd(0, CGLP_PI / 7) * RNDPM();
  } else {
    a = w->angle;
  }
  float rr = CLEANROBO_ROBOT_RADIUS * 1.7;
  TIMES(3, i) {
    Vector* p = &w->poss[i];
    vectorSet(p, rr, 0);
    if (index == 0) {
      rotate(p, a + CGLP_PI - CGLP_PI * 2 / 3 * (i - 1));
      vectorAdd(p, 100 - w->dist, 50);
    } else {
      rotate(p, a - CGLP_PI * 2 / 3 * (-i + 1));
      vectorAdd(p, 100 + w->dist, 50);
    }
  }
  w->angle = a;
  Vector* p1 = &w->poss[1];
  cleanroboGarbage.x = p1->x - 7 * (index * 2 - 1);
  cleanroboGarbage.y = (p1->y - 50) * 0.5 + 50;
  cleanroboHasGarbage = true;
}

void cleanroboDrawRoom() {
  Collision scratch;
  thickness = 3;
  TIMES(2, wi) {
    CleanroboWall* w = &cleanroboWalls[wi];
    color = PURPLE;
    Vector* ps = w->poss;
    if (ps[1].x < 0 || ps[1].x > 199) {
      color = RED;
      play(RANDOM);
      gameOver();
    }
    line(ps[0].x, 0, ps[0].x, ps[0].y, &scratch);
    line(ps[0].x, ps[0].y, ps[1].x, ps[1].y, &scratch);
    line(ps[1].x, ps[1].y, ps[2].x, ps[2].y, &scratch);
    line(ps[2].x, ps[2].y, ps[2].x, 99, &scratch);
  }
}

void cleanroboUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&cleanroboRobot.pos, 90, 50);
    cleanroboRobot.angle = 0;
    cleanroboRobot.vx = 1;
    cleanroboRobot.speed = 1;
    // Vircon32 port note: JS recreates `walls` from scratch each game
    // (`walls = times(2, () => ({..., dist: 50, ...}))`), but this port's
    // cleanroboWalls is a persistent global that setWall() only ever
    // reads .dist from, never resets - without this, a replay would
    // start from whatever .dist gameplay had nudged it to by the end of
    // the previous game (clamped to [30,100], not always 50), which
    // could place a wall off-screen and trip the x<0||x>199 gameOver()
    // check in drawRoom() on the very first frame of the new game.
    cleanroboWalls[0].dist = 50;
    cleanroboWalls[1].dist = 50;
    cleanroboSetWall(0, true);
    cleanroboSetWall(1, true);
    cleanroboMultiplier = 1;
    cleanroboHasLeftGarbage = false;
  }
  cleanroboDrawRoom();
  float gs = 4;
  if (cleanroboHasLeftGarbage) {
    color = RED;
    box(cleanroboLeftGarbage.x, cleanroboLeftGarbage.y, gs, gs, &scratch);
  }
  if (cleanroboHasGarbage) {
    color = YELLOW;
    box(cleanroboGarbage.x, cleanroboGarbage.y, gs, gs, &scratch);
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    cleanroboRobot.speed += 0.3;
  } else {
    cleanroboRobot.angle += 0.05 * difficulty;
    cleanroboRobot.speed += (1 - cleanroboRobot.speed) * 0.2;
    if (cleanroboMultiplier > 1) {
      cleanroboMultiplier -= 0.1;
    }
  }
  cleanroboRobot.pos.x += cleanroboRobot.vx * cleanroboRobot.speed * difficulty * 0.2;
  bool icw = false;
  bool icg = false;
  color = LIGHT_BLACK;
  Vector[3] rps;
  TIMES(3, i) {
    vectorSet(&rps[i], CLEANROBO_ROBOT_RADIUS, 0);
    rotate(&rps[i], cleanroboRobot.angle + CGLP_PI * 2 / 3 * i);
    vectorAdd(&rps[i], cleanroboRobot.pos.x, cleanroboRobot.pos.y);
    thickness = 3;
    bar(rps[i].x, rps[i].y, 5, ticks * 0.7 + i, &scratch);
  }
  color = BLACK;
  TIMES(3, i) {
    int nextI = (i + 1) % 3;
    thickness = 3;
    line(rps[i].x, rps[i].y, rps[nextI].x, rps[nextI].y, &scratch);
    if (scratch.isColliding.rect[PURPLE]) {
      icw = true;
    }
    if (scratch.isColliding.rect[YELLOW]) {
      icg = true;
    }
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar((int)ceil(cleanroboMultiplier)));
  text(multText, 3, 9, &scratch);
  if (icg) {
    play(POWER_UP);
    addScore(ceil(cleanroboMultiplier), cleanroboGarbage.x, cleanroboGarbage.y);
    cleanroboMultiplier += 10;
    cleanroboHasGarbage = false;
  }
  if ((cleanroboRobot.pos.x - 100) * cleanroboRobot.vx > 0 && icw) {
    cleanroboRobot.vx *= -1;
    cleanroboHasLeftGarbage = false;
    if (cleanroboHasGarbage) {
      play(EXPLOSION);
      cleanroboLeftGarbage = cleanroboGarbage;
      cleanroboHasLeftGarbage = true;
      cleanroboRobot.speed = 1;
      color = RED;
      particle(cleanroboGarbage.x, cleanroboGarbage.y, 9, 1, 0, CGLP_PI * 2);
    } else {
      play(CLICK);
    }
    int wli = (int)((-cleanroboRobot.vx + 1) / 2);
    float distDelta;
    if (!cleanroboHasGarbage) {
      distDelta = -10;
    } else {
      distDelta = 20;
    }
    cleanroboWalls[wli].dist = clamp(cleanroboWalls[wli].dist + distDelta, 30, 100);
    cleanroboSetWall(wli, true);
    int wai = (int)((cleanroboRobot.vx + 1) / 2);
    cleanroboSetWall(wai, false);
  }
}

void addGameCleanrobo() {
  addGame(cleanroboTitle, cleanroboDescription, cleanroboCharacters,
          cleanroboCharactersCount, &cleanroboOptions, false, &cleanroboUpdate);
}
