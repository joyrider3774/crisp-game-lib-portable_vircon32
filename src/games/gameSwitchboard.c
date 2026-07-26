#include "../cglp.h"

int* switchboardTitle = "SWITCHBOARD";
int* switchboardDescription = "[Hold]\n Connect calls";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] switchboardCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int switchboardCharactersCount = 0;

Options switchboardOptions = {100, 100, 1, false};

#define SWITCHBOARD_PLUG_COUNT 9

struct SwitchboardPlug {
  int id;
  Vector pos;
  bool active;
};
SwitchboardPlug[SWITCHBOARD_PLUG_COUNT] switchboardPlugs;

struct SwitchboardCursor {
  int currentPlugIndex;
  float angle;
  float speed;
};
SwitchboardCursor switchboardCursor;

struct SwitchboardRequest {
  int from;
  int to;
  float timeLimit;
  float maxTimeLimit;
  bool isAlive;
};
// Every request/connection ties up 2 of the 9 plugs, so at most ~4 of
// either can ever coexist - sized generously above that hard ceiling.
#define SWITCHBOARD_MAX_REQUEST_COUNT 16
SwitchboardRequest[SWITCHBOARD_MAX_REQUEST_COUNT] switchboardRequests;
int switchboardRequestIndex;

struct SwitchboardConnection {
  int from;
  int to;
  Vector fromPos;
  Vector toPos;
  float duration;
  float timeLeft;
  bool isAlive;
};
#define SWITCHBOARD_MAX_CONNECTION_COUNT 16
SwitchboardConnection[SWITCHBOARD_MAX_CONNECTION_COUNT] switchboardConnections;
int switchboardConnectionIndex;

struct SwitchboardConnLine {
  bool active;
  Vector startPos;
  Vector currentPos;
  int startPlugId;
  int endPlugId;
};
SwitchboardConnLine switchboardConnLine;

int switchboardContractsLost;
float switchboardNextRequestTimer;
float switchboardMultiplier;

void switchboardGenerateNewConnectionRequest() {
  bool[SWITCHBOARD_PLUG_COUNT] busy;
  TIMES(SWITCHBOARD_PLUG_COUNT, bi) {
    busy[bi] = false;
  }
  FOR_EACH(switchboardConnections, ci) {
    ASSIGN_ARRAY_ITEM(switchboardConnections, ci, SwitchboardConnection, c);
    SKIP_IS_NOT_ALIVE(c);
    busy[c->from - 1] = true;
    busy[c->to - 1] = true;
  }
  FOR_EACH(switchboardRequests, ri) {
    ASSIGN_ARRAY_ITEM(switchboardRequests, ri, SwitchboardRequest, r);
    SKIP_IS_NOT_ALIVE(r);
    busy[r->from - 1] = true;
    busy[r->to - 1] = true;
  }

  int[SWITCHBOARD_PLUG_COUNT] availablePlugIds;
  int availableCount = 0;
  TIMES(SWITCHBOARD_PLUG_COUNT, plugIdx) {
    if (!busy[plugIdx]) {
      availablePlugIds[availableCount] = plugIdx + 1;
      availableCount++;
    }
  }

  int[SWITCHBOARD_PLUG_COUNT] activeAvailablePlugs;
  int activeAvailableCount = 0;
  TIMES(availableCount, ai) {
    int id = availablePlugIds[ai];
    if (switchboardPlugs[id - 1].active) {
      activeAvailablePlugs[activeAvailableCount] = id;
      activeAvailableCount++;
    }
  }

  if (activeAvailableCount >= 1 && availableCount >= 2) {
    int fromIndex = rndi(0, activeAvailableCount);
    int fromId = activeAvailablePlugs[fromIndex];

    int toId;
    do {
      int toIndex = rndi(0, availableCount);
      toId = availablePlugIds[toIndex];
    } while (toId == fromId);

    float timeLimit = floor(rnd(600, 1200) / sqrt(difficulty));

    ASSIGN_ARRAY_ITEM(switchboardRequests, switchboardRequestIndex, SwitchboardRequest, nr);
    nr->from = fromId;
    nr->to = toId;
    nr->timeLimit = timeLimit;
    nr->maxTimeLimit = timeLimit;
    nr->isAlive = true;
    switchboardRequestIndex = cgl_wrap(switchboardRequestIndex + 1, 0, SWITCHBOARD_MAX_REQUEST_COUNT);
  }
}

