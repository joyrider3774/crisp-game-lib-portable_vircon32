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

## v32opt - integrated, using only individually-verified-safe passes

[v32opt](https://github.com/wedge1020/v32opt) is a third-party,
optional post-compile Vircon32 assembly optimizer (not part of the
official toolchain). `Make.sh`/`Make.bat` run it - if it's found on
`PATH` - between `compile` and `assemble`, using six of its nine passes
individually. Skipped automatically if it's not installed, or if
`SKIP_V32OPT=1` is set (`SKIP_V32OPT=1 ./Make.sh`, or `set
SKIP_V32OPT=1` before `Make.bat`) - either way the build falls back to
the unoptimized assembly from `compile`, same as if this step didn't
exist. Nothing else in the build depends on it.

**Deliberately not using `-O1`/`-O2`/`-O3` at all** - every preset
level bundles at least one pass with a confirmed correctness bug, found
by building v32opt from source and testing it against this project's
own real compiled output, not just reading its documentation:

- **`-fopt_inline`** (also pulled in by `-O3`) - an inlined function
  body keeps its original parameter references (`[BP+2]`-style stack-
  frame offsets) unchanged, but those offsets were only valid relative
  to the callee's *own* prologue-established `BP`. Once the body is
  copied into the caller with no new prologue, `BP` still points at the
  caller's frame, so the inlined code reads whatever happens to occupy
  that unrelated offset in the caller's own stack instead of the
  argument actually passed in. Traced two independent call sites (one
  passing a local variable, one a global) through the transformation -
  both showed the identical wrong pattern, not a one-off.
- **`-fopt_strength_reduction`** (pulled in by `-O1` and up) - emits a
  bare `SHR` mnemonic. Vircon32 doesn't have a right-shift instruction
  (only `SHL` is real, confirmed against `VIRCON32_C_DIALECT.md`'s own
  hardware instruction table) - the real assembler rejects it outright
  with a parser error. This "optimization" would also be pointless even
  if it worked: `IDIV` is already 1 cycle on this hardware (see §16.3),
  identical cost to a shift.
- **`-fopt_dce`** (pulled in by `-O2` and up) - a false positive in its
  reachability analysis deleted `select_texture`, a function this
  project genuinely calls, and the assembler then failed with "label
  was not declared."
- **`-fopt_constant_folding`** (pulled in by `-O2` and up) - the most
  dangerous of the four, since it produces no error at all. Its
  cross-block constant tracking leaks across function boundaries and
  function calls: traced one transformed site and found `mov R1, R0`,
  sitting at the very start of a freshly-entered function immediately
  after that function's own first `call` (to `pow()`, mid-formula in a
  MIDI-note-to-frequency conversion), had been folded to a hardcoded
  `MOV R1, 0x0`. R0 holds `pow()`'s real return value there - nothing
  in that function's own code ever set it to 0. The tool silently
  replaces a real computed value with a stale, unrelated constant left
  over from somewhere else entirely. Compiles and assembles fine either
  way; only tracing the actual instructions catches it.

**What's actually enabled** - `-fopt_peephole`, `-fopt_algebraic`,
`-fopt_forwarding`, `-fopt_jump_next`, `-fopt_redundant_movs`,
`-fopt_combine_immediates`, passed individually. Algebraic, forwarding,
and combine_immediates were each verified the same way as the broken
ones above - run against real compiled output, then a specific
transformed call site traced by hand and confirmed mathematically
correct (`IMUL R,2` → `IADD R,R`; a redundant post-store memory read
forwarded to the register that was just written; two adjacent
`IADD`/`ISUB` immediates merged into one). Peephole, jump_next, and
redundant_movs didn't happen to trigger on this project's own compiled
output, so they're unverified by tracing specifically - but they're
simple, local, single-pattern passes, structurally unlike any of the
four broken ones (none of them move code across a function boundary or
track state across blocks, which is where all four confirmed bugs
live).

If revisiting this later (e.g. after an upstream fix to one of the
broken passes), the same discipline applies: build v32opt from source,
run it against a real compile, and trace actual transformed call sites
by hand rather than trusting that a clean compile/assemble/pack means
the output is correct - three of these four bugs produced no error
anywhere in the toolchain and would have shipped silently.

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

