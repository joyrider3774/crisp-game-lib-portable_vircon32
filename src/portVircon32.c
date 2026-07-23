// =============================================================================
// Vircon32 port layer: implements machineDependent.h for real Vircon32
// hardware, and provides the program's entry point (main).
// =============================================================================
// Video: Vircon32's GPU only knows how to draw TEXTURED regions - there is
// no "fill a rectangle with a flat color" primitive in video.h. Since
// crisp-game-lib-portable's whole visual style is flat-colored rectangles,
// this port embeds a single 1x1 white pixel texture (assets/white.png,
// converted to white.vtex - see the ROM definition and README) and draws
// every rectangle/character-pixel as that texture stretched with
// set_drawing_scale() and tinted with set_multiply_color(). This is the
// standard Vircon32 idiom for solid-color fills.
//
// Sound: the sound *engine* (sound.c) generates a stream of {freq, duration,
// when} tone events, calling md_playTone()/md_stopTone() to actually sound
// them - see machineDependent.h. Those are implemented below using
// libs/PlayNote (a small library vendored in libs/ - see
// libs/PlayNote/playnote.h), which plays arbitrary frequencies from one
// embedded waveform sample via the SPU's native per-channel speed control,
// with no per-frame envelope/LFO processing needed. sound.c itself is
// untouched: it has no idea what's playing its notes, it just keeps
// calling the same three machineDependent.h hooks it always did.
//
// Input: Vircon32 has no mouse, only a gamepad - mouse-using games from
// the original 42-game list use a d-pad-driven virtual cursor instead (see
// main() below).
// =============================================================================

#include "cglp.h"
#include "video.h"
#include "input.h"
#include "time.h"
#include "math.h"
#include "memcard.h"
#include "../libs/PlayNote/playnote.h"

int screenW = screen_width;
int screenH = screen_height;

// Skips the actual GPU output on FRAME_SKIP out of every (FRAME_SKIP+1)
// frames, while every other part of the frame - input, game logic,
// collision detection, particles, scoring, sound, replay recording -
// still runs at full rate every single frame. This is safe specifically
// because collision detection in cglp.c is done entirely through its own
// hitBoxes bookkeeping (see checkHitBox()/endAddingRects() in cglp.c), which
// happens *before* addRect()/drawCharacter() ever call down into
// md_drawRect()/md_drawCharacter() - so skipping only the GPU calls
// below never skips a hit test. 0 (default) draws every frame - raise it
// if a game's still CPU/GPU-bound after the character-rect-caching and
// view-clipping work elsewhere in this file.
#define FRAME_SKIP 0

int frameSkipCounter = 0;
bool shouldDrawThisFrame = true;

// Tracks the GPU's actual current GPU_MultiplyColor/GPU_DrawingScaleX/Y
// register values (see set_multiply_color()/set_drawing_scale() in
// video.h - each is a single `out` port write, and like any hardware
// register it holds its value until explicitly changed again, so this
// cache is valid to persist across draws and across frames, not just
// within one). drawSolidRect() and md_drawCharacter() below both skip
// the actual GPU call when the value they'd set is unchanged from last
// time - consecutive draws sharing a color is common (a run of same-
// colored HUD text, a wall of same-colored tiles, several rects from one
// character glyph that are often similarly sized), and each skipped
// call saves a full function call's overhead on top of the GPU write
// itself. Initialized to values no real draw call can produce, so the
// very first draw always goes through.
int lastGpuR = -1;
int lastGpuG = -1;
int lastGpuB = -1;
float lastGpuScaleX = -1.0;
float lastGpuScaleY = -1.0;

// Computed once per game by md_initView() to fit the game's logical view
// (e.g. 100x100 for Pin Climb) into the physical 640x360 screen, centered,
// preserving aspect ratio - same idea as the Tufty2350 port's md_initView.
float viewScale = 1.0;
int viewOffsetX = 0;
int viewOffsetY = 0;
int viewOrigW = 0;
int viewOrigH = 0;

