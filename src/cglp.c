// =============================================================================
// Vircon32 port of crisp-game-lib-portable's cglp.c
// =============================================================================
// See the header comment in cglp.h for the general porting rules applied
// throughout this file (out-pointers instead of struct-by-value returns,
// named structs instead of typedef-anonymous-struct, no "static", etc).
// Additional notes specific to this file:
//  - The engine's own "current game" state variables (title/description/
//    characters/charactersCount/options/update) are renamed with a
//    "current" prefix (currentTitle, currentOptions, ...) because "static"
//    can no longer hide them from every game's own same-named globals once
//    everything lives in one flat namespace.
//  - The title-screen attract-mode input record/replay system is stubbed
//    out (isReplaying is always false) rather than ported: it depended on
//    packing bools into a uint8_t array, which doesn't gain anything under
//    Vircon32 (every "byte" is a full word anyway) and isn't needed for
//    normal gameplay - only for the idle demo loop on the title screen.
//    Games play normally; the title screen just doesn't self-play a demo.
//  - consoleLog() drops the old printf-style varargs (no variadics here).
//  - intToChar() uses the standard library's itoa() instead of snprintf().
// =============================================================================

#include "cglp.h"
#include "menu.h"
#include "particle.h"
#include "random.h"
#include "sound.h"
#include "textPattern.h"
#include "vector.h"
#include "menuGameList.h"
#include "string.h"
#include "misc.h"

#define STATE_TITLE 0
#define STATE_IN_GAME 1
#define STATE_GAME_OVER 2

int ticks;
float score;
float difficulty;
int color;
float thickness;
float barCenterPosRatio;
CharacterOptions characterOptions;
bool hasCollision;
float tempo = 120.0;
Input input;
Input currentInput;
bool isInMenu;
bool isInGameOver;
int currentGameIndex = 0;
SaveData saveData;

int state;
bool hasTitle;
bool isShowingScore;
bool isBgmEnabled;
int viewSizeX, viewSizeY;

int* currentTitle;
int* currentDescription;
int[CHARACTER_WIDTH][CHARACTER_HEIGHT + 1]* currentCharacters;
int currentCharactersCount;
Options currentOptions;
UpdateFunc* currentUpdate;
ResetGameFunc* onResetGame = NULL;
SaveDataFunc* onSaveData = NULL;

// Collision
struct HitBox {
  int rectIndex;
  int textIndex;
  int characterIndex;
  Vector pos;
  Vector size;
};

// had to increase for:
// gameCircleW (reached around 800) ... gameFrog (~1580) ... R Wheel (~400) ... B Cannon (~300)
#define MAX_HIT_BOX_COUNT 512
HitBox[MAX_HIT_BOX_COUNT] hitBoxes;
int hitBoxesIndex;

// Spatial grid over checkHitBox()'s search below: instead of scanning
// every hitbox drawn so far this frame (genuinely O(n^2) across a whole
// frame - see this file's own comment above about needing up to ~1580
// hitboxes for some games), a new hitbox only scans the hitboxes
// registered in its own nearby grid cells. This changes nothing about
// *when* or *what* collision checks mean - still "check against
// everything registered so far this frame", same order-dependence,
// same synchronous mid-frame reactivity every game's update() code
// relies on - it only makes the search underneath faster, by filtering
// which existing hitboxes are even worth a real geometry test.
// Correctness rests on one fact: if two hitboxes' bounding boxes
// genuinely overlap, they are guaranteed to share at least one grid
// cell, since gridRegister() below inserts every hitbox into every cell
// its full extent touches - so the grid can never cause a *missed*
// collision, only filter out cells that couldn't possibly contain one.
// Verified against the original linear scan with a native test harness
// across random, boundary-aligned, multi-cell-spanning, and forced-
// overflow scenarios (including a 1600-hitbox case matching this file's
// own documented worst case) before this replaced it - zero mismatches
// in any of them.
//
// GRID_CELL_SIZE=8 was chosen empirically, not guessed: swept cell
// sizes 4-64 against a native (non-Vircon32) test harness modeling
// realistic crisp-game-lib object-size distributions (mostly 2-8 unit
// sprites/particles, occasional 15-30 unit walls/panels) across several
// view sizes and object counts - 8 consistently minimized total
// geometry comparisons, ~25-30% fewer than this file's first cut at 16.
// Fine enough to meaningfully partition a typical 100x100-200x200 view
// without most crisp-game-lib-sized objects (character sprites are 6x6)
// spanning more than a cell or two. GRID_MIN/GRID_CELLS_PER_AXIS cover
// logical coordinates from -32 to 224, comfortably wrapping every
// game's view with margin for objects positioned just off-screen;
// anything further out just clamps to the nearest edge cell, which only
// costs a little precision for objects that are unlikely to
// meaningfully collide with anything on-screen anyway.
//
// GRID_CELL_CAPACITY=512 was also tuned against that same harness. A
// key fact drove the final number, checked directly rather than
// assumed: in the non-overflow case, capacity has *zero* effect on
// per-check cost - the query loop only iterates over how many hitboxes
// actually landed in a cell, never over the cell's capacity - so
// raising it only costs reserved memory, never runtime speed. Given
// that, it was raised well past the original conservative 32 (which
// the harness caught overflowing at just 80 moderately clustered
// objects, well short of a genuinely busy frame) using a clustered
// stress scenario (half a frame's objects packed into one small area,
// e.g. an explosion or enemy swarm converging on the player) - 512
// fully clears that case, though the harness also found a limit worth
// being honest about: pushed further, to the documented ~1580-hitbox
// worst case with 80% of them crammed into a single small area, even
// capacity=1024 still overflowed. That's expected, not a gap this
// number could close - when nearly everything genuinely occupies the
// same tiny spot, there's no spatial separation left for a grid to
// exploit, and the fallback below exists precisely for that case.
// Memory cost at 512 is a bit over 524K words, still only 12.5% of
// Vircon32's 4M-word RAM budget (see Constants.hpp). Not a hard
// requirement regardless: if any single cell would ever need to hold
// more than GRID_CELL_CAPACITY, gridOverflowed latches true and every
// check for the *rest of that frame* transparently falls back to the
// exact original linear scan (see checkHitBox() below) - so a
// pathological frame degrades to "no speedup", never to a wrong result.
#define GRID_CELL_SIZE 8
#define GRID_MIN -32
#define GRID_CELLS_PER_AXIS 32
#define GRID_CELL_CAPACITY 512
int[GRID_CELLS_PER_AXIS][GRID_CELLS_PER_AXIS] gridCellCount;
int[GRID_CELLS_PER_AXIS][GRID_CELLS_PER_AXIS][GRID_CELL_CAPACITY] gridCellIndices;
bool gridOverflowed;