## Replay determinism (particleRandom, soundEffectPlayedTimes, particles[], tone queues, score, input, character-pattern cache)

`startReplay()` (`cglp.c`) resets `gameRandom` to a fixed, recorded seed
every time a replay (re)starts, making the game's own logic - the part
that actually matters for reproducing a recorded high-score run -
correctly deterministic across every loop. Seven other pieces of state
don't get the same treatment by default, and all seven were found the
same way: by actually running the replay repeatedly and noticing the
debug overlay's `AVG`/`PREV` cycle counts (see below) didn't match
between loops of what should have been the exact same, fully
deterministic playback - isolating candidates one at a time (e.g.
setting `isSoundEnabled = false`, and separately hard-coding
`setRandomSeedWithTime()` to always return a fixed seed, to rule
sound and wall-clock-seeded randomness in or out entirely), then, once
isolating individual candidates stopped turning up new ones, a
systematic sweep of every global variable both `cglp.c` and the game
being tested with (`gamePakuPaku.c`) declare, cross-referenced against
what `startReplay()` actually resets - rather than continuing to guess
one candidate at a time indefinitely.

**`particleRandom`** (`particle.c`) is normally seeded from wall-clock
time (`initParticle()` -> `setRandomSeedWithTime()`), which is fine for
real gameplay - particles looking a little different each time you
play is desirable, not a bug - but `initParticle()` only runs once,
when the title screen is first *entered*, not on every replay loop
restart. Left alone, that means `particleRandom` keeps advancing
continuously across every loop of the same replay, never resetting, so
particle spawn behavior - and therefore how much update/draw work each
frame actually does - differs between consecutive loops even though
the recorded input and `gameRandom` are both identical every time.

**`soundEffectPlayedTimes`** (`sound.c`) is a related case one level
deeper. `playSoundEffect()` compares against `md_getAudioTime()` -
`get_frame_counter()/FPS`, the console's global, ever-increasing frame
count since boot, never reset per game or per replay loop - to decide
whether a given sound-effect request actually plays or gets silently
deduplicated as "too soon after the last one of this type". That
threshold array is only zeroed once, in `initSound()` at the start of
the whole game session, not on every loop restart. Left alone, a
threshold value carried over from the previous loop's playback can
incorrectly suppress that same sound type's first occurrence in the
next loop - the *game logic* requesting a sound effect is fully
deterministic (same ticks, same input), but whether that request
actually results in an audible sound (and the `md_playTone()` work that
goes with it) depends on how many total frames have elapsed since boot
by the time this particular loop happens to start, which differs every
time.