// Vircon32 has no mouse, only a gamepad. A handful of the original 42
// games use a mouse (drag-style controls); for those, the d-pad instead
// moves a virtual cursor fed in through setMousePos() - the same idea
// the Tufty2350 port uses, which also has no mouse. mouseX/mouseY live
// in the same logical coordinate space the game draws in (not screen
// pixels). Global (not local to main()) so md_initView() below can
// reset them to the center of whatever view it just set up - see there.
float mouseX = 0.0;
float mouseY = 0.0;

// Vircon32's GPU has no scissor/clip rect at all (checked video.h - there
// is genuinely nothing like it), so any draw call that lands even
// partially outside the game's logical [0,viewOrigW]x[0,viewOrigH] view
// paints into the letterbox/border area of the physical screen. That
// border is only cleared once, at game start (md_clearScreen) - not
// every frame like the view itself (md_clearView) - so anything drawn
// out there accumulates as permanent garbage the moment a sprite,
// particle, or rect moves near/through a screen edge. This clamps a
// rectangle (in logical view coordinates) down to the visible box in
// place, returning false if nothing of it remains to draw.
bool clampRectToView(float* x, float* y, float* w, float* h) {
  float x2 = *x + *w;
  float y2 = *y + *h;
  if (*x < 0) {
    *x = 0;
  }
  if (*y < 0) {
    *y = 0;
  }
  if (x2 > viewOrigW) {
    x2 = viewOrigW;
  }
  if (y2 > viewOrigH) {
    y2 = viewOrigH;
  }
  *w = x2 - *x;
  *h = y2 - *y;
  return *w > 0 && *h > 0;
}

// Draws a solid-colored rectangle using the embedded 1x1 white texture,
// tinted and stretched to size. Shared by md_drawRect and md_clearView -
// clamping here (rather than in each caller) covers every draw path,
// including particle.c's md_drawRect() calls, which bypass cglp.c's own
// view-bounds check in addRect().
void drawSolidRect(float x, float y, float w, float h, int r, int g, int b) {
  if (!shouldDrawThisFrame) {
    return;
  }
  if (w < 0) {
    x += w;
    w = -w;
  }
  if (h < 0) {
    y += h;
    h = -h;
  }
  if (!clampRectToView(&x, &y, &w, &h)) {
    return;
  }
  if (r != lastGpuR || g != lastGpuG || b != lastGpuB) {
    set_multiply_color(make_color_rgb(r, g, b));
    lastGpuR = r;
    lastGpuG = g;
    lastGpuB = b;
  }
  float scaleX = w * viewScale;
  float scaleY = h * viewScale;
  if (scaleX != lastGpuScaleX || scaleY != lastGpuScaleY) {
    set_drawing_scale(scaleX, scaleY);
    lastGpuScaleX = scaleX;
    lastGpuScaleY = scaleY;
  }
  draw_region_zoomed_at((int)(viewOffsetX + x * viewScale), (int)(viewOffsetY + y * viewScale));
}

void md_drawRect(float x, float y, float w, float h, int r, int g, int b) {
  drawSolidRect(x, y, w, h, r, g, b);
}

