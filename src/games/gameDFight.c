#include "../cglp.h"

char* dfightTitle = "D FIGHT";
char* dfightDescription = "[Slide] Move";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] dfightCharacters = {
    {
        " l    ",
        "llll  ",
        " l    ",
        "l l   ",
        "l  l  ",
        "      ",
    },
    {
        " l    ",
        "llll  ",
        " l    ",
        "  l   ",
        " l    ",
        "      ",
    },
    {
        " ll   ",
        "llllll",
        " ll   ",
        " ll   ",
        "ll ll ",
        "ll  ll",
    },
    {
        " ll   ",
        "llllll",
        " ll   ",
        " ll   ",
        "  ll  ",
        " ll   ",
    }
};

int dfightCharactersCount = 4;

Options dfightOptions = {200, 100, 800, false};

struct DfightDome {
  float x;
  bool isAlive;
};

struct DfightAgent {
  Vector pos;
  Vector vel;
  DfightAgent* target;
  float targetDistance;
  float aimLength;
  float fireLength;
  int moveTicks;
  int mirrorX;
  int type;  // 'p' for player, 'a' for ally, 'e' for enemy
  bool isAlive;
};

#define DFIGHT_MAX_DOME_COUNT 10
DfightDome[DFIGHT_MAX_DOME_COUNT] dfightDomes;
int dfightDomeIndex;
float dfightNextDomeDist;

#define DFIGHT_MAX_AGENT_COUNT 25
DfightAgent[DFIGHT_MAX_AGENT_COUNT] dfightAgents;
int dfightAgentIndex;
float dfightNextAllyAgentTicks;
float dfightNextEnemyAgentTicks;
int dfightEnemyCount;
int dfightAllyCount;
float dfightDomeRadius = 40;
float dfightMaxAimDistance = 99;

// Forward declarations
DfightAgent* dfightGetNearest(DfightAgent* a);
void dfightAddAgent(int type);

void dfightUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(dfightDomes);
    dfightDomeIndex = 0;

    ASSIGN_ARRAY_ITEM(dfightDomes, dfightDomeIndex, DfightDome, d);
    d->x = 100;
    d->isAlive = true;
    dfightDomeIndex = cgl_wrap(dfightDomeIndex + 1, 0, DFIGHT_MAX_DOME_COUNT);

    dfightNextDomeDist = 0;

    INIT_UNALIVED_ARRAY(dfightAgents);
    dfightAgentIndex = 0;

    ASSIGN_ARRAY_ITEM(dfightAgents, dfightAgentIndex, DfightAgent, a);
    vectorSet(&a->pos, 99, 87);
    vectorSet(&a->vel, 0, 0);
    a->target = NULL;
    a->targetDistance = 0;
    a->aimLength = 0;
    a->fireLength = 0;
    a->moveTicks = 0;
    a->mirrorX = 1;
    a->type = 'p';
    a->isAlive = true;
    dfightAgentIndex = cgl_wrap(dfightAgentIndex + 1, 0, DFIGHT_MAX_AGENT_COUNT);

    dfightNextAllyAgentTicks = 9;
    dfightNextEnemyAgentTicks = 0;
    dfightEnemyCount = 0;
    dfightAllyCount = 0;
  }

  float sd = sqrt(difficulty);
  float scr = sd * 0.2;

  // Update domes
  dfightNextDomeDist -= scr;
  if (dfightNextDomeDist < 0) {
    ASSIGN_ARRAY_ITEM(dfightDomes, dfightDomeIndex, DfightDome, d);
    d->x = 200 + dfightDomeRadius;
    d->isAlive = true;
    dfightDomeIndex = cgl_wrap(dfightDomeIndex + 1, 0, DFIGHT_MAX_DOME_COUNT);
    dfightNextDomeDist += rnd(70, 150);
  }

  color = YELLOW;
  thickness = 3;
  FOR_EACH(dfightDomes, i) {
    ASSIGN_ARRAY_ITEM(dfightDomes, i, DfightDome, d);
    SKIP_IS_NOT_ALIVE(d);

    d->x -= scr;
    arc(d->x, 90, dfightDomeRadius, M_PI, M_PI * 2, &scratch);

    d->isAlive = d->x >= -dfightDomeRadius;
  }
  thickness = 1;

  // Draw ground
  color = LIGHT_BLACK;
  rect(0, 90, 200, 10, &scratch);

  // Update ally agents
  dfightNextAllyAgentTicks--;
  if (dfightNextAllyAgentTicks < 0) {
    dfightAddAgent('a');
    dfightAllyCount++;
    dfightNextAllyAgentTicks = (rnd(120, 150) * dfightAllyCount) / sd;
  }

  // Update enemy agents
  if (dfightEnemyCount == 0) {
    dfightNextEnemyAgentTicks = 0;
  }
  dfightNextEnemyAgentTicks--;
  if (dfightNextEnemyAgentTicks < 0) {
    dfightAddAgent('e');
    dfightEnemyCount++;
    dfightNextEnemyAgentTicks += (rnd(60, 80) * dfightEnemyCount) / sd;
  }

  dfightAllyCount = 0;
  dfightEnemyCount = 0;

  Vector p;
  vectorSet(&p, 0, 0);

  FOR_EACH(dfightAgents, i) {
    ASSIGN_ARRAY_ITEM(dfightAgents, i, DfightAgent, a);
    SKIP_IS_NOT_ALIVE(a);

    if (a->type == 'p') {
      a->pos.x += (clamp(input.pos.x, 3, 197) - a->pos.x) * 0.2;
    } else {
      if (a->type == 'a') {
        dfightAllyCount++;
      } else {
        dfightEnemyCount++;
      }

      a->vel.y += difficulty * 0.05;

      if (a->pos.x < 3 && a->vel.x < 0) {
        a->pos.x = 3;
        a->vel.x *= -1;
      }

      if (a->pos.x > 197 && a->vel.x > 0) {
        a->pos.x = 197;
        a->vel.x *= -1;
      }

      if (a->pos.y >= 87) {
        a->pos.y = 87;
        a->vel.y = 0;
        if (rnd(0, 1) < 0.01) {
          a->vel.y = -rnd(3, 6) * sd;
        }
      }
    }

    vectorAdd(&a->pos, a->vel.x, a->vel.y);
    vectorMul(&a->vel, 0.9);

    if (a->target == NULL) {
      a->target = dfightGetNearest(a);
      continue;
    }

    float d = distanceTo(&a->pos, a->target->pos.x, a->target->pos.y);
    float an = angleTo(&a->pos, a->target->pos.x, a->target->pos.y);
    if (fabs(an) < M_PI / 2) {
      a->mirrorX = 1;
    } else {
      a->mirrorX = -1;
    }

    if (!a->target->isAlive || d > dfightMaxAimDistance) {
      a->target = NULL;
      a->aimLength = 0;
      a->fireLength = 0;
      continue;
    }

    if (a->type != 'p') {
      float vx = -1;
      if (a->pos.x > a->target->pos.x) {
        vx *= -1;
      }
      if (d > a->targetDistance) {
        vx *= -1;
      }
      a->vel.x += vx * sd * 0.1;
    }

    if (a->fireLength > 0) {
      a->fireLength += sd * 3;

      if (a->type == 'e') {
        color = RED;
      } else {
        color = BLUE;
      }

      vectorSet(&p, a->pos.x, a->pos.y);
      addWithAngle(&p, an, a->fireLength);

      barCenterPosRatio = 1.0;
      thickness = 4;
      Collision c;
      bar(p.x, p.y, 9, an, &c);
      thickness = 1;
      a->aimLength = d;

      if (c.isColliding.rect[YELLOW] || p.y > 90 || a->fireLength > dfightMaxAimDistance) {
        a->fireLength = 0;
        a->aimLength = 0;

        if (c.isColliding.rect[YELLOW]) {
          play(HIT);
          particle(p.x, p.y, 9, 3, an, 0.3);
        }
      }
    } else {
      float aimFactor;
      if (a->type == 'e') {
        aimFactor = 0.4;
      } else if (a->type == 'a') {
        aimFactor = 0.2;
      } else {
        aimFactor = 0.9;
      }
      a->aimLength += sd * aimFactor * d * 0.03;

      if (a->aimLength > d) {
        play(LASER);
        a->fireLength = 5;
      }

      if (a->type == 'e') {
        color = RED;
      } else if (a->type == 'a') {
        color = BLUE;
      } else {
        color = GREEN;
      }

      vectorSet(&p, a->pos.x, a->pos.y);
      addWithAngle(&p, an, a->aimLength);
      text("x", p.x, p.y, &scratch);

      if (a->type == 'e') {
        color = LIGHT_RED;
      } else {
        color = LIGHT_BLUE;
      }

      barCenterPosRatio = 0.0;
      thickness = 1;
      bar(a->pos.x, a->pos.y, a->aimLength, an, &scratch);
    }
  }

  // Check collisions and remove agents
  FOR_EACH(dfightAgents, i) {
    ASSIGN_ARRAY_ITEM(dfightAgents, i, DfightAgent, a);
    SKIP_IS_NOT_ALIVE(a);

    if (a->type == 'e') {
      color = PURPLE;
    } else if (a->type == 'a') {
      color = BLUE;
    } else {
      color = GREEN;
    }

    a->moveTicks++;

    characterOptions.isMirrorX = a->mirrorX < 0;

    int[2] charToUse;
    if (a->type == 'p') {
      charToUse[0] = 'c';
    } else {
      charToUse[0] = 'a';
    }
    if (a->pos.y < 87) {
      charToUse[0] += 0;
    } else {
      charToUse[0] += (a->moveTicks / 15) % 2;
    }
    charToUse[1] = 0;

    Collision c;
    character(charToUse, a->pos.x, a->pos.y, &c);

    if (a->type == 'e' && c.isColliding.rect[BLUE]) {
      play(POWER_UP);
      particle(a->pos.x, a->pos.y, 5, 1, 0, M_PI * 2);
      addScore(1, a->pos.x, a->pos.y);
      a->isAlive = false;
    } else if (a->type == 'a' && c.isColliding.rect[RED]) {
      play(COIN);
      particle(a->pos.x, a->pos.y, 5, 1, 0, M_PI * 2);
      a->isAlive = false;
    } else if (a->type == 'p' && c.isColliding.rect[RED]) {
      play(EXPLOSION);
      gameOver();
    }
  }

  barCenterPosRatio = 0.5;
}

