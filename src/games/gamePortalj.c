#include "../cglp.h"

int* portaljTitle = "PORTAL J";
int* portaljDescription = "[Tap]\n Portal jump";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] portaljCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int portaljCharactersCount = 1;

Options portaljOptions = {200, 50, 6, false};

struct PortaljFloor {
  float x;
  float vx;
  float width;
  bool isAlive;
};
#define PORTALJ_MAX_FLOOR_COUNT 32
PortaljFloor[PORTALJ_MAX_FLOOR_COUNT] portaljFloors;
int portaljFloorIndex;

struct PortaljGold {
  float x;
  bool isAlive;
};
#define PORTALJ_MAX_GOLD_COUNT 32
PortaljGold[PORTALJ_MAX_GOLD_COUNT] portaljGolds;
int portaljGoldIndex;

float portaljPlayerX;
float portaljPortalTicks;
float portaljFallTicks;
float portaljPortalX;
float portaljBackX;
float portaljNextFloorDist;
float portaljNextGoldDist;
int portaljMultiplier;
float portaljNextScore;

void portaljUpdate() {
  Collision scratch;
  if (!ticks) {
    portaljPlayerX = 30;
    portaljPortalTicks = 0;
    portaljFallTicks = 0;
    portaljPortalX = 36;
    portaljBackX = 0;
    INIT_UNALIVED_ARRAY_FAST(portaljFloors);
    portaljFloorIndex = 0;
    ASSIGN_ARRAY_ITEM(portaljFloors, portaljFloorIndex, PortaljFloor, f1);
    f1->x = 0;
    f1->vx = 0;
    f1->width = 100;
    f1->isAlive = true;
    portaljFloorIndex = cgl_wrap(portaljFloorIndex + 1, 0, PORTALJ_MAX_FLOOR_COUNT);
    ASSIGN_ARRAY_ITEM(portaljFloors, portaljFloorIndex, PortaljFloor, f2);
    f2->x = 110;
    f2->vx = 0;
    f2->width = 80;
    f2->isAlive = true;
    portaljFloorIndex = cgl_wrap(portaljFloorIndex + 1, 0, PORTALJ_MAX_FLOOR_COUNT);
    portaljNextFloorDist = 0;
    INIT_UNALIVED_ARRAY_FAST(portaljGolds);
    portaljGoldIndex = 0;
    portaljNextGoldDist = 30;
    portaljMultiplier = 1;
    portaljNextScore = 0;
  }
  float scrX;
  if (portaljPlayerX > 30) {
    scrX = (portaljPlayerX - 30) * 0.1;
  } else {
    scrX = 0;
  }
  scrX += difficulty * 0.1;
  color = LIGHT_BLACK;
  portaljBackX = cgl_wrap(portaljBackX - scrX, 0, 200);
  TIMES(2, i) { rect(cgl_wrap(portaljBackX + i * 50, 0, 200), 0, 1, 50, &scratch); }
  portaljNextFloorDist -= scrX;
  if (portaljNextFloorDist < 0) {
    float itv = rnd(50, 99);
    float fr = rnd(0.4, 0.7);
    float vx;
    if (rnd(0, 1) > 1 / difficulty) {
      vx = rnd(1, difficulty) * RNDPM() * 0.25;
    } else {
      vx = 0;
    }
    ASSIGN_ARRAY_ITEM(portaljFloors, portaljFloorIndex, PortaljFloor, nf);
    nf->x = 200 + portaljNextFloorDist + itv * rnd(0, 1 - fr);
    nf->vx = vx;
    nf->width = itv * fr;
    nf->isAlive = true;
    portaljFloorIndex = cgl_wrap(portaljFloorIndex + 1, 0, PORTALJ_MAX_FLOOR_COUNT);
    portaljNextFloorDist += itv;
  }
  color = GREEN;
  FOR_EACH(portaljFloors, i) {
    ASSIGN_ARRAY_ITEM(portaljFloors, i, PortaljFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->x += f->vx - scrX;
    FOR_EACH(portaljFloors, j) {
      ASSIGN_ARRAY_ITEM(portaljFloors, j, PortaljFloor, af);
      SKIP_IS_NOT_ALIVE(af);
      if (af->x >= f->x) {
        continue;
      }
      if (af->x + af->width >= f->x) {
        f->vx *= -1;
        af->vx *= -1;
        f->x += f->vx * 2;
      }
    }
    rect(f->x, 40, f->width, 8, &scratch);
    f->isAlive = f->x + f->width >= 0;
  }
  portaljPlayerX -= scrX;
  portaljPortalX -= scrX;
  float py = 39;
  if (portaljFallTicks > 0) {
    portaljFallTicks += difficulty * 0.2;
    py += portaljFallTicks;
  }
  if (portaljPortalTicks > 0) {
    portaljPortalTicks += difficulty;
    if (portaljPortalTicks < 5) {
      py += portaljPortalTicks;
    } else {
      if (portaljPlayerX < portaljPortalX) {
        play(LASER);
        portaljNextScore = portaljPortalX - portaljPlayerX;
        portaljNextScore = ceil(portaljNextScore * sqrt(portaljNextScore) * 0.1 * portaljMultiplier);
        portaljPlayerX = portaljPortalX;
      }
      py = 42 - (portaljPortalTicks - 5);
      if (py < 39) {
        play(POWER_UP);
        addScore(portaljNextScore, portaljPlayerX, 36);
        py = 39;
        portaljPortalTicks = 0;
      }
    }
  } else {
    portaljPortalX += difficulty * 2;
    if (portaljPortalX > 200) {
      portaljPortalX = portaljPlayerX + 6;
    }
    if (input.isJustPressed) {
      play(HIT);
      portaljPortalTicks = 1;
    }
  }
  color = RED;
  Collision c;
  box(portaljPlayerX, py, 5, 5, &c);
  color = PURPLE;
  box(portaljPortalX, 43, 7, 5, &scratch);
  if (portaljPortalTicks > 0 && portaljPortalTicks < 5) {
    box(portaljPlayerX, 43 + portaljFallTicks, 7, 5, &scratch);
  }
  if (portaljPortalTicks == 0) {
    if (c.isColliding.rect[GREEN]) {
      portaljFallTicks = 0;
    } else if (portaljFallTicks == 0) {
      portaljFallTicks = 1;
    }
  }
  if (py > 52 || portaljPlayerX < 0) {
    play(EXPLOSION);
    gameOver();
  }
  portaljNextGoldDist -= scrX;
  if (portaljNextGoldDist < 0) {
    ASSIGN_ARRAY_ITEM(portaljGolds, portaljGoldIndex, PortaljGold, ng);
    ng->x = 203;
    ng->isAlive = true;
    portaljGoldIndex = cgl_wrap(portaljGoldIndex + 1, 0, PORTALJ_MAX_GOLD_COUNT);
    portaljNextGoldDist += rnd(99, 120);
  }
  color = YELLOW;
  FOR_EACH(portaljGolds, i) {
    ASSIGN_ARRAY_ITEM(portaljGolds, i, PortaljGold, g);
    SKIP_IS_NOT_ALIVE(g);
    g->x -= scrX;
    Collision gc;
    text("$", g->x, 36, &gc);
    if (gc.isColliding.rect[RED]) {
      play(COIN);
      portaljMultiplier++;
      g->isAlive = false;
      continue;
    }
    g->isAlive = g->x >= -3;
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(portaljMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGamePortalj() {
  addGame(portaljTitle, portaljDescription, portaljCharacters,
          portaljCharactersCount, &portaljOptions, false, &portaljUpdate);
}