void md_drawCharacter(CharRect* rects, int rectCount, float x, float y, int hash) {
  // hash is part of the machineDependent.h interface (a port could use it
  // to key its own cache) but this port doesn't need a second cache on
  // top of cglp.c's own hash-keyed CharacterPattern cache - the self-
  // assignment below is just this compiler's idiom for "intentionally
  // unused parameter" ((void)hash; isn't accepted here).
  hash = hash;
  if (!shouldDrawThisFrame) {
    return;
  }
  // Each rect's screen-space box is computed from its logical CORNERS
  // (lx1,ly1)-(lx2,ly2), not from a position+size multiplication, so
  // that two rects sharing a logical boundary (e.g. a red run right next
  // to a blue run in the same character) always compute the exact same
  // screen coordinate at that shared edge - no hairline gap and no
  // overlap, regardless of how viewScale rounds. Clamping the logical
  // corners to the view bounds first (same idea as clampRectToView())
  // means an edge-of-view rect is trimmed rather than overdrawn, with no
  // separate padding/shrinking pass needed.
  for (int i = 0; i < rectCount; i++) {
    CharRect* cr = &rects[i];
    float lx1 = x + cr->x;
    float ly1 = y + cr->y;
    float lx2 = lx1 + cr->w;
    float ly2 = ly1 + cr->h;
    if (lx1 < 0) {
      lx1 = 0;
    }
    if (ly1 < 0) {
      ly1 = 0;
    }
    if (lx2 > viewOrigW) {
      lx2 = viewOrigW;
    }
    if (ly2 > viewOrigH) {
      ly2 = viewOrigH;
    }
    if (lx2 <= lx1 || ly2 <= ly1) {
      continue;
    }
    int screenX1 = (int)(viewOffsetX + lx1 * viewScale);
    int screenY1 = (int)(viewOffsetY + ly1 * viewScale);
    int sizeX = (int)(viewOffsetX + lx2 * viewScale) - screenX1;
    int sizeY = (int)(viewOffsetY + ly2 * viewScale) - screenY1;
    if (sizeX <= 0 || sizeY <= 0) {
      continue;
    }
    if (cr->r != lastGpuR || cr->g != lastGpuG || cr->b != lastGpuB) {
      set_multiply_color(make_color_rgb(cr->r, cr->g, cr->b));
      lastGpuR = cr->r;
      lastGpuG = cr->g;
      lastGpuB = cr->b;
    }
    if ((float)sizeX != lastGpuScaleX || (float)sizeY != lastGpuScaleY) {
      set_drawing_scale(sizeX, sizeY);
      lastGpuScaleX = sizeX;
      lastGpuScaleY = sizeY;
    }
    draw_region_zoomed_at(screenX1, screenY1);
  }
}

void md_clearView(int r, int g, int b) {
  drawSolidRect(0, 0, viewOrigW, viewOrigH, r, g, b);
}

void md_clearScreen(int r, int g, int b) {
  // The border/letterbox area is the whole physical screen, so this can
  // use the GPU's fast whole-screen clear command directly instead of
  // drawing a textured rect.
  clear_screen(make_color_rgb(r, g, b));
}

// =============================================================================
// Sound: driving libs/PlayNote against one embedded sawtooth wavetable
// =============================================================================
// sound.c generates arbitrary-Hz notes (not tempered MIDI notes) and can
// schedule several of them at once with a `when` slightly in the future
// (see playSoundEffect() in sound.c - a sound effect is a short burst of
// notes at staggered offsets). PlayNote has no built-in delayed-start
// scheduling of its own (see playnote.h), so md_playTone() drops each
// request into a small pending-tone queue; updateSynthTones() (called once
// a frame from main()) starts anything whose time has come via
// playnote_start(), tracks the channel it was given in a second
// "currently sounding" queue, and calls playnote_stop() once its duration
// has elapsed. Both queues are sized independently of sound.c's own
// note-array constants: 40 pending covers a full-length sound effect
// burst (sound.c caps a single effect at 32 notes) with room for BGM
// overlapping it, and 16 active matches Vircon32's actual SPU channel
// count - playnote_start() already returns -1 once every channel is
// busy, so tracking more here would just be dead bookkeeping.

// wt_saw.vsnd is cartridge sound asset index 0 - the only sound this ROM
// embeds at all, see rom.xml. Saw over sine specifically for its
// brighter, more percussive harmonic content - closer to a classic
// chiptune blip than a plain sine's very soft, muffled tone.
#define PLAYNOTE_SAW_SOUND_ID 0

#define MAX_PENDING_TONES 40
struct PendingTone {
  float freq;
  float duration;
  float startTime;
  bool active;
};
PendingTone[MAX_PENDING_TONES] pendingTones;

