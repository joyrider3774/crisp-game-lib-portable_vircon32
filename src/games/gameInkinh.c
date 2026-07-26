#include "../cglp.h"

int* inkinhTitle = "INKINH";
int* inkinhDescription = "[Hold] Thrust & Inhale";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] inkinhCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int inkinhCharactersCount = 0;

Options inkinhOptions = {100, 100, 0, false};

struct InkinhPlayer {
  Vector pos;
  Vector vel;
  float angle;
  float stretch;
};
InkinhPlayer inkinhPlayer;

struct InkinhOrb {
  Vector pos;
  int timer;
  float phase;
  bool isAlive;
};
// The JS itself caps concurrent orbs at 8 (spawn is gated on
// "orbs.length < 8"), so this is an exact cap, not an estimate.
#define INKINH_MAX_ORB_COUNT 8
InkinhOrb[INKINH_MAX_ORB_COUNT] inkinhOrbs;
int inkinhOrbIndex;

struct InkinhObstacle {
  Vector pos;
  Vector vel;
  float rot;
  float rotSpeed;
  bool isAlive;
};
// Lifetime (~236/sqrt(difficulty) frames) and spawn interval
// (~72/sqrt(difficulty) frames) both scale the same way with difficulty,
// so concurrent count stays near a constant ~3.3 - sized with headroom.
#define INKINH_MAX_OBSTACLE_COUNT 24
InkinhObstacle[INKINH_MAX_OBSTACLE_COUNT] inkinhObstacles;
int inkinhObstacleIndex;

struct InkinhTrail {
  Vector pos;
  float alpha;
  bool isAlive;
};
#define INKINH_MAX_TRAIL_COUNT 16
InkinhTrail[INKINH_MAX_TRAIL_COUNT] inkinhTrails;
int inkinhTrailIndex;

int inkinhCombo;
int inkinhComboTimer;

void inkinhUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&inkinhPlayer.pos, 50, 80);
    vectorSet(&inkinhPlayer.vel, 0, 0);
    inkinhPlayer.angle = -CGLP_PI / 2;
    inkinhPlayer.stretch = 1;
    INIT_UNALIVED_ARRAY_FAST(inkinhOrbs);
    inkinhOrbIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(inkinhObstacles);
    inkinhObstacleIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(inkinhTrails);
    inkinhTrailIndex = 0;
    inkinhCombo = 0;
    inkinhComboTimer = 0;
  }

  inkinhComboTimer--;
  if (inkinhComboTimer <= 0) {
    inkinhCombo = 0;
  }

  int spawnRate = (int)floor(72 / sqrt(difficulty));
  if (spawnRate < 1) {
    spawnRate = 1;
  }
  if (ticks % spawnRate == 0) {
    float ox = rnd(10, 90);
    float speed = 0.5 * sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(inkinhObstacles, inkinhObstacleIndex, InkinhObstacle, no);
    vectorSet(&no->pos, ox, -8);
    vectorSet(&no->vel, rnd(-0.2, 0.2), speed);
    no->rot = 0;
    no->rotSpeed = rnd(-0.1, 0.1);
    no->isAlive = true;
    inkinhObstacleIndex = cgl_wrap(inkinhObstacleIndex + 1, 0, INKINH_MAX_OBSTACLE_COUNT);
  }

  int orbRate = 30;
  COUNT_IS_ALIVE(inkinhOrbs, orbAliveCount);
  if (ticks % orbRate == 0 && orbAliveCount < 8) {
    Vector spawnPos;
    int attempt;
    for (attempt = 0; attempt < 9; attempt++) {
      vectorSet(&spawnPos, rnd(15, 85), rnd(20, 90));
      if (distanceTo(&spawnPos, inkinhPlayer.pos.x, inkinhPlayer.pos.y) > 20) {
        break;
      }
    }
    ASSIGN_ARRAY_ITEM(inkinhOrbs, inkinhOrbIndex, InkinhOrb, orb);
    orb->pos = spawnPos;
    orb->timer = 300;
    orb->phase = rnd(0, CGLP_PI * 2);
    orb->isAlive = true;
    inkinhOrbIndex = cgl_wrap(inkinhOrbIndex + 1, 0, INKINH_MAX_ORB_COUNT);
  }

  float pressTurn;
  if (input.isPressed) {
    pressTurn = 0.02;
  } else {
    pressTurn = 0.05;
  }
  inkinhPlayer.angle += pressTurn * sqrt(difficulty);

  if (input.isPressed) {
    float thrust = 0.15;
    inkinhPlayer.vel.x += cos(inkinhPlayer.angle) * thrust;
    inkinhPlayer.vel.y += sin(inkinhPlayer.angle) * thrust;

    if (ticks % 3 == 0) {
      color = LIGHT_BLACK;
      float px = inkinhPlayer.pos.x - cos(inkinhPlayer.angle) * 5;
      float py = inkinhPlayer.pos.y - sin(inkinhPlayer.angle) * 5;
      particle(px, py, 3, 0.5, -inkinhPlayer.angle, CGLP_PI / 4);
    }

    FOR_EACH(inkinhOrbs, oi) {
      ASSIGN_ARRAY_ITEM(inkinhOrbs, oi, InkinhOrb, o);
      SKIP_IS_NOT_ALIVE(o);
      float dx = inkinhPlayer.pos.x - o->pos.x;
      float dy = inkinhPlayer.pos.y - o->pos.y;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist < 30 && dist > 0) {
        float pull = 0.6 / dist;
        o->pos.x += dx * pull;
        o->pos.y += dy * pull;
      }
    }
  }

  inkinhPlayer.vel.x *= 0.96;
  inkinhPlayer.vel.y *= 0.96;
  Vector scaledVel;
  scaledVel = inkinhPlayer.vel;
  vectorMul(&scaledVel, sqrt(difficulty));
  vectorAdd(&inkinhPlayer.pos, scaledVel.x, scaledVel.y);

  inkinhPlayer.pos.x = clamp(inkinhPlayer.pos.x, 5, 95);
  inkinhPlayer.pos.y = clamp(inkinhPlayer.pos.y, 5, 95);

  float speed = sqrt(inkinhPlayer.vel.x * inkinhPlayer.vel.x + inkinhPlayer.vel.y * inkinhPlayer.vel.y);

  float targetStretch = 1 + speed * 0.3;
  inkinhPlayer.stretch += (targetStretch - inkinhPlayer.stretch) * 0.2;

  if (speed > 0.8 && ticks % 2 == 0) {
    ASSIGN_ARRAY_ITEM(inkinhTrails, inkinhTrailIndex, InkinhTrail, nt);
    vectorSet(&nt->pos, inkinhPlayer.pos.x, inkinhPlayer.pos.y);
    nt->alpha = 1;
    nt->isAlive = true;
    inkinhTrailIndex = cgl_wrap(inkinhTrailIndex + 1, 0, INKINH_MAX_TRAIL_COUNT);
  }

  color = LIGHT_CYAN;
  FOR_EACH(inkinhTrails, ti) {
    ASSIGN_ARRAY_ITEM(inkinhTrails, ti, InkinhTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->alpha -= 0.15;
    if (t->alpha > 0) {
      box(t->pos.x, t->pos.y, 4 * t->alpha, 4 * t->alpha, &scratch);
    } else {
      t->isAlive = false;
      continue;
    }
  }

  if (input.isPressed) {
    color = LIGHT_BLACK;
    thickness = 2;
    arc(inkinhPlayer.pos.x, inkinhPlayer.pos.y, 15, inkinhPlayer.angle - 0.6, inkinhPlayer.angle + 0.6, &scratch);
  }

  color = CYAN;
  float stretchX = 6 / inkinhPlayer.stretch;
  float stretchY = 6 * inkinhPlayer.stretch;
  box(inkinhPlayer.pos.x, inkinhPlayer.pos.y, stretchX, stretchY, &scratch);

  color = WHITE;
  float eyeOffsetX;
  float eyeOffsetY;
  if (speed > 0.3) {
    eyeOffsetX = inkinhPlayer.vel.x * 0.5;
    eyeOffsetY = inkinhPlayer.vel.y * 0.5;
  } else {
    eyeOffsetX = 0;
    eyeOffsetY = 0;
  }
  box(inkinhPlayer.pos.x - 1.5 + eyeOffsetX, inkinhPlayer.pos.y - 1 + eyeOffsetY, 2, 2, &scratch);
  box(inkinhPlayer.pos.x + 1.5 + eyeOffsetX, inkinhPlayer.pos.y - 1 + eyeOffsetY, 2, 2, &scratch);
  color = BLACK;
  box(inkinhPlayer.pos.x - 1.5 + eyeOffsetX * 1.5, inkinhPlayer.pos.y - 1 + eyeOffsetY * 1.5, 1, 1, &scratch);
  box(inkinhPlayer.pos.x + 1.5 + eyeOffsetX * 1.5, inkinhPlayer.pos.y - 1 + eyeOffsetY * 1.5, 1, 1, &scratch);

  float dirLen = 10;
  thickness = 2;
  line(inkinhPlayer.pos.x, inkinhPlayer.pos.y, inkinhPlayer.pos.x + cos(inkinhPlayer.angle) * dirLen,
       inkinhPlayer.pos.y + sin(inkinhPlayer.angle) * dirLen, &scratch);

  color = YELLOW;
  FOR_EACH(inkinhOrbs, oi2) {
    ASSIGN_ARRAY_ITEM(inkinhOrbs, oi2, InkinhOrb, o2);
    SKIP_IS_NOT_ALIVE(o2);
    o2->timer--;
    o2->phase += 0.1;
    float alpha;
    if (o2->timer > 60) {
      alpha = 1;
    } else {
      alpha = (float)o2->timer / 60;
    }
    float breathe = 1 + sin(o2->phase) * 0.15;
    float sz = (4 + alpha * 2) * breathe;
    color = YELLOW;
    Collision orbCollision;
    box(o2->pos.x, o2->pos.y, sz, sz, &orbCollision);
    bool coll = orbCollision.isColliding.rect[CYAN];

    if (alpha > 0.3) {
      float toPlayerX = inkinhPlayer.pos.x - o2->pos.x;
      float toPlayerY = inkinhPlayer.pos.y - o2->pos.y;
      float toPlayerLen = sqrt(toPlayerX * toPlayerX + toPlayerY * toPlayerY);
      float eyeDirX = 0;
      float eyeDirY = 0;
      if (toPlayerLen > 0) {
        eyeDirX = toPlayerX / toPlayerLen;
        eyeDirY = toPlayerY / toPlayerLen;
      }
      color = WHITE;
      box(o2->pos.x + eyeDirX * 0.5, o2->pos.y + eyeDirY * 0.5, 2, 2, &scratch);
      color = BLACK;
      box(o2->pos.x + eyeDirX * 1, o2->pos.y + eyeDirY * 1, 1, 1, &scratch);
      color = YELLOW;
    }

    if (coll) {
      play(COIN);
      color = YELLOW;
      particle(o2->pos.x, o2->pos.y, 8, 1.5, 0, CGLP_PI * 2);
      inkinhCombo++;
      inkinhComboTimer = 60;
      addScore(inkinhCombo, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      o2->isAlive = false;
      continue;
    }
    if (o2->timer <= 0) {
      o2->isAlive = false;
      continue;
    }
  }

  color = RED;
  FOR_EACH(inkinhObstacles, oi3) {
    ASSIGN_ARRAY_ITEM(inkinhObstacles, oi3, InkinhObstacle, ob);
    SKIP_IS_NOT_ALIVE(ob);
    vectorAdd(&ob->pos, ob->vel.x, ob->vel.y);
    ob->rot += ob->rotSpeed;
    if (ob->pos.y >= 110) {
      ob->isAlive = false;
      continue;
    }

    float sz = 3.5;
    float c = cos(ob->rot);
    float s = sin(ob->rot);
    Vector p1;
    Vector p2;
    Vector p3;
    Vector p4;
    vectorSet(&p1, ob->pos.x + sz * c, ob->pos.y + sz * s);
    vectorSet(&p2, ob->pos.x - sz * s, ob->pos.y + sz * c);
    vectorSet(&p3, ob->pos.x - sz * c, ob->pos.y - sz * s);
    vectorSet(&p4, ob->pos.x + sz * s, ob->pos.y - sz * c);
    color = RED;
    thickness = 2;
    line(p1.x, p1.y, p2.x, p2.y, &scratch);
    line(p2.x, p2.y, p3.x, p3.y, &scratch);
    line(p3.x, p3.y, p4.x, p4.y, &scratch);
    line(p4.x, p4.y, p1.x, p1.y, &scratch);

    color = RED;
    Collision obCollision;
    box(ob->pos.x, ob->pos.y, 7, 7, &obCollision);
    if (obCollision.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      color = RED;
      particle(ob->pos.x, ob->pos.y, 15, 2, 0, CGLP_PI * 2);
      gameOver();
    }
  }

  if (inkinhCombo > 1) {
    color = BLACK;
    int[16] comboText;
    strcpy(comboText, "x");
    strcat(comboText, intToChar(inkinhCombo));
    text(comboText, 3, 9, &scratch);
  }
}

void addGameInkinh() {
  addGame(inkinhTitle, inkinhDescription, inkinhCharacters, inkinhCharactersCount,
          &inkinhOptions, false, &inkinhUpdate);
}