**`particles[]`** (`particle.c`) is the same pattern again, and was
found once the other two were already fixed and the discrepancy
persisted (confirmed sound wasn't the (remaining) cause at that point
by setting `isSoundEnabled = false` and observing the inconsistency
didn't go away). `initParticle()` clears the array (`memset` to -1, the
"empty slot" sentinel `updateParticles()`'s own `ticks < 0` check looks
for), but again, only once, at title-screen entry, not on every loop
restart. A particle spawned near the tail end of one loop's playback -
a game-over effect right as the recorded input runs out, for instance
- can still be mid-lifetime (`ticks >= 0`) the instant the loop
restarts, carrying it, and the update/draw work it costs every frame
until it naturally expires, into the next loop. Worse, this compounds
across successive loops: loop 2 can inherit leftovers from loop 1,
loop 3 from loop 2 (which already included loop 1's), and so on, until
the 32-slot circular buffer (`MAX_PARTICLE_COUNT`) fills and starts
overwriting still-active entries.

**Tone queues** (`portVircon32.c`) are one layer further down the
audio stack than `soundEffectPlayedTimes` above, and were found after
that fix and the `particles[]` fix both still weren't enough - this
time isolated by hard-coding `setRandomSeedWithTime()` (`random.c`) to
always return a fixed seed instead of `get_time() ^
get_cycle_counter()`, which rules out *every* wall-clock-seeded random
source at once (not just `particleRandom` specifically) and the
inconsistency still didn't go away, pointing at something with no
random component at all. `md_stopTone()` already existed and already
clears exactly the right state - the pending/active tone queues that
drive actual audible playback, plus a hard stop on every SPU channel -
but it's normally only called from `disableSound()`, when the player
turns sound off entirely, not on a replay loop restart. A sound effect
scheduled (via `md_playTone()`) near the tail end of one loop's
playback, not yet started or not yet finished by the time the loop
restarts, carries its playback - and the per-frame cost of
`updateSynthTones()`/`playnote_update()` processing it - into the next
loop: the same carry-over problem as `particles[]`, one step later in
the pipeline (`soundEffectPlayedTimes` controls whether a tone gets
*scheduled* at all; this controls what happens to one that already
was).

**`score`** (`cglp.c`) was found the same way, after the tone-queue fix
still wasn't enough: a systematic sweep of every global `cglp.c`
declares, cross-referenced against what `startReplay()` actually
resets, rather than continuing to guess one candidate at a time.
Paku Paku's own update logic (`score += pakupakuMultiplier`,
`addScore()`) modifies it during replay too, via the same
`currentUpdate()` call `updateTitle()` uses for playback - and `score`
was only ever reset in `initInGame()`, never here. The title screen's
own score display shows `prevScore`, not this live value, so the
growth isn't necessarily visible on screen - but the underlying number
still grows unbounded across successive loops regardless, and any
other game logic reading `score` directly (rather than just
accumulating into it) would see a different, wrong value on loop 2
than it saw on loop 1.