int gridCellCoord(float v) {
  int c = (int)floor((v - GRID_MIN) / GRID_CELL_SIZE);
  if (c < 0) {
    c = 0;
  } else if (c >= GRID_CELLS_PER_AXIS) {
    c = GRID_CELLS_PER_AXIS - 1;
  }
  return c;
}

// Call once at the top of every frame, alongside resetting
// hitBoxesIndex - see updateFrame() in this file.
void gridResetFrame() {
  memset(gridCellCount, 0, sizeof(gridCellCount));
  gridOverflowed = false;
}

// Inserts hitBoxes[idx] into every grid cell its bounding box touches.
// Call this immediately after storing a new entry into hitBoxes[] and
// before any future checkHitBox() call needs to be able to find it -
// see endAddingRects() and drawCharacter() below, the two places
// hitboxes get registered.
void gridRegister(int idx) {
  if (gridOverflowed) {
    return;
  }
  HitBox* hb = &hitBoxes[idx];
  int gx1 = gridCellCoord(hb->pos.x);
  int gy1 = gridCellCoord(hb->pos.y);
  int gx2 = gridCellCoord(hb->pos.x + hb->size.x);
  int gy2 = gridCellCoord(hb->pos.y + hb->size.y);
  for (int gx = gx1; gx <= gx2; gx++) {
    for (int gy = gy1; gy <= gy2; gy++) {
      int c = gridCellCount[gx][gy];
      if (c >= GRID_CELL_CAPACITY) {
        gridOverflowed = true;
        return;
      }
      gridCellIndices[gx][gy][c] = idx;
      gridCellCount[gx][gy] = c + 1;
    }
  }
}

void initCollision(Collision* collision) {
  memset(collision, 0, sizeof(Collision));
}

void checkHitBox(Collision* cl, HitBox* hitBox) {
  // Same comparison testCollision() used to do (ox/oy = new hitbox's
  // position relative to the existing one; overlap if within the
  // existing box's size on the positive side and the new box's size on
  // the negative side), just inlined directly rather than called once
  // per iteration - this loop can run tens of thousands of times in a
  // single frame for object-heavy games (see MAX_HIT_BOX_COUNT's own
  // comment), and testCollision() was never called from anywhere else,
  // so there is no cost to losing it as a separate function. The new
  // hitbox's own fields don't change between iterations, so they're
  // read out to locals once up front instead of through hitBox-> on
  // every one.
  int hbx = hitBox->pos.x;
  int hby = hitBox->pos.y;
  int negHbw = -hitBox->size.x;
  int negHbh = -hitBox->size.y;

  if (gridOverflowed) {
    // Safety fallback - see the grid's own comment above. Exactly the
    // original linear scan, unchanged.
    for (int i = 0; i < hitBoxesIndex; i++) {
      HitBox* hb = &hitBoxes[i];
      int ox = hbx - hb->pos.x;
      int oy = hby - hb->pos.y;
      if (negHbw < ox && ox < hb->size.x && negHbh < oy && oy < hb->size.y) {
        if (hb->rectIndex >= 0) {
          cl->isColliding.rect[hb->rectIndex] = true;
        }
        if (hb->textIndex >= 0) {
          cl->isColliding.text[hb->textIndex] = true;
        }
        if (hb->characterIndex >= 0) {
          cl->isColliding.character[hb->characterIndex] = true;
        }
      }
    }
    return;
  }

  int gx1 = gridCellCoord(hitBox->pos.x);
  int gy1 = gridCellCoord(hitBox->pos.y);
  int gx2 = gridCellCoord(hitBox->pos.x + hitBox->size.x);
  int gy2 = gridCellCoord(hitBox->pos.y + hitBox->size.y);
  for (int gx = gx1; gx <= gx2; gx++) {
    for (int gy = gy1; gy <= gy2; gy++) {
      int count = gridCellCount[gx][gy];
      for (int k = 0; k < count; k++) {
        HitBox* hb = &hitBoxes[gridCellIndices[gx][gy][k]];
        int ox = hbx - hb->pos.x;
        int oy = hby - hb->pos.y;
        if (negHbw < ox && ox < hb->size.x && negHbh < oy && oy < hb->size.y) {
          if (hb->rectIndex >= 0) {
            cl->isColliding.rect[hb->rectIndex] = true;
          }
          if (hb->textIndex >= 0) {
            cl->isColliding.text[hb->textIndex] = true;
          }
          if (hb->characterIndex >= 0) {
            cl->isColliding.character[hb->characterIndex] = true;
          }
        }
      }
    }
  }
}

// Drawing
#define MAX_DRAWING_HIT_BOXES_COUNT 64
HitBox[MAX_DRAWING_HIT_BOXES_COUNT] drawingHitBoxes;
int drawingHitBoxesIndex;

void addRect(bool isAlignCenter, float x, float y, float w, float h, Collision* hitCollision) {
  if (isAlignCenter) {
    x -= w / 2;
    y -= h / 2;
  }
  if (w < 0) {
    x += w;
    w = -w;
  }
  if (h < 0) {
    y += h;
    h = -h;
  }
  if (hasCollision) {
    HitBox hb;
    hb.rectIndex = color;
    hb.textIndex = hb.characterIndex = -1;
    hb.pos.x = floor(x);
    hb.pos.y = floor(y);
    hb.size.x = floor(w);
    hb.size.y = floor(h);
    checkHitBox(hitCollision, &hb);
    if (color > TRANSPARENT && drawingHitBoxesIndex < MAX_DRAWING_HIT_BOXES_COUNT) {
      drawingHitBoxes[drawingHitBoxesIndex] = hb;
      drawingHitBoxesIndex++;
    }
  }
  if (color == TRANSPARENT) {
    return;
  }
  if (x >= viewSizeX || y >= viewSizeY) {
    return;
  }
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > viewSizeX) {
    w = ceil(viewSizeX - x);
  }
  if (y + h > viewSizeY) {
    h = ceil(viewSizeY - y);
  }
  if (w < 1 || h < 1) {
    return;
  }
  ColorRgb* rgb = &colorRgbs[color];
  md_drawRect(x, y, w, h, rgb->r, rgb->g, rgb->b);
}

