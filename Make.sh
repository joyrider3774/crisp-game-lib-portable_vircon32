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
echo Optimize the ASM code - v32opt, optional
echo --------------------------
# v32opt (https://github.com/wedge1020/v32opt) is a third-party,
# optional post-compile assembly optimizer - not part of the official
# Vircon32 toolchain, and not required to build this project. Skipped
# automatically if not found on PATH, or if SKIP_V32OPT=1 is set
# (e.g. `SKIP_V32OPT=1 ./Make.sh`) - either way, the unoptimized
# assembly from compile.exe is used directly, same as if this step
# didn't exist.
#
# Only six of v32opt's nine passes are used here, individually, NOT
# via -O1/-O2/-O3 - every preset level bundles at least one pass with a
# confirmed correctness bug (verified directly against this project's
# own compiled output, tracing real transformed call sites, not just
# checking whether the result assembled - see PORTING.md for the full
# writeup and evidence for each):
#   -fopt_inline              BROKEN - inlined bodies keep their
#                              original [BP+N] parameter references,
#                              which after inlining point at the wrong
#                              stack slot in the caller's frame instead
#                              of the actual argument passed in.
#   -fopt_strength_reduction  BROKEN - emits a bare "SHR" mnemonic
#                              Vircon32 doesn't have (only SHL is a
#                              real instruction), which the real
#                              assembler rejects outright.
#   -fopt_dce                 BROKEN - false positive in its
#                              reachability analysis deleted a function
#                              this project genuinely calls
#                              (select_texture), which the assembler
#                              then couldn't find.
#   -fopt_constant_folding    BROKEN, and the most dangerous of the
#                              four - its cross-block constant tracking
#                              leaks across function boundaries and
#                              function calls, silently replacing a
#                              real computed value (traced example: the
#                              result of a pow() call, mid-formula in a
#                              MIDI-note-to-frequency conversion) with
#                              a stale, unrelated constant. No assemble
#                              error - just silently wrong output.
# The six enabled below (peephole, algebraic, forwarding, jump_next,
# redundant_movs, combine_immediates) were checked the same way -
# individually run against this project's real compiled output,
# assembled, and for algebraic/forwarding/combine_immediates
# specifically, a real transformed call site traced by hand and
# confirmed correct. The other three didn't happen to trigger on this
# specific file, so they're unverified by tracing, but are simple,
# local, single-pattern peephole-style passes structurally unlike any
# of the four broken ones above.
ASM_TO_ASSEMBLE=obj/main.asm
if [ "$SKIP_V32OPT" = "1" ]; then
    echo SKIP_V32OPT=1 set - skipping, using unoptimized assembly
elif command -v v32opt >/dev/null 2>&1; then
    v32opt obj/main.asm obj/main_opt.asm \
        -fopt_peephole -fopt_algebraic -fopt_forwarding \
        -fopt_jump_next -fopt_redundant_movs -fopt_combine_immediates \
        && ASM_TO_ASSEMBLE=obj/main_opt.asm
else
    echo v32opt not found on PATH - skipping, using unoptimized assembly
fi

echo
echo Assemble the ASM code
echo --------------------------
assemble "$ASM_TO_ASSEMBLE" -o obj/main.vbin || abort_build

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
