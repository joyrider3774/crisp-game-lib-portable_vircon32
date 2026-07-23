#!/bin/bash
# Build script for crisp-game-lib-portable for Vircon32.
# Assumes compile, assemble, png2vircon, wav2vircon and packrom are on
# PATH (same layout as Make.bat / this project's other Vircon32 ports).

abort_build()
{
    echo
    echo BUILD FAILED
    exit 1
}

# create obj and bin folders if non existing, since the
# development tools will not create them themselves
mkdir -p obj
mkdir -p bin

echo
echo Compile the C code
echo --------------------------
compile src/main.c -o obj/main.asm || abort_build

echo
echo Assemble the ASM code
echo --------------------------
assemble obj/main.asm -o obj/main.vbin || abort_build

echo
echo Convert the PNG textures
echo --------------------------
png2vircon assets/white.png -o obj/white.vtex || abort_build

echo
echo Convert the PlayNote wavetable
echo --------------------------
wav2vircon "libs/PlayNote/sounds/wt_saw.wav" -o "obj/wt_saw.vsnd" || abort_build

echo
echo Pack the ROM
echo --------------------------
packrom rom.xml -o "bin/crisp-game-lib.v32" || abort_build

echo
echo BUILD SUCCESSFUL