void switchboardUpdate() {
  Collision scratch;
  // Never reads a Collision result - connections are decided by distance/angle math.
  hasCollision = false;
  float centerX = 50;
  float centerY = 50;
  float radius = 30;

  if (!ticks) {
    TIMES(SWITCHBOARD_PLUG_COUNT, i) {
      float angle = (i * CGLP_PI * 2) / 9 - CGLP_PI / 2;
      switchboardPlugs[i].id = i + 1;
      vectorSet(&switchboardPlugs[i].pos, centerX + cos(angle) * radius, centerY + sin(angle) * radius);
      switchboardPlugs[i].active = true;
    }

    switchboardCursor.currentPlugIndex = 0;
    switchboardCursor.angle = -CGLP_PI / 2;
    switchboardCursor.speed = 0.03;

    INIT_UNALIVED_ARRAY_FAST(switchboardRequests);
    switchboardRequestIndex = 0;
    ASSIGN_ARRAY_ITEM(switchboardRequests, switchboardRequestIndex, SwitchboardRequest, initR);
    initR->from = 3;
    initR->to = 7;
    initR->timeLimit = 900;
    initR->maxTimeLimit = 900;
    initR->isAlive = true;
    switchboardRequestIndex = cgl_wrap(switchboardRequestIndex + 1, 0, SWITCHBOARD_MAX_REQUEST_COUNT);

    switchboardNextRequestTimer = 180;

    switchboardConnLine.active = false;
    vectorSet(&switchboardConnLine.startPos, 0, 0);
    vectorSet(&switchboardConnLine.currentPos, 0, 0);
    switchboardConnLine.startPlugId = -1;
    switchboardConnLine.endPlugId = -1;

    INIT_UNALIVED_ARRAY_FAST(switchboardConnections);
    switchboardConnectionIndex = 0;
    switchboardContractsLost = 0;
    switchboardMultiplier = 1;
  }

  if (input.isPressed) {
    switchboardCursor.angle += switchboardCursor.speed * sqrt(difficulty) * 0.8;
  } else {
    switchboardCursor.angle += switchboardCursor.speed * sqrt(difficulty) * 1.4;
  }
  if (switchboardCursor.angle >= CGLP_PI * 2 - CGLP_PI / 2) {
    switchboardCursor.angle = -CGLP_PI / 2;
  }

  float cursorX = centerX + cos(switchboardCursor.angle) * radius;
  float cursorY = centerY + sin(switchboardCursor.angle) * radius;

  float minDistance = 999999;
  int closestPlugIndex = 0;
  TIMES(SWITCHBOARD_PLUG_COUNT, cpi) {
    float d = distanceTo(&switchboardPlugs[cpi].pos, cursorX, cursorY);
    if (d < minDistance) {
      minDistance = d;
      closestPlugIndex = cpi;
    }
  }
  switchboardCursor.currentPlugIndex = closestPlugIndex;

  switchboardMultiplier -= (switchboardMultiplier - 1) * 0.001;

  switchboardNextRequestTimer -= 1;
  if (switchboardNextRequestTimer <= 0) {
    switchboardGenerateNewConnectionRequest();
    switchboardNextRequestTimer = floor(rnd(100, 200) / sqrt(difficulty));
  }

  // Unlike upstream's splice-during-iterate quirk, the isAlive ring buffer checks every request once.
  FOR_EACH(switchboardRequests, ti) {
    ASSIGN_ARRAY_ITEM(switchboardRequests, ti, SwitchboardRequest, r);
    SKIP_IS_NOT_ALIVE(r);
    r->timeLimit -= 1;
    if (r->timeLimit <= 0) {
      switchboardPlugs[r->from - 1].active = false;
      switchboardContractsLost++;
      play(LASER);
      if (switchboardContractsLost >= 3) {
        gameOver();
      }
      r->isAlive = false;
      continue;
    }
  }

  if (input.isJustPressed && !switchboardConnLine.active) {
    float minD = 999999;
    int closestId = -1;
    TIMES(SWITCHBOARD_PLUG_COUNT, fpi) {
      if (switchboardPlugs[fpi].active) {
        float d = distanceTo(&switchboardPlugs[fpi].pos, cursorX, cursorY);
        if (d < minD) {
          minD = d;
          closestId = switchboardPlugs[fpi].id;
        }
      }
    }
    if (closestId != -1) {
      switchboardConnLine.active = true;
      vectorSet(&switchboardConnLine.startPos, switchboardPlugs[closestId - 1].pos.x, switchboardPlugs[closestId - 1].pos.y);
      switchboardConnLine.startPlugId = closestId;
      vectorSet(&switchboardConnLine.currentPos, switchboardPlugs[closestId - 1].pos.x, switchboardPlugs[closestId - 1].pos.y);
      play(SELECT);
    }
  }

  if (input.isPressed && switchboardConnLine.active) {
    vectorSet(&switchboardConnLine.currentPos, cursorX, cursorY);
  }

  if (input.isJustReleased && switchboardConnLine.active) {
    float minD2 = 999999;
    int closestId2 = -1;
    TIMES(SWITCHBOARD_PLUG_COUNT, epi) {
      float d = distanceTo(&switchboardPlugs[epi].pos, cursorX, cursorY);
      if (d < minD2) {
        minD2 = d;
        closestId2 = switchboardPlugs[epi].id;
      }
    }

    if (closestId2 != -1) {
      switchboardConnLine.endPlugId = closestId2;

      bool requestMatched = false;
      FOR_EACH(switchboardRequests, mri) {
        ASSIGN_ARRAY_ITEM(switchboardRequests, mri, SwitchboardRequest, req);
        SKIP_IS_NOT_ALIVE(req);
        bool isCorrectConnection =
            (switchboardConnLine.startPlugId == req->from && closestId2 == req->to) ||
            (switchboardConnLine.startPlugId == req->to && closestId2 == req->from);
        if (isCorrectConnection) {
          SwitchboardPlug* fromPlug = &switchboardPlugs[req->from - 1];
          SwitchboardPlug* toPlug = &switchboardPlugs[req->to - 1];
          float duration = floor(rnd(100, 200) / sqrt(difficulty));

          ASSIGN_ARRAY_ITEM(switchboardConnections, switchboardConnectionIndex, SwitchboardConnection, nc);
          nc->from = req->from;
          nc->to = req->to;
          vectorSet(&nc->fromPos, fromPlug->pos.x, fromPlug->pos.y);
          vectorSet(&nc->toPos, toPlug->pos.x, toPlug->pos.y);
          nc->duration = duration;
          nc->timeLeft = duration;
          nc->isAlive = true;
          switchboardConnectionIndex = cgl_wrap(switchboardConnectionIndex + 1, 0, SWITCHBOARD_MAX_CONNECTION_COUNT);

          if (!fromPlug->active) {
            fromPlug->active = true;
            switchboardContractsLost = max(0, switchboardContractsLost - 1);
          }
          if (!toPlug->active) {
            toPlug->active = true;
            switchboardContractsLost = max(0, switchboardContractsLost - 1);
          }

          float scoreGained = round(switchboardMultiplier);
          addScore(scoreGained, cursorX, cursorY);

          switchboardMultiplier++;

          // Upstream's 7 custom "connectingN.mp3" clips have no equivalent; substituted with COIN.
          play(COIN);
          req->isAlive = false;
          requestMatched = true;
        }
      }

      if (!requestMatched) {
        FOR_EACH(switchboardRequests, wri) {
          ASSIGN_ARRAY_ITEM(switchboardRequests, wri, SwitchboardRequest, req2);
          SKIP_IS_NOT_ALIVE(req2);
          if (switchboardConnLine.startPlugId == req2->from || closestId2 == req2->from) {
            SwitchboardPlug* fromPlug2 = &switchboardPlugs[req2->from - 1];
            fromPlug2->active = false;
            switchboardContractsLost++;
            play(LASER);
            if (switchboardContractsLost >= 3) {
              gameOver();
            }
            req2->isAlive = false;
          }
        }
      }
    }

    switchboardConnLine.active = false;
    vectorSet(&switchboardConnLine.startPos, 0, 0);
    vectorSet(&switchboardConnLine.currentPos, 0, 0);
    switchboardConnLine.startPlugId = -1;
    switchboardConnLine.endPlugId = -1;
  }

  FOR_EACH(switchboardConnections, uci) {
    ASSIGN_ARRAY_ITEM(switchboardConnections, uci, SwitchboardConnection, c);
    SKIP_IS_NOT_ALIVE(c);
    c->timeLeft -= 1;
    if (c->timeLeft <= 0) {
      c->isAlive = false;
      play(CLICK);
      continue;
    }
  }

  // Draw plugs
  FOR_EACH(switchboardPlugs, dpi) {
    ASSIGN_ARRAY_ITEM(switchboardPlugs, dpi, SwitchboardPlug, plug);
    color = BLACK;
    box(plug->pos.x, plug->pos.y, 7, 7, &scratch);

    if (plug->active) {
      color = WHITE;
    } else {
      color = LIGHT_BLACK;
    }
    box(plug->pos.x, plug->pos.y, 4, 4, &scratch);

    float toCenterAngle = cgl_atan2(centerY - plug->pos.y, centerX - plug->pos.x);
    Vector numberPos;
    vectorSet(&numberPos, plug->pos.x, plug->pos.y);
    addWithAngle(&numberPos, toCenterAngle, 10);

    color = BLACK;
    // Vircon32 port note: upstream draws this with an isSmallText option
    // this engine has no small-font variant for - drawn at normal size.
    text(intToChar(plug->id), numberPos.x, numberPos.y, &scratch);
  }

  // Display all connection requests
  FOR_EACH(switchboardRequests, dri) {
    ASSIGN_ARRAY_ITEM(switchboardRequests, dri, SwitchboardRequest, r);
    SKIP_IS_NOT_ALIVE(r);
    SwitchboardPlug* fromPlug = &switchboardPlugs[r->from - 1];

    float fromCenterAngle = cgl_atan2(fromPlug->pos.y - centerY, fromPlug->pos.x - centerX);
    Vector requestPos;
    vectorSet(&requestPos, fromPlug->pos.x, fromPlug->pos.y);
    addWithAngle(&requestPos, fromCenterAngle, 10);

    color = RED;
    text(intToChar(r->to), requestPos.x, requestPos.y, &scratch);

    float timeProgress = r->timeLimit / (1000 / sqrt(difficulty));
    float barWidth = 10 * timeProgress;

    color = RED;
    rect(requestPos.x - 5, requestPos.y + 4, barWidth, 1, &scratch);
  }

  // Draw cursor
  color = BLUE;
  box(cursorX, cursorY, 6, 6, &scratch);

  // Draw active connection lines
  FOR_EACH(switchboardConnections, dci) {
    ASSIGN_ARRAY_ITEM(switchboardConnections, dci, SwitchboardConnection, c);
    SKIP_IS_NOT_ALIVE(c);
    color = CYAN;
    float distance = distanceTo(&c->fromPos, c->toPos.x, c->toPos.y);
    float sag = distance * 0.15;

    float midX = (c->fromPos.x + c->toPos.x) / 2;
    float midY = (c->fromPos.y + c->toPos.y) / 2 + sag;

    int segments = 10;
    thickness = 3;
    TIMES(segments, si) {
      float t1 = (float)si / segments;
      float t2 = (float)(si + 1) / segments;

      float p1x = (1 - t1) * (1 - t1) * c->fromPos.x + 2 * (1 - t1) * t1 * midX + t1 * t1 * c->toPos.x;
      float p1y = (1 - t1) * (1 - t1) * c->fromPos.y + 2 * (1 - t1) * t1 * midY + t1 * t1 * c->toPos.y;
      float p2x = (1 - t2) * (1 - t2) * c->fromPos.x + 2 * (1 - t2) * t2 * midX + t2 * t2 * c->toPos.x;
      float p2y = (1 - t2) * (1 - t2) * c->fromPos.y + 2 * (1 - t2) * t2 * midY + t2 * t2 * c->toPos.y;

      line(p1x, p1y, p2x, p2y, &scratch);
    }

    SwitchboardPlug* fromPlug = &switchboardPlugs[c->from - 1];
    float fromCenterAngle = cgl_atan2(fromPlug->pos.y - centerY, fromPlug->pos.x - centerX);
    Vector barPos;
    vectorSet(&barPos, fromPlug->pos.x, fromPlug->pos.y);
    addWithAngle(&barPos, fromCenterAngle, 10);
    barPos.x -= 5;
    barPos.y += 4;

    float timeProgress = c->timeLeft / (500 / sqrt(difficulty));
    float barWidth = 10 * timeProgress;

    color = BLUE;
    rect(barPos.x, barPos.y, barWidth, 1, &scratch);
  }

  // Draw connection line in progress
  if (switchboardConnLine.active) {
    color = YELLOW;
    thickness = 2;
    line(switchboardConnLine.startPos.x, switchboardConnLine.startPos.y, switchboardConnLine.currentPos.x,
         switchboardConnLine.currentPos.y, &scratch);
  }

  color = BLACK;
  int[16] switchboardMultText;
  strcpy(switchboardMultText, "x");
  strcat(switchboardMultText, intToChar((int)round(switchboardMultiplier)));
  text(switchboardMultText, 3, 9, &scratch);
}

void addGameSwitchboard() {
  addGame(switchboardTitle, switchboardDescription, switchboardCharacters,
          switchboardCharactersCount, &switchboardOptions, false, &switchboardUpdate);
}