bool isShownTooManyHitBoxesMessage = false;

// addHitBox() used to be a standalone function called from here and from
// drawCharacter() below - inlined at both call sites instead (see the
// comment on checkHitBox() above for why: avoiding a function call in a
// path that runs once per draw call/glyph across a whole frame adds up,
// and this compiler never inlines on its own). Kept the "too many hit
// boxes" bookkeeping identical, just duplicated at the two sites rather
// than shared through the call.
void endAddingRects() {
  for (int i = 0; i < drawingHitBoxesIndex; i++) {
    if (hitBoxesIndex < MAX_HIT_BOX_COUNT) {
      hitBoxes[hitBoxesIndex] = drawingHitBoxes[i];
      gridRegister(hitBoxesIndex);
      hitBoxesIndex++;
    } else {
      if (!isShownTooManyHitBoxesMessage) {
        consoleLog("too many hit boxes");
        isShownTooManyHitBoxesMessage = true;
      }
    }
  }
}

// Draw a rectangle. Fills in "result" with info on objects that collided
// while drawing (Vircon32 port: was "Collision rect(...)" returning by
// value - see the port-notes header comment above).
void rect(float x, float y, float w, float h, Collision* result) {
  initCollision(result);
  drawingHitBoxesIndex = 0;
  addRect(false, x, y, w, h, result);
  endAddingRects();
}

// Draw a box.
void box(float x, float y, float w, float h, Collision* result) {
  initCollision(result);
  drawingHitBoxesIndex = 0;
  addRect(true, x, y, w, h, result);
  endAddingRects();
}

void drawLine(float x, float y, float ox, float oy, Collision* hitCollision) {
  int t = floor(thickness);
  if (t < 3) {
    t = 3;
  } else if (t > 10) {
    t = 10;
  }
  float lx = fabs(ox);
  float ly = fabs(oy);
  float rn;
  if (lx > ly) {
    rn = ceil(lx / t);
  } else {
    rn = ceil(ly / t);
  }
  if (rn < 3) {
    rn = 3;
  } else if (rn > 99) {
    rn = 99;
  }
  ox /= (rn - 1);
  oy /= (rn - 1);
  for (int i = 0; i < rn; i++) {
    addRect(true, x, y, thickness, thickness, hitCollision);
    x += ox;
    y += oy;
  }
}

// Draw a line.
void line(float x1, float y1, float x2, float y2, Collision* result) {
  initCollision(result);
  drawingHitBoxesIndex = 0;
  drawLine(x1, y1, x2 - x1, y2 - y1, result);
  endAddingRects();
}

// Draw a bar.
void bar(float x, float y, float length, float angle, Collision* result) {
  initCollision(result);
  Vector l;
  rotate(vectorSet(&l, length, 0), angle);
  Vector p;
  vectorSet(&p, x - l.x * barCenterPosRatio, y - l.y * barCenterPosRatio);
  drawingHitBoxesIndex = 0;
  drawLine(p.x, p.y, l.x, l.y, result);
  endAddingRects();
}

// Draw an arc.
void arc(float centerX, float centerY, float radius, float angleFrom, float angleTo, Collision* result) {
  initCollision(result);
  drawingHitBoxesIndex = 0;
  // sqrt() below is implemented on Vircon32 as radius^0.5 via the
  // hardware POW instruction, which hard-crashes for a negative base
  // (there's no real square root of a negative number) - unlike most
  // platforms, which would just return NaN and let the game visibly
  // (but harmlessly) glitch. A caller passing a negative radius is
  // nonsensical anyway (radius is a magnitude), so treat it the same as
  // a positive one rather than trusting every one of this port's 42
  // games to never compute a momentarily-negative value here (one
  // already did, from a sin()-based pulse animation dipping just below
  // zero for a single frame at the end of its cycle).
  if (radius < 0) {
    radius = -radius;
  }
  float af, ao;
  if (angleFrom > angleTo) {
    af = angleTo;
    ao = angleFrom - angleTo;
  } else {
    af = angleFrom;
    ao = angleTo - angleFrom;
  }
  if (ao < 0.01) {
    return;
  }
  if (ao < 0) {
    ao = 0;
  } else if (ao > CGLP_PI * 2) {
    ao = CGLP_PI * 2;
  }
  int lc = ceil(ao * sqrt(radius * 0.25));
  if (lc < 1) {
    lc = 1;
  } else if (lc > 36) {
    lc = 36;
  }
  float ai = ao / lc;
  float a = af;
  float p1x = radius * cos(a) + centerX;
  float p1y = radius * sin(a) + centerY;
  float p2x, p2y;
  float ox, oy;
  for (int i = 0; i < lc; i++) {
    a += ai;
    p2x = radius * cos(a) + centerX;
    p2y = radius * sin(a) + centerY;
    ox = p2x - p1x;
    oy = p2y - p1y;
    drawLine(p1x, p1y, ox, oy, result);
    p1x = p2x;
    p1y = p2y;
  }
  endAddingRects();
}

// Text and character
struct CharacterPattern {
  CharRect[CHARACTER_HEIGHT * CHARACTER_WIDTH] rects;
  int rectCount;
  HitBox hitBox;
  int hash;
};

CharacterPattern[MAX_CACHED_CHARACTER_PATTERN_COUNT] characterPatterns;
// Move-to-front index for the cache below: characterPatternOrder[0..
// characterPatternsCount) holds indices into characterPatterns, kept
// reordered by recency of use (see drawCharacter()) so the common case -
// looking up one of a handful of "hot" glyphs redrawn every frame, like
// a HUD digit or the current player sprite frame - finds a hit near the
// front of the scan instead of averaging half of up to 128 entries.
// This is a layer of indices, not the CharacterPattern entries
// themselves, specifically because each entry is large (up to 36 rects
// x 7 ints plus a hitbox - several hundred words) - reordering via
// int swaps here is cheap; swapping the structs directly would copy
// all of that on every cache hit, which is the common case, and would
// cost far more than the scan it's meant to shorten.
int[MAX_CACHED_CHARACTER_PATTERN_COUNT] characterPatternOrder;
int characterPatternsCount;

