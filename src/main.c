// =============================================================================
// crisp-game-lib-portable for Vircon32 - master build file
// =============================================================================
// Vircon32 has no linker: a program is exactly one translation unit, built
// from a single .c file passed to compile.exe (everything else must be
// pulled in via #include, full implementations and all - see
// VIRCON32_C_DIALECT.md section 11). This file is that single entry
// point: it #includes every module in dependency order, then
// portVircon32.c last (which defines the machine-dependent hooks and
// main()).
//
// Build: compile.exe main.c -o main.asm && assemble.exe main.asm -o
// main.vbin && packrom.exe rom.xml -o crisp-game-lib.v32
// (see Make.bat / Make.sh at the project root for the exact commands and
// folder layout used to build this project).
// =============================================================================

#include "cglp.h"

#include "vector.c"
#include "random.c"
#include "particle.c"
#include "textPattern.c"
#include "sound.c"
#include "cglp.c"
#include "menu.c"
#include "menuGameList.c"

// ---- Ported games (add more #includes here as you port them) ----
#include "games/gamePakuPaku.c"
#include "games/gamePinClimb.c"
#include "games/gameHexmin.c"
#include "games/gameFracave.c"
#include "games/gameBallTour.c"
#include "games/gameGrapplingH.c"
#include "games/gameBamboo.c"
#include "games/gameBaroll.c"
#include "games/gameTimberTest.c"
#include "games/gameBWalls.c"
#include "games/gameThunder.c"
#include "games/gameAeralBar.c"
#include "games/gameColorRoll.c"
#include "games/gameFroooog.c"
#include "games/gameRWheel.c"
#include "games/gameReflector.c"
#include "games/gameCastN.c"
#include "games/gameSurvivor.c"
#include "games/gameInTow.c"
#include "games/gameLadderDrop.c"
#include "games/gameBCannon.c"
#include "games/gameBallsBombs.c"
#include "games/gameAntLion.c"
#include "games/gameTwoFaced.c"
#include "games/gameBombUp.c"
#include "games/gameBSFish.c"
#include "games/gameCatapult.c"
#include "games/gameCateP.c"
#include "games/gameChargeBeam.c"
#include "games/gameCounterB.c"
#include "games/gameCTower.c"
#include "games/gameDescents.c"
#include "games/gameDivarr.c"
#include "games/gameDLaser.c"
#include "games/gameDarkCave.c"
#include "games/gameBBlast.c"
#include "games/gameBalance.c"
#include "games/gameBreedC.c"
#include "games/gameCardQ.c"
#include "games/gameCNodes.c"
#include "games/gameCrossLine.c"
#include "games/gameDFight.c"

// ---- Vircon32-specific machine-dependent layer + main() ----
#include "portVircon32.c"
