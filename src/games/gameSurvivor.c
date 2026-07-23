#include "../cglp.h"

char* survivorTitle = "SURVIVOR";
char* survivorDescription = "[Tap] Jump";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] survivorCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  ll",
        " l    ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  l ",
        "    l ",
    },
    {
        " l  l ",
        " l  l ",
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
    }};
int survivorCharactersCount = 3;

Options survivorOptions = {100, 100, 2, false};

struct SurvivorPlayer {
  Vector pos;
  Vector vel;
  bool isOnFloor;
  bool isJumping;
  bool isJumped;
  SurvivorPlayer* underFoot;
  SurvivorPlayer* onHead;
  int ticks;
  bool isAlive;
};

struct DownedPlayer {
  Vector pos;
  Vector vel;
  bool isAlive;
};

struct SurvivorBarrel {
  Vector pos;
  float vx;
  float r;
  float angle;
  bool isAlive;
};

#define SURVIVOR_PLAYER_COUNT 9
SurvivorPlayer[SURVIVOR_PLAYER_COUNT] survivorPlayers;
DownedPlayer[SURVIVOR_PLAYER_COUNT] survivorDownedPlayers;
SurvivorBarrel survivorBarrel;

void survivorInitPlayers() {
  FOR_EACH(survivorPlayers, i) {
    survivorPlayers[i].isAlive = false;
    survivorDownedPlayers[i].isAlive = false;
  }
}

void survivorAddPlayer() {
  FOR_EACH(survivorPlayers, i) {
    ASSIGN_ARRAY_ITEM(survivorPlayers, i, SurvivorPlayer, pl);
    if (!pl->isAlive) {
      vectorSet(&pl->pos, rnd(10, 40), rnd(-9, 9));
      vectorSet(&pl->vel, 0, 0);
      pl->isOnFloor = false;
      pl->isJumping = false;
      pl->isJumped = false;
      pl->underFoot = NULL;
      pl->onHead = NULL;
      pl->ticks = rndi(0, 60);
      pl->isAlive = true;
      break;
    }
  }
}

void survivorAddPlayers() {
  play(POWER_UP);
  FOR_EACH(survivorPlayers, i) { survivorAddPlayer(); }
}

// Vircon32 port note: "Vector pos" was passed BY VALUE here (2 words -
// over the 1-word function-boundary limit), so it now takes a pointer.
void survivorAddDownedPlayer(Vector* pos, float vx, float vy) {
  FOR_EACH(survivorDownedPlayers, i) {
    ASSIGN_ARRAY_ITEM(survivorDownedPlayers, i, DownedPlayer, dp);
    if (!dp->isAlive) {
      dp->pos = *pos;
      vectorSet(&dp->vel, vx, vy);
      dp->isAlive = true;
      break;
    }
  }
}