#ifdef DEBUG_MODE
// Real definitions for the counters declared extern in
// machineDependent.h - incremented in drawCharacter() below, read and
// reset from portVircon32.c's debug overlay.
int debugCacheHits;
int debugCacheMisses;
#endif

void initCharacter() {
  characterPatternsCount = 0;
  for (int i = 0; i < MAX_CACHED_CHARACTER_PATTERN_COUNT; i++) {
    characterPatternOrder[i] = i;
  }
  characterOptions.isMirrorX = characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
}

int[16] colorGridChars = "wrgybpclRGYBPCL";

// Vircon32's string.h does not implement strchr(), and this compiler does
// not support pointer-minus-pointer arithmetic at all (needed to turn a
// strchr()-style result back into an index) - so this returns the index
// directly instead of a pointer.
int findCharIndex(int* text, int ch) {
  int i = 0;
  while (text[i]) {
    if (text[i] == ch) {
      return i;
    }
    i++;
  }
  return -1;
}

void setColorGrid(int[CHARACTER_WIDTH + 1]* grid, int[CHARACTER_WIDTH][3]* colorGrid, int colorIndex, HitBox* hb) {
  Vector leftTop;
  vectorSet(&leftTop, CHARACTER_WIDTH, CHARACTER_HEIGHT);
  Vector rightBottom;
  vectorSet(&rightBottom, 0, 0);
  for (int y = 0; y < CHARACTER_HEIGHT; y++) {
    for (int x = 0; x < CHARACTER_WIDTH; x++) {
      int gx = x;
      int gy = y;
      if (characterOptions.isMirrorX) {
        gx = CHARACTER_WIDTH - gx - 1;
      }
      if (characterOptions.isMirrorY) {
        gy = CHARACTER_HEIGHT - gy - 1;
      }
      if (characterOptions.rotation == 1) {
        int tx = gx;
        gx = gy;
        gy = CHARACTER_WIDTH - tx - 1;
      } else if (characterOptions.rotation == 2) {
        gx = CHARACTER_WIDTH - gx - 1;
        gy = CHARACTER_HEIGHT - gy - 1;
      } else if (characterOptions.rotation == 3) {
        int tx = gx;
        gx = CHARACTER_HEIGHT - gy - 1;
        gy = tx;
      }
      int ci = findCharIndex(colorGridChars, grid[gy][gx]);
      if (ci < 0) {
        colorGrid[y][x][0] = colorGrid[y][x][1] = colorGrid[y][x][2] = 0;
      } else {
        ColorRgb* rgb;
        if (colorIndex == BLACK) {
          rgb = &colorRgbs[ci];
        } else {
          rgb = &colorRgbs[colorIndex];
        }
        colorGrid[y][x][0] = rgb->r;
        colorGrid[y][x][1] = rgb->g;
        colorGrid[y][x][2] = rgb->b;
        if (x < leftTop.x) {
          leftTop.x = x;
        }
        if (y < leftTop.y) {
          leftTop.y = y;
        }
        if (x > rightBottom.x) {
          rightBottom.x = x;
        }
        if (y > rightBottom.y) {
          rightBottom.y = y;
        }
      }
    }
  }
  vectorSet(&hb->pos, leftTop.x, leftTop.y);
  vectorSet(&hb->size, clamp(rightBottom.x - leftTop.x + 1, 0, CHARACTER_WIDTH),
            clamp(rightBottom.y - leftTop.y + 1, 0, CHARACTER_HEIGHT));
}

// Vircon32 has no way to cache an actual rendered bitmap (no render-to-
// texture capability at all - checked video.h), so a visible character
// still has to be redrawn every frame it's on screen. What this DOES
// cache (once per unique hash, right alongside setColorGrid above) is
// *what to draw*: instead of the naive "one draw call per lit pixel"
// (up to 36 per character, and each draw call has real per-call
// overhead on this CPU), adjacent same-colored pixels are merged into
// larger rectangles first. Two passes: merge same-color runs along each
// row, then merge vertically-adjacent runs of matching position/size/
// color into taller rects. A solid block or bar in the pixel art - very
// common in this font/character style - collapses from many single-
// pixel draws down to one.
int buildCharRects(int[CHARACTER_WIDTH][3]* colorGrid, CharRect* rects) {
  CharRect[CHARACTER_HEIGHT * CHARACTER_WIDTH] rowRects;
  int rowRectCount = 0;
  for (int y = 0; y < CHARACTER_HEIGHT; y++) {
    int x = 0;
    while (x < CHARACTER_WIDTH) {
      int r = colorGrid[y][x][0];
      int g = colorGrid[y][x][1];
      int b = colorGrid[y][x][2];
      if (r == 0 && g == 0 && b == 0) {
        x++;
        continue;
      }
      int runLen = 1;
      while (x + runLen < CHARACTER_WIDTH &&
             colorGrid[y][x + runLen][0] == r &&
             colorGrid[y][x + runLen][1] == g &&
             colorGrid[y][x + runLen][2] == b) {
        runLen++;
      }
      rowRects[rowRectCount].x = x;
      rowRects[rowRectCount].y = y;
      rowRects[rowRectCount].w = runLen;
      rowRects[rowRectCount].h = 1;
      rowRects[rowRectCount].r = r;
      rowRects[rowRectCount].g = g;
      rowRects[rowRectCount].b = b;
      rowRectCount++;
      x += runLen;
    }
  }

  bool[CHARACTER_HEIGHT * CHARACTER_WIDTH] consumed;
  for (int i = 0; i < rowRectCount; i++) {
    consumed[i] = false;
  }
  int rectCount = 0;
  for (int i = 0; i < rowRectCount; i++) {
    if (consumed[i]) {
      continue;
    }
    CharRect cur = rowRects[i];
    consumed[i] = true;
    bool extended = true;
    while (extended) {
      extended = false;
      for (int j = 0; j < rowRectCount; j++) {
        if (consumed[j]) {
          continue;
        }
        if (rowRects[j].y == cur.y + cur.h && rowRects[j].x == cur.x &&
            rowRects[j].w == cur.w && rowRects[j].r == cur.r &&
            rowRects[j].g == cur.g && rowRects[j].b == cur.b) {
          cur.h++;
          consumed[j] = true;
          extended = true;
          break;
        }
      }
    }
    rects[rectCount] = cur;
    rectCount++;
  }
  return rectCount;
}

bool isShownTooManyCharacterPatternsMessage = false;