#define MAX_ACTIVE_TONES 16
struct ActiveTone {
  int channel;
  float endTime;
  bool active;
};
ActiveTone[MAX_ACTIVE_TONES] activeTones;

void md_playTone(float freq, float duration, float when) {
  for (int i = 0; i < MAX_PENDING_TONES; i++) {
    if (!pendingTones[i].active) {
      pendingTones[i].freq = freq;
      pendingTones[i].duration = duration;
      pendingTones[i].startTime = when;
      pendingTones[i].active = true;
      return;
    }
  }
  // Queue is full (every slot pending) - drop the note. sound.c has no
  // way to know a tone didn't play, same as it wouldn't on a system with
  // no free SPU voice.
}

void md_stopTone() {
  // Called from cglp.c's disableSound() - the player turned sound off,
  // so silence everything immediately: clear our own queues (otherwise
  // a still-pending tone would start moments later, ignoring the mute)
  // and hard-stop every channel.
  for (int i = 0; i < MAX_PENDING_TONES; i++) {
    pendingTones[i].active = false;
  }
  for (int i = 0; i < MAX_ACTIVE_TONES; i++) {
    activeTones[i].active = false;
  }
  playnote_stop_all();
}

float md_getAudioTime() {
  // sound.c uses this for its own scheduling math, and now also as the
  // clock the pending/active tone queues above compare against - must be
  // non-decreasing and track real time, which the frame counter gives
  // for free since this port runs at a fixed 60 FPS.
  return (float)get_frame_counter() / (float)FPS;
}

// Starts any pending tone whose time has arrived, and releases any
// active tone whose duration has elapsed. Called once a frame from
// main(), alongside the required playnote_update() - see the comment
// block above. Unlike the previous synth-based version of this file,
// there's no minimum-hold-floor needed here: PlayNote's own fade-out
// always starts from whatever volume a note actually reached by the
// time it's stopped (see playnote_stop() in playnote.h), so even a note
// shorter than one frame is briefly audible rather than silently doing
// nothing - it just doesn't get much louder than a click before fading
// back out, which is the correct, honest result for a genuinely
// sub-frame-length request.
void updateSynthTones() {
  float now = md_getAudioTime();
  for (int i = 0; i < MAX_PENDING_TONES; i++) {
    if (pendingTones[i].active && now >= pendingTones[i].startTime) {
      int channel = playnote_start(pendingTones[i].freq, 0.22);
      if (channel >= 0) {
        for (int j = 0; j < MAX_ACTIVE_TONES; j++) {
          if (!activeTones[j].active) {
            activeTones[j].channel = channel;
            activeTones[j].endTime = now + pendingTones[i].duration;
            activeTones[j].active = true;
            break;
          }
        }
      }
      pendingTones[i].active = false;
    }
  }
  for (int i = 0; i < MAX_ACTIVE_TONES; i++) {
    if (activeTones[i].active && now >= activeTones[i].endTime) {
      playnote_stop(activeTones[i].channel);
      activeTones[i].active = false;
    }
  }
}

void md_initView(int w, int h) {
  viewOrigW = w;
  viewOrigH = h;
  float scaleX = (float)screenW / w;
  float scaleY = (float)screenH / h;
  if (scaleY < scaleX) {
    viewScale = scaleY;
  } else {
    viewScale = scaleX;
  }
  int viewW = (int)floor((float)w * viewScale);
  int viewH = (int)floor((float)h * viewScale);
  viewOffsetX = (screenW - viewW) / 2;
  viewOffsetY = (screenH - viewH) / 2;

  // Every game gets a fresh view via this function whenever it (re)starts
  // - reset the virtual cursor to its center at the same time, so a
  // mouse-using game always begins with the cursor in a known, sensible
  // spot instead of wherever it was left in a previous game (or before
  // any game had run at all). Harmless to do unconditionally even for
  // the non-mouse games that call this too - the cursor is never drawn
  // or read for those, so resetting it costs nothing and there's no
  // "is this a mouse game" check convenient to make from here anyway.
  mouseX = w / 2.0;
  mouseY = h / 2.0;
}

