#include "../cglp.h"

int* geyserhopTitle = "GEYSER HOP";
int* geyserhopDescription = "[Hold] Rise\nStomp geysers!";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] geyserhopCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int geyserhopCharactersCount = 0;

Options geyserhopOptions = {100, 100, 0, false};

struct GeyserhopPlayer {
  float x;
  float y;
  float vy;
};
GeyserhopPlayer geyserhopPlayer;

struct GeyserhopGeyser {
  float x;
  float height;
  float vx;
  bool stomped;
  // Vircon32 port note: upstream identifies the tutorial-hint geyser by
  // array index 0 (the very first one ever pushed); the ring buffer here
  // recycles slots, so track that same geyser explicitly instead.
  bool isFirst;
  bool isAlive;
};
// Sized generously above the ~1.4 concurrent geysers estimated (spawn rate and speed cancel out).
#define GEYSERHOP_MAX_GEYSER_COUNT 16
GeyserhopGeyser[GEYSERHOP_MAX_GEYSER_COUNT] geyserhopGeysers;
int geyserhopGeyserIndex;

struct GeyserhopTrail {
  float x;
  float y;
  int life;
  bool isAlive;
};
// One trail spawned per tick while moving fast, each lives 8 ticks - so at
// most ~8 concurrent; doubled for headroom.
#define GEYSERHOP_MAX_TRAIL_COUNT 16
GeyserhopTrail[GEYSERHOP_MAX_TRAIL_COUNT] geyserhopTrails;
int geyserhopTrailIndex;

float geyserhopSpawnTicks;
float geyserhopCeiling;
float geyserhopGroundY;
int geyserhopStompTimer;
int geyserhopMultiplier;
float geyserhopPrevVy;

