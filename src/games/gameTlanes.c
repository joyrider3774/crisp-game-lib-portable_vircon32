#include "../cglp.h"

int* tlanesTitle = "T LANES";
int* tlanesDescription = "[Tap]\n Change direction";

int[6][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tlanesCharacters = {
    {
        "yyyyy ",
        "yllly ",
        "yllly ",
        "yyyyy ",
    },
    {
        " yyy  ",
        "yllly ",
        " yllly",
        "  ylll",
        "   yyy",
    },
    {
        "rrrrr ",
        " rrrr ",
        "  rrr ",
        "   rr ",
        "    r ",
    },
    {
        "l     ",
        "lll   ",
        "lllll ",
        "lll   ",
        "l     ",
    },
    {
        "    b ",
        "   bb ",
        "  bbb ",
        " bbbb ",
        "bbbbb ",
    },
    {
        "pLwwLp",
        "LpwwpL",
        "wwwwww",
        "LpwwpL",
        "pLwwLp",
    },
};
int tlanesCharactersCount = 6;

Options tlanesOptions = {200, 100, 9, false};

#define TLANES_LANE_COUNT 5
#define TLANES_LANE_INTERVAL ((100.0 - 20) / (TLANES_LANE_COUNT - 1))

struct TlanesCross {
  Vector pos;
  int angle;
  int currentAngle;
  bool isAlive;
};
#define TLANES_MAX_CROSS_COUNT 32
TlanesCross[TLANES_MAX_CROSS_COUNT] tlanesCrosses;
int tlanesCrossIndex;
float tlanesNextCrossDist;
bool[TLANES_LANE_COUNT * 2] tlanesNextCrossLanes;
int tlanesCrossCount;

struct TlanesStop {
  Vector pos;
  bool isAlive;
};
#define TLANES_MAX_STOP_COUNT 32
TlanesStop[TLANES_MAX_STOP_COUNT] tlanesStops;
int tlanesStopIndex;
float tlanesNextStopDist;

struct TlanesCar {
  Vector pos;
  int angle;
  float speed;
  float ty;
  bool onArrow;
  float invDist;
  bool isAlive;
};
#define TLANES_MAX_CAR_COUNT 32
TlanesCar[TLANES_MAX_CAR_COUNT] tlanesCars;
int tlanesCarIndex;
float tlanesNextCarDist;

float tlanesScr;

void tlanesUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(tlanesCrosses);
    tlanesCrossIndex = 0;
    tlanesNextCrossDist = 0;
    tlanesCrossCount = 0;
    INIT_UNALIVED_ARRAY_FAST(tlanesStops);
    tlanesStopIndex = 0;
    tlanesNextStopDist = 199;
    float cy = 10 + TLANES_LANE_INTERVAL * floor(TLANES_LANE_COUNT / 2.0);
    INIT_UNALIVED_ARRAY_FAST(tlanesCars);
    tlanesCarIndex = 0;
    ASSIGN_ARRAY_ITEM(tlanesCars, tlanesCarIndex, TlanesCar, c1);
    vectorSet(&c1->pos, 20, cy);
    c1->angle = 0;
    c1->speed = 0.1;
    c1->ty = 0;
    c1->onArrow = false;
    c1->invDist = 0;
    c1->isAlive = true;
    tlanesCarIndex = cgl_wrap(tlanesCarIndex + 1, 0, TLANES_MAX_CAR_COUNT);
    ASSIGN_ARRAY_ITEM(tlanesCars, tlanesCarIndex, TlanesCar, c2);
    vectorSet(&c2->pos, 99, cy);
    c2->angle = 0;
    c2->speed = 0;
    c2->ty = 0;
    c2->onArrow = false;
    c2->invDist = 0;
    c2->isAlive = true;
    tlanesCarIndex = cgl_wrap(tlanesCarIndex + 1, 0, TLANES_MAX_CAR_COUNT);
    tlanesNextCarDist = 0;
    tlanesScr = 0;
  }
  int carCount = 0;
  FOR_EACH(tlanesCars, cci) {
    ASSIGN_ARRAY_ITEM(tlanesCars, cci, TlanesCar, cc);
    SKIP_IS_NOT_ALIVE(cc);
    if (cc->speed > 0) {
      carCount++;
    }
  }
  if (carCount == 0) {
    gameOver();
  }
  tlanesNextCrossDist -= tlanesScr;
  if (tlanesNextCrossDist < 0) {
    addScore(carCount, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    tlanesCrossCount--;
    if (tlanesCrossCount < 0) {
      TIMES(TLANES_LANE_COUNT * 2, li) {
        tlanesNextCrossLanes[li] = !(li == TLANES_LANE_COUNT - 1 || li == TLANES_LANE_COUNT);
      }
      tlanesCrossCount = TLANES_LANE_COUNT * 2 - 3;
    }
    int ci = rndi(0, TLANES_LANE_COUNT * 2);
    while (!tlanesNextCrossLanes[ci]) {
      ci = (int)cgl_wrap(ci + 1, 0, TLANES_LANE_COUNT * 2);
    }
    tlanesNextCrossLanes[ci] = false;
    int angle;
    if (ci < TLANES_LANE_COUNT) {
      angle = 1;
    } else {
      angle = -1;
    }
    int currentAngle;
    if (rnd(0, 1) < 0.5) {
      currentAngle = 0;
    } else {
      currentAngle = angle;
    }
    ASSIGN_ARRAY_ITEM(tlanesCrosses, tlanesCrossIndex, TlanesCross, nc);
    vectorSet(&nc->pos, 200, 10 + TLANES_LANE_INTERVAL * (ci % TLANES_LANE_COUNT));
    nc->angle = angle;
    nc->currentAngle = currentAngle;
    nc->isAlive = true;
    tlanesCrossIndex = cgl_wrap(tlanesCrossIndex + 1, 0, TLANES_MAX_CROSS_COUNT);
    tlanesNextCrossDist = 24;
  }
  color = LIGHT_BLACK;
  TIMES(TLANES_LANE_COUNT, li2) {
    float y = 10 + TLANES_LANE_INTERVAL * li2;
    rect(0, y - 1, 200, 1, &scratch);
    rect(0, y + 1, 200, 1, &scratch);
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  FOR_EACH(tlanesCrosses, cri) {
    ASSIGN_ARRAY_ITEM(tlanesCrosses, cri, TlanesCross, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= tlanesScr;
    if (input.isJustPressed) {
      if (c->currentAngle == 0) {
        c->currentAngle = c->angle;
      } else {
        c->currentAngle = 0;
      }
    }
    if (c->currentAngle == 0) {
      color = LIGHT_BLACK;
    } else if (c->currentAngle == -1) {
      color = RED;
    } else {
      color = BLUE;
    }
    thickness = 1;
    line(c->pos.x, c->pos.y - 1, c->pos.x + TLANES_LANE_INTERVAL,
         c->pos.y - 1 + TLANES_LANE_INTERVAL * c->angle, &scratch);
    line(c->pos.x, c->pos.y + 1, c->pos.x + TLANES_LANE_INTERVAL,
         c->pos.y + 1 + TLANES_LANE_INTERVAL * c->angle, &scratch);
    color = BLACK;
    int[2] cc2;
    cc2[0] = 'c' + c->currentAngle + 1;
    cc2[1] = 0;
    character(cc2, c->pos.x + 3, c->pos.y, &scratch);
    if (c->pos.x < -TLANES_LANE_INTERVAL) {
      c->isAlive = false;
      continue;
    }
  }
  tlanesNextStopDist -= tlanesScr;
  if (tlanesNextStopDist < 0) {
    ASSIGN_ARRAY_ITEM(tlanesStops, tlanesStopIndex, TlanesStop, ns);
    vectorSet(&ns->pos, 205, 10 + TLANES_LANE_INTERVAL * rndi(0, TLANES_LANE_COUNT));
    ns->isAlive = true;
    tlanesStopIndex = cgl_wrap(tlanesStopIndex + 1, 0, TLANES_MAX_STOP_COUNT);
    tlanesNextStopDist += rnd(99, 120);
  }
  color = BLACK;
  FOR_EACH(tlanesStops, sti) {
    ASSIGN_ARRAY_ITEM(tlanesStops, sti, TlanesStop, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x -= tlanesScr;
    Collision sc;
    character("f", s->pos.x, s->pos.y, &sc);
    if (sc.isColliding.character['c'] || sc.isColliding.character['d'] ||
        sc.isColliding.character['e']) {
      s->isAlive = false;
      continue;
    }
    if (s->pos.x < -3) {
      s->isAlive = false;
      continue;
    }
  }
  tlanesNextCarDist -= tlanesScr;
  if (tlanesNextCarDist < 0) {
    ASSIGN_ARRAY_ITEM(tlanesCars, tlanesCarIndex, TlanesCar, nc2);
    vectorSet(&nc2->pos, 203, 10 + TLANES_LANE_INTERVAL * rndi(0, TLANES_LANE_COUNT));
    nc2->angle = 0;
    nc2->speed = 0;
    nc2->ty = 0;
    nc2->onArrow = false;
    nc2->invDist = 0;
    nc2->isAlive = true;
    tlanesCarIndex = cgl_wrap(tlanesCarIndex + 1, 0, TLANES_MAX_CAR_COUNT);
    tlanesNextCarDist += rnd(40, 60);
  }
  float maxX = 0;
  FOR_EACH(tlanesCars, cai) {
    ASSIGN_ARRAY_ITEM(tlanesCars, cai, TlanesCar, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= tlanesScr;
    if (input.isJustPressed && c->pos.x < 50) {
      c->pos.x -= (50 - c->pos.x) * 0.1;
    }
    if (c->speed > 0) {
      c->speed += (1 - c->speed) * 0.05;
    }
    addWithAngle(&c->pos, (CGLP_PI / 4) * c->angle, c->speed * sqrt(difficulty));
    if ((c->angle == -1 && c->pos.y < c->ty) || (c->angle == 1 && c->pos.y > c->ty)) {
      c->pos.y = c->ty;
      c->angle = 0;
      c->invDist = 5;
    }
    color = BLACK;
    Collision cl;
    if (c->angle == 0) {
      characterOptions.isMirrorY = false;
      character("a", c->pos.x, c->pos.y, &cl);
    } else {
      characterOptions.isMirrorY = c->angle < 0;
      character("b", c->pos.x, c->pos.y, &cl);
    }
    characterOptions.isMirrorY = false;
    c->invDist -= sqrt(difficulty);
    if (c->angle == 0 && c->invDist < 0 && cl.isColliding.character['f']) {
      if (c->speed > 0) {
        play(EXPLOSION);
        particle(c->pos.x, c->pos.y, 16, 1, 0, CGLP_PI * 2);
      }
      c->isAlive = false;
      continue;
    }
    if (c->speed == 0 && cl.isColliding.character['a']) {
      play(POWER_UP);
      addScore(carCount * 10, c->pos.x, c->pos.y);
      c->speed = rnd(0.01, 0.3);
    }
    if (c->speed > 0 && c->angle == 0) {
      if (!c->onArrow) {
        int na;
        if (cl.isColliding.character['c']) {
          na = -1;
        } else if (cl.isColliding.character['e']) {
          na = 1;
        } else {
          na = 0;
        }
        if (na != 0) {
          play(LASER);
          color = TRANSPARENT;
          TIMES(9, bi) {
            c->pos.x--;
            Collision cl2;
            character("a", c->pos.x, c->pos.y, &cl2);
            if (!(cl2.isColliding.character['c'] || cl2.isColliding.character['e'])) {
              break;
            }
          }
          c->angle = na;
          c->ty = c->pos.y + TLANES_LANE_INTERVAL * na;
          c->onArrow = true;
        }
        if (cl.isColliding.character['d']) {
          c->onArrow = true;
        }
      } else {
        if (!(cl.isColliding.character['c'] || cl.isColliding.character['d'] ||
              cl.isColliding.character['e'])) {
          c->onArrow = false;
        }
      }
    }
    if (c->speed > 0 && c->pos.x > maxX) {
      maxX = c->pos.x;
    }
    if (c->pos.x < -3) {
      c->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(tlanesCars, aliveCarsNow);
  if (aliveCarsNow == 0) {
    gameOver();
  }
  if (maxX > 50) {
    tlanesScr = (maxX - 50) * 0.1;
  } else {
    tlanesScr = 0;
  }
}

void addGameTlanes() {
  addGame(tlanesTitle, tlanesDescription, tlanesCharacters, tlanesCharactersCount,
          &tlanesOptions, false, &tlanesUpdate);
}