// No text-output channel exists on real Vircon32 hardware (no serial/log
// device in the standard library), so this is a no-op. If you need to
// debug, temporarily route this through print_at() from video.h instead.
void md_consoleLog(int* msg) {
  msg = msg;
}

// =============================================================================
// Memory card save/load for high scores
// =============================================================================
// Standard Vircon32 pattern: a 20-word "signature" identifies which game
// a card slot belongs to (so this ROM never reads or clobbers another
// game's save data on the same card), followed by the actual save data
// written right after it. saveData (cglp's SaveData struct - a magic
// number, the per-game high score table, and a checksum) is exactly what
// gets read/written; magic+checksum catch a torn write or a card with
// unrelated data past the signature.
#define SAVE_MAGIC 0x44444444

game_signature saveCardSignature = "CGLPVIRCON32SAVE01";

int calcSaveChecksum() {
  int sum = 0;
  int* words = (int*)&saveData;
  int wordCount = sizeof(SaveData) / sizeof(int);
  // sum every word except the checksum field itself (always the last one)
  for (int i = 0; i < wordCount - 1; i++) {
    sum += words[i];
  }
  return sum;
}

void loadHighScoresFromCard() {
  if (!card_is_connected()) {
    return; // saveData already holds the initHiScores() defaults
  }
  if (!card_signature_matches(&saveCardSignature)) {
    return; // no save from this game on this card - keep defaults
  }
  card_read_data(&saveData, 20, sizeof(SaveData));
  if (saveData.magic != SAVE_MAGIC || saveData.crc != calcSaveChecksum()) {
    // corrupted or torn write - fall back to defaults
    initHiScores();
  }
}

void saveHighScoresToCard() {
  if (!card_is_connected()) {
    return;
  }
  saveData.magic = SAVE_MAGIC;
  saveData.crc = calcSaveChecksum();
  card_write_signature(&saveCardSignature);
  card_write_data(&saveData, 20, sizeof(SaveData));
}

#ifdef DEBUG_MODE
// Same GPU-call pattern as drawSolidRect() above (including its
// redundant-state-skip caching), but for coordinates that are already
// in physical screen pixels - drawSolidRect() itself can't be reused
// here, since it applies the game's logical-to-physical view transform
// (viewScale/viewOffsetX/Y), which would put this rect in the wrong
// place relative to print_at()'s coordinate system, which is physical
// pixels from the start. No shouldDrawThisFrame check, matching
// print_at() itself - see drawDebugOverlay() below for why.
void drawDebugPhysicalRect(int x, int y, int w, int h, int r, int g, int b) {
  if (r != lastGpuR || g != lastGpuG || b != lastGpuB) {
    set_multiply_color(make_color_rgb(r, g, b));
    lastGpuR = r;
    lastGpuG = g;
    lastGpuB = b;
  }
  float scaleX = (float)w;
  float scaleY = (float)h;
  if (scaleX != lastGpuScaleX || scaleY != lastGpuScaleY) {
    set_drawing_scale(scaleX, scaleY);
    lastGpuScaleX = scaleX;
    lastGpuScaleY = scaleY;
  }
  draw_region_zoomed_at(x, y);
}
#endif