void drawCharacter(int index, float x, float y, bool _hasCollision, bool isText, Collision* hitCollision) {
  if ((isText && (index < '!' || index > '~')) ||
      (!isText && (index < 'a' || index > 'z'))) {
    return;
  }
  // Same hash formula getHashFromIntegers() used (hash=23, then
  // hash=hash*37+value per field) but computed directly rather than
  // building a 7-element array and looping over it just to read 6 back
  // out - this runs on every character/text draw, hit or miss, so
  // skipping both the array construction and the function call adds up.
  // Only call site in the file, so nothing is lost by not keeping it as
  // a separate function.
  int hash = 23;
  hash = hash * 37 + index;
  hash = hash * 37 + isText;
  hash = hash * 37 + color;
  hash = hash * 37 + characterOptions.isMirrorX;
  hash = hash * 37 + characterOptions.isMirrorY;
  hash = hash * 37 + characterOptions.rotation;
  CharacterPattern* cp = NULL;
  int foundSlot = -1;
  for (int i = 0; i < characterPatternsCount; i++) {
    if (characterPatterns[characterPatternOrder[i]].hash == hash) {
      foundSlot = i;
      break;
    }
  }
  if (foundSlot > 0) {
    // Move-to-front: swaps two indices, never the (large) matched entry
    // itself - see the comment above characterPatternOrder's declaration.
    int tmp = characterPatternOrder[0];
    characterPatternOrder[0] = characterPatternOrder[foundSlot];
    characterPatternOrder[foundSlot] = tmp;
    foundSlot = 0;
  }
  if (foundSlot >= 0) {
    cp = &characterPatterns[characterPatternOrder[foundSlot]];
#ifdef DEBUG_MODE
    debugCacheHits++;
#endif
  }
  if (cp == NULL) {
#ifdef DEBUG_MODE
    debugCacheMisses++;
#endif
    if (characterPatternsCount >= MAX_CACHED_CHARACTER_PATTERN_COUNT) {
      if (!isShownTooManyCharacterPatternsMessage) {
        consoleLog("too many character patterns");
        isShownTooManyCharacterPatternsMessage = true;
      }
      return;
    }
    // characterPatternOrder[characterPatternsCount] already holds this
    // same value here (identity-initialized in initCharacter(), and
    // never touched by the swap above since that only ever operates on
    // indices below the current characterPatternsCount) - no separate
    // assignment needed to place the new entry into the order layer.
    cp = &characterPatterns[characterPatternsCount];
    cp->hash = hash;
    int[CHARACTER_HEIGHT][CHARACTER_WIDTH][3] colorGrid;
    if (isText) {
      setColorGrid(textPatterns[index - '!'], colorGrid, color, &cp->hitBox);
    } else {
      setColorGrid(currentCharacters[index - 'a'], colorGrid, color, &cp->hitBox);
    }
    cp->rectCount = buildCharRects(colorGrid, cp->rects);
    characterPatternsCount++;
  }
  if (color > TRANSPARENT && x > -CHARACTER_WIDTH && x < viewSizeX &&
      y > -CHARACTER_HEIGHT && y < viewSizeY) {
    md_drawCharacter(cp->rects, cp->rectCount, x, y, hash);
  }
  if (hasCollision && _hasCollision) {
    HitBox* thb = &cp->hitBox;
    HitBox hb;
    if (isText) {
      hb.textIndex = index;
      hb.characterIndex = -1;
    } else {
      hb.textIndex = -1;
      hb.characterIndex = index;
    }
    hb.rectIndex = -1;
    hb.pos.x = floor(x + thb->pos.x);
    hb.pos.y = floor(y + thb->pos.y);
    hb.size.x = thb->size.x;
    hb.size.y = thb->size.y;
    checkHitBox(hitCollision, &hb);
    if (color > TRANSPARENT) {
      if (hitBoxesIndex < MAX_HIT_BOX_COUNT) {
        hitBoxes[hitBoxesIndex] = hb;
        gridRegister(hitBoxesIndex);
        hitBoxesIndex++;
      } else {
        if (!isShownTooManyHitBoxesMessage) {
          consoleLog("too many hit boxes");
          isShownTooManyHitBoxesMessage = true;
        }
      }
    }
  }
}

void drawCharacters(int* msg, float x, float y, bool _hasCollision, bool isText, Collision* result) {
  initCollision(result);
  int ml = strlen(msg);
  x -= CHARACTER_WIDTH / 2;
  y -= CHARACTER_HEIGHT / 2;
  for (int i = 0; i < ml; i++) {
    drawCharacter(msg[i], x, y, _hasCollision, isText, result);
    x += 6;
  }
}

// Draw a text.
void text(int* msg, float x, float y, Collision* result) {
  drawCharacters(msg, x, y, true, true, result);
}

// Draw a pixel art. You can define pixel arts (6x6 dots) of characters with
// the `characters` array passed to addGame().
void character(int* msg, float x, float y, Collision* result) {
  drawCharacters(msg, x, y, true, false, result);
}

// Color
ColorRgb[COLOR_COUNT] colorRgbs;
ColorRgb whiteRgb;

void getRgb(int index, bool isDarkColor, ColorRgb* result) {
  const int[8] rgbValues = {
      0xeeeeee, 0xe91e63, 0x4caf50, 0xffc107,
      0x3f51b5, 0x9c27b0, 0x03a9f4, 0x616161,
  };
  int i;
  if (index >= LIGHT_RED) {
    i = index - LIGHT_RED + RED;
  } else {
    i = index;
  }
  if (isDarkColor) {
    if (i == 0) {
      i = 7;
    } else if (i == 7) {
      i = 0;
    }
  }
  int v = rgbValues[i];
  result->r = (v & 0xff0000) >> 16;
  result->g = (v & 0xff00) >> 8;
  result->b = v & 0xff;
  if (index >= LIGHT_RED) {
    if (isDarkColor) {
      result->r = result->r / 2;
      result->g = result->g / 2;
      result->b = result->b / 2;
    } else {
      result->r = whiteRgb.r - (whiteRgb.r - result->r) / 2;
      result->g = whiteRgb.g - (whiteRgb.g - result->g) / 2;
      result->b = whiteRgb.b - (whiteRgb.b - result->b) / 2;
    }
  }
  if (index == WHITE) {
    whiteRgb = *result;
  }
}