**`input`** (`cglp.c`) is the most consequential of the six, found in
that same sweep. `updateButtons()` computes `isJustPressed` as
`isPressed && !previousIsPressed` - it reads the button state left
over from whatever the *last* call to it set, per button and overall.
`replayInput()` writes into `input` (not `currentInput` - that's the
live gamepad, untouched by any of this) via `updateButtons(&input,
...)`, and games read directly from `input` (e.g.
`gamePakuPaku.c`'s `input.left.isJustPressed`). `initInput()` already
exists to clear this exact state, but it's only ever called once, from
`initGame()` at console boot - never again, not on every game
selection, not on every replay loop restart. Left alone, the very
first frame of a new loop computes `isJustPressed` by comparing the
loop's first recorded button state against whatever
`input.left.isPressed` (etc.) happened to be at the *end* of the
previous loop's playback, not a genuine fresh start - so if the
previous loop ended with a direction held down and the new loop's
first frame also has it held, `isJustPressed` incorrectly reads
`false` (since `isPressed` was already `true`), where a true fresh
start would have correctly read `true`. Unlike the other five fixes
here, this one isn't just additive cost carried forward - a wrong
`isJustPressed` on frame one can flip a direction, a menu selection,
anything a game's own logic branches on, and cascade into a genuinely
different playthrough for the rest of that loop, not just a
different cycle count for an otherwise-identical one. `initInput()`
itself only clears the six per-button sub-structs
(left/right/up/down/b/a), not `input`'s own top-level
`isPressed`/`isJustPressed`/`isJustReleased` - harmless for its own
one-time, already-zero-initialized case at boot, but those top-level
flags do carry real state forward once the game has actually been
running, so they're cleared explicitly here too rather than just
calling `initInput()` and assuming it's sufficient. `input.pos` isn't
included in the reset - `replayInput()` unconditionally overwrites it
from the recorded data every single call, so it never needs one.

**Character-pattern cache** (`characterPatternOrder`/`characterPatternsCount`,
`cglp.c`) is a smaller, subtler case than the other six, found in the
same systematic sweep as `score`/`input` above. It's the move-to-front
cache `drawCharacter()` uses to avoid recomputing a character's pixel
pattern it's already seen. Unlike everything else in this section, its
state doesn't affect what actually gets drawn - a cache miss just
means recomputing the same pattern, not a different one - so it can't
cause a gameplay-level divergence the way `input` above can. But it
does affect how many cache hits vs misses happen, and therefore cycle
cost, and it's only ever reset by `initCharacter()`, called once from
`resetGame()` at game-session start, never on a replay loop restart.
Left alone, each new loop's first few frames are querying a cache
still ordered by whatever the previous loop's tail happened to access
most recently, not the fresh, empty state a real "first ever draw"
would see - a smaller effect than `input`'s, since the cache
re-converges to the loop's own deterministic access pattern within a
few frames regardless, but still not a genuine fresh start.

All seven fixed the same way: reset inside `startReplay()` itself,
scoped only to the replay path, so real gameplay is untouched -
`particleRandom` reseeded with the same recorded seed as `gameRandom`,
`soundEffectPlayedTimes` zeroed the same way `initSound()` does for a
brand new session, `particles[]` cleared the same way `initParticle()`
does (plus resetting `particleIndex`, which `initParticle()` itself
doesn't bother with - fine for its own one-time-at-title-screen case,
but worth doing properly here since every loop needs to start from an
identical state), the tone queues cleared by calling the
already-existing `md_stopTone()`, `score` zeroed directly, `input`'s
button state cleared more thoroughly than `initInput()` itself bothers
with, and the character-pattern cache reset the same way
`initCharacter()` does. None of the seven are gated behind
`DEBUG_MODE`: all are real determinism fixes to the replay system
itself, independent of whether the debug overlay is being used to
observe them.

**Not fixed, and worth knowing about**: even with all three resets,
`md_getAudioTime()`'s absolute value at the *start* of each loop still
differs (since the global frame counter never resets), which shifts
the phase of `playSoundEffect()`'s own quantization
(`ceil(ct / QUANTIZED_DURATION) * QUANTIZED_DURATION`) relative to
each loop's own timeline. In principle this could still cause small,
loop-to-loop differences in exactly when a sound effect's dedup window
closes, later in a loop's playback - a smaller, subtler version of the
same underlying problem. Not fixed here: `md_getAudioTime()` is also
the clock actual tone scheduling depends on (`playnote_update()`'s
pending/active tone queues), and the comment on its own definition
notes it "must be non-decreasing and track real time" - changing how
it behaves risks breaking real audio playback correctness for the sake
of a debug-only measurement, which isn't a trade worth making
casually. If this residual variance turns out to matter in practice,
it needs a more careful look at the sound-scheduling system
specifically, not a quick patch here.

## Debug mode

Uncomment `#define DEBUG_MODE` near the top of `src/machineDependent.h`
to enable a small performance overlay, drawn in the bottom-left corner of
every game:

```
OBJ 87/412
CYC 4213/9871
AVG 5023
PREV 4890/5230
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
  the highest seen since the last reset.
- **AVG** - running average of cycles-per-frame since the last reset,
  on its own line below CYC. Resets on Y press, and automatically at
  the start of every replay loop, via a dedicated `md_onReplayStart()`
  hook called from `startReplay()` (`cglp.c`) - the one function
  actually called at every replay (re)start, both the very first time
  and every subsequent loop restart once the recorded input runs out
  (see `updateTitle()` in `cglp.c`). Deliberately scoped to replay
  playback only, not real player gameplay: earlier versions of this
  feature also reset on real gameplay starting/ending (via
  `md_initView()`, then later dedicated `md_onGameplayStart()`/
  `md_onGameOver()` hooks), but this measurement is meant purely for
  tracing the replay loop's own performance - a fully deterministic,
  no-input-needed benchmark - so those were removed rather than left
  as unused complexity. Unlike the other three metrics (OBJ/CYC max,
  FPS min, CACHE), which only reset on Y, AVG resets on both.
- **PREV** - current/max, like OBJ and CYC above. Current is whatever
  AVG was reading right before its most recent reset - lets one replay
  loop's average stay visible for comparison after the next loop has
  already started accumulating its own. Resets via the same
  `md_onReplayStart()` hook and Y press as AVG. Max is the highest
  that current value has reached, but - unlike every other "max" this
  overlay tracks, and unlike PREV's own current value right next to it
  - resets *only* on Y, never on a replay loop restart, so a single
  unusually expensive loop stays visible even after several cheaper
  loops have since replaced it as PREV's current value.

Both AVG and PREV are computed/updated the same way: an incremental
running mean (`avg += (new - avg) / count`) rather than a running sum
divided by frame count - a plain sum risks overflowing a 32-bit int
over a long enough session at up to ~250,000 cycles/frame, where the
incremental form never accumulates a large value regardless of session
length. Vircon32's own docs note emulators make no timing guarantees
for cycles *within* a frame, so treat all of CYC/AVG/PREV as a relative
"is this getting worse" signal.
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

### Stats recording mode

Also gated behind `DEBUG_MODE`. Press X anywhere to start an
unattended benchmark run: it steps through every game in turn,
auto-plays each one just long enough to record a replay, measures that
replay's own `PREV` max over a fixed number of loops, records the
result, and moves to the next game - ending in a results screen
listing every game's number, meant to be screenshotted and OCR'd or
transcribed into a spreadsheet.

Per game, in order:

1. **Select and enter the game.** `restartGame()` always clears any
   previous replay first (`initReplay()`, inside `resetGame()`), so a
   `hasTitle` game lands on a replay-less title screen - this calls
   `initInGame()` directly rather than waiting for a synthetic press to
   do it a frame later, one less state transition to reason about.
2. **Auto-play.** While `state == STATE_IN_GAME`, a synthetic A press
   fires once every `STATS_A_PRESS_INTERVAL_FRAMES` frames (default 60
   - `#define`d in `portVircon32.c`, next to the other stats-recording
   constants). Deliberately A-only, matching how these games were
   asked to be driven - no directional input is simulated, so the 8
   mouse-driven games in particular may take a while (or hit the
   timeout below) rather than progressing quickly. If the game hasn't
   reached a game over within `STATS_IN_GAME_TIMEOUT_FRAMES` (default
   3600, one full minute of real gameplay time), this forces one
   directly rather than let a single unusually-durable game hang the
   whole unattended run - whatever got recorded up to that point is
   still a real, fully deterministic sequence, just not one that ends
   in a natural death, which is fine for this mode's purposes.