#ifdef DEBUG_MODE
// Draws the performance debug overlay in the bottom-left corner: the
// collision-tracked object count this frame (hitBoxesIndex in cglp.c -
// every rect/character/text drawn with collision enabled registers one
// here, reset to 0 at the top of every updateFrame() - this is what the
// engine's O(n^2) collision scan scales with, so it's the most directly
// useful "how much is this frame actually dealing with" number, not a
// count of the game's own objects, which aren't uniformly accessible
// across all 42 games the way this engine-level count is) and the max
// seen since the last reset; cycles used this frame (and the max seen);
// instantaneous FPS (and the min seen); and the character-pattern
// cache's hit/miss counts.
//
// Uses print_at() (video.h) - Vircon32's own built-in BIOS-font text
// primitive - instead of the engine's text()/drawCharacters(), since a
// debug overlay that goes through the full engine draw pipeline (hash
// computation, character-pattern cache lookup, hitbox registration,
// Collision bookkeeping) would inflate the very cycle/object counts
// it's trying to measure. print_at() bypasses all of that - it draws
// straight from the console's built-in font texture, with no knowledge
// of cglp.c's collision/cache machinery at all - which also means it
// never checks shouldDrawThisFrame the way md_drawCharacter() does, so
// unlike everything else in this file, this needs no special handling
// to keep drawing through a frame-skipped or menu-idle-skipped frame:
// it always draws, automatically.
//
// Two things print_at() does NOT do that the engine's own draw
// functions handle automatically, so this does them by hand:
//  - It doesn't set a draw color - text comes out in whatever
//    multiply_color was last set to, so this sets it explicitly, then
//    updates portVircon32.c's own lastGpuR/G/B cache (see
//    drawSolidRect() above) to match. Without that second step, the
//    *next* normal engine draw call could wrongly think the GPU's color
//    register already matches what it wants (since, as far as that
//    cache knows, nothing changed it) and skip re-asserting its own
//    color - drawing in yellow by mistake.
//  - It draws in absolute physical screen pixels (screenW/screenH),
//    not the engine's scaled, letterboxed game-logical coordinates -
//    since it has no concept of a game's view at all - and at the BIOS
//    font's fixed native size (bios_character_width/height, 10x20),
//    unaffected by set_drawing_scale() either way, so there's no
//    equivalent scale-cache concern to handle.
//  - Critically, it also never gets cleared: cglp.c's clearView() only
//    clears the game's own logical view, which is scaled/letterboxed
//    within the physical screen - it doesn't touch the letterbox
//    border, and depending on a game's view size and where this
//    overlay's physical coordinates happen to fall, that's often
//    exactly where they land (see clampRectToView()'s comment above,
//    which notes the border is only ever cleared once, at game start,
//    not per frame like the view itself). Without explicitly clearing
//    it here, old digits stay on screen underneath new ones the moment
//    a number gets shorter (99 -> 9 would show a stray "9" from the old
//    "99"). drawDebugPhysicalRect() below paints a plain black backing
//    rect across the whole overlay first, every frame, before any text
//    goes on top of it.
void drawDebugOverlay(int cyclesThisFrame, int maxCycles, float instantFps, float minFps, int objCount, int maxObjCount) {
  int lineX = 4;
  int lineY = screenH - 4 * bios_character_height - 4;

  // Sized generously wide (200px, ~20 BIOS characters) to comfortably
  // fit the longest realistic line ("CACHE 99999/99999" is 18) with
  // margin either side, tall enough for all 4 lines plus a couple of
  // pixels of padding top and bottom.
  drawDebugPhysicalRect(lineX - 2, lineY - 2, 200, 4 * bios_character_height + 4, 0, 0, 0);

  set_multiply_color(make_color_rgb(255, 255, 0));
  lastGpuR = 255;
  lastGpuG = 255;
  lastGpuB = 0;

  int[32] line0;
  strcpy(line0, "OBJ ");
  strcat(line0, intToChar(objCount));
  strcat(line0, "/");
  strcat(line0, intToChar(maxObjCount));
  print_at(lineX, lineY, line0);

  int[32] line1;
  strcpy(line1, "CYC ");
  strcat(line1, intToChar(cyclesThisFrame));
  strcat(line1, "/");
  strcat(line1, intToChar(maxCycles));
  print_at(lineX, lineY + bios_character_height, line1);

  int[32] line2;
  strcpy(line2, "FPS ");
  strcat(line2, intToChar((int)instantFps));
  strcat(line2, "/");
  strcat(line2, intToChar((int)minFps));
  print_at(lineX, lineY + 2 * bios_character_height, line2);

  int[32] line3;
  strcpy(line3, "CACHE ");
  strcat(line3, intToChar(debugCacheHits));
  strcat(line3, "/");
  strcat(line3, intToChar(debugCacheMisses));
  print_at(lineX, lineY + 3 * bios_character_height, line3);
}
#endif

