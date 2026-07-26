#include "../cglp.h"

int* neonpollinatorTitle = "NEON POLLINATOR";
int* neonpollinatorDescription = "[Hold] Glow & Rise";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] neonpollinatorCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int neonpollinatorCharactersCount = 0;

Options neonpollinatorOptions = {100, 100, 2, true};

struct NeonpollinatorPlayer {
  Vector pos;
  float energy;
  float glowRadius;
};
NeonpollinatorPlayer neonpollinatorPlayer;

struct NeonpollinatorFlower {
  Vector pos;
  float size;
  bool isPollinated;
  float glowRadius;
  bool isAlive;
};
#define NEONPOLLINATOR_MAX_FLOWER_COUNT 16
NeonpollinatorFlower[NEONPOLLINATOR_MAX_FLOWER_COUNT] neonpollinatorFlowers;
int neonpollinatorFlowerIndex;
float neonpollinatorNextFlowerDist;

struct NeonpollinatorOrb {
  Vector pos;
  bool isAlive;
};
#define NEONPOLLINATOR_MAX_ORB_COUNT 16
NeonpollinatorOrb[NEONPOLLINATOR_MAX_ORB_COUNT] neonpollinatorOrbs;
int neonpollinatorOrbIndex;
float neonpollinatorNextOrbDist;

float neonpollinatorGardenScrollSpeed;
int neonpollinatorMultiplier;

void neonpollinatorUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&neonpollinatorPlayer.pos, 20, 50);
    neonpollinatorPlayer.energy = 100;
    neonpollinatorPlayer.glowRadius = 5;
    INIT_UNALIVED_ARRAY_FAST(neonpollinatorFlowers);
    neonpollinatorFlowerIndex = 0;
    neonpollinatorNextFlowerDist = 0;
    INIT_UNALIVED_ARRAY_FAST(neonpollinatorOrbs);
    neonpollinatorOrbIndex = 0;
    neonpollinatorNextOrbDist = 0;
    neonpollinatorGardenScrollSpeed = 1;
    neonpollinatorMultiplier = 1;
  }
  neonpollinatorGardenScrollSpeed = difficulty * 0.5;
  if (input.isJustPressed) {
    neonpollinatorMultiplier = 1;
    if (neonpollinatorPlayer.energy > 0) {
      play(LASER);
    } else {
      play(HIT);
    }
  }
  if (input.isPressed && neonpollinatorPlayer.energy > 0) {
    float gr = neonpollinatorPlayer.glowRadius + 0.25 * difficulty;
    if (gr > 25) {
      gr = 25;
    }
    neonpollinatorPlayer.glowRadius = gr;
    neonpollinatorPlayer.pos.y -= difficulty;
    neonpollinatorPlayer.energy -= 0.5 * difficulty;
  } else {
    float gr = neonpollinatorPlayer.glowRadius - 0.5 * difficulty;
    if (gr < 3) {
      gr = 3;
    }
    neonpollinatorPlayer.glowRadius = gr;
    neonpollinatorPlayer.pos.y += 0.7 * difficulty;
    neonpollinatorPlayer.energy -= 0.1 * difficulty;
  }
  neonpollinatorPlayer.energy = clamp(neonpollinatorPlayer.energy, 0, 100);
  if (neonpollinatorPlayer.glowRadius > 3) {
    color = YELLOW;
    thickness = 3;
    arc(neonpollinatorPlayer.pos.x, neonpollinatorPlayer.pos.y, neonpollinatorPlayer.glowRadius, 0,
        CGLP_PI * 2, &scratch);
  }
  color = CYAN;
  thickness = 3;
  arc(neonpollinatorPlayer.pos.x, neonpollinatorPlayer.pos.y, 3, 0, CGLP_PI * 2, &scratch);
  if (neonpollinatorPlayer.pos.y < 0 || neonpollinatorPlayer.pos.y > 100) {
    play(EXPLOSION);
    gameOver();
  }
  neonpollinatorNextFlowerDist -= neonpollinatorGardenScrollSpeed;
  if (neonpollinatorNextFlowerDist < 0) {
    ASSIGN_ARRAY_ITEM(neonpollinatorFlowers, neonpollinatorFlowerIndex, NeonpollinatorFlower, nf);
    vectorSet(&nf->pos, 115, rnd(10, 90));
    nf->size = rnd(5, 15);
    nf->isPollinated = false;
    nf->glowRadius = 0;
    nf->isAlive = true;
    neonpollinatorFlowerIndex =
        cgl_wrap(neonpollinatorFlowerIndex + 1, 0, NEONPOLLINATOR_MAX_FLOWER_COUNT);
    neonpollinatorNextFlowerDist += 20;
  }
  FOR_EACH(neonpollinatorFlowers, i) {
    ASSIGN_ARRAY_ITEM(neonpollinatorFlowers, i, NeonpollinatorFlower, f);
    SKIP_IS_NOT_ALIVE(f);
    f->pos.x -= neonpollinatorGardenScrollSpeed;
    if (f->pos.x < -10) {
      f->isAlive = false;
    }
  }
  FOR_EACH(neonpollinatorFlowers, i) {
    ASSIGN_ARRAY_ITEM(neonpollinatorFlowers, i, NeonpollinatorFlower, f);
    SKIP_IS_NOT_ALIVE(f);
    if (f->isPollinated && f->glowRadius > 0) {
      color = YELLOW;
      thickness = 1;
      arc(f->pos.x, f->pos.y, f->glowRadius, 0, CGLP_PI * 2, &scratch);
    }
  }
  FOR_EACH(neonpollinatorFlowers, i) {
    ASSIGN_ARRAY_ITEM(neonpollinatorFlowers, i, NeonpollinatorFlower, f);
    SKIP_IS_NOT_ALIVE(f);
    if (f->isPollinated) {
      color = YELLOW;
    } else {
      color = LIGHT_PURPLE;
    }
    thickness = 3;
    arc(f->pos.x, f->pos.y, f->size, 0, CGLP_PI * 2, &scratch);
    if (scratch.isColliding.rect[YELLOW] && !f->isPollinated) {
      f->isPollinated = true;
      f->glowRadius = f->size * 3;
      play(SELECT);
      addScore(neonpollinatorMultiplier, f->pos.x, f->pos.y);
      neonpollinatorMultiplier++;
    }
    if (f->isPollinated) {
      float gr2 = f->glowRadius - 0.2 * difficulty;
      if (gr2 < 0) {
        gr2 = 0;
      }
      f->glowRadius = gr2;
    }
  }
  neonpollinatorNextOrbDist -= neonpollinatorGardenScrollSpeed;
  if (neonpollinatorNextOrbDist < 0) {
    ASSIGN_ARRAY_ITEM(neonpollinatorOrbs, neonpollinatorOrbIndex, NeonpollinatorOrb, no);
    vectorSet(&no->pos, 100, rnd(10, 90));
    no->isAlive = true;
    neonpollinatorOrbIndex = cgl_wrap(neonpollinatorOrbIndex + 1, 0, NEONPOLLINATOR_MAX_ORB_COUNT);
    neonpollinatorNextOrbDist += rnd(16, 20);
  }
  FOR_EACH(neonpollinatorOrbs, i) {
    ASSIGN_ARRAY_ITEM(neonpollinatorOrbs, i, NeonpollinatorOrb, o);
    SKIP_IS_NOT_ALIVE(o);
    bool isVisible = false;
    FOR_EACH(neonpollinatorFlowers, j) {
      ASSIGN_ARRAY_ITEM(neonpollinatorFlowers, j, NeonpollinatorFlower, f);
      SKIP_IS_NOT_ALIVE(f);
      if (f->isPollinated && distanceTo(&o->pos, f->pos.x, f->pos.y) < f->glowRadius) {
        isVisible = true;
      }
    }
    if (distanceTo(&o->pos, neonpollinatorPlayer.pos.x, neonpollinatorPlayer.pos.y) <
        neonpollinatorPlayer.glowRadius) {
      isVisible = true;
    }
    if (isVisible) {
      color = CYAN;
    } else {
      color = TRANSPARENT;
    }
    box(o->pos.x, o->pos.y, 5, 5, &scratch);
    if (scratch.isColliding.rect[CYAN]) {
      float ne = neonpollinatorPlayer.energy + 20;
      if (ne > 100) {
        ne = 100;
      }
      neonpollinatorPlayer.energy = ne;
      color = CYAN;
      particle(o->pos.x, o->pos.y, 9, 2, 0, CGLP_PI * 2);
      play(COIN);
      o->isAlive = false;
      continue;
    }
    o->pos.x -= neonpollinatorGardenScrollSpeed;
    if (o->pos.x < -10) {
      o->isAlive = false;
      continue;
    }
  }
  color = LIGHT_YELLOW;
  rect(0, 97, neonpollinatorPlayer.energy, 3, &scratch);
  int barColor;
  if (neonpollinatorPlayer.energy < 30) {
    if (ticks % 20 < 10) {
      barColor = RED;
    } else {
      barColor = TRANSPARENT;
    }
  } else {
    barColor = LIGHT_BLACK;
  }
  color = barColor;
  rect(neonpollinatorPlayer.energy, 97, 100 - neonpollinatorPlayer.energy, 3, &scratch);
}

void addGameNeonpollinator() {
  addGame(neonpollinatorTitle, neonpollinatorDescription, neonpollinatorCharacters,
          neonpollinatorCharactersCount, &neonpollinatorOptions, false,
          &neonpollinatorUpdate);
}