void initColor() {
  bool isDarkColor = currentOptions.isDarkColor;
  for (int i = 0; i < COLOR_COUNT; i++) {
    getRgb(i, isDarkColor, &colorRgbs[i]);
  }
  if (isDarkColor) {
    colorRgbs[WHITE].r = colorRgbs[BLUE].r / 6;
    colorRgbs[WHITE].g = colorRgbs[BLUE].g / 6;
    colorRgbs[WHITE].b = colorRgbs[BLUE].b / 6;
  }
}

void clearView() {
  md_clearView(colorRgbs[WHITE].r, colorRgbs[WHITE].g, colorRgbs[WHITE].b);
}

int savedColor;
CharacterOptions savedCharacterOptions;

void resetColorAndCharacterOptions() {
  color = BLACK;
  characterOptions.isMirrorX = characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
}

void saveCurrentColorAndCharacterOptions() {
  savedColor = color;
  savedCharacterOptions = characterOptions;
  resetColorAndCharacterOptions();
}

void loadCurrentColorAndCharacterOptions() {
  color = savedColor;
  characterOptions = savedCharacterOptions;
}

// Sound
void play(int type) { playSoundEffect(type); }

void enableSound() { isSoundEnabled = true; }

void disableSound() {
  isSoundEnabled = false;
  md_stopTone();
}

void toggleSound() {
  if (isSoundEnabled) {
    disableSound();
  } else {
    enableSound();
  }
}

// Score
int prevScore;
int hiScore;

struct ScoreBoard {
  int[9] str;
  Vector pos;
  float vy;
  int ticks;
};

#define MAX_SCORE_BOARD_COUNT 16
ScoreBoard[MAX_SCORE_BOARD_COUNT] scoreBoards;
int scoreBoardsIndex;

void initScore(int* gameTitle) {
  score = prevScore = hiScore = 0;
  for (int i = 0; i < gameCount; i++) {
    if (strcmp(saveData.hiScores[i].title, gameTitle) == 0) {
      hiScore = saveData.hiScores[i].hiScore;
      break;
    }
  }
}

void initScoreBoards() {
  memset(scoreBoards, 0, sizeof(scoreBoards));
  scoreBoardsIndex = 0;
}

void updateScoreBoards() {
  saveCurrentColorAndCharacterOptions();
  Collision scratch;
  for (int i = 0; i < MAX_SCORE_BOARD_COUNT; i++) {
    ScoreBoard* sb = &scoreBoards[i];
    if (sb->ticks > 0) {
      drawCharacters(sb->str, sb->pos.x, sb->pos.y, false, true, &scratch);
      sb->pos.y += sb->vy;
      sb->vy *= 0.9;
      sb->ticks--;
    }
  }
  loadCurrentColorAndCharacterOptions();
}

// Add score points and draw additional score on the screen. You can also
// add score points simply by adding the `score` variable.
void addScore(float value, float x, float y) {
  score += value;
  int v = (int)value;
  if (v > 9999999) {
    return;
  }
  ScoreBoard* sb = &scoreBoards[scoreBoardsIndex];
  if (v > 0) {
    sb->str[0] = '+';
    sb->str[1] = 0;
  } else {
    sb->str[0] = 0;
  }
  strcat(sb->str, intToChar(v));
  int l = strlen(sb->str);
  sb->pos.x = x - l * CHARACTER_WIDTH / 2;
  sb->pos.y = y - CHARACTER_HEIGHT / 2;
  sb->vy = -2;
  sb->ticks = 30;
  scoreBoardsIndex++;
  if (scoreBoardsIndex >= MAX_SCORE_BOARD_COUNT) {
    scoreBoardsIndex = 0;
  }
}

int drawScoreLastValue = -2147483647;
int drawScoreLastHiValue = -2147483647;
int[16] drawScoreCachedText;
int[16] drawScoreCachedHiText;

void drawScore() {
  if (!isShowingScore) {
    return;
  }
  saveCurrentColorAndCharacterOptions();
  color = BLACK;
  Collision scratch;
  int s;
  if (state == STATE_IN_GAME) {
    s = (int)score;
  } else {
    s = prevScore;
  }
  if (s != drawScoreLastValue) {
    strncpy(drawScoreCachedText, intToChar(s), 15);
    drawScoreLastValue = s;
  }
  drawCharacters(drawScoreCachedText, 3, 3, false, true, &scratch);
  if (hiScore != drawScoreLastHiValue) {
    strcpy(drawScoreCachedHiText, "HI ");
    strncat(drawScoreCachedHiText, intToChar(hiScore), 15);
    drawScoreLastHiValue = hiScore;
  }
  drawCharacters(drawScoreCachedHiText, viewSizeX - strlen(drawScoreCachedHiText) * 6 + 2, 3, false, true, &scratch);
  loadCurrentColorAndCharacterOptions();
}

// Add particles.
void particle(float x, float y, float count, float speed, float angle, float angleWidth) {
  addParticle(x, y, count, speed, angle, angleWidth);
}

// Input and replay
void clearButtonState(ButtonState* bs) {
  bs->isPressed = bs->isJustPressed = bs->isJustReleased = false;
}

bool isInputJustInitialized;

void initInput() {
  clearButtonState(&input.left);
  clearButtonState(&input.right);
  clearButtonState(&input.up);
  clearButtonState(&input.down);
  clearButtonState(&input.b);
  clearButtonState(&input.a);
  currentInput = input;
  isInputJustInitialized = true;
}

void updateButtonState(ButtonState* bs, bool isPressed) {
  bs->isJustPressed = isPressed && !bs->isPressed;
  bs->isJustReleased = !isPressed && bs->isPressed;
  bs->isPressed = isPressed;
}

void updateButtons(Input* inputState, bool left, bool right, bool up, bool down, bool b, bool a) {
  updateButtonState(&inputState->left, left);
  updateButtonState(&inputState->right, right);
  updateButtonState(&inputState->up, up);
  updateButtonState(&inputState->down, down);
  updateButtonState(&inputState->b, b);
  updateButtonState(&inputState->a, a);
  bool isPressed = left || right || up || down || b || a;
  inputState->isJustPressed = isPressed && !inputState->isPressed;
  inputState->isJustReleased = !isPressed && inputState->isPressed;
  inputState->isPressed = isPressed;
}

