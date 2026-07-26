#include "../cglp.h"

int* rebirthTitle = "REBIRTH";
int* rebirthDescription = "[Tap]\n Jump / Land";

int[7][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] rebirthCharacters = {
    {
        " ll   ",
        "lll l ",
        "lll ll",
        "lll ll",
        "llllll",
        " l  l ",
    },
    {
        "   ll ",
        "  l   ",
        " lll  ",
        " l    ",
        "l l   ",
        "   l  ",
    },
    {
        "  ll  ",
        "l l   ",
        " llll ",
        "  l   ",
        " l ll ",
        "l     ",
    },
    {
        "  ll  ",
        "l l l ",
        " lll  ",
        "  l   ",
        " l ll ",
        "l     ",
    },
    {
        "  ll  ",
        "  l   ",
        " lll  ",
        "l l l ",
        " l ll ",
        "ll  l ",
    },
    {
        "  ll  ",
        " l    ",
        "ll    ",
        "l l   ",
        " l l  ",
        "  l l ",
    },
    {
        " llll ",
        "ll  ll",
        "l ll l",
        " llll ",
        "  ll  ",
    },
};
int rebirthCharactersCount = 7;

Options rebirthOptions = {200, 50, 2000, true};

struct RebirthTrack {
  float x;
  float vx;
  int world;
  bool isAlive;
};
#define REBIRTH_MAX_TRACK_COUNT 64
RebirthTrack[REBIRTH_MAX_TRACK_COUNT] rebirthTracks;
int rebirthTrackIndex;
float rebirthNextTrackTicks;
int rebirthTrackCount;

struct RebirthDia {
  Vector pos;
  float vx;
  int world;
  bool isAlive;
};
#define REBIRTH_MAX_DIA_COUNT 64
RebirthDia[REBIRTH_MAX_DIA_COUNT] rebirthDias;
int rebirthDiaIndex;
float rebirthNextDiaTicks;
int rebirthDiaWorld;
int rebirthDiaCount;

#define REBIRTH_STATE_RUN 0
#define REBIRTH_STATE_JUMP 1
#define REBIRTH_STATE_LAND 2

struct RebirthPlayer {
  Vector pos;
  float ox;
  Vector vel;
  int world;
  int state;
};
RebirthPlayer rebirthPlayer;

int rebirthMultiplier;

void rebirthUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(rebirthTracks);
    rebirthTrackIndex = 0;
    rebirthNextTrackTicks = 0;
    rebirthTrackCount = 0;
    INIT_UNALIVED_ARRAY_FAST(rebirthDias);
    rebirthDiaIndex = 0;
    rebirthNextDiaTicks = 60;
    rebirthDiaWorld = 1;
    rebirthDiaCount = 1;
    vectorSet(&rebirthPlayer.pos, 110, 37);
    rebirthPlayer.ox = 10;
    vectorSet(&rebirthPlayer.vel, 0, 0);
    rebirthPlayer.world = 1;
    rebirthPlayer.state = REBIRTH_STATE_RUN;
    rebirthMultiplier = 1;
  }
  color = BLACK;
  rect(100, 40, 100, 10, &scratch);
  color = LIGHT_BLACK;
  rect(0, 0, 100, 40, &scratch);
  rebirthNextTrackTicks--;
  if (rebirthNextTrackTicks < 0) {
    int world;
    if (rebirthTrackCount % 2 == 0) {
      world = 1;
    } else {
      world = -1;
    }
    ASSIGN_ARRAY_ITEM(rebirthTracks, rebirthTrackIndex, RebirthTrack, nt);
    if (world > 0) {
      nt->x = 203;
    } else {
      nt->x = -3;
    }
    float vxDir;
    if (world > 0) {
      vxDir = -1;
    } else {
      vxDir = 1;
    }
    nt->vx = rnd(1, sqrt(difficulty)) * vxDir;
    nt->world = world;
    nt->isAlive = true;
    rebirthTrackIndex = cgl_wrap(rebirthTrackIndex + 1, 0, REBIRTH_MAX_TRACK_COUNT);
    rebirthTrackCount++;
    rebirthNextTrackTicks = rnd(50, 60) / sqrt(difficulty);
  }
  FOR_EACH(rebirthTracks, ti) {
    ASSIGN_ARRAY_ITEM(rebirthTracks, ti, RebirthTrack, t);
    SKIP_IS_NOT_ALIVE(t);
    t->x += t->vx;
    if (t->world > 0) {
      color = BLACK;
    } else {
      color = WHITE;
    }
    characterOptions.isMirrorX = t->world > 0;
    character("a", t->x, 37, &scratch);
    characterOptions.isMirrorX = false;
    bool shouldRemove;
    if (t->world > 0) {
      shouldRemove = t->x < 103;
    } else {
      shouldRemove = t->x > 97;
    }
    if (shouldRemove) {
      t->isAlive = false;
      continue;
    }
  }
  vectorAdd(&rebirthPlayer.pos, rebirthPlayer.vel.x, rebirthPlayer.vel.y);
  vectorMul(&rebirthPlayer.vel, 0.99);
  rebirthPlayer.vel.x *= 0.8;
  int[2] pc;
  pc[1] = 0;
  if (rebirthPlayer.state == REBIRTH_STATE_RUN) {
    rebirthPlayer.pos.x +=
        (100 + rebirthPlayer.world * rebirthPlayer.ox - rebirthPlayer.pos.x) * 0.05;
    rebirthPlayer.ox = clamp(rebirthPlayer.ox + sqrt(difficulty) * 0.3, 10, 80);
    if (input.isJustPressed) {
      play(JUMP);
      rebirthPlayer.state = REBIRTH_STATE_JUMP;
      rebirthPlayer.vel.y = -2 * sqrt(difficulty);
    }
    pc[0] = 'b' + (ticks / 10) % 2;
  } else if (rebirthPlayer.state == REBIRTH_STATE_JUMP) {
    float dvy;
    if (input.isPressed) {
      dvy = 0.07;
    } else {
      dvy = 0.14;
    }
    rebirthPlayer.vel.y += dvy * difficulty;
    if (rebirthPlayer.pos.y > 37) {
      rebirthPlayer.pos.y = 37;
      rebirthPlayer.vel.y = 0;
      rebirthPlayer.state = REBIRTH_STATE_RUN;
    }
    if (input.isJustPressed) {
      play(LASER);
      rebirthPlayer.state = REBIRTH_STATE_LAND;
      rebirthPlayer.vel.y = 4 * sqrt(difficulty);
    }
    pc[0] = 'd';
  } else if (rebirthPlayer.state == REBIRTH_STATE_LAND) {
    if (rebirthPlayer.pos.y > 37) {
      rebirthPlayer.pos.y = 37;
      rebirthPlayer.vel.y = 0;
      rebirthPlayer.state = REBIRTH_STATE_RUN;
    }
    pc[0] = 'e';
  }
  if (fabs(rebirthPlayer.vel.x) > 1) {
    pc[0] = 'f';
  }
  int pw;
  if (rebirthPlayer.pos.x < 100) {
    pw = -1;
  } else {
    pw = 1;
  }
  if (pw > 0) {
    color = BLACK;
  } else {
    color = WHITE;
  }
  characterOptions.isMirrorX = pw < 0;
  Collision pcoll;
  character(pc, rebirthPlayer.pos.x, rebirthPlayer.pos.y, &pcoll);
  characterOptions.isMirrorX = false;
  if (pcoll.isColliding.character['a'] && fabs(rebirthPlayer.vel.x) < 2) {
    play(HIT);
    float dir;
    if (rebirthPlayer.world > 0) {
      dir = -1;
    } else {
      dir = 1;
    }
    rebirthPlayer.vel.x = dir * rebirthPlayer.ox * 0.2 * sqrt(difficulty);
    if (rebirthPlayer.world > 0) {
      rebirthPlayer.world = -1;
    } else {
      rebirthPlayer.world = 1;
    }
    rebirthPlayer.ox = 10;
    if (rebirthMultiplier > 1) {
      rebirthMultiplier--;
    }
  }
  rebirthNextDiaTicks--;
  if (rebirthNextDiaTicks < 0) {
    rebirthDiaCount--;
    if (rebirthDiaCount < 0) {
      if (rebirthDiaWorld > 0) {
        rebirthDiaWorld = -1;
      } else {
        rebirthDiaWorld = 1;
      }
      rebirthDiaCount = rndi(0, 4);
    }
    ASSIGN_ARRAY_ITEM(rebirthDias, rebirthDiaIndex, RebirthDia, nd);
    float dx;
    if (rebirthDiaWorld > 0) {
      dx = 203;
    } else {
      dx = -3;
    }
    float dy;
    if (rnd(0, 1) < 0.5) {
      dy = 37;
    } else {
      dy = rnd(20, 30);
    }
    vectorSet(&nd->pos, dx, dy);
    float dvxDir;
    if (rebirthDiaWorld > 0) {
      dvxDir = -1;
    } else {
      dvxDir = 1;
    }
    nd->vx = (rnd(1, sqrt(difficulty)) * dvxDir) / 3;
    nd->world = rebirthDiaWorld;
    nd->isAlive = true;
    rebirthDiaIndex = cgl_wrap(rebirthDiaIndex + 1, 0, REBIRTH_MAX_DIA_COUNT);
    rebirthNextDiaTicks = rnd(120, 150) / sqrt(difficulty);
  }
  FOR_EACH(rebirthDias, di) {
    ASSIGN_ARRAY_ITEM(rebirthDias, di, RebirthDia, d);
    SKIP_IS_NOT_ALIVE(d);
    d->pos.x += d->vx;
    color = YELLOW;
    characterOptions.isMirrorX = d->world > 0;
    Collision dc;
    character("g", d->pos.x, d->pos.y, &dc);
    characterOptions.isMirrorX = false;
    if (dc.isColliding.character['b'] || dc.isColliding.character['c'] ||
        dc.isColliding.character['d'] || dc.isColliding.character['e'] ||
        dc.isColliding.character['f']) {
      play(COIN);
      addScore(rebirthMultiplier, d->pos.x, d->pos.y);
      rebirthMultiplier++;
      d->isAlive = false;
      continue;
    }
    bool offscreen;
    if (d->world > 0) {
      offscreen = d->pos.x < 103;
    } else {
      offscreen = d->pos.x > 97;
    }
    if (offscreen) {
      play(EXPLOSION);
      color = RED;
      text("X", 100, d->pos.y, &scratch);
      gameOver();
    }
  }
}

void addGameRebirth() {
  addGame(rebirthTitle, rebirthDescription, rebirthCharacters, rebirthCharactersCount,
          &rebirthOptions, false, &rebirthUpdate);
}
