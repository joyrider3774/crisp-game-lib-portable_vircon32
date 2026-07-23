# Porting notes

Technical detail on how this port is put together, for anyone extending
it, fixing something, or porting a similar engine to Vircon32. For the
Vircon32 C dialect itself (what compiles, what doesn't, and why), see
[VIRCON32_C_DIALECT.md](VIRCON32_C_DIALECT.md) instead - this file
covers this project's own architecture, not the language.

## Layout

```
src/
  machineDependent.h    - the per-port interface (md_* functions)
  vector.h/.c            - 2D vector math (unchanged in spirit)
  random.h/.c             - xorshift RNG
  particle.h/.c           - particle effects
  textPattern.h/.c         - built-in 6x6 font glyph data (94 patterns)
  sound.h/.c                - procedural sound effect / bgm generator
                              (drives real audio via libs/PlayNote - see
                              "Sound" below)
  cglp.h/.c                   - the core engine (draw calls, collision,
                                 game state machine, menu plumbing)
  menu.h/.c                    - game-selection menu screen
  menuGameList.h/.c             - registers all 42 ported games
  games/
    gamePinClimb.c               - read this first - it's commented as
                                    the porting example the rest follow
    game*.c                       - the other 41 ported games (42 total)
  portVircon32.c                  - the actual Vircon32-specific bit: video/
                                     input implementation of machineDependent.h,
                                     mouse-cursor emulation, the audio backend,
                                     plus main()
  main.c                           - the single file passed to compile.exe;
                                     #includes everything above in order
                                     (Vircon32 has no linker - see
                                     VIRCON32_C_DIALECT.md)
libs/
  PlayNote/                          - small MIT-licensed library written
                                       for this port, plays the actual
                                       audio in portVircon32.c (see
                                       "Sound" below and its own README)
assets/
  white.png                         - a 1x1 white texture (see "Drawing solid
                                       rectangles" below)
obj/                                 - intermediate build artifacts (.asm,
                                       .vbin, .vtex, .vsnd) - created by
                                       Make.bat/Make.sh, not checked in
bin/
  crisp-game-lib.v32                  - the built ROM (ready to run)
rom.xml                               - ROM definition passed to packrom
Make.bat / Make.sh                    - build everything from source
Run.bat / Run.sh                      - launch the built ROM in the
                                         Vircon32 emulator (edit the
                                         emulator path at the top first)
```

## The `Collision`-by-value problem

crisp-game-lib's drawing API (`rect()`, `box()`, `line()`, `bar()`,
`arc()`, `text()`, `character()`) all returned a `Collision` struct by
value - a ~270-word object, far over Vircon32's one-word function-return
limit (see `VIRCON32_C_DIALECT.md` §5/§17.2). Every one of these was
reworked to an out-pointer form, and every call site across all 42 games
converted by hand:

```c
// before (doesn't compile on Vircon32 - Collision is way over 1 word)
Collision c = rect(x, y, w, h);
if (c.isColliding.rect[BLUE]) { ... }

// after
Collision c;
rect(x, y, w, h, &c);
if (c.isColliding.rect[BLUE]) { ... }
```

Several games also had their own by-value struct/`Vector` parameters
(`gameSurvivor.c`'s `addDownedPlayer(Vector pos, ...)`, `gameCardQ.c`'s
`addPlacedCard(Card card, ...)`, etc.) - converted to pointer parameters
the same way.

## Drawing solid rectangles

Vircon32's `video.h` has no "fill a rectangle with a flat color"
primitive - the GPU only draws textured regions. `portVircon32.c` embeds
a single 1x1 white pixel texture (`assets/white.png` -> `white.vtex`,
converted with `png2vircon`) and draws every rectangle as that texture,
tinted with `set_multiply_color()` and stretched with
`set_drawing_scale()` + `draw_region_zoomed_at()` - the standard idiom
for solid-color fills on this console.

## Sound

