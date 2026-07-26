#include "../cglp.h"

int* sshakeTitle = "S SHAKE";
int* sshakeDescription = "[Tap]\n Shake";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] sshakeCharacters = {
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
int sshakeCharactersCount = 2;

Options sshakeOptions = {200, 100, 7, false};

struct SshakeGround {
  Vector pos;
  float angle;
  float height;
};
#define SSHAKE_GROUND_COUNT 22
SshakeGround[SSHAKE_GROUND_COUNT] sshakeGrounds;
float sshakeHeightRatio;

struct SshakeEnemy {
  Vector pos;
  Vector vel;
  bool isOnGround;
  float ticks;
  bool isAlive;
};
// Concurrent enemy count grows roughly with difficulty^1.5 (spawn interval
// shrinks as 1/difficulty^2 while lifetime only shrinks as ~1/sqrt(difficulty)
// via the speed formula below) - at difficulty 1 that's already ~11
// concurrent enemies, ~59 at difficulty 4, ~150 at difficulty 9. Sized well
// above what a long play session can reach so the ring buffer never wraps
// into a still-alive enemy (which silently replaces it - looks exactly like
// enemies vanishing when new ones spawn).
#define SSHAKE_MAX_ENEMY_COUNT 512
SshakeEnemy[SSHAKE_MAX_ENEMY_COUNT] sshakeEnemies;
int sshakeEnemyIndex;
float sshakeNextEnemyTicks;
int sshakeMultiplier;

void sshakeUpdate() {
  Collision scratch;
  if (!ticks) {
    float angle = 0;
    TIMES(SSHAKE_GROUND_COUNT, i) {
      angle += (CGLP_PI * 4) / 20;
      vectorSet(&sshakeGrounds[i].pos, i * 10, 0);
      sshakeGrounds[i].angle = angle;
      sshakeGrounds[i].height = 9;
    }
    sshakeHeightRatio = 1;
    INIT_UNALIVED_ARRAY_FAST(sshakeEnemies);
    sshakeEnemyIndex = 0;
    sshakeNextEnemyTicks = 0;
    sshakeMultiplier = 1;
  }
  float scr = difficulty * 0.3;
  if (input.isJustPressed) {
    play(JUMP);
    sshakeHeightRatio = 3;
    sshakeMultiplier = 1;
  }
  sshakeHeightRatio += (1 - sshakeHeightRatio) * 0.05;
  float maxY = 0;
  TIMES(SSHAKE_GROUND_COUNT, i) {
    SshakeGround* g = &sshakeGrounds[i];
    g->pos.x += scr;
    if (g->pos.x > 210) {
      g->pos.x -= 220;
      SshakeGround* ng = &sshakeGrounds[(int)cgl_wrap(i + 1, 0, 22)];
      g->angle = ng->angle - ((CGLP_PI * 4) / 20) * rnd(0.5, 1.5);
      g->height = ng->height + rnd(0, 1) * RNDPM();
      g->height += (9 - g->height) * 0.05;
    }
    g->pos.y = sin(g->angle) * g->height;
    if (g->pos.y > maxY) {
      maxY = g->pos.y;
    }
  }
  bool hasPp = false;
  Vector pp;
  TIMES(SSHAKE_GROUND_COUNT, i) {
    SshakeGround* g = &sshakeGrounds[i];
    g->pos.y = (g->pos.y - maxY) * sshakeHeightRatio + 99;
    if (hasPp && pp.x < g->pos.x) {
      line(pp.x, pp.y, g->pos.x, g->pos.y, &scratch);
    }
    pp = g->pos;
    hasPp = true;
  }
  Vector fp = sshakeGrounds[0].pos;
  Vector lp = sshakeGrounds[SSHAKE_GROUND_COUNT - 1].pos;
  if (lp.x < fp.x) {
    line(lp.x, lp.y, fp.x, fp.y, &scratch);
  }
  sshakeNextEnemyTicks--;
  if (sshakeNextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(sshakeEnemies, sshakeEnemyIndex, SshakeEnemy, ne);
    vectorSet(&ne->pos, 203, 50);
    vectorSet(&ne->vel, -rnd(1, sqrt(difficulty)) * 0.3 * sqrt(difficulty), 0);
    ne->isOnGround = true;
    ne->ticks = 0;
    ne->isAlive = true;
    sshakeEnemyIndex = cgl_wrap(sshakeEnemyIndex + 1, 0, SSHAKE_MAX_ENEMY_COUNT);
    sshakeNextEnemyTicks = rnd(0, 120) / difficulty / difficulty;
  }
  FOR_EACH(sshakeEnemies, i) {
    ASSIGN_ARRAY_ITEM(sshakeEnemies, i, SshakeEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    e->ticks -= e->vel.x;
    color = TRANSPARENT;
    if (e->isOnGround) {
      if (input.isJustPressed) {
        float vyy = 0;
        TIMES(99, k) {
          e->pos.y--;
          vyy--;
          Collision bc;
          box(e->pos.x, e->pos.y, 6, 6, &bc);
          if (bc.isColliding.rect[BLACK]) {
            e->vel.y = vyy * sqrt(difficulty) * 0.3;
            e->isOnGround = false;
            break;
          }
        }
      }
      Collision bc2;
      box(e->pos.x, e->pos.y, 6, 6, &bc2);
      if (bc2.isColliding.rect[BLACK]) {
        TIMES(99, k) {
          e->pos.y--;
          Collision bc3;
          box(e->pos.x, e->pos.y, 6, 6, &bc3);
          if (!bc3.isColliding.rect[BLACK]) {
            break;
          }
        }
      } else {
        TIMES(99, k) {
          e->pos.y++;
          Collision bc4;
          box(e->pos.x, e->pos.y, 6, 6, &bc4);
          if (bc4.isColliding.rect[BLACK]) {
            e->pos.y--;
            break;
          }
        }
      }
    } else {
      e->vel.y += 0.03 * difficulty;
      Collision bc5;
      box(e->pos.x, e->pos.y, 6, 6, &bc5);
      if (bc5.isColliding.rect[BLACK]) {
        e->isOnGround = true;
        e->vel.y = 0;
      } else if (e->vel.y > 0) {
        float ey = e->pos.y;
        TIMES(9, k) {
          ey -= 3;
          Collision bc6;
          box(e->pos.x, ey, 6, 6, &bc6);
          if (bc6.isColliding.rect[BLACK]) {
            e->pos.y = ey - 5;
            e->isOnGround = true;
            e->vel.y = 0;
            break;
          }
        }
      }
    }
    color = BLACK;
    int[2] ec;
    ec[0] = 'a' + (int)floor(e->ticks / 9) % 2;
    ec[1] = 0;
    characterOptions.isMirrorX = true;
    character(ec, e->pos.x, e->pos.y, &scratch);
    characterOptions.isMirrorX = false;
    if (e->pos.y < -3) {
      play(COIN);
      addScore(sshakeMultiplier, e->pos.x, clamp(9 + sshakeMultiplier * 3, 9, 60));
      sshakeMultiplier++;
      e->isAlive = false;
      continue;
    }
    if (e->pos.x < 3) {
      play(EXPLOSION);
      gameOver();
    }
    e->isAlive = e->pos.y <= 103;
  }
}

void addGameSshake() {
  addGame(sshakeTitle, sshakeDescription, sshakeCharacters,
          sshakeCharactersCount, &sshakeOptions, false, &sshakeUpdate);
}
