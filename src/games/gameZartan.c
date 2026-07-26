#include "../cglp.h"

int* zartanTitle = "ZARTAN";
int* zartanDescription = "[Hold]\n Hold rope";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] zartanCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int zartanCharactersCount = 0;

Options zartanOptions = {100, 100, 2, false};

struct ZartanPile {
  Vector pos;
  bool isAlive;
};
#define ZARTAN_MAX_PILE_COUNT 32
ZartanPile[ZARTAN_MAX_PILE_COUNT] zartanPiles;
int zartanPileIndex;

Vector zartanP;
Vector zartanV;
// Vircon32 port note: upstream's `anchor`/`nearest` are live references
// directly into Vector objects living inside the `piles` array. There are
// no shared references across fixed arrays in this dialect (see
// gameAccelb.c's AccelbPlayerMissile port note for the general pattern), so
// the currently-grabbed pile is tracked by index instead.
// ZARTAN_MAX_PILE_COUNT is sized well above the handful of piles ever
// concurrently on screen (piles spawn no faster than every 9 ticks and
// take a while to cross the whole view before being removed), so the
// ring-buffer write index can never wrap back onto a still-anchored pile's
// slot before it's naturally released (by input release, by the rope
// breaking off-screen, or by grabbing a different pile).
bool zartanHasAnchor;
int zartanAnchorIndex;
float zartanNextAnchorDist;

void zartanUpdate() {
  Collision scratch;
  // Vircon32 port note: nothing in this game ever reads a Collision result
  // (game-over is plain y-position math, and the "nearest pile" search is
  // plain distance math over the piles array) - same optimization as
  // gameBmath.c, skipping the engine's per-frame hitbox bookkeeping
  // entirely since every box()/line() call below would otherwise register
  // hitboxes purely for the checkHitBox() machinery to never be asked
  // about. Restored automatically when the next game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(zartanPiles);
    zartanPileIndex = 0;
    zartanNextAnchorDist = 9;
    vectorSet(&zartanP, 99, 9);
    vectorSet(&zartanV, 0, 0);
    zartanHasAnchor = false;
    zartanAnchorIndex = -1;
  }
  float scr;
  if (zartanP.x > 30) {
    scr = (zartanP.x - 30) * 0.1 + difficulty * 0.1;
  } else {
    scr = difficulty * 0.1;
  }
  score += scr;
  zartanP.x -= scr;
  zartanV.y += 0.02;
  if (zartanV.y < 0 && zartanP.y < 0) {
    zartanV.y *= -1;
  }
  if (zartanP.y > 99) {
    play(RANDOM);
    gameOver();
  }
  vectorMul(&zartanV, 0.99);
  vectorAdd(&zartanP, zartanV.x, zartanV.y);
  color = GREEN;
  box(zartanP.x, zartanP.y, 7, 7, &scratch);

  float minDist = 99;
  int nearestIndex = -1;
  FOR_EACH(zartanPiles, mi) {
    ASSIGN_ARRAY_ITEM(zartanPiles, mi, ZartanPile, m);
    SKIP_IS_NOT_ALIVE(m);
    float dist = fabs(m->pos.y - zartanP.y);
    if (m->pos.x > zartanP.x && dist < minDist) {
      minDist = dist;
      nearestIndex = mi;
    }
  }
  color = CYAN;
  if (nearestIndex >= 0) {
    ZartanPile* nearest = &zartanPiles[nearestIndex];
    box(nearest->pos.x, nearest->pos.y, 9, 9, &scratch);
    if (input.isJustPressed) {
      play(SELECT);
      zartanHasAnchor = true;
      zartanAnchorIndex = nearestIndex;
    }
  }
  if (input.isPressed && zartanHasAnchor) {
    ZartanPile* a = &zartanPiles[zartanAnchorIndex];
    Vector d;
    vectorSet(&d, a->pos.x - zartanP.x, a->pos.y - zartanP.y);
    vectorMul(&d, 1.0 / 199);
    vectorAdd(&zartanV, d.x, d.y);
    line(zartanP.x, zartanP.y, a->pos.x, a->pos.y, &scratch);
    if (a->pos.x < 0) {
      zartanHasAnchor = false;
    }
  }
  if (input.isJustReleased) {
    zartanHasAnchor = false;
  }
  zartanNextAnchorDist -= scr;
  if (zartanNextAnchorDist < 0) {
    zartanNextAnchorDist += rnd(9, 66);
    ASSIGN_ARRAY_ITEM(zartanPiles, zartanPileIndex, ZartanPile, np);
    vectorSet(&np->pos, 99, rnd(0, 66));
    np->isAlive = true;
    zartanPileIndex = cgl_wrap(zartanPileIndex + 1, 0, ZARTAN_MAX_PILE_COUNT);
  }
  color = BLACK;
  FOR_EACH(zartanPiles, pileIdx) {
    ASSIGN_ARRAY_ITEM(zartanPiles, pileIdx, ZartanPile, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.x -= scr;
    box(p->pos.x, p->pos.y, 5, 5, &scratch);
    if (p->pos.x <= 0) {
      p->isAlive = false;
      continue;
    }
  }
}

void addGameZartan() {
  addGame(zartanTitle, zartanDescription, zartanCharacters,
          zartanCharactersCount, &zartanOptions, false, &zartanUpdate);
}
