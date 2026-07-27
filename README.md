# crisp-game-lib-portable for Vircon32
![DownloadCountTotal](https://img.shields.io/github/downloads/joyrider3774/crisp-game-lib-portable_vircon32/total?label=total%20downloads&style=plastic) ![DownloadCountLatest](https://img.shields.io/github/downloads/joyrider3774/crisp-game-lib-portable_vircon32/latest/total?style=plastic) ![LatestVersion](https://img.shields.io/github/v/tag/joyrider3774/crisp-game-lib-portable_vircon32?label=Latest%20version&style=plastic) ![License](https://img.shields.io/github/license/joyrider3774/crisp-game-lib-portable_vircon32?style=plastic)

A port of 266 games from [crisp-game-lib](https://github.com/abagames/crisp-game-lib)
to the [Vircon32](https://www.vircon32.com/) fantasy console.

## Credits and lineage

- **[crisp-game-lib](https://github.com/abagames/crisp-game-lib)** by
  [ABAGames](https://github.com/abagames) - the original engine and games
  this project is built on.
- **[crisp-game-lib-portable](https://github.com/abagames/crisp-game-lib-portable)** -
  the portable C rewrite of the original JavaScript engine that this and
  the ports below are descended from.
- **[crisp-game-lib-portable-sdl](https://github.com/joyrider3774/crisp-game-lib-portable-sdl)**
  by [joyrider3774](https://github.com/joyrider3774) - an SDL-targeted port
  of that portable C rewrite.
- **[crisp-game-lib-portable-tufty2350](https://github.com/joyrider3774/crisp-game-lib-portable-tufty2350)**
  by [joyrider3774](https://github.com/joyrider3774) - my own earlier port
  of the above, targeting the Tufty2350 handheld.
- **This Vircon32 port** is based directly on the Tufty2350 port above,
  adapted to Vircon32's own C dialect, video/audio hardware, and lack of
  a mouse.

This port - the engine adaptation, the Vircon32-specific audio/video
backend, the performance work, and this documentation - was built with
the help of [Claude](https://www.anthropic.com/claude) (Anthropic).

## Game Sources

The individual games themselves come from a handful of upstream
[ABAGames](https://github.com/abagames) repositories:

- **[crisp-game-lib-games](https://github.com/abagames/crisp-game-lib-games)** -
  games made with crisp-game-lib v1.0
- **[crisp-game-lib-11-games](https://github.com/abagames/crisp-game-lib-11-games)** -
  games made with crisp-game-lib v1.1
- **[claude-one-button-game-creation](https://github.com/abagames/claude-one-button-game-creation)** -
  one-button games designed with the help of Claude

## Controls

D-pad to move, **A**/**B** for each game's action buttons, **START** to
open the game-selection menu. Vircon32 has no mouse; the handful of
games that originally used mouse-drag controls move a virtual cursor
with the d-pad instead (see [PORTING.md](PORTING.md) for details).

## Menu Navigation

- **Up**/**Down** - move the selection one game at a time (wraps around;
  category headers are skipped)
- **Left**/**Right** - jump a full page at a time (wraps to the last/first
  page)
- **A** - start the highlighted game
- **B** - also moves the selection down, same as **Down**

## Building

Get the Vircon32 development tools from
[vircon32/ComputerSoftware](https://github.com/vircon32/ComputerSoftware)
(`DevelopmentTools/`) and make sure `compile`, `assemble`, `png2vircon`,
`wav2vircon`, and `packrom` are on your `PATH`. Then:

```
./Make.sh     # Linux/Mac
Make.bat      # Windows
```

This writes the built ROM to `bin/crisp-game-lib.v32`. `Run.sh`/`Run.bat`
launch it in the emulator - edit the emulator path at the top of
whichever one you use first.

## Documentation

- **[PORTING.md](PORTING.md)** - file layout, how the engine was adapted
  (collision detection, audio, input), the debug overlay, and how to
  port additional games into this project.
- **[VIRCON32_C_DIALECT.md](VIRCON32_C_DIALECT.md)** - everything found
  about the Vircon32 C dialect itself along the way: rejected/accepted
  syntax, hardware traps, and a performance model, useful for porting
  other C codebases to this console, not just this one.

## Status

All games run, however certain games may reach 100% cpu usuage and this could cause audio to
cut or games graphics to flicker because vircon32 emulator is trying to catch up by dropping instructions
it should however be fine or you can omit playing those games, notable examples are "R Wheel", "Ladder Drop" and "B Blast"

## History

### V4.0

- Sped up the game-selection menu by replacing its character-cache lookup with a hash table

### V3.0

- Restored every game's title and description to match the original
  JavaScript sources exactly - these feed directly into the seed used to
  procedurally generate each game's background music, so
  titles/descriptions that had drifted from the original text were
  silently generating the wrong tune
- Added **Left**/**Right** page navigation to the game-selection menu

### V2.0

- Added 224 more games, bringing the total number of ported games to 266
- Fixed `addScore()` so a score can be added without showing an on-screen
  popup, and so a zero-value score no longer shows an empty popup
- Fixed a rounding gap in line drawing that could leave a thickness-sized
  hole in long lines

### V1.0

- Initial Release

## License

`LICENSE.txt` (original MIT license, ABA Games / joyrider3774, carried
over unchanged) covers this port's own code. `libs/PlayNote/` is a
separate, small library also written for this port and is
MIT-licensed on its own - see `libs/PlayNote/README.md`, which also
credits the one wavetable asset it uses from another project.
