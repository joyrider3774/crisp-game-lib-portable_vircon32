#include "../cglp.h"

int* ropecorralTitle = "ROPE CORRAL";
int* ropecorralDescription = "[Hold] Anchor & swing\n[Release] Roll on";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] ropecorralCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int ropecorralCharactersCount = 0;

Options ropecorralOptions = {100, 100, 45, false};

#define ROPECORRAL_ROPE 22

struct RopecorralPlayer {
  Vector pos;
  float vx;
  float vy;
};
RopecorralPlayer ropecorralPlayer;

struct RopecorralBuddy {
  Vector pos;
};
RopecorralBuddy ropecorralBuddy;

struct RopecorralEnemy {
  Vector pos;
  float ang;
  bool isAlive;
};
#define ROPECORRAL_MAX_ENEMY_COUNT 32
RopecorralEnemy[ROPECORRAL_MAX_ENEMY_COUNT] ropecorralEnemies;
int ropecorralEnemyIndex;

struct RopecorralSpike {
  Vector pos;
  float warm;
  bool isAlive;
};
#define ROPECORRAL_MAX_SPIKE_COUNT 16
RopecorralSpike[ROPECORRAL_MAX_SPIKE_COUNT] ropecorralSpikes;
int ropecorralSpikeIndex;

float ropecorralSwingA;
float ropecorralSwept;
float ropecorralNextCritT;
float ropecorralHitstop;
float ropecorralShakeMag;
float ropecorralShakeT;

// A single-pixel "eye" toward a facing angle - bodies here are only 5px,
// too small for a readable whites+pupil pair, so one offset dot stands in.
void ropecorralEye(float x, float y, float ang, float dist) {
  Collision scratch;
  color = BLACK;
  box(x + cos(ang) * dist, y + sin(ang) * dist, 1, 1, &scratch);
}

void ropecorralSpawnCritter() {
  ASSIGN_ARRAY_ITEM(ropecorralEnemies, ropecorralEnemyIndex, RopecorralEnemy, ne);
  vectorSet(&ne->pos, rnd(15, 85), rnd(15, 85));
  ne->ang = rnd(0, CGLP_PI * 2);
  ne->isAlive = true;
  ropecorralEnemyIndex = cgl_wrap(ropecorralEnemyIndex + 1, 0, ROPECORRAL_MAX_ENEMY_COUNT);
}

