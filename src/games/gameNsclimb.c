#include "../cglp.h"

int* nsclimbTitle = "NS CLIMB";
int* nsclimbDescription = "[Tap] Reverse";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] nsclimbCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int nsclimbCharactersCount = 1;

Options nsclimbOptions = {100, 100, 4, false};

#define NSCLIMB_TYPE_NONE 0
#define NSCLIMB_TYPE_N 1
#define NSCLIMB_TYPE_S 2

struct NsclimbMagnet {
  Vector pos;
  int type;
  int wallIndex;
};
#define NSCLIMB_MAGNET_COUNT 22
NsclimbMagnet[NSCLIMB_MAGNET_COUNT] nsclimbMagnets;

struct NsclimbWall {
  int type;
  int prevType;
  int count;
  float x;
  float vx;
};
#define NSCLIMB_WALL_COUNT 2
NsclimbWall[NSCLIMB_WALL_COUNT] nsclimbWalls;

Vector nsclimbPos;
Vector nsclimbVel;
int nsclimbType;

void nsclimbUpdate() {
  Collision scratch;
  if (!ticks) {
    TIMES(NSCLIMB_MAGNET_COUNT, i) {
      float mx;
      if (i % 2 == 0) {
        mx = 20;
      } else {
        mx = 80;
      }
      vectorSet(&nsclimbMagnets[i].pos, mx, i * 5 - 5);
      if (i < 10) {
        nsclimbMagnets[i].type = NSCLIMB_TYPE_S;
      } else {
        nsclimbMagnets[i].type = NSCLIMB_TYPE_NONE;
      }
      nsclimbMagnets[i].wallIndex = i % 2;
    }
    nsclimbWalls[0].type = NSCLIMB_TYPE_NONE;
    nsclimbWalls[0].prevType = NSCLIMB_TYPE_S;
    nsclimbWalls[0].count = rndi(2, 4);
    nsclimbWalls[0].x = 20;
    nsclimbWalls[0].vx = 0;
    nsclimbWalls[1].type = NSCLIMB_TYPE_NONE;
    nsclimbWalls[1].prevType = NSCLIMB_TYPE_S;
    nsclimbWalls[1].count = rndi(2, 4);
    nsclimbWalls[1].x = 80;
    nsclimbWalls[1].vx = 0;
    vectorSet(&nsclimbPos, 50, 90);
    vectorSet(&nsclimbVel, 0, -1);
    nsclimbType = NSCLIMB_TYPE_N;
  }
  if (input.isJustPressed) {
    if (nsclimbType == NSCLIMB_TYPE_N) {
      nsclimbType = NSCLIMB_TYPE_S;
      play(SELECT);
    } else {
      nsclimbType = NSCLIMB_TYPE_N;
      play(LASER);
    }
  }
  vectorAdd(&nsclimbPos, nsclimbVel.x, nsclimbVel.y);
  nsclimbVel.x *= 0.98;
  nsclimbVel.y += 0.001 * difficulty;
  float scr = 0;
  if (nsclimbPos.y < 70) {
    scr = (70 - nsclimbPos.y) * 0.1;
  }
  if (nsclimbPos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  nsclimbPos.y += scr;
  addScore(scr, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  if (nsclimbType == NSCLIMB_TYPE_N) {
    color = RED;
  } else {
    color = BLUE;
  }
  box(nsclimbPos.x, nsclimbPos.y, 7, 7, &scratch);
  color = WHITE;
  int[2] typeChar;
  if (nsclimbType == NSCLIMB_TYPE_N) {
    typeChar[0] = 'N';
  } else {
    typeChar[0] = 'S';
  }
  typeChar[1] = 0;
  text(typeChar, nsclimbPos.x - 1, nsclimbPos.y - 1, &scratch);
  TIMES(NSCLIMB_MAGNET_COUNT, i) {
    NsclimbMagnet* m = &nsclimbMagnets[i];
    m->pos.y += scr;
    if (m->type != NSCLIMB_TYPE_NONE) {
      float d = distanceTo(&nsclimbPos, m->pos.x, m->pos.y);
      float a = angleTo(&nsclimbPos, m->pos.x, m->pos.y);
      float mag;
      if (m->type == nsclimbType) {
        mag = -1;
      } else {
        mag = 1;
      }
      addWithAngle(&nsclimbVel, a, (difficulty / d / d) * mag);
    }
    if (m->type == NSCLIMB_TYPE_NONE) {
      color = LIGHT_BLACK;
    } else if (m->type == NSCLIMB_TYPE_N) {
      color = RED;
    } else {
      color = BLUE;
    }
    Collision mc;
    box(m->pos.x, m->pos.y, 9, 9, &mc);
    if (mc.isColliding.rect[BLUE] || mc.isColliding.rect[RED]) {
      play(HIT);
      float ofs;
      if (m->wallIndex == 0) {
        ofs = 10;
      } else {
        ofs = -10;
      }
      nsclimbPos.x = m->pos.x + ofs;
      if ((m->wallIndex == 0 && nsclimbVel.x < 0) ||
          (m->wallIndex == 1 && nsclimbVel.x > 0)) {
        nsclimbVel.x *= -0.7;
      }
    }
    color = WHITE;
    int[2] mTypeChar;
    if (m->type == NSCLIMB_TYPE_NONE) {
      mTypeChar[0] = 0;
    } else if (m->type == NSCLIMB_TYPE_N) {
      mTypeChar[0] = 'N';
    } else {
      mTypeChar[0] = 'S';
    }
    mTypeChar[1] = 0;
    text(mTypeChar, m->pos.x - 1, m->pos.y - 1, &scratch);
    if (m->pos.y > 105) {
      m->pos.y -= 110;
      NsclimbWall* w = &nsclimbWalls[m->wallIndex];
      w->x += w->vx;
      m->pos.x = w->x;
      w->vx += rnd(0, 0.1) * RNDPM();
      if (m->wallIndex == 0) {
        if ((w->x < 10 && w->vx < 0) || (w->x > 40 && w->vx > 0)) {
          w->vx *= -0.5;
        }
      } else {
        if ((w->x < 60 && w->vx < 0) || (w->x > 90 && w->vx > 0)) {
          w->vx *= -0.5;
        }
      }
      w->count--;
      if (w->count < 0) {
        if (w->type == NSCLIMB_TYPE_NONE) {
          if (w->prevType == NSCLIMB_TYPE_N) {
            w->type = NSCLIMB_TYPE_S;
          } else {
            w->type = NSCLIMB_TYPE_N;
          }
          w->prevType = w->type;
          w->count = rndi(3, 9);
        } else {
          w->type = NSCLIMB_TYPE_NONE;
          w->count = rndi(2, 4);
        }
      }
      m->type = w->type;
    }
  }
}

void addGameNsclimb() {
  addGame(nsclimbTitle, nsclimbDescription, nsclimbCharacters,
          nsclimbCharactersCount, &nsclimbOptions, false, &nsclimbUpdate);
}