`sound.c` (the procedural sound-effect/BGM generator) is ported and
compiled in full for every game - it schedules notes, tracks timing,
etc., unmodified. The machine-dependent hooks it calls
(`md_playTone`/`md_stopTone`/`md_getAudioTime` in `portVircon32.c`)
drive real audio via [libs/PlayNote](libs/PlayNote/), a small
general-purpose library written for this port (see its own README for
the full API). It plays arbitrary frequencies from one embedded
single-cycle waveform using the SPU's native per-channel
`set_channel_speed()` control, entirely in hardware once a note starts
- no per-frame envelope/LFO/arpeggiator processing, since crisp-game-lib
never needs those features (every note here is a short blip or melody
note). The only per-frame cost is a short linear volume ramp at the
start/end of each note, purely to avoid the click an abrupt digital stop
can cause.

sound.c generates arbitrary-Hz notes with a `when` that can be slightly
in the future (a sound effect is a short burst of staggered notes);
`md_playTone()` drops each request into a small pending-tone queue in
`portVircon32.c`, and a per-frame dispatcher starts/stops notes via
`playnote_start()`/`playnote_stop()` once their scheduled time arrives.
Everything audio-related is confined to `portVircon32.c` -
`sound.c`/`cglp.c` have no idea what's actually playing their notes.

An earlier version of this port used a full wavetable synth library
instead, before being replaced with PlayNote - disassembling both for an
identical note sequence showed the full synth costing roughly 1.85x the
instructions per frame, dominated by an unconditional per-frame
scheduler tick and feature-flag checks for envelope/LFO/arpeggiator
features this port never uses. See `libs/PlayNote/README.md` for the
full API and its wavetable's own attribution.

## Input and the mouse games

Vircon32 has a gamepad, not a mouse. `main()` in `portVircon32.c` maps
d-pad + A/B directly via `input.h` for normal games.

For the games that originally used mouse-drag controls (`gameBBlast`,
`gameBalance`, `gameBreedC`, `gameCardQ`, `gameCNodes`, `gameCrossLine`,
`gameDarkCave`, `gameDFight`), `main()` instead moves a virtual cursor
with the d-pad and feeds it through `setMousePos()` every frame. The
cursor resets to the center of a game's own view every time that view
is (re)created (`md_initView()`), and is clamped to that same view's
actual bounds rather than a fixed margin - both tied to `viewOrigW`/
`viewOrigH`, which `md_initView()` already tracks per game. There's no
visible cursor sprite drawn, matching the original mouse-driven games,
which are driven by `input.pos`/`input.isPressed` directly rather than a
rendered pointer.

## Collision detection

`checkHitBox()` (in `cglp.c`) is the core of every collision check in
every game - `rect()`, `box()`, `character()`, `text()` all funnel
through it. It answers "does this new hitbox overlap anything already
drawn so far this frame" - which a game's own `update()` code uses to
know "did I just hit something," checked *immediately*, synchronously,
with real side effects (`gameOver()`, `isAlive = false`, `addScore()`)
that can affect the rest of that same frame's logic. That's load-bearing,
not incidental - the engine uses draw order itself as part of how it
decides what collided with what (draw the player before the enemies, so
each enemy's check can see the player but not other enemies drawn later
the same frame). That rules out batching every check to the end of a
frame, not just as an optimization but as a correctness requirement.

What *is* safe to change is *how fast* each check's search runs, without
changing *when* it runs or *what* it means. Naively, checking a new
hitbox means scanning every hitbox already registered this frame -
genuinely O(n²) across a whole frame, and some games reach 400-1580
hitboxes in a single frame. `checkHitBox()` now searches through a small
spatial grid (`gridCellCount`/`gridCellIndices` in `cglp.c`) instead of
the flat `hitBoxes[]` array directly: a new hitbox only scans the
handful of other hitboxes registered in its own nearby cells. Correctness
rests on one fact: if two hitboxes' bounding boxes genuinely overlap,
they're guaranteed to share at least one grid cell, since
`gridRegister()` inserts each hitbox into every cell its full extent
touches - so the grid can only filter out cells that couldn't possibly
contain a real overlap, never cause a missed one. If any single cell
would ever need to hold more than `GRID_CELL_CAPACITY` hitboxes -
possible under sufficiently extreme clustering - the grid transparently
falls back to the exact original linear scan for the rest of that frame,
rather than risk a wrong result from an undersized cell: a pathological
frame degrades to no speedup, never to a wrong result.

