# crisp-game-lib-portable for Vircon32

A port of all 42 games from [crisp-game-lib](https://github.com/abagames/crisp-game-lib)
to the [Vircon32](https://www.vircon32.com/) fantasy console.

## Credits and lineage

- **[crisp-game-lib](https://github.com/abagames/crisp-game-lib)** by
  [ABAGames](https://github.com/abagames) - the original engine and all 42
  games this project is built on.
- **crisp-game-lib-portable** - the portable C rewrite of the original
  JavaScript engine that this and the ports below are descended from.
- An SDL-targeted port of that portable C rewrite.
- **[crisp-game-lib-portable-tufty2350](https://github.com/joyrider3774/crisp-game-lib-portable-tufty2350)**
  by [joyrider3774](https://github.com/joyrider3774) - my own earlier port
  of the above, targeting the Tufty2350 handheld.
- **This Vircon32 port** is based directly on the Tufty2350 port above,
  adapted to Vircon32's own C dialect, video/audio hardware, and lack of
  a mouse.

This port - the engine adaptation, the Vircon32-specific audio/video
backend, the performance work, and this documentation - was built with
the help of [Claude](https://www.anthropic.com/claude) (Anthropic).

## Controls

D-pad to move, **A**/**B** for each game's action buttons, **START** to
open the game-selection menu. Vircon32 has no mouse; the handful of
games that originally used mouse-drag controls move a virtual cursor
with the d-pad instead (see [PORTING.md](PORTING.md) for details).

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

## License

`LICENSE.txt` (original MIT license, ABA Games / joyrider3774, carried
over unchanged) covers this port's own code. `libs/PlayNote/` is a
separate, small library also written for this port and is
MIT-licensed on its own - see `libs/PlayNote/README.md`, which also
credits the one wavetable asset it uses from another project.
