#include "../cglp.h"

int* linebTitle = "LINE B";
int* linebDescription = "[Tap]\n Grow";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] linebCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int linebCharactersCount = 1;

Options linebOptions = {100, 100, 30, false};

struct LinebPoint {
  Vector pos;
  Vector vel;
};
#define LINEB_POINT_COUNT 2
LinebPoint[LINEB_POINT_COUNT] linebPoints;

struct LinebHistory {
  Vector p1;
  Vector p2;
};
#define LINEB_MAX_HISTORY_COUNT 48
LinebHistory[LINEB_MAX_HISTORY_COUNT] linebHistory;
int linebHistoryCount;

bool linebGoldActive;
Vector linebGoldPos;
float linebGoldRadius;

void linebUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(LINEB_POINT_COUNT, i) {
      vectorSet(&linebPoints[i].pos, 50, 50);
      vectorSet(&linebPoints[i].vel, 1, 0);
      rotate(&linebPoints[i].vel, rnd(0, CGLP_PI / 3) * RNDPM() + i * CGLP_PI);
    }
    linebHistoryCount = 0;
    linebGoldActive = false;
  }
  if (!linebGoldActive) {
    color = WHITE;
    thickness = 20;
    line(linebPoints[0].pos.x, linebPoints[0].pos.y, linebPoints[1].pos.x,
         linebPoints[1].pos.y, &scratch);
    Vector pos;
    color = TRANSPARENT;
    TIMES(99, i) {
      vectorSet(&pos, rnd(20, 80), rnd(20, 80));
      Collision bc;
      box(pos.x, pos.y, 20, 20, &bc);
      if (!bc.isColliding.rect[WHITE]) {
        break;
      }
    }
    linebGoldPos = pos;
    linebGoldRadius = 1;
    linebGoldActive = true;
  }
  color = LIGHT_BLACK;
  TIMES(linebHistoryCount, i) {
    if (i % 9 == 8) {
      thickness = clamp(4 - ceil(i / 9), 1, 3);
      line(linebHistory[i].p1.x, linebHistory[i].p1.y, linebHistory[i].p2.x,
           linebHistory[i].p2.y, &scratch);
    }
  }
  color = LIGHT_PURPLE;
  rect(0, 0, 100, 3, &scratch);
  rect(0, 97, 100, 3, &scratch);
  rect(0, 0, 3, 100, &scratch);
  rect(97, 0, 3, 100, &scratch);
  color = YELLOW;
  bool isHittingWall = false;
  Collision arcc;
  thickness = 3;
  arc(linebGoldPos.x, linebGoldPos.y, linebGoldRadius, 0, CGLP_PI * 2, &arcc);
  if (arcc.isColliding.rect[LIGHT_PURPLE]) {
    isHittingWall = true;
  }
  linebGoldRadius += 0.05 * difficulty;
  if (input.isJustPressed) {
    play(LASER);
    Vector cp;
    vectorSet(&cp, linebPoints[0].pos.x, linebPoints[0].pos.y);
    vectorAdd(&cp, linebPoints[1].pos.x, linebPoints[1].pos.y);
    vectorMul(&cp, 0.5);
    TIMES(LINEB_POINT_COUNT, i) {
      float ang = angleTo(&linebPoints[i].pos, cp.x, cp.y);
      addWithAngle(&linebPoints[i].vel, ang, 9);
    }
  }
  TIMES(LINEB_POINT_COUNT, i) {
    LinebPoint* p = &linebPoints[i];
    p->pos.x += p->vel.x * sqrt(difficulty);
    p->pos.y += p->vel.y * sqrt(difficulty);
    if ((p->pos.x < 4 && p->vel.x < 0) || (p->pos.x > 96 && p->vel.x > 0)) {
      play(HIT);
      p->vel.x *= -1;
      p->vel.y += (rnd(0.1, 0.2) * RNDPM()) / (fabs(p->vel.y) + 1);
    }
    if ((p->pos.y < 5 && p->vel.y < 0) || (p->pos.y > 96 && p->vel.y > 0)) {
      play(HIT);
      p->vel.y *= -1;
      p->vel.x += (rnd(0.1, 0.2) * RNDPM()) / (fabs(p->vel.x) + 1);
    }
    if (vectorLength(&p->vel) > 1) {
      vectorMul(&p->vel, 0.9);
    }
    vectorMul(&p->vel, 0.999);
  }
  int shiftCount = linebHistoryCount;
  if (shiftCount > LINEB_MAX_HISTORY_COUNT - 1) {
    shiftCount = LINEB_MAX_HISTORY_COUNT - 1;
  }
  for (int k = shiftCount - 1; k >= 0; k--) {
    linebHistory[k + 1] = linebHistory[k];
  }
  linebHistory[0].p1 = linebPoints[0].pos;
  linebHistory[0].p2 = linebPoints[1].pos;
  if (linebHistoryCount < LINEB_MAX_HISTORY_COUNT) {
    linebHistoryCount++;
  }
  color = PURPLE;
  thickness = 3;
  Collision lc;
  line(linebPoints[0].pos.x, linebPoints[0].pos.y, linebPoints[1].pos.x,
       linebPoints[1].pos.y, &lc);
  if (lc.isColliding.rect[YELLOW]) {
    play(POWER_UP);
    addScore(ceil(linebGoldRadius * sqrt(linebGoldRadius)), linebGoldPos.x,
             linebGoldPos.y);
    linebGoldActive = false;
  } else if (isHittingWall) {
    play(EXPLOSION);
    color = RED;
    TIMES(4, i) {
      Vector p;
      vectorSet(&p, linebGoldPos.x, linebGoldPos.y);
      addWithAngle(&p, (i * CGLP_PI) / 2, linebGoldRadius);
      if (p.x < 5 || p.x > 95 || p.y < 5 || p.y > 95) {
        text("X", p.x, p.y, &scratch);
      }
    }
    gameOver();
  }
}

void addGameLineb() {
  addGame(linebTitle, linebDescription, linebCharacters, linebCharactersCount,
          &linebOptions, false, &linebUpdate);
}