The grid's cell size and capacity were both tuned empirically (swept
against a native, off-console test harness modeling realistic object-size
distributions and deliberately clustered stress scenarios) rather than
guessed - see the constants' own comments directly above their
`#define`s in `cglp.c` for the full tuning story and numbers, and
`VIRCON32_C_DIALECT.md` §17.7 for the general pattern this follows.
Before replacing the original linear scan, this was checked against it
across random, boundary-aligned, multi-cell-spanning, and forced-overflow
scenarios with zero mismatches, and measured at roughly 60x fewer
geometry comparisons for a realistic busy frame in that same harness.

## Debug mode

Uncomment `#define DEBUG_MODE` near the top of `src/machineDependent.h`
to enable a small performance overlay, drawn in the bottom-left corner of
every game:

```
OBJ 87/412
CYC 4213/9871
FPS 60/47
CACHE 812/23
```

- **OBJ** - `hitBoxesIndex` (see `cglp.c`): the number of collision-
  tracked objects registered so far this frame, and the highest value
  seen since the last reset. This is what `checkHitBox()`'s search
  scales with (see "Collision detection" above), so it's the most
  useful single number for spotting *why* a frame got expensive. Not a
  count of a game's own objects (enemies, bullets, etc.) - those live in
  differently-named arrays per game, with no single variable that means
  the same thing across all 42 the way this engine-level count does.
- **CYC** - `get_cycle_counter()`'s reading for the current frame, and
  the highest seen since the last reset. Vircon32's own docs note
  emulators make no timing guarantees for cycles *within* a frame, so
  treat this as a relative "is this getting worse" signal.
- **FPS** - actual instantaneous frame rate, measured by comparing
  `get_frame_counter()` immediately before/after each `end_frame()`
  wait, so it reflects a real missed vsync rather than an assumed
  constant 60. Lags by one frame, same as any real-time FPS counter
  necessarily does - and the lowest value seen since the last reset.
- **CACHE** - the character-pattern cache's (`cglp.c`) hit/miss counts
  since the last reset.

Drawn with `print_at()` (Vircon32's built-in BIOS-font text primitive)
rather than the engine's own `text()`, since routing the overlay through
the full engine draw pipeline would inflate the very cycle/object counts
it exists to measure - see `VIRCON32_C_DIALECT.md` §17.5 for what that
requires handling by hand (color, screen-space coordinates, and clearing
its own background every frame, since `print_at()` bypasses
`clearView()` too).

Fully compiled out (zero cost, zero code) when `DEBUG_MODE` isn't
defined - every part of it is behind `#ifdef DEBUG_MODE`.

## Porting an additional game into this project

Every game follows the same recipe, documented with a worked example at
the top of `src/games/gamePinClimb.c`:

1. Prefix every file-scope name (title/description/characters/
   charactersCount/options/update, plus any per-game helper types/
   functions/globals) with a short unique tag - required since Vircon32
   has no linker and every included game shares one namespace (see
   `VIRCON32_C_DIALECT.md` §11/§17.1).
2. Convert `typedef struct {...} Foo;` / `typedef enum {...} Foo;` to
   `struct Foo {...};` / `enum Foo {...};`.
3. Convert designated initializers to positional, in declared field
   order.
4. Convert every `rect()`/`box()`/`line()`/`bar()`/`arc()`/`text()`/
   `character()` call to the out-pointer form (see "The `Collision`-by-
   value problem" above).
5. Fix array declarations to `type[N] name` order, remove ternaries/
   comma operators/compound literals/switch statements, convert any
   by-value struct or `Vector` function parameters to pointers.
6. Add `#include "games/gameXxx.c"` to `main.c`, and register it (forward
   declare + call `addGameXxx()`) in `menuGameList.c`.
7. `./Make.sh` and fix whatever the real compiler complains about - see
   `VIRCON32_C_DIALECT.md` for what's likely to come up.