void main() {
  // The embedded 1x1 white texture (region 0 of texture 0) is used for
  // every rectangle/pixel draw in the whole program, so it only needs to
  // be selected once, up front - see drawSolidRect() above.
  select_texture(0);
  select_region(0);
  define_region_topleft(0, 0, 0, 0);

  // wave_period=2048 matches libs/PlayNote/sounds/wt_saw.wav (see
  // libs/PlayNote/README.md for how that number is derived from the
  // wav file itself, if you swap in a different one). Must happen
  // before initGame(), since a game with BGM can start sounding notes
  // as soon as it does.
  playnote_init(PLAYNOTE_SAW_SOUND_ID, 2048);

  // initGame() resets saveData to defaults (initHiScores()); load any
  // existing card save right after so it can override those defaults,
  // and hook onSaveData so cglp.c writes a new high score back to the
  // card the moment one is set (see initGameOver() in cglp.c).
  onSaveData = &saveHighScoresToCard;
  initGame();
  loadHighScoresFromCard();

  // Edge-detects the START button ourselves - cglp's setButtonState()
  // has no concept of a "start" button (the original games never used
  // one), so goToMenu() is triggered directly here instead.
  bool prevStartPressed = false;

  // The menu (menuUpdate() in menu.c) has no continuous animation at all -
  // its entire visible output (game list, cursor, page indicator) is a
  // pure function of gameIndex, which only ever changes in response to
  // the down/up/b/a buttons. So on any frame where none of those four are
  // currently held, the menu's output is *guaranteed* identical to last
  // frame - see the menu-idle check below, which uses this to skip both
  // the clear and the redraw entirely on those frames (not just the
  // redraw - both go through the same shouldDrawThisFrame gate, so
  // skipping it skips clearView() too, which is exactly what's wanted:
  // nothing to clear if nothing's being redrawn over it).
  bool hasMenuBeenDrawnThisVisit = false;

#ifdef DEBUG_MODE
  int debugMaxCycles = 0;
  float debugMinFps = 999.0;
  // FPS for the frame currently being drawn can't be known until after
  // this iteration's end_frame() wait completes - see where it's
  // measured below - so this always displays the *previous* frame's
  // measured value while the current one is in progress, same as any
  // real-time FPS counter necessarily lags by one frame.
  float debugInstantFps = 60.0;
  int debugPrevFrameCounter = get_frame_counter();
  int debugMaxObjCount = 0;
  bool prevYPressed = false;
#endif


  while (true) {
    bool left = gamepad_left() > 0;
    bool right = gamepad_right() > 0;
    bool up = gamepad_up() > 0;
    bool down = gamepad_down() > 0;
    bool a = gamepad_button_a() > 0;
    bool b = gamepad_button_b() > 0;
    bool startPressed = gamepad_button_start() > 0;

    if (startPressed && !prevStartPressed && !isInMenu) {
      goToMenu();
    }
    prevStartPressed = startPressed;

#ifdef DEBUG_MODE
    bool yPressed = gamepad_button_y() > 0;
    if (yPressed && !prevYPressed) {
      debugMaxCycles = 0;
      debugMinFps = 999.0;
      debugCacheHits = 0;
      debugCacheMisses = 0;
      debugMaxObjCount = 0;
    }
    prevYPressed = yPressed;
#endif

    bool usesMouse = getGame(currentGameIndex)->usesMouse;

    if (usesMouse) {
      float cursorSpeed = 1.5;
      if (left) {
        mouseX -= cursorSpeed;
      }
      if (right) {
        mouseX += cursorSpeed;
      }
      if (up) {
        mouseY -= cursorSpeed;
      }
      if (down) {
        mouseY += cursorSpeed;
      }
      mouseX = clamp(mouseX, 0.0, (float)viewOrigW);
      mouseY = clamp(mouseY, 0.0, (float)viewOrigH);
      setMousePos(mouseX, mouseY);
      setButtonState(false, false, false, false, b, a);
    } else {
      setButtonState(left, right, up, down, b, a);
    }

    bool isMenuScreen = (currentGameIndex == 0);
    if (!isMenuScreen) {
      // Not in the menu - reset so the *next* time we enter it, the
      // first frame is forced to draw rather than possibly being
      // skipped if no button happens to be held at that exact moment.
      hasMenuBeenDrawnThisVisit = false;
    }
    bool anyMenuNavButtonHeld = down || up || b || a;
    bool menuIsIdleThisFrame = isMenuScreen && hasMenuBeenDrawnThisVisit && !anyMenuNavButtonHeld;

    shouldDrawThisFrame = (frameSkipCounter == 0) && !menuIsIdleThisFrame;
    frameSkipCounter++;
    if (frameSkipCounter > FRAME_SKIP) {
      frameSkipCounter = 0;
    }
    if (isMenuScreen && shouldDrawThisFrame) {
      hasMenuBeenDrawnThisVisit = true;
    }

    updateFrame();

    // Vircon32 has no real mouse, so the 8 mouse-driven games can't show
    // a real cursor either - this draws a synthetic one, matching the
    // Tufty2350 port's cursor design exactly (same hot-pink "+" shape,
    // same two-rect layout, same "hide it during game over" rule). It's
    // drawn straight to the GPU via drawSolidRect() - not through
    // cglp.c's rect()/box(), which would register a hitbox and make the
    // cursor itself collide with the game - and *after* updateFrame(),
    // so it always ends up on top of whatever the game just drew.
    if (usesMouse && !isInGameOver) {
      drawSolidRect(mouseX - 3, mouseY - 1, 7, 3, 255, 105, 180);
      drawSolidRect(mouseX - 1, mouseY - 3, 3, 7, 255, 105, 180);
    }

    // updateSynthTones() dispatches anything md_playTone() queued that's
    // now due; playnote_update() advances any in-progress fade in/out
    // (see playnote.h) - order between the two doesn't matter, but both
    // must run once every frame.
    updateSynthTones();
    playnote_update();

#ifdef DEBUG_MODE
    int debugCyclesThisFrame = get_cycle_counter();
    if (debugCyclesThisFrame > debugMaxCycles) {
      debugMaxCycles = debugCyclesThisFrame;
    }
    if (hitBoxesIndex > debugMaxObjCount) {
      debugMaxObjCount = hitBoxesIndex;
    }
    drawDebugOverlay(debugCyclesThisFrame, debugMaxCycles, debugInstantFps, debugMinFps, hitBoxesIndex, debugMaxObjCount);
#endif

    end_frame();

#ifdef DEBUG_MODE
    // Only measurable *after* the wait completes: if this iteration's
    // work took longer than one frame's budget, end_frame() had to wait
    // for more than one vsync, and get_frame_counter() will have
    // advanced by more than 1 - directly reflecting a real, missed
    // frame rather than an assumed/fixed 60 FPS.
    int debugNewFrameCounter = get_frame_counter();
    int debugFrameDelta = debugNewFrameCounter - debugPrevFrameCounter;
    if (debugFrameDelta < 1) {
      debugFrameDelta = 1;
    }
    debugInstantFps = 60.0 / debugFrameDelta;
    if (debugInstantFps < debugMinFps) {
      debugMinFps = debugInstantFps;
    }
    debugPrevFrameCounter = debugNewFrameCounter;
#endif
  }
}
