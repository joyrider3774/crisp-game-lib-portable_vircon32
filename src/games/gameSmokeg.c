#include "../cglp.h"

int* smokegTitle = "SMOKE G";
int* smokegDescription = "[Tap]\n Smoke";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] smokegCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
};
int smokegCharactersCount = 4;

Options smokegOptions = {100, 100, 9, false};

#define SMOKEG_SHOT_RANGE 50

struct SmokegGrenade {
  Vector pos;
  Vector target;
  float ticks;
  bool isAlive;
};
#define SMOKEG_MAX_GRENADE_COUNT 32
SmokegGrenade[SMOKEG_MAX_GRENADE_COUNT] smokegGrenades;
int smokegGrenadeIndex;

struct SmokegSmoke {
  Vector pos;
  float radius;
  bool isExtending;
  bool isAlive;
};
#define SMOKEG_MAX_SMOKE_COUNT 16
SmokegSmoke[SMOKEG_MAX_SMOKE_COUNT] smokegSmokes;
int smokegSmokeIndex;

struct SmokegEnemy {
  Vector pos;
  float angle;
  bool isAlive;
};
#define SMOKEG_MAX_ENEMY_COUNT 64
SmokegEnemy[SMOKEG_MAX_ENEMY_COUNT] smokegEnemies;
int smokegEnemyIndex;
float smokegNextEnemyDist;
int smokegCurrentSide;
float smokegNextSideChangeCount;

struct SmokegPlayer {
  Vector pos;
  float angle;
};
SmokegPlayer smokegPlayer;

void smokegUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(smokegGrenades);
    smokegGrenadeIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(smokegSmokes);
    smokegSmokeIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(smokegEnemies);
    smokegEnemyIndex = 0;
    smokegNextEnemyDist = 0;
    smokegCurrentSide = 1;
    smokegNextSideChangeCount = 0;
    vectorSet(&smokegPlayer.pos, 50, 90);
    smokegPlayer.angle = -CGLP_PI / 2;
  }
  float scr = sqrt(difficulty) * 0.1;
  COUNT_IS_ALIVE(smokegSmokes, smokegAliveSmokeCount);
  if (input.isJustPressed && smokegAliveSmokeCount < 9) {
    play(SELECT);
    ASSIGN_ARRAY_ITEM(smokegGrenades, smokegGrenadeIndex, SmokegGrenade, ng);
    ng->pos = smokegPlayer.pos;
    vectorSet(&ng->target, clamp(input.pos.x, 0, 99), clamp(input.pos.y, 0, 99));
    ng->ticks = 0;
    ng->isAlive = true;
    smokegGrenadeIndex = cgl_wrap(smokegGrenadeIndex + 1, 0, SMOKEG_MAX_GRENADE_COUNT);
  }
  color = LIGHT_BLACK;
  FOR_EACH(smokegGrenades, gi) {
    ASSIGN_ARRAY_ITEM(smokegGrenades, gi, SmokegGrenade, g);
    SKIP_IS_NOT_ALIVE(g);
    g->ticks += sqrt(difficulty);
    g->pos.x += (g->target.x - g->pos.x) * 0.1;
    g->pos.y += (g->target.y - g->pos.y) * 0.1;
    g->pos.y += cos((g->ticks / 30) * CGLP_PI * 4) + scr;
    box(g->pos.x, g->pos.y, 4, 4, &scratch);
    if (g->ticks > 30) {
      play(HIT);
      ASSIGN_ARRAY_ITEM(smokegSmokes, smokegSmokeIndex, SmokegSmoke, ns);
      ns->pos = g->target;
      ns->radius = 0;
      ns->isExtending = true;
      ns->isAlive = true;
      smokegSmokeIndex = cgl_wrap(smokegSmokeIndex + 1, 0, SMOKEG_MAX_SMOKE_COUNT);
      g->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(smokegSmokes, si) {
    ASSIGN_ARRAY_ITEM(smokegSmokes, si, SmokegSmoke, s);
    SKIP_IS_NOT_ALIVE(s);
    if (s->isExtending) {
      s->radius += (10 - s->radius) * 0.2 * sqrt(difficulty);
      if (s->radius > 9) {
        s->isExtending = false;
        s->radius = 10;
      }
    } else {
      s->radius *= 1 - 0.005 * sqrt(difficulty);
    }
    s->pos.y += scr;
    thickness = 3;
    arc(s->pos.x, s->pos.y, s->radius, 0, CGLP_PI * 2, &scratch);
    if (!s->isExtending && s->radius < 2) {
      s->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(smokegEnemies, smokegAliveEnemyCount0);
  if (smokegAliveEnemyCount0 == 0) {
    smokegNextEnemyDist = 0;
  }
  smokegNextEnemyDist -= scr;
  if (smokegNextEnemyDist < 0) {
    smokegNextSideChangeCount--;
    if (smokegNextSideChangeCount < 0) {
      smokegCurrentSide *= -1;
      smokegNextSideChangeCount = rnd(1, 5);
      smokegNextEnemyDist += 7;
    }
    float ex = 50 + rnd(0, 40) * smokegCurrentSide;
    float ey = -3;
    ASSIGN_ARRAY_ITEM(smokegEnemies, smokegEnemyIndex, SmokegEnemy, ne);
    vectorSet(&ne->pos, ex, ey);
    ne->angle = angleTo(&smokegPlayer.pos, ex, ey) + rnd(0, 0.2) * RNDPM();
    ne->isAlive = true;
    smokegEnemyIndex = cgl_wrap(smokegEnemyIndex + 1, 0, SMOKEG_MAX_ENEMY_COUNT);
    smokegNextEnemyDist += rnd(5, 9);
  }
  bool hasTe = false;
  int teIndex = -1;
  float minDist = 99;
  color = TRANSPARENT;
  FOR_EACH(smokegEnemies, ei) {
    ASSIGN_ARRAY_ITEM(smokegEnemies, ei, SmokegEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float ta = angleTo(&smokegPlayer.pos, e->pos.x, e->pos.y);
    float d = distanceTo(&smokegPlayer.pos, e->pos.x, e->pos.y);
    thickness = 2;
    barCenterPosRatio = 0;
    Collision ec;
    bar(smokegPlayer.pos.x, smokegPlayer.pos.y, d, ta, &ec);
    if (ec.isColliding.rect[BLACK]) {
      continue;
    }
    if (d < minDist) {
      minDist = d;
      hasTe = true;
      teIndex = ei;
    }
  }
  if (hasTe) {
    float ta2 = angleTo(&smokegPlayer.pos, smokegEnemies[teIndex].pos.x,
                         smokegEnemies[teIndex].pos.y);
    float oa = cgl_wrap(ta2 - smokegPlayer.angle, -CGLP_PI, CGLP_PI);
    float av = 0.012 * sqrt(difficulty);
    if (fabs(oa) < av) {
      smokegPlayer.angle = ta2;
    } else {
      if (oa < 0) {
        smokegPlayer.angle -= av;
      } else {
        smokegPlayer.angle += av;
      }
    }
  }
  color = LIGHT_CYAN;
  thickness = 2;
  barCenterPosRatio = 0;
  bar(smokegPlayer.pos.x, smokegPlayer.pos.y, SMOKEG_SHOT_RANGE * 1.1, smokegPlayer.angle,
      &scratch);
  color = BLUE;
  int[2] pc;
  pc[0] = 'a' + (ticks / 20) % 2;
  pc[1] = 0;
  float wrappedPlayerAngle = cgl_wrap(smokegPlayer.angle, -CGLP_PI, CGLP_PI);
  characterOptions.isMirrorX = fabs(wrappedPlayerAngle) >= CGLP_PI / 2;
  character(pc, smokegPlayer.pos.x, smokegPlayer.pos.y, &scratch);
  characterOptions.isMirrorX = false;
  FOR_EACH(smokegEnemies, ei2) {
    ASSIGN_ARRAY_ITEM(smokegEnemies, ei2, SmokegEnemy, e2);
    SKIP_IS_NOT_ALIVE(e2);
    e2->pos.y += scr;
    color = TRANSPARENT;
    float d1 = distanceTo(&e2->pos, smokegPlayer.pos.x, smokegPlayer.pos.y);
    float ta1 = angleTo(&e2->pos, smokegPlayer.pos.x, smokegPlayer.pos.y);
    thickness = 2;
    barCenterPosRatio = 0;
    Collision c1;
    bar(e2->pos.x, e2->pos.y, d1, ta1, &c1);
    float av2 = 0.008 * sqrt(difficulty);
    bool blocked = c1.isColliding.rect[BLACK];
    if (!blocked) {
      float ta2b = angleTo(&e2->pos, smokegPlayer.pos.x, smokegPlayer.pos.y);
      float oa2 = cgl_wrap(ta2b - e2->angle, -CGLP_PI, CGLP_PI);
      if (fabs(oa2) < av2) {
        e2->angle = ta2b;
      } else {
        if (oa2 < 0) {
          e2->angle -= av2;
        } else {
          e2->angle += av2;
        }
      }
      color = LIGHT_PURPLE;
    } else {
      color = LIGHT_BLACK;
    }
    thickness = 2;
    barCenterPosRatio = 0;
    Collision c2;
    bar(e2->pos.x, e2->pos.y, SMOKEG_SHOT_RANGE, e2->angle, &c2);
    if (!blocked && (c2.isColliding.character['a'] || c2.isColliding.character['b'])) {
      play(EXPLOSION);
      color = PURPLE;
      thickness = 4;
      barCenterPosRatio = 0;
      bar(e2->pos.x, e2->pos.y, SMOKEG_SHOT_RANGE, e2->angle, &scratch);
      gameOver();
    }
    if (blocked) {
      color = LIGHT_RED;
    } else {
      color = RED;
    }
    int[2] ec2;
    ec2[0] = 'c' + (ticks / 30) % 2;
    ec2[1] = 0;
    float wrappedEnemyAngle = cgl_wrap(e2->angle, -CGLP_PI, CGLP_PI);
    characterOptions.isMirrorX = fabs(wrappedEnemyAngle) >= CGLP_PI / 2;
    Collision c3;
    character(ec2, e2->pos.x, e2->pos.y, &c3);
    characterOptions.isMirrorX = false;
    if (c3.isColliding.rect[LIGHT_CYAN]) {
      play(LASER);
      color = CYAN;
      thickness = 4;
      barCenterPosRatio = 0;
      bar(smokegPlayer.pos.x, smokegPlayer.pos.y, SMOKEG_SHOT_RANGE, smokegPlayer.angle,
          &scratch);
      particle(smokegPlayer.pos.x, smokegPlayer.pos.y, 20, 3, smokegPlayer.angle, 0);
      color = RED;
      particle(e2->pos.x, e2->pos.y, 16, 1, 0, CGLP_PI * 2);
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      e2->isAlive = false;
      continue;
    }
    if (e2->pos.y > 99) {
      play(EXPLOSION);
      color = RED;
      text("X", e2->pos.x, 96, &scratch);
      gameOver();
    }
  }
}

void addGameSmokeg() {
  addGame(smokegTitle, smokegDescription, smokegCharacters, smokegCharactersCount, &smokegOptions,
          true, &smokegUpdate);
}
