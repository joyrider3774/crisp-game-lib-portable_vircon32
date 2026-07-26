#include "../cglp.h"

int* skyraftsmanTitle = "SKY RAFTSMAN";
int* skyraftsmanDescription = "[Tap]\n Jump";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] skyraftsmanCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int skyraftsmanCharactersCount = 0;

Options skyraftsmanOptions = {100, 100, 2, false};

#define SKYRAFTSMAN_JUMP_POWER 2
#define SKYRAFTSMAN_GRAVITY 0.1

struct SkyraftsmanLog {
  Vector pos;
  float size;
  float rotationSpeed;
  float angle;
  Vector vel;
  float yDist;
  bool isAlive;
};
#define SKYRAFTSMAN_MAX_LOG_COUNT 32
SkyraftsmanLog[SKYRAFTSMAN_MAX_LOG_COUNT] skyraftsmanLogs;
int skyraftsmanLogIndex;
float skyraftsmanNextLogDist;

struct SkyraftsmanCloud {
  Vector pos;
  Vector size;
  bool isAlive;
};
#define SKYRAFTSMAN_MAX_CLOUD_COUNT 64
SkyraftsmanCloud[SKYRAFTSMAN_MAX_CLOUD_COUNT] skyraftsmanClouds;
int skyraftsmanCloudIndex;
float skyraftsmanNextCloudDist;

struct SkyraftsmanRaftsman {
  Vector pos;
  bool isJumping;
  int currentLogIndex; // index into skyraftsmanLogs instead of a live pointer (see gameAccelb.c's AccelbPlayerMissile)
  Vector vel;
};
SkyraftsmanRaftsman skyraftsmanRaftsman;

float skyraftsmanScrollDist;