void geyserhopUpdate() {
  Collision scratch;
  if (!ticks) {
    geyserhopGroundY = 92;
    geyserhopCeiling = 3;
    geyserhopPlayer.x = 40;
    geyserhopPlayer.y = 40;
    geyserhopPlayer.vy = 0;
    INIT_UNALIVED_ARRAY_FAST(geyserhopGeysers);
    geyserhopGeyserIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(geyserhopTrails);
    geyserhopTrailIndex = 0;
    geyserhopStompTimer = 0;
    ASSIGN_ARRAY_ITEM(geyserhopGeysers, geyserhopGeyserIndex, GeyserhopGeyser, initG);
    initG->x = 95;
    initG->height = 40;
    initG->vx = -1;
    initG->stomped = false;
    initG->isFirst = true;
    initG->isAlive = true;
    geyserhopGeyserIndex = cgl_wrap(geyserhopGeyserIndex + 1, 0, GEYSERHOP_MAX_GEYSER_COUNT);
    geyserhopSpawnTicks = 60;
    geyserhopMultiplier = 1;
    geyserhopPrevVy = 0;
  }

  float baseSpeed = sqrt(difficulty);

  geyserhopSpawnTicks--;
  if (geyserhopSpawnTicks < 0) {
    float h = 30 + floor(rnd(10, 30));
    ASSIGN_ARRAY_ITEM(geyserhopGeysers, geyserhopGeyserIndex, GeyserhopGeyser, ng);
    ng->x = 110;
    ng->height = h;
    ng->vx = -baseSpeed;
    ng->stomped = false;
    ng->isFirst = false;
    ng->isAlive = true;
    geyserhopGeyserIndex = cgl_wrap(geyserhopGeyserIndex + 1, 0, GEYSERHOP_MAX_GEYSER_COUNT);
    geyserhopSpawnTicks = 90 / sqrt(difficulty);
  }

  geyserhopStompTimer++;
  if (geyserhopStompTimer > 200) {
    geyserhopCeiling += 0.1;
  }

  if (input.isPressed) {
    geyserhopPlayer.vy -= 0.16;
  } else {
    geyserhopPlayer.vy += 0.13;
  }
  geyserhopPlayer.vy = clamp(geyserhopPlayer.vy, -2.2, 3);

  if (geyserhopPrevVy < 0 && geyserhopPlayer.vy > 0) {
    color = CYAN;
    particle(geyserhopPlayer.x, geyserhopPlayer.y - 3, 5, 1, -CGLP_PI / 2, CGLP_PI / 4);
  }
  geyserhopPrevVy = geyserhopPlayer.vy;

  geyserhopPlayer.y += geyserhopPlayer.vy * sqrt(difficulty);

  if (fabs(geyserhopPlayer.vy) > 1) {
    ASSIGN_ARRAY_ITEM(geyserhopTrails, geyserhopTrailIndex, GeyserhopTrail, nt);
    nt->x = geyserhopPlayer.x;
    nt->y = geyserhopPlayer.y;
    nt->life = 8;
    nt->isAlive = true;
    geyserhopTrailIndex = cgl_wrap(geyserhopTrailIndex + 1, 0, GEYSERHOP_MAX_TRAIL_COUNT);
  }

  if (geyserhopPlayer.y < geyserhopCeiling || geyserhopPlayer.y > geyserhopGroundY) {
    play(EXPLOSION);
    gameOver();
  }

  color = RED;
  rect(0, 0, 100, geyserhopCeiling, &scratch);
  if (geyserhopStompTimer > 150) {
    color = YELLOW;
    text("!", 50, geyserhopCeiling + 7, &scratch);
  }

  color = GREEN;
  rect(0, geyserhopGroundY, 100, 8, &scratch);

  FOR_EACH(geyserhopTrails, ti) {
    ASSIGN_ARRAY_ITEM(geyserhopTrails, ti, GeyserhopTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life--;
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
    color = LIGHT_CYAN;
    float tsize = 4 * ((float)t->life / 8);
    box(t->x, t->y, tsize, tsize, &scratch);
  }

  float stretch = clamp(-geyserhopPlayer.vy * 0.4, -1.5, 1.5);
  float pw = 6 - stretch;
  float ph = 6 + stretch;
  color = CYAN;
  box(geyserhopPlayer.x, geyserhopPlayer.y, pw, ph, &scratch);

  float eyeOffsetY;
  if (geyserhopPlayer.vy > 0) {
    eyeOffsetY = 0.8;
  } else if (geyserhopPlayer.vy < -0.5) {
    eyeOffsetY = -0.8;
  } else {
    eyeOffsetY = 0;
  }
  color = WHITE;
  box(geyserhopPlayer.x - 1.5, geyserhopPlayer.y - 0.5, 2.5, 3, &scratch);
  box(geyserhopPlayer.x + 1.5, geyserhopPlayer.y - 0.5, 2.5, 3, &scratch);
  color = BLACK;
  rect(geyserhopPlayer.x - 2, geyserhopPlayer.y - 0.5 + eyeOffsetY, 1, 1.5, &scratch);
  rect(geyserhopPlayer.x + 1, geyserhopPlayer.y - 0.5 + eyeOffsetY, 1, 1.5, &scratch);

  FOR_EACH(geyserhopGeysers, gi) {
    ASSIGN_ARRAY_ITEM(geyserhopGeysers, gi, GeyserhopGeyser, g);
    SKIP_IS_NOT_ALIVE(g);
    g->x += g->vx;

    if (g->x < -15) {
      if (!g->stomped) {
        geyserhopMultiplier = max(geyserhopMultiplier - 1, 1);
      }
      g->isAlive = false;
      continue;
    }

    float gTop = geyserhopGroundY - g->height;

    color = BLUE;
    Collision bodyCol;
    rect(g->x - 5, gTop + 4, 10, g->height - 4, &bodyCol);

    if (g->stomped) {
      color = PURPLE;
    } else {
      color = YELLOW;
    }
    Collision topCol;
    rect(g->x - 6, gTop, 12, 6, &topCol);

    if (!g->stomped) {
      float lookX = clamp((geyserhopPlayer.x - g->x) * 0.05, -1, 1);
      float lookY = clamp((geyserhopPlayer.y - gTop) * 0.03, -1, 1);
      color = WHITE;
      box(g->x - 2.5, gTop + 3, 3, 4, &scratch);
      box(g->x + 2.5, gTop + 3, 3, 4, &scratch);
      color = BLACK;
      rect(g->x - 3 + lookX, gTop + 2.5 + lookY, 1, 2, &scratch);
      rect(g->x + 2 + lookX, gTop + 2.5 + lookY, 1, 2, &scratch);
    }

    color = LIGHT_BLUE;
    particle(g->x, gTop, 1, 0.7, -CGLP_PI / 2, CGLP_PI / 5);

    if (g->isFirst && !g->stomped && score == 0) {
      color = BLACK;
      text("v STOMP!", g->x, gTop - 12, &scratch);
    }

    if (!g->stomped && topCol.isColliding.rect[CYAN]) {
      if (geyserhopPlayer.vy > 0) {
        play(POWER_UP);
        addScore(geyserhopMultiplier, g->x, gTop);
        geyserhopMultiplier = min(geyserhopMultiplier + 1, 16);
        color = YELLOW;
        particle(g->x, gTop, 30, 3, 0, CGLP_PI);
        geyserhopPlayer.vy = -2.8;
        g->stomped = true;
        g->height = 8;
        geyserhopStompTimer = 0;
        geyserhopCeiling = fmax(3, geyserhopCeiling - 8);
      } else {
        play(HIT);
        gameOver();
      }
    }

    if (!g->stomped && bodyCol.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    }
  }

  geyserhopPlayer.x += (45 - geyserhopPlayer.x) * 0.01;

  color = BLACK;
  int[16] geyserhopMultText;
  strcpy(geyserhopMultText, "x");
  strcat(geyserhopMultText, intToChar(geyserhopMultiplier));
  text(geyserhopMultText, 3, 9, &scratch);
}

void addGameGeyserhop() {
  addGame(geyserhopTitle, geyserhopDescription, geyserhopCharacters,
          geyserhopCharactersCount, &geyserhopOptions, false, &geyserhopUpdate);
}