3. **Game over.** The real ~2-second game-over-screen timeout
   (`updateGameOver()` in `cglp.c`) is skipped by calling `initTitle()`
   directly the moment `state == STATE_GAME_OVER` is seen - nothing
   about that wait is part of what's being measured, so there's no
   reason to actually sit through it once per game.
4. **Replay starts.** `isReplayRecorded` is now true, so `initTitle()`
   starts the replay that was just auto-played. This is loop 1.
5. **On loop 2 starting**, the Y button's reset is performed directly
   (there's no real Y press to synthesize a frame in advance of, so
   this calls the same reset logic Y's own handler does, rather than
   trying to fake a button press that would only be read a frame
   later anyway) - clearing out whatever the first loop measured, so
   what gets recorded reflects loops that ran under identical,
   already-reset conditions throughout.
6. **10 more loops run** (`STATS_REPLAY_LOOPS_TO_MEASURE`), counted
   from the reset in step 5, not from game over - "10 more" means 10
   loops after that reset, not 10 minus the 2 it took to get there.
7. **Record and advance.** `debugMaxPrevAvgCycles` - now reflecting the
   max across exactly those 10 loops - is stored into
   `statsResults[gameIndex]`, and the whole sequence repeats for the
   next game.

All of the counting, resetting, and recording for steps 4-7 happens
inside `md_onReplayStart()` (see its own comment), triggered
automatically as each loop (re)starts - `updateStatsRecording()` (the
function driving steps 1-3) doesn't need to know anything about replay
loops itself, only about get into a game and drive its own gameplay.

Once every game (`gameCount - 1`, skipping the menu at index 0) has a
recorded result, the results screen replaces normal rendering entirely
- white background, black BIOS-font text, three columns of `N: value`
lines, sized to comfortably fit even the largest possible game count
(`MAX_GAME_COUNT - 1`, 48) within the screen's height. Chosen
specifically for OCR: a plain screenshot should be enough to run
through OCR cleanly, rather than needing to fight the low-res in-game
font or a colored background first.

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