void survivorUpdate() {
  Collision scratch;
  if (!ticks) {
    survivorInitPlayers();
    survivorBarrel.isAlive = false;
  }
  if (!survivorBarrel.isAlive) {
    survivorAddPlayers();
    float r = rnd(5, 25);
    vectorSet(&survivorBarrel.pos, 120 + r, 93 - r);
    survivorBarrel.vx = rnd(1, 2) / sqrt(r * 0.3 + 1);
    survivorBarrel.r = r;
    survivorBarrel.angle = rnd(0, M_PI * 2);
    survivorBarrel.isAlive = true;
  }
  survivorBarrel.pos.x -= survivorBarrel.vx * difficulty;
  thickness = 3 + survivorBarrel.r * 0.1;
  arc(survivorBarrel.pos.x, survivorBarrel.pos.y, survivorBarrel.r, survivorBarrel.angle, survivorBarrel.angle + M_PI * 2, &scratch);
  survivorBarrel.angle -= survivorBarrel.vx / survivorBarrel.r;
  particle(survivorBarrel.pos.x, survivorBarrel.pos.y + survivorBarrel.r, survivorBarrel.r * 0.05,
           survivorBarrel.vx * 5, -0.1, 0.2);
  rect(0, 93, 100, 7, &scratch);
  int addingPlayerCount = 0;
  FOR_EACH(survivorPlayers, i) {
    ASSIGN_ARRAY_ITEM(survivorPlayers, i, SurvivorPlayer, p);
    SKIP_IS_NOT_ALIVE(p);
    p->ticks++;
    if (p->underFoot == NULL) {
      FOR_EACH(survivorPlayers, j) {
        ASSIGN_ARRAY_ITEM(survivorPlayers, j, SurvivorPlayer, ap);
        SKIP_IS_NOT_ALIVE(ap);
        if (i != j && p->isOnFloor &&
            distanceTo(&p->pos, ap->pos.x, ap->pos.y) < 4) {
          play(SELECT);
          SurvivorPlayer* bp = p;
          for (int k = 0; k < 99; k++) {
            if (bp->underFoot == NULL) {
              break;
            }
            bp = bp->underFoot;
          }
          SurvivorPlayer* tp = ap;
          for (int k = 0; k < 99; k++) {
            if (tp->onHead == NULL) {
              break;
            }
            tp = tp->onHead;
          }
          tp->onHead = bp;
          bp->underFoot = tp;
          SurvivorPlayer* rp = p;
          for (int k = 0; k < 99; k++) {
            rp->isJumped = rp->isOnFloor = false;
            if (rp->onHead == NULL) {
              break;
            }
            rp = rp->onHead;
          }
          rp = p;
          for (int k = 0; k < 99; k++) {
            rp->isJumped = rp->isOnFloor = false;
            if (rp->underFoot == NULL) {
              break;
            }
            rp = rp->underFoot;
          }
        }
      }
    }
    if (input.isJustPressed &&
        (p->isOnFloor || (p->underFoot != NULL && p->underFoot->isJumped))) {
      play(JUMP);
      vectorSet(&p->vel, 0, -1.5);
      particle(p->pos.x, p->pos.y, 10, 2, M_PI / 2, 0.5);
      p->isOnFloor = false;
      p->isJumping = true;
      if (p->underFoot != NULL) {
        p->underFoot->onHead = NULL;
        p->underFoot = NULL;
      }
    }
    if (p->underFoot != NULL) {
      p->pos = p->underFoot->pos;
      p->pos.y -= 6;
      vectorSet(&p->vel, 0, 0);
    } else {
      vectorAdd(&(p->pos), p->vel.x * difficulty, p->vel.y * difficulty);
      p->vel.x *= 0.95;
      if ((p->pos.x < 7 && p->vel.x < 0) || (p->pos.x >= 77 && p->vel.x > 0)) {
        p->vel.x *= -0.5;
      }
      if (p->pos.x < 50) {
        p->vel.x += 0.01 * sqrt(50 - p->pos.x + 1);
      } else {
        p->vel.x -= 0.01 * sqrt(p->pos.x - 50 + 1);
      }
      if (p->isOnFloor) {
        if (p->pos.x < survivorBarrel.pos.x) {
          p->vel.x -=
              (0.1 * sqrt(survivorBarrel.r)) / sqrt(survivorBarrel.pos.x - p->pos.x + 1);
        }
      } else {
        p->vel.y += 0.1;
        if (p->pos.y > 90) {
          p->pos.y = 90;
          p->isOnFloor = true;
          p->isJumped = false;
          p->vel.y = 0;
        }
      }
      if (p->pos.y < 0 && p->vel.y < 0) {
        p->vel.y *= -0.5;
      }
    }
    int[2] pc;
    pc[0] = 'a' + (p->ticks / 30) % 2;
    pc[1] = 0;
    Collision pcl;
    character(pc, p->pos.x, p->pos.y, &pcl);
    if (pcl.isColliding.rect[BLACK]) {
      if (p->onHead != NULL) {
        p->onHead->underFoot = NULL;
        p->onHead->isJumping = true;
      }
      if (p->underFoot != NULL) {
        p->underFoot->onHead = NULL;
      }
      play(HIT);
      survivorAddDownedPlayer(&p->pos, p->vel.x - survivorBarrel.vx * 2, p->vel.y);
      p->isAlive = false;
      continue;
    }
    if (p->pos.x < 0 || p->pos.x > 100 || p->pos.y < -50 || p->pos.y > 100) {
      addingPlayerCount++;
      p->isAlive = false;
      continue;
    }
  }
  TIMES(addingPlayerCount, i) { survivorAddPlayer(); }
  FOR_EACH(survivorPlayers, i) {
    ASSIGN_ARRAY_ITEM(survivorPlayers, i, SurvivorPlayer, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->isJumping) {
      p->isJumped = true;
      p->isJumping = false;
    }
  }
  FOR_EACH(survivorDownedPlayers, i) {
    ASSIGN_ARRAY_ITEM(survivorDownedPlayers, i, DownedPlayer, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.x += p->vel.x;
    p->pos.y += p->vel.y;
    p->vel.y += 0.2;
    character("c", p->pos.x, p->pos.y, &scratch);
    p->isAlive = p->pos.y < 105;
  }
  COUNT_IS_ALIVE(survivorPlayers, playerCount);
  if (playerCount == 0) {
    play(RANDOM);
    gameOver();
  }
  if (survivorBarrel.pos.x < -survivorBarrel.r) {
    survivorBarrel.isAlive = false;
    addScore(playerCount, 10, 50);
  }
}

void addGameSurvivor() {
  addGame(survivorTitle, survivorDescription, survivorCharacters,
          survivorCharactersCount, &survivorOptions, false, &survivorUpdate);
}