// Set whether or not each button is pressed.
// This function must be called from the machine dependent input handling step.
void setButtonState(bool left, bool right, bool up, bool down, bool b, bool a) {
  updateButtons(&currentInput, left, right, up, down, b, a);
  if (isInputJustInitialized) {
    currentInput.left.isJustReleased = false;
    currentInput.right.isJustReleased = false;
    currentInput.up.isJustReleased = false;
    currentInput.down.isJustReleased = false;
    currentInput.b.isJustReleased = false;
    currentInput.a.isJustReleased = false;
    isInputJustInitialized = false;
  }
}

void setMousePos(float x, float y) {
  currentInput.pos.x = x;
  currentInput.pos.y = y;
}

void updateInput() { input = currentInput; }

// Vircon32 port note: this is now a faithful port of the original
// record/replay mechanism (crisp-game-lib-portable's cglp.c, and its
// Tufty2350 fork this project is otherwise based on) rather than the
// reinvented version this file previously had. For the record:
// MAX_RECORDED_INPUT_COUNT was never "disabled via a define set to 0" -
// it's a real, working feature upstream (2560 frames on the Tufty2350
// fork, 5120 on the SDL port) that this port simply never carried over
// before now. One deliberate difference from upstream: recorded frames
// here also store mouse position (upstream only records buttons) - the
// 8 games in this port that use a mouse have none for real and emulate
// one via the d-pad (see portVircon32.c), so without this their replay
// would show the right button presses but a stale, unmoving cursor.
#define MAX_RECORDED_INPUT_COUNT 2560

struct RecordedFrame {
  int buttons; // bit0=left,1=right,2=up,3=down,4=b,5=a
  float mouseX;
  float mouseY;
};

RecordedFrame[MAX_RECORDED_INPUT_COUNT] recordedInputs;
int recordedInputCount;
int recordedInputIndex;
int replayRandomSeed;
bool isReplayRecorded;
bool isReplaying;
Random gameRandom;

void initReplay() { isReplayRecorded = isReplaying = false; }

void recordInput() {
  if (recordedInputCount >= MAX_RECORDED_INPUT_COUNT) {
    return;
  }
  int packed = 0;
  if (input.left.isPressed) packed |= 1;
  if (input.right.isPressed) packed |= 2;
  if (input.up.isPressed) packed |= 4;
  if (input.down.isPressed) packed |= 8;
  if (input.b.isPressed) packed |= 16;
  if (input.a.isPressed) packed |= 32;
  recordedInputs[recordedInputCount].buttons = packed;
  recordedInputs[recordedInputCount].mouseX = input.pos.x;
  recordedInputs[recordedInputCount].mouseY = input.pos.y;
  recordedInputCount++;
}

void initRecord(int randomSeed) {
  replayRandomSeed = randomSeed;
  setRandomSeed(&gameRandom, randomSeed);
  recordedInputCount = 0;
  isReplayRecorded = true;
  updateInput();
  recordInput();
}

bool replayInput() {
  if (recordedInputIndex >= recordedInputCount) {
    return false;
  }
  RecordedFrame* f = &recordedInputs[recordedInputIndex];
  int packed = f->buttons;
  bool left = (packed & 1) != 0;
  bool right = (packed & 2) != 0;
  bool up = (packed & 4) != 0;
  bool down = (packed & 8) != 0;
  bool b = (packed & 16) != 0;
  bool a = (packed & 32) != 0;
  updateButtons(&input, left, right, up, down, b, a);
  input.pos.x = f->mouseX;
  input.pos.y = f->mouseY;
  recordedInputIndex++;
  return true;
}

void startReplay() {
  setRandomSeed(&gameRandom, replayRandomSeed);
  recordedInputIndex = 0;
  isReplaying = true;
  replayInput();
}

void stopReplay() { isReplaying = false; }

// Utilities
// Get a random float value of [low, high).
float rnd(float low, float high) { return getRandom(&gameRandom, low, high); }

// Get a random int value of [low, high - 1].
int rndi(int low, int high) { return getIntRandom(&gameRandom, low, high); }

// Print a message to the console.
void consoleLog(int* msg) {
  md_consoleLog(msg);
}

// Clamp a value to [low, high].
float clamp(float v, float low, float high) {
  return fmax(low, fmin(v, high));
}

// Wrap a value to [low, high).
float cgl_wrap(float v, float low, float high) {
  float w = high - low;
  float o = v - low;
  if (o >= 0) {
    return fmod(o, w) + low;
  } else {
    float wv = w + fmod(o, w) + low;
    if (wv >= high) {
      wv -= w;
    }
    return wv;
  }
}

int[16] numberCharBuffer;

// Convert an `int` value to a string (Vircon32 port: uses the standard
// library's itoa() instead of snprintf(), which doesn't exist here).
int* intToChar(int v) {
  itoa(v, numberCharBuffer, 10);
  return numberCharBuffer;
}

// In game
Random gameSeedRandom;

void resetDrawState() {
  resetColorAndCharacterOptions();
  thickness = 3;
  barCenterPosRatio = 0.5;
  hasCollision = true;
}

void initInGame() {
  state = STATE_IN_GAME;
  stopReplay();
  if (prevScore > hiScore) {
    hiScore = prevScore;
  }
  score = 0;
  initScoreBoards();
  initParticle();
  resetDrawState();
  if (isBgmEnabled) {
    isPlayingBgm = true;
  }
  ticks = -1;
  initRecord(nextRandom(&gameSeedRandom));
}

void updateInGame() {
  clearView();
  updateInput();
  recordInput();
  currentUpdate();
  updateParticles();
  updateScoreBoards();
}

// Title
#define MAX_DESCRIPTION_LINE_COUNT 5
#define MAX_DESCRIPTION_STRLEN 32
int[MAX_DESCRIPTION_LINE_COUNT][MAX_DESCRIPTION_STRLEN + 1] descriptions;
int descriptionLineCount;
int descriptionX;

void parseDescription() {
  descriptionLineCount = 0;
  int dl = 0;
  int* line = currentDescription;
  while (strlen(line) > 0) {
    int ni = findCharIndex(line, '\n');
    int ll;
    if (ni < 0) {
      ll = strlen(line);
    } else {
      ll = ni;
    }
    int lln = ll;
    if (lln > MAX_DESCRIPTION_STRLEN) {
      lln = MAX_DESCRIPTION_STRLEN;
    }
    strncpy(descriptions[descriptionLineCount], line, lln);
    descriptions[descriptionLineCount][lln] = 0;
    if (lln > dl) {
      dl = lln;
    }
    descriptionLineCount++;
    if (descriptionLineCount >= MAX_DESCRIPTION_LINE_COUNT) {
      break;
    }
    if (ni < 0) {
      break;
    }
    line += ll + 1;
  }
  descriptionX = (viewSizeX - dl * CHARACTER_WIDTH) / 2;
}