void dfightAddAgent(int type) {
  float x;
  if (rnd(0, 1) < 0.3) {
    x = -3;
  } else {
    x = 203;
  }
  float vxSign;
  if (x < 0) {
    vxSign = 1;
  } else {
    vxSign = -1;
  }
  float vx = vxSign * sqrt(difficulty);
  int mx;
  if (x < 0) {
    mx = 1;
  } else {
    mx = -1;
  }

  ASSIGN_ARRAY_ITEM(dfightAgents, dfightAgentIndex, DfightAgent, a);
  vectorSet(&a->pos, x, 87);
  vectorSet(&a->vel, vx, 0);
  a->target = NULL;
  a->targetDistance = rnd(30, 60);
  a->aimLength = 0;
  a->fireLength = 0;
  a->moveTicks = 0;
  a->mirrorX = mx;
  a->type = type;
  a->isAlive = true;
  dfightAgentIndex = cgl_wrap(dfightAgentIndex + 1, 0, DFIGHT_MAX_AGENT_COUNT);
}

DfightAgent* dfightGetNearest(DfightAgent* a) {
  float nd = dfightMaxAimDistance;
  DfightAgent* nearest = NULL;

  FOR_EACH(dfightAgents, i) {
    ASSIGN_ARRAY_ITEM(dfightAgents, i, DfightAgent, aa);
    SKIP_IS_NOT_ALIVE(aa);

    if (a == aa) {
      continue;
    }

    if ((a->type == 'p' || a->type == 'a') && (aa->type == 'p' || aa->type == 'a')) {
      continue;
    }

    if (a->type == 'e' && aa->type == 'e') {
      continue;
    }

    float d = distanceTo(&a->pos, aa->pos.x, aa->pos.y);
    if (d < nd) {
      nd = d;
      nearest = aa;
    }
  }

  return nearest;
}

void addGameDFight() {
  addGame(dfightTitle, dfightDescription, dfightCharacters, dfightCharactersCount,
          &dfightOptions, true, &dfightUpdate);
}