void skyraftsmanUpdateClouds() {
  FOR_EACH(skyraftsmanClouds, ci) {
    ASSIGN_ARRAY_ITEM(skyraftsmanClouds, ci, SkyraftsmanCloud, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.y += skyraftsmanScrollDist / 2;
    if (c->pos.y < -20) {
      c->isAlive = false;
      continue;
    }
  }
  skyraftsmanNextCloudDist += skyraftsmanScrollDist;
  if (skyraftsmanNextCloudDist < 0) {
    int cc = rndi(3, 6);
    float cx = rnd(0, 100);
    TIMES(cc, i) {
      ASSIGN_ARRAY_ITEM(skyraftsmanClouds, skyraftsmanCloudIndex, SkyraftsmanCloud, nc);
      vectorSet(&nc->pos, cx + rnd(0, 20) * RNDPM(), 120 + rnd(0, 10));
      vectorSet(&nc->size, rnd(15, 25), rnd(5, 15));
      nc->isAlive = true;
      skyraftsmanCloudIndex = cgl_wrap(skyraftsmanCloudIndex + 1, 0, SKYRAFTSMAN_MAX_CLOUD_COUNT);
    }
    skyraftsmanNextCloudDist += rnd(120, 160);
  }
}

void skyraftsmanUpdateRaftsman() {
  SkyraftsmanLog* curLog = &skyraftsmanLogs[skyraftsmanRaftsman.currentLogIndex];
  if (input.isJustPressed && !skyraftsmanRaftsman.isJumping) {
    play(JUMP);
    skyraftsmanRaftsman.isJumping = true;
    vectorSet(&skyraftsmanRaftsman.vel, 0, 0);
    addWithAngle(&skyraftsmanRaftsman.vel, curLog->angle, SKYRAFTSMAN_JUMP_POWER);
    vectorAdd(&skyraftsmanRaftsman.pos, skyraftsmanRaftsman.vel.x * 2, skyraftsmanRaftsman.vel.y * 2);
    vectorSet(&curLog->vel, -skyraftsmanRaftsman.vel.x, -skyraftsmanRaftsman.vel.y);
  }
  if (skyraftsmanRaftsman.isJumping) {
    vectorAdd(&skyraftsmanRaftsman.pos, skyraftsmanRaftsman.vel.x * difficulty, skyraftsmanRaftsman.vel.y * difficulty);
    vectorMul(&skyraftsmanRaftsman.vel, 0.99);
    float gravDiv = 1;
    if (input.isPressed) {
      gravDiv = 2;
    }
    skyraftsmanRaftsman.vel.y += SKYRAFTSMAN_GRAVITY / gravDiv;
    skyraftsmanRaftsman.pos.y += skyraftsmanScrollDist;
  } else {
    vectorSet(&skyraftsmanRaftsman.pos, curLog->pos.x, curLog->pos.y);
    addWithAngle(&skyraftsmanRaftsman.pos, curLog->angle, curLog->size / 2);
  }
  if ((skyraftsmanRaftsman.pos.x < 3 && skyraftsmanRaftsman.vel.x < 0) ||
      (skyraftsmanRaftsman.pos.x > 97 && skyraftsmanRaftsman.vel.x > 0)) {
    skyraftsmanRaftsman.vel.x = -skyraftsmanRaftsman.vel.x;
  }
  if (skyraftsmanRaftsman.pos.y > 100) {
    play(EXPLOSION);
    gameOver();
  }
}

void skyraftsmanUpdateLogs() {
  FOR_EACH(skyraftsmanLogs, li) {
    ASSIGN_ARRAY_ITEM(skyraftsmanLogs, li, SkyraftsmanLog, log);
    SKIP_IS_NOT_ALIVE(log);
    vectorAdd(&log->pos, log->vel.x * difficulty, log->vel.y * difficulty);
    log->yDist += log->vel.y * difficulty;
    if (log->yDist > 20) {
      log->yDist -= 20;
      float s = floor(log->vel.y * difficulty * 5);
      if (s > 0) {
        if (log->pos.y < 100) {
          play(COIN);
          addScore(s, log->pos.x, log->pos.y);
        } else {
          addScore(s, log->pos.x, 99);
        }
      }
    }
    vectorMul(&log->vel, 0.99);
    log->pos.y += skyraftsmanScrollDist;
    if ((log->pos.x < log->size / 2 && log->vel.x < 0) ||
        (log->pos.x > 100 - log->size / 2 && log->vel.x > 0)) {
      log->vel.x = -log->vel.x;
    }
    log->angle += log->rotationSpeed * difficulty;
    if (log->pos.y < -99) {
      log->isAlive = false;
      continue;
    }
  }
  skyraftsmanNextLogDist += skyraftsmanScrollDist;
  if (skyraftsmanNextLogDist < 0 && !skyraftsmanRaftsman.isJumping) {
    ASSIGN_ARRAY_ITEM(skyraftsmanLogs, skyraftsmanLogIndex, SkyraftsmanLog, nl);
    vectorSet(&nl->pos, rnd(10, 90), 125);
    nl->size = rnd(15, 25);
    nl->rotationSpeed = rnd(0.04, 0.08) * RNDPM();
    nl->angle = 0;
    vectorSet(&nl->vel, 0, 0);
    nl->yDist = 0;
    nl->isAlive = true;
    skyraftsmanLogIndex = cgl_wrap(skyraftsmanLogIndex + 1, 0, SKYRAFTSMAN_MAX_LOG_COUNT);
    skyraftsmanNextLogDist = 100;
  }
}

void skyraftsmanDrawGame() {
  Collision scratch;
  color = LIGHT_CYAN;
  FOR_EACH(skyraftsmanClouds, ci) {
    ASSIGN_ARRAY_ITEM(skyraftsmanClouds, ci, SkyraftsmanCloud, c);
    SKIP_IS_NOT_ALIVE(c);
    box(c->pos.x, c->pos.y, c->size.x, c->size.y, &scratch);
  }
  color = RED;
  box(skyraftsmanRaftsman.pos.x, skyraftsmanRaftsman.pos.y, 6, 6, &scratch);
  color = YELLOW;
  FOR_EACH(skyraftsmanLogs, li) {
    ASSIGN_ARRAY_ITEM(skyraftsmanLogs, li, SkyraftsmanLog, log);
    SKIP_IS_NOT_ALIVE(log);
    if (log->pos.y > 100 + log->size / 2) {
      color = LIGHT_YELLOW;
      rect(log->pos.x - log->size / 2, 98, log->size, 2, &scratch);
    } else {
      color = YELLOW;
      thickness = 3;
      arc(log->pos.x, log->pos.y, log->size / 2, log->angle, log->angle + CGLP_PI * 2, &scratch);
      if (scratch.isColliding.rect[RED] && skyraftsmanRaftsman.isJumping) {
        play(CLICK);
        skyraftsmanRaftsman.isJumping = false;
        skyraftsmanRaftsman.currentLogIndex = li;
        log->angle = angleTo(&log->pos, skyraftsmanRaftsman.pos.x, skyraftsmanRaftsman.pos.y);
      }
    }
  }
}

void skyraftsmanUpdate() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(skyraftsmanLogs);
    skyraftsmanLogIndex = 0;
    TIMES(5, i) {
      ASSIGN_ARRAY_ITEM(skyraftsmanLogs, skyraftsmanLogIndex, SkyraftsmanLog, log);
      if (i == 0) {
        vectorSet(&log->pos, 50, 75 + i * 30);
        log->size = 20;
        log->rotationSpeed = -0.05;
      } else {
        vectorSet(&log->pos, rnd(10, 90), 75 + i * 30);
        log->size = rnd(15, 25);
        log->rotationSpeed = rnd(0.04, 0.08) * RNDPM();
      }
      log->angle = 0;
      vectorSet(&log->vel, 0, 0);
      log->yDist = 0;
      log->isAlive = true;
      skyraftsmanLogIndex = cgl_wrap(skyraftsmanLogIndex + 1, 0, SKYRAFTSMAN_MAX_LOG_COUNT);
    }
    vectorSet(&skyraftsmanRaftsman.pos, 0, 0);
    skyraftsmanRaftsman.isJumping = false;
    skyraftsmanRaftsman.currentLogIndex = 0;
    vectorSet(&skyraftsmanRaftsman.vel, 0, 0);
    skyraftsmanNextLogDist = 0;
    INIT_UNALIVED_ARRAY_FAST(skyraftsmanClouds);
    skyraftsmanCloudIndex = 0;
    skyraftsmanNextCloudDist = 0;
    skyraftsmanScrollDist = 0;
  }

  skyraftsmanScrollDist = -0.05;
  COUNT_IS_ALIVE(skyraftsmanLogs, logsAliveCount);
  if (skyraftsmanRaftsman.pos.y > 50 && logsAliveCount > 0) {
    skyraftsmanScrollDist += (50 - skyraftsmanRaftsman.pos.y) * 0.2;
  } else if (skyraftsmanRaftsman.pos.y < 20) {
    skyraftsmanScrollDist += (20 - skyraftsmanRaftsman.pos.y) * 0.1;
  }
  skyraftsmanScrollDist *= difficulty;

  skyraftsmanUpdateClouds();
  skyraftsmanUpdateRaftsman();
  skyraftsmanUpdateLogs();
  skyraftsmanDrawGame();
}

void addGameSkyraftsman() {
  addGame(skyraftsmanTitle, skyraftsmanDescription, skyraftsmanCharacters,
          skyraftsmanCharactersCount, &skyraftsmanOptions, false, &skyraftsmanUpdate);
}