void ropecorralUpdate() {
  Collision scratch;
  // Never reads a Collision result - corralling/hits are decided by distance math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&ropecorralPlayer.pos, 30, 50);
    ropecorralPlayer.vx = 0.75;
    ropecorralPlayer.vy = 0.32;
    vectorSet(&ropecorralBuddy.pos, 20, 50);
    INIT_UNALIVED_ARRAY_FAST(ropecorralEnemies);
    ropecorralEnemyIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(ropecorralSpikes);
    ropecorralSpikeIndex = 0;
    TIMES(5, ci) {
      Vector p;
      vectorSet(&p, rnd(15, 85), rnd(15, 85));
      if (distanceTo(&p, 30, 50) < 18) {
        p.x = cgl_wrap(p.x + 40, 15, 85);
      }
      ASSIGN_ARRAY_ITEM(ropecorralEnemies, ropecorralEnemyIndex, RopecorralEnemy, ne0);
      ne0->pos = p;
      ne0->ang = rnd(0, CGLP_PI * 2);
      ne0->isAlive = true;
      ropecorralEnemyIndex = cgl_wrap(ropecorralEnemyIndex + 1, 0, ROPECORRAL_MAX_ENEMY_COUNT);
    }
    TIMES(3, si) {
      Vector p2;
      vectorSet(&p2, 50, 50);
      int k;
      for (k = 0; k < 8; k++) {
        vectorSet(&p2, rnd(10, 90), rnd(10, 90));
        bool ok = distanceTo(&p2, 30, 50) >= 20;
        FOR_EACH(ropecorralSpikes, sj) {
          ASSIGN_ARRAY_ITEM(ropecorralSpikes, sj, RopecorralSpike, sChk0);
          SKIP_IS_NOT_ALIVE(sChk0);
          if (distanceTo(&sChk0->pos, p2.x, p2.y) < 24) {
            ok = false;
          }
        }
        if (ok) {
          break;
        }
      }
      ASSIGN_ARRAY_ITEM(ropecorralSpikes, ropecorralSpikeIndex, RopecorralSpike, ns0);
      ns0->pos = p2;
      ns0->warm = 0;
      ns0->isAlive = true;
      ropecorralSpikeIndex = cgl_wrap(ropecorralSpikeIndex + 1, 0, ROPECORRAL_MAX_SPIKE_COUNT);
    }
    ropecorralSwingA = 0;
    ropecorralSwept = 0;
    ropecorralNextCritT = 200;
    ropecorralHitstop = 0;
    ropecorralShakeMag = 0;
    ropecorralShakeT = 0;
  }

  // difficulty = pace: everything moves faster
  float pace = 1 + (difficulty - 1) * 0.35;
  bool frozen = ropecorralHitstop > 0;
  if (frozen) {
    ropecorralHitstop--;
  }

  if (input.isPressed) {
    // buddy anchors; you swing around it
    if (input.isJustPressed) {
      ropecorralSwingA = angleTo(&ropecorralBuddy.pos, ropecorralPlayer.pos.x, ropecorralPlayer.pos.y);
      ropecorralSwept = 0;
      play(SELECT);
    }
    float w = 0.075 * pace;
    if (!frozen) {
      ropecorralSwingA += w;
      ropecorralSwept += w;
    }
    vectorSet(&ropecorralPlayer.pos, ropecorralBuddy.pos.x + cos(ropecorralSwingA) * ROPECORRAL_ROPE,
              ropecorralBuddy.pos.y + sin(ropecorralSwingA) * ROPECORRAL_ROPE);
    ropecorralPlayer.vx = -sin(ropecorralSwingA) * (ROPECORRAL_ROPE * w * 0.8);
    ropecorralPlayer.vy = cos(ropecorralSwingA) * (ROPECORRAL_ROPE * w * 0.8);
    // full revolution -> corral everything inside the circle (critters AND spikes)
    if (ropecorralSwept >= CGLP_PI * 2) {
      ropecorralSwept = 0;
      int nCrit = 0;
      FOR_EACH(ropecorralEnemies, ei) {
        ASSIGN_ARRAY_ITEM(ropecorralEnemies, ei, RopecorralEnemy, e);
        SKIP_IS_NOT_ALIVE(e);
        if (distanceTo(&e->pos, ropecorralBuddy.pos.x, ropecorralBuddy.pos.y) < ROPECORRAL_ROPE - 2) {
          nCrit++;
        }
      }
      int nSpike = 0;
      FOR_EACH(ropecorralSpikes, si2) {
        ASSIGN_ARRAY_ITEM(ropecorralSpikes, si2, RopecorralSpike, s2);
        SKIP_IS_NOT_ALIVE(s2);
        if (distanceTo(&s2->pos, ropecorralBuddy.pos.x, ropecorralBuddy.pos.y) < ROPECORRAL_ROPE - 2) {
          nSpike++;
        }
      }
      int n = nCrit + nSpike;
      if (n > 0) {
        // particle color/count escalate with corral size - the jackpot gets louder
        int chainColor;
        if (n >= 6) {
          // Vircon32 port note: no "magenta" in this engine's 15-color
          // palette (see cglp.h) - the biggest corral tier borrows
          // light_purple instead.
          chainColor = LIGHT_PURPLE;
        } else if (n >= 3) {
          chainColor = LIGHT_RED;
        } else {
          chainColor = LIGHT_YELLOW;
        }
        color = chainColor;
        FOR_EACH(ropecorralEnemies, ei2) {
          ASSIGN_ARRAY_ITEM(ropecorralEnemies, ei2, RopecorralEnemy, e2);
          SKIP_IS_NOT_ALIVE(e2);
          if (distanceTo(&e2->pos, ropecorralBuddy.pos.x, ropecorralBuddy.pos.y) < ROPECORRAL_ROPE - 2) {
            particle(e2->pos.x, e2->pos.y, 10 + n * 2, 2 + n * 0.25, 0, CGLP_PI * 2);
            e2->isAlive = false;
          }
        }
        FOR_EACH(ropecorralSpikes, si3) {
          ASSIGN_ARRAY_ITEM(ropecorralSpikes, si3, RopecorralSpike, s3);
          SKIP_IS_NOT_ALIVE(s3);
          if (distanceTo(&s3->pos, ropecorralBuddy.pos.x, ropecorralBuddy.pos.y) < ROPECORRAL_ROPE - 2) {
            particle(s3->pos.x, s3->pos.y, 10 + n * 2, 2 + n * 0.25, 0, CGLP_PI * 2);
            s3->isAlive = false;
          }
        }
        particle(ropecorralBuddy.pos.x, ropecorralBuddy.pos.y, 16 + n * 3, 2.5 + n * 0.3, 0, CGLP_PI * 2);
        addScore(n * n * 4, ropecorralBuddy.pos.x, ropecorralBuddy.pos.y);
        play(POWER_UP);
        ropecorralHitstop = clamp(2 + n, 2, 8);
        ropecorralShakeMag = clamp(1.5 + n, 1.5, 7);
        ropecorralShakeT = 8;
        // everything removed (critters AND spikes) is immediately replaced
        // by a critter - clearing a spike this way nets +1 critter on
        // screen, not just a like-for-like swap
        TIMES(n, spawnIdx) {
          ropecorralSpawnCritter();
        }
      } else {
        play(CLICK);
      }
    }
  } else {
    // roll free; buddy trails behind on the rope
    if (!frozen) {
      ropecorralPlayer.pos.x += ropecorralPlayer.vx * pace;
      ropecorralPlayer.pos.y += ropecorralPlayer.vy * pace;
      if (ropecorralPlayer.pos.x < 6 || ropecorralPlayer.pos.x > 94) {
        ropecorralPlayer.vx = -ropecorralPlayer.vx;
      }
      if (ropecorralPlayer.pos.y < 6 || ropecorralPlayer.pos.y > 94) {
        ropecorralPlayer.vy = -ropecorralPlayer.vy;
      }
      ropecorralPlayer.pos.x = clamp(ropecorralPlayer.pos.x, 6, 94);
      ropecorralPlayer.pos.y = clamp(ropecorralPlayer.pos.y, 6, 94);
      float d = distanceTo(&ropecorralBuddy.pos, ropecorralPlayer.pos.x, ropecorralPlayer.pos.y);
      if (d > ROPECORRAL_ROPE) {
        float a = angleTo(&ropecorralBuddy.pos, ropecorralPlayer.pos.x, ropecorralPlayer.pos.y);
        vectorSet(&ropecorralBuddy.pos, ropecorralPlayer.pos.x - cos(a) * ROPECORRAL_ROPE,
                  ropecorralPlayer.pos.y - sin(a) * ROPECORRAL_ROPE);
      }
    }
  }

  if (!frozen) {
    ropecorralNextCritT--;
    COUNT_IS_ALIVE(ropecorralEnemies, enemyCountNow);
    if (ropecorralNextCritT < 0 && enemyCountNow < 8) {
      ropecorralSpawnCritter();
      ropecorralNextCritT = rnd(140, 220) / sqrt(difficulty);
    }
  }

  // spikes slowly accrue (telegraphed), spaced away from other spikes
  if (ticks % 700 == 699) {
    COUNT_IS_ALIVE(ropecorralSpikes, spikeCountNow);
    if (spikeCountNow < 6) {
      Vector p3;
      vectorSet(&p3, 50, 50);
      int k2;
      for (k2 = 0; k2 < 8; k2++) {
        vectorSet(&p3, rnd(10, 90), rnd(10, 90));
        bool ok2 = distanceTo(&p3, ropecorralPlayer.pos.x, ropecorralPlayer.pos.y) >= 20;
        FOR_EACH(ropecorralSpikes, sk) {
          ASSIGN_ARRAY_ITEM(ropecorralSpikes, sk, RopecorralSpike, sChk);
          SKIP_IS_NOT_ALIVE(sChk);
          if (distanceTo(&sChk->pos, p3.x, p3.y) < 24) {
            ok2 = false;
          }
        }
        if (ok2) {
          break;
        }
      }
      ASSIGN_ARRAY_ITEM(ropecorralSpikes, ropecorralSpikeIndex, RopecorralSpike, ns);
      ns->pos = p3;
      ns->warm = 60;
      ns->isAlive = true;
      ropecorralSpikeIndex = cgl_wrap(ropecorralSpikeIndex + 1, 0, ROPECORRAL_MAX_SPIKE_COUNT);
    }
  }

  // critters wander; direct touch COLLECTS one (+1)
  int collected = 0;
  FOR_EACH(ropecorralEnemies, eci) {
    ASSIGN_ARRAY_ITEM(ropecorralEnemies, eci, RopecorralEnemy, ec);
    SKIP_IS_NOT_ALIVE(ec);
    ec->ang += rnd(-0.2, 0.2);
    addWithAngle(&ec->pos, ec->ang, 0.28 * pace);
    ec->pos.x = clamp(ec->pos.x, 8, 92);
    ec->pos.y = clamp(ec->pos.y, 8, 92);
    if (ec->pos.x <= 8 || ec->pos.x >= 92) {
      ec->ang = CGLP_PI - ec->ang;
    }
    if (ec->pos.y <= 8 || ec->pos.y >= 92) {
      ec->ang = -ec->ang;
    }
    if (distanceTo(&ec->pos, ropecorralPlayer.pos.x, ropecorralPlayer.pos.y) < 4.5) {
      addScore(1, ec->pos.x, ec->pos.y);
      play(COIN);
      // Upstream left this particle's color incidental; explicitly yellow here (matches critter's own color).
      color = YELLOW;
      particle(ec->pos.x, ec->pos.y, 8, 1.5, 0, CGLP_PI * 2);
      collected++;
      ec->isAlive = false;
      continue;
    }
  }
  // each collected critter is immediately replaced, so the trickle never runs dry
  TIMES(collected, spawnIdx2) {
    ropecorralSpawnCritter();
  }

  // decaying screen shake on a corral capture - decorative rope/arc only,
  // never the box() hitboxes
  float shakeX = 0;
  float shakeY = 0;
  if (ropecorralShakeT > 0) {
    ropecorralShakeT--;
    float m = ropecorralShakeMag * (ropecorralShakeT / 8);
    shakeX = rnd(-m, m);
    shakeY = rnd(-m, m);
  }

  // draw: swing sweep indicator
  if (input.isPressed) {
    color = LIGHT_YELLOW;
    thickness = 1;
    arc(ropecorralBuddy.pos.x + shakeX, ropecorralBuddy.pos.y + shakeY, ROPECORRAL_ROPE,
        ropecorralSwingA - ropecorralSwept, ropecorralSwingA, &scratch);
  }
  color = LIGHT_BLACK;
  thickness = 1;
  line(ropecorralPlayer.pos.x + shakeX, ropecorralPlayer.pos.y + shakeY, ropecorralBuddy.pos.x + shakeX,
       ropecorralBuddy.pos.y + shakeY, &scratch);

  // afterimage trail, algebraic from current velocity (straight-line roll
  // only, no history array; curved swing motion is excluded since a
  // straight offset would misread the arc)
  if (!input.isPressed) {
    float spd = sqrt(ropecorralPlayer.vx * ropecorralPlayer.vx + ropecorralPlayer.vy * ropecorralPlayer.vy) *
                pace;
    if (spd > 1.5) {
      color = LIGHT_CYAN;
      box(clamp(ropecorralPlayer.pos.x - ropecorralPlayer.vx * pace * 2.5, 6, 94),
          clamp(ropecorralPlayer.pos.y - ropecorralPlayer.vy * pace * 2.5, 6, 94), 4, 4, &scratch);
      color = CYAN;
      box(clamp(ropecorralPlayer.pos.x - ropecorralPlayer.vx * pace * 5, 6, 94),
          clamp(ropecorralPlayer.pos.y - ropecorralPlayer.vy * pace * 5, 6, 94), 3, 3, &scratch);
    }
  }

  // spikes kill YOUR ball
  FOR_EACH(ropecorralSpikes, sdi) {
    ASSIGN_ARRAY_ITEM(ropecorralSpikes, sdi, RopecorralSpike, sd);
    SKIP_IS_NOT_ALIVE(sd);
    if (sd->warm > 0) {
      sd->warm--;
      if (((int)sd->warm) % 8 < 4) {
        color = LIGHT_RED;
        box(sd->pos.x, sd->pos.y, 2, 2, &scratch);
      }
      continue;
    }
    color = RED;
    box(sd->pos.x, sd->pos.y, 3, 3, &scratch);
    if (distanceTo(&sd->pos, ropecorralPlayer.pos.x, ropecorralPlayer.pos.y) < 4) {
      play(EXPLOSION);
      color = RED;
      particle(ropecorralPlayer.pos.x, ropecorralPlayer.pos.y, 25, 3, 0, CGLP_PI * 2);
      gameOver();
    }
  }

  // critters (collectible - yellow), eye toward their own wander direction
  FOR_EACH(ropecorralEnemies, edi) {
    ASSIGN_ARRAY_ITEM(ropecorralEnemies, edi, RopecorralEnemy, ed);
    SKIP_IS_NOT_ALIVE(ed);
    color = YELLOW;
    box(ed->pos.x, ed->pos.y, 5, 4, &scratch);
    ropecorralEye(ed->pos.x, ed->pos.y, ed->ang, 1.2);
  }

  color = LIGHT_BLUE;
  box(ropecorralBuddy.pos.x, ropecorralBuddy.pos.y, 5, 5, &scratch);
  ropecorralEye(ropecorralBuddy.pos.x, ropecorralBuddy.pos.y,
                angleTo(&ropecorralBuddy.pos, ropecorralPlayer.pos.x, ropecorralPlayer.pos.y), 1.4);
  color = CYAN;
  box(ropecorralPlayer.pos.x, ropecorralPlayer.pos.y, 5, 5, &scratch);
  ropecorralEye(ropecorralPlayer.pos.x, ropecorralPlayer.pos.y,
                cgl_atan2(ropecorralPlayer.vy, ropecorralPlayer.vx), 1.4);
}

void addGameRopecorral() {
  addGame(ropecorralTitle, ropecorralDescription, ropecorralCharacters, ropecorralCharactersCount,
          &ropecorralOptions, false, &ropecorralUpdate);
}
