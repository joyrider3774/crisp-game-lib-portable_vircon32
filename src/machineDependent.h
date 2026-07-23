#ifndef MACHINE_DEPENDENT_H
#define MACHINE_DEPENDENT_H

// -----------------------------------------------------------------------------
// Vircon32 port notes:
//  - Vircon32 C has no linker (single translation unit); "extern" there only
//    works for variable declarations, not function prototypes, so plain
//    forward declarations are used here. The actual bodies live in
//    portVircon32.c and are pulled in later by #include.
//  - "unsigned char" does not exist in this dialect (only int/float/bool/void,
//    all one word) so all r/g/b color components are passed as plain int
//    (0-255), same numeric range as before, just 4 bytes instead of 1.
// -----------------------------------------------------------------------------

#define CHARACTER_WIDTH 6
#define CHARACTER_HEIGHT 6

// Uncomment to enable the performance debug overlay: cycles used this
// frame (and the max seen), instantaneous FPS (and the min seen), and
// the character-pattern cache's hit/miss counts - drawn every frame in
// the corner of the screen (see drawDebugOverlay() in portVircon32.c).
// Press Y to reset the max-cycles/min-FPS/cache-count tracking back to
// a fresh baseline. Fully compiled out (zero cost, zero code) when this
// isn't defined - every use of it below is behind #ifdef DEBUG_MODE.
// #define DEBUG_MODE

#ifdef DEBUG_MODE
// Incremented in drawCharacter() (cglp.c) - defined there, not here,
// since Vircon32 has no linker (single translation unit): an "extern"
// declaration just needs the real definition to appear somewhere later
// in the same file, which cglp.c's is (see main.c's include order).
extern int debugCacheHits;
extern int debugCacheMisses;
#endif

// A single merged run of same-colored pixels within a character's 6x6
// grid, in character-local pixel units (see buildCharRects() in cglp.c).
// Drawing a handful of these instead of up to 36 individual pixels is
// the caching win described in cglp.c's comment above CharacterPattern -
// Vircon32 has no way to cache an actual rendered bitmap (no render-to-
// texture capability at all), so this is the next best thing: the
// merging itself only happens once per unique glyph+color+orientation
// (cached by hash, same as before), not every frame.
struct CharRect {
  int x, y, w, h;
  int r, g, b;
};

void md_drawRect(float x, float y, float w, float h, int r, int g, int b);
void md_drawCharacter(CharRect* rects, int rectCount, float x, float y, int hash);
void md_clearView(int r, int g, int b);
void md_clearScreen(int r, int g, int b);
void md_playTone(float freq, float duration, float when);
void md_stopTone();
float md_getAudioTime();
void md_initView(int w, int h);
void md_consoleLog(int* msg);

#endif