void initTitle() {
  state = STATE_TITLE;
  ticks = -1;
  resetDrawState();
  if (isReplayRecorded) {
    initParticle();
    resetDrawState();
    startReplay();
  }
}

void updateTitle() {
  clearView();
  Collision scratch;
  if (currentInput.isJustPressed) {
    initInGame();
    return;
  }
  if (isReplaying) {
    if (!replayInput()) {
      ticks = -1;
      startReplay();
    } else {
      currentUpdate();
      updateParticles();
    }
  }
  saveCurrentColorAndCharacterOptions();
  drawCharacters(currentTitle, (viewSizeX - strlen(currentTitle) * CHARACTER_WIDTH) / 2,
                 viewSizeY * 0.25, false, true, &scratch);
  if (ticks > 30) {
    for (int i = 0; i < descriptionLineCount; i++) {
      drawCharacters(descriptions[i], descriptionX,
                     currentOptions.viewSizeY * 0.55 + i * CHARACTER_HEIGHT, false,
                     true, &scratch);
    }
  }
  loadCurrentColorAndCharacterOptions();
}

// Game over
int gameOverTicks;
int[10] gameOverText = "GAME OVER";

void drawGameOver() {
  Collision scratch;
  drawCharacters(gameOverText,
                 (viewSizeX - strlen(gameOverText) * CHARACTER_WIDTH) / 2,
                 viewSizeY * 0.5, false, true, &scratch);
}

void initGameOver() {
  state = STATE_GAME_OVER;
  if (isReplaying) {
    stopReplay();
  } else {
    isPlayingBgm = false;
    prevScore = (int)score;
    int* gtitle = getGame(currentGameIndex)->title;
    int foundIndex = -1;
    int freeIndex = -1;
    for (int i = 0; i < gameCount; i++) {
      if (strlen(saveData.hiScores[i].title) == 0 && saveData.hiScores[i].hiScore == 0 && freeIndex == -1) {
        freeIndex = i;
      }
      if (strcmp(saveData.hiScores[i].title, gtitle) == 0) {
        foundIndex = i;
        break;
      }
    }
    int index;
    if (foundIndex > -1) {
      index = foundIndex;
    } else {
      index = freeIndex;
    }
    if (index > -1) {
      if (prevScore > saveData.hiScores[index].hiScore) {
        strncpy(saveData.hiScores[index].title, gtitle, 19);
        saveData.hiScores[index].hiScore = prevScore;
        if (onSaveData) {
          onSaveData();
        }
      }
    }
  }
  gameOverTicks = 0;
  saveCurrentColorAndCharacterOptions();
  drawGameOver();
  loadCurrentColorAndCharacterOptions();
}

void updateGameOver() {
  isInGameOver = true;
  if (gameOverTicks == 20) {
    saveCurrentColorAndCharacterOptions();
    drawGameOver();
    loadCurrentColorAndCharacterOptions();
  }
  if (gameOverTicks > 20 && currentInput.isJustPressed) {
    isInGameOver = false;
    initInGame();
  } else if (hasTitle && gameOverTicks > 120) {
    isInGameOver = false;
    initTitle();
  }
  gameOverTicks++;
}

// Transit to the game-over state.
void gameOver() { initGameOver(); }

void resetGame(int gameIndex) {
  currentGameIndex = gameIndex;
  Game* game = getGame(gameIndex);
  if (onResetGame) {
    onResetGame(game);
  }
  currentTitle = game->title;
  currentDescription = game->description;
  currentCharacters = game->characters;
  currentCharactersCount = game->charactersCount;
  currentOptions = game->options;
  viewSizeX = currentOptions.viewSizeX;
  viewSizeY = currentOptions.viewSizeY;
  currentUpdate = game->update;
  md_initView(viewSizeX, viewSizeY);
  initColor();
  initCharacter();
  initScore(currentTitle);
  initParticle();
  initSound(currentTitle, currentDescription, currentOptions.soundSeed);
  initReplay();
  parseDescription();
  setRandomSeedWithTime(&gameSeedRandom);
  int cc;
  if (currentOptions.isDarkColor) {
    cc = 0x10;
  } else {
    cc = 0xe0;
  }
  md_clearScreen(cc, cc, cc);
  resetDrawState();
  hasTitle = strlen(currentTitle) + strlen(currentDescription) > 0;
  if (hasTitle) {
    initTitle();
  } else {
    initInGame();
  }
  ticks = 0;
}

// Go to the game selection menu screen.
void goToMenu() {
  isShowingScore = false;
  isBgmEnabled = false;
  isInMenu = true;
  isInGameOver = false;
  resetGame(0);
}

// Restart another game specified by `gameIndex`.
void restartGame(int gameIndex) {
  isShowingScore = true;
  isBgmEnabled = true;
  isInMenu = false;
  resetGame(gameIndex);
}

// Initialize all games' highscore table (used for saving)
void initHiScores() {
  memset(saveData.hiScores, 0, sizeof(saveData.hiScores));
}

// Initialize all games and go to the game selection menu screen.
// This function must be called from the machine dependent setup step.
void initGame() {
  initHiScores();
  initInput();
  addMenu();
  addGames();
  if (gameCount == 2) {
    restartGame(1);
  } else {
    goToMenu();
  }
}

// Update game frames every 1/60 second.
// This function must be called from the machine dependent update step.
void updateFrame() {
  hitBoxesIndex = 0;
  gridResetFrame();
  difficulty = (float)ticks / 60 / FPS + 1;
  if (state == STATE_TITLE) {
    updateTitle();
    drawScore();
  } else if (state == STATE_IN_GAME) {
    updateInGame();
    drawScore();
  } else if (state == STATE_GAME_OVER) {
    updateGameOver();
  }
  updateSound();
  ticks++;
  if (currentInput.up.isPressed && currentInput.down.isPressed) {
    if (currentInput.a.isJustPressed) {
      goToMenu();
    }
    if (currentInput.b.isJustPressed) {
      toggleSound();
    }
  }
}
