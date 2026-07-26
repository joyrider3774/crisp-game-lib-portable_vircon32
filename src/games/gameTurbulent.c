#include "../cglp.h"

int* turbulentTitle = "TURBULENT";
int* turbulentDescription = "[Tap] Jump";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] turbulentCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int turbulentCharactersCount = 0;

Options turbulentOptions = {100, 100, 300, true};

struct TurbulentWave {
  float height;
  float angle;
  float va;
  float x;
};
TurbulentWave[7] turbulentWaves;

Vector[25] turbulentPoints;

struct TurbulentMine {
  float x;
  float vx;
  bool isAlive;
};
#define TURBULENT_MAX_MINE_COUNT 32
TurbulentMine[TURBULENT_MAX_MINE_COUNT] turbulentMines;
int turbulentMineIndex;
float turbulentNextMineDist;

#define TURBULENT_STATE_FLOAT 0
#define TURBULENT_STATE_JUMP 1
struct TurbulentShip {
  Vector pos;
  Vector pp;
  Vector vel;
  float angle;
  int state;
};
TurbulentShip turbulentShip;
float turbulentJumpX;

bool turbulentGetPoints(float x, int* ppIndexOut, int* npIndexOut) {
  TIMES(25, i) {
    int ppIdx = (int)cgl_wrap(i - 1, 0, 25);
    int npIdx = i;
    Vector* pp = &turbulentPoints[ppIdx];
    Vector* np = &turbulentPoints[npIdx];
    if (pp->x > np->x) {
      continue;
    }
    if (pp->x <= x && x < np->x) {
      *ppIndexOut = ppIdx;
      *npIndexOut = npIdx;
      return true;
    }
  }
  return false;
}

void turbulentUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(7, wi) {
      turbulentWaves[wi].height = rnd(10, 30);
      turbulentWaves[wi].angle = (wi % 2) * CGLP_PI + rnd(0, CGLP_PI / 4) * RNDPM();
      turbulentWaves[wi].va = rnd(0.01, 0.02);
      turbulentWaves[wi].x = wi * 20 - 20;
    }
    TIMES(25, pi5) { vectorSet(&turbulentPoints[pi5], 0, 0); }
    INIT_UNALIVED_ARRAY_FAST(turbulentMines);
    turbulentMineIndex = 0;
    turbulentNextMineDist = 0;
    vectorSet(&turbulentShip.pos, 40, 60);
    vectorSet(&turbulentShip.pp, 40, 60);
    vectorSet(&turbulentShip.vel, 0, 0);
    turbulentShip.angle = 0;
    turbulentShip.state = TURBULENT_STATE_FLOAT;
  }
  float scr = 0.1 * difficulty;
  if (turbulentShip.pos.x > 50) {
    scr += (turbulentShip.pos.x - 50) * 0.1;
  }
  TIMES(7, wi2) {
    TurbulentWave* w = &turbulentWaves[wi2];
    w->x -= scr;
    if (w->x < -20) {
      w->x += 140;
      w->height = rnd(10, 30);
      w->angle = rnd(0, CGLP_PI * 2);
      w->va = rnd(0.01, 0.02 * sqrt(difficulty));
    }
    w->angle += w->va;
    vectorSet(&turbulentPoints[wi2 * 4], w->x, 60 + sin(w->angle) * w->height);
  }
  color = BLUE;
  TIMES(25, pii) {
    int im = pii % 4;
    if (im != 0) {
      Vector* pp2 = &turbulentPoints[(pii / 4) * 4];
      Vector* np2 = &turbulentPoints[(pii / 4 + 1) * 4];
      float rr;
      if (im == 1) {
        rr = 0.2;
      } else if (im == 2) {
        rr = 0.5;
      } else {
        rr = 0.8;
      }
      vectorSet(&turbulentPoints[pii], pp2->x + 5 * im, pp2->y * (1 - rr) + np2->y * rr);
    }
    int prevIdx = (int)cgl_wrap(pii - 1, 0, 25);
    Vector* prevP = &turbulentPoints[prevIdx];
    Vector* curP = &turbulentPoints[pii];
    if (prevP->x < curP->x) {
      line(prevP->x, prevP->y, curP->x, curP->y, &scratch);
    }
  }
  turbulentNextMineDist -= scr;
  if (turbulentNextMineDist < 0) {
    ASSIGN_ARRAY_ITEM(turbulentMines, turbulentMineIndex, TurbulentMine, nm);
    nm->x = 103;
    nm->vx = 0;
    nm->isAlive = true;
    turbulentMineIndex = cgl_wrap(turbulentMineIndex + 1, 0, TURBULENT_MAX_MINE_COUNT);
    turbulentNextMineDist = rnd(100, 120) / sqrt(difficulty);
  }
  color = RED;
  FOR_EACH(turbulentMines, mi) {
    ASSIGN_ARRAY_ITEM(turbulentMines, mi, TurbulentMine, m);
    SKIP_IS_NOT_ALIVE(m);
    m->x -= scr;
    int ppIdx, npIdx;
    bool found = turbulentGetPoints(m->x, &ppIdx, &npIdx);
    if (!found) {
      m->isAlive = false;
      continue;
    }
    Vector* pp3 = &turbulentPoints[ppIdx];
    Vector* np3 = &turbulentPoints[npIdx];
    float oy = np3->y - pp3->y;
    m->vx += oy * 0.001;
    m->vx *= 0.9;
    m->x += m->vx * sqrt(difficulty);
    float r2 = (m->x - pp3->x) / (np3->x - pp3->x);
    text("*", m->x, pp3->y + oy * r2 - 5, &scratch);
    if (m->x < -3) {
      m->isAlive = false;
      continue;
    }
  }
  float sa = 0;
  if (turbulentShip.state == TURBULENT_STATE_FLOAT) {
    int ppIdx2, npIdx2;
    bool found2 = turbulentGetPoints(turbulentShip.pos.x, &ppIdx2, &npIdx2);
    if (found2) {
      Vector* pp4 = &turbulentPoints[ppIdx2];
      Vector* np4 = &turbulentPoints[npIdx2];
      float oy2 = np4->y - pp4->y;
      turbulentShip.vel.x += oy2 * 0.002;
      turbulentShip.vel.x *= 0.925;
      turbulentShip.vel.x += 0.025;
      turbulentShip.pos.x += turbulentShip.vel.x;
      float r3 = (turbulentShip.pos.x - pp4->x) / (np4->x - pp4->x);
      turbulentShip.pos.y = pp4->y + oy2 * r3;
      sa = angleTo(pp4, np4->x, np4->y);
    }
    if (input.isJustPressed) {
      play(JUMP);
      turbulentJumpX = turbulentShip.pos.x;
      turbulentShip.vel.x = (turbulentShip.pos.x - turbulentShip.pp.x) * 2;
      turbulentShip.vel.y = (turbulentShip.pos.y - turbulentShip.pp.y) * 5;
      addWithAngle(&turbulentShip.vel, sa - CGLP_PI / 2, 1);
      if (turbulentShip.vel.y > -1) {
        turbulentShip.vel.y = -1;
      }
      vectorAdd(&turbulentShip.pos, turbulentShip.vel.x, turbulentShip.vel.y);
      turbulentShip.state = TURBULENT_STATE_JUMP;
    }
  } else {
    turbulentJumpX -= scr;
    turbulentShip.vel.x += 0.005;
    float dvy;
    if (input.isPressed) {
      dvy = 0.02;
    } else {
      dvy = 0.1;
    }
    turbulentShip.vel.y += dvy;
    vectorMul(&turbulentShip.vel, 0.98);
    vectorAdd(&turbulentShip.pos, turbulentShip.vel.x, turbulentShip.vel.y);
    sa = vectorAngle(&turbulentShip.vel);
  }
  turbulentShip.pos.x -= scr;
  turbulentShip.pos.x = clamp(turbulentShip.pos.x, 5, 95);
  turbulentShip.pos.y = clamp(turbulentShip.pos.y, 5, 95);
  turbulentShip.pp = turbulentShip.pos;
  turbulentShip.angle += cgl_wrap(sa - turbulentShip.angle, -CGLP_PI, CGLP_PI) * 0.1;
  sa = turbulentShip.angle;
  Vector p2;
  vectorSet(&p2, turbulentShip.pos.x, turbulentShip.pos.y);
  addWithAngle(&p2, sa - CGLP_PI * 0.5, 2);
  color = RED;
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(p2.x, p2.y, 3, sa, &scratch);
  addWithAngle(&p2, sa - CGLP_PI * 0.4, 2);
  color = BLACK;
  bar(p2.x, p2.y, 4, sa, &scratch);
  addWithAngle(&p2, sa - CGLP_PI * 0.6, 2);
  Collision c;
  bar(p2.x, p2.y, 1, sa, &c);
  if (turbulentShip.state == TURBULENT_STATE_JUMP && c.isColliding.rect[BLUE]) {
    float d = turbulentShip.pos.x - turbulentJumpX;
    play(HIT);
    if (d > 0) {
      play(POWER_UP);
      addScore(ceil(sqrt(d * d)), turbulentShip.pos.x, turbulentShip.pos.y);
    }
    turbulentShip.state = TURBULENT_STATE_FLOAT;
    turbulentShip.vel.x *= 0.5;
  }
  if (c.isColliding.text['*']) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameTurbulent() {
  addGame(turbulentTitle, turbulentDescription, turbulentCharacters, turbulentCharactersCount,
          &turbulentOptions, false, &turbulentUpdate);
}
