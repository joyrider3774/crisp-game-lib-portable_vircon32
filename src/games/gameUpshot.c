#include "../cglp.h"

int* upshotTitle = "UP SHOT";
int* upshotDescription = "[Hold]\n Stop & Shoot\n[Release]\n Run";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] upshotCharacters = {
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
        "      ",
    },
};
int upshotCharactersCount = 2;

Options upshotOptions = {100, 100, 50, false};

struct UpshotRockSpawn {
  float x;
  float size;
  float speed;
  float interval;
  float intervalVariation;
  float ticks;
  bool isAlive;
};
#define UPSHOT_MAX_ROCK_SPAWN_COUNT 16
UpshotRockSpawn[UPSHOT_MAX_ROCK_SPAWN_COUNT] upshotRockSpawns;
int upshotRockSpawnIndex;
float upshotNextRockSpawnDist;

struct UpshotRock {
  Vector pos;
  float vy;
  float size;
  float speed;
  bool isAlive;
};
// Rock spawn interval shrinks as difficulty^-1.25 while fall lifetime only shrinks as difficulty^-0.25, so concurrent rocks grow ~linearly with difficulty (minutes elapsed) - 64 overflowed after ~10-12 min.
#define UPSHOT_MAX_ROCK_COUNT 512
UpshotRock[UPSHOT_MAX_ROCK_COUNT] upshotRocks;
int upshotRockIndex;

struct UpshotPlayer {
  Vector pos;
  float vx;
  float shotTicks;
};
UpshotPlayer upshotPlayer;

struct UpshotShot {
  Vector pos;
  float vy;
  bool isAlive;
};
#define UPSHOT_MAX_SHOT_COUNT 32
UpshotShot[UPSHOT_MAX_SHOT_COUNT] upshotShots;
int upshotShotIndex;

float upshotLx;
int upshotMultiplier;

void upshotUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(upshotRockSpawns);
    upshotRockSpawnIndex = 0;
    upshotNextRockSpawnDist = 0;
    INIT_UNALIVED_ARRAY_FAST(upshotRocks);
    upshotRockIndex = 0;
    vectorSet(&upshotPlayer.pos, 10, 87);
    upshotPlayer.vx = 0;
    upshotPlayer.shotTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(upshotShots);
    upshotShotIndex = 0;
    upshotLx = 50;
    upshotMultiplier = 1;
  }
  float scr = difficulty * 0.1;
  if (upshotPlayer.pos.x > 30) {
    scr += (upshotPlayer.pos.x - 30) * 0.05;
  }
  upshotLx = cgl_wrap(upshotLx - scr, 0, 99);
  color = LIGHT_BLACK;
  rect(0, 90, 100, 10, &scratch);
  color = WHITE;
  rect(upshotLx, 90, 1, 10, &scratch);
  upshotPlayer.shotTicks--;
  if (input.isPressed) {
    if (input.isJustPressed) {
      upshotMultiplier = 1;
      upshotPlayer.vx = 0;
    }
    if (upshotPlayer.shotTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(upshotShots, upshotShotIndex, UpshotShot, s);
      vectorSet(&s->pos, upshotPlayer.pos.x, upshotPlayer.pos.y);
      s->vy = -2 * difficulty;
      s->isAlive = true;
      upshotShotIndex = cgl_wrap(upshotShotIndex + 1, 0, UPSHOT_MAX_SHOT_COUNT);
      upshotPlayer.shotTicks = 10 / difficulty;
    }
  } else if (input.isJustReleased) {
    play(SELECT);
    upshotPlayer.vx = difficulty * 1.2;
  }
  upshotPlayer.pos.x += upshotPlayer.vx - scr;
  color = BLACK;
  FOR_EACH(upshotShots, i) {
    ASSIGN_ARRAY_ITEM(upshotShots, i, UpshotShot, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x -= scr;
    s->pos.y += s->vy;
    box(s->pos.x, s->pos.y, 5, 9, &scratch);
    s->isAlive = s->pos.y >= 0;
  }
  upshotNextRockSpawnDist -= scr;
  if (upshotNextRockSpawnDist < 0) {
    float size = rnd(5, 15);
    float interval = rnd(10, 50) / difficulty;
    float speed = (rnd(5, 10) / sqrt(size)) * sqrt(difficulty);
    interval /= sqrt(speed) / sqrt(size);
    ASSIGN_ARRAY_ITEM(upshotRockSpawns, upshotRockSpawnIndex, UpshotRockSpawn, rs);
    rs->x = 200;
    rs->size = size;
    rs->speed = speed;
    rs->interval = interval;
    rs->intervalVariation = rnd(0.3, 0.9);
    rs->ticks = rnd(0, interval);
    rs->isAlive = true;
    upshotRockSpawnIndex = cgl_wrap(upshotRockSpawnIndex + 1, 0, UPSHOT_MAX_ROCK_SPAWN_COUNT);
    upshotNextRockSpawnDist += rnd(50, 60);
  }
  FOR_EACH(upshotRockSpawns, i) {
    ASSIGN_ARRAY_ITEM(upshotRockSpawns, i, UpshotRockSpawn, rs);
    SKIP_IS_NOT_ALIVE(rs);
    rs->x -= scr;
    rs->ticks--;
    if (rs->ticks < 0) {
      ASSIGN_ARRAY_ITEM(upshotRocks, upshotRockIndex, UpshotRock, r);
      vectorSet(&r->pos, rs->x, -rs->size / 2);
      r->vy = 0;
      r->size = rs->size;
      r->speed = rs->speed;
      r->isAlive = true;
      upshotRockIndex = cgl_wrap(upshotRockIndex + 1, 0, UPSHOT_MAX_ROCK_COUNT);
      rs->ticks = rs->interval * (1 + rnd(0, rs->intervalVariation) * RNDPM());
    }
    rs->isAlive = rs->x >= 0;
  }
  color = RED;
  FOR_EACH(upshotRocks, i) {
    ASSIGN_ARRAY_ITEM(upshotRocks, i, UpshotRock, r);
    SKIP_IS_NOT_ALIVE(r);
    r->vy += r->speed * 0.01;
    r->pos.x -= scr;
    r->pos.y += r->vy;
    Collision rc;
    box(r->pos.x, r->pos.y, r->size, r->size, &rc);
    if (rc.isColliding.rect[BLACK]) {
      r->size *= 0.7;
      color = BLACK;
      particle(r->pos.x, r->pos.y, 5, 3, CGLP_PI / 2, 0.5);
      color = RED;
      if (r->size < 5) {
        play(POWER_UP);
        addScore(upshotMultiplier * 10, r->pos.x, clamp(r->pos.y, 20, 99));
        particle(r->pos.x, r->pos.y, 19, 3, 0, CGLP_PI * 2);
        r->isAlive = false;
        continue;
      } else {
        play(HIT);
        addScore(upshotMultiplier, r->pos.x, clamp(r->pos.y, 20, 99));
        upshotMultiplier++;
      }
    }
    if (r->pos.y > 90 - r->size / 2) {
      particle(r->pos.x, r->pos.y, r->size * 0.3, sqrt(r->size) * 0.3, 0, CGLP_PI * 2);
      r->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  int[2] pc;
  if (input.isPressed) {
    pc[0] = 'b';
  } else {
    pc[0] = 'a' + (int)floor(ticks / 20) % 2;
  }
  pc[1] = 0;
  Collision cc;
  character(pc, upshotPlayer.pos.x, upshotPlayer.pos.y, &cc);
  if (cc.isColliding.rect[RED] || upshotPlayer.pos.x < -2) {
    play(EXPLOSION);
    gameOver();
  }
  color = TRANSPARENT;
  FOR_EACH(upshotShots, i) {
    ASSIGN_ARRAY_ITEM(upshotShots, i, UpshotShot, s);
    SKIP_IS_NOT_ALIVE(s);
    Collision sc;
    box(s->pos.x, s->pos.y, 5, 9, &sc);
    if (sc.isColliding.rect[RED]) {
      s->isAlive = false;
    }
  }
}

void addGameUpshot() {
  addGame(upshotTitle, upshotDescription, upshotCharacters,
          upshotCharactersCount, &upshotOptions, false, &upshotUpdate);
}
