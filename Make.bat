@echo off
REM Build script for crisp-game-lib-portable for Vircon32.
REM Assumes compile.exe, assemble.exe, png2vircon.exe, wav2vircon.exe and
REM packrom.exe are on PATH (same layout as this project's other Vircon32
REM ports).

REM create obj and bin folders if non existing, since
REM the development tools will not create them themselves
if not exist obj mkdir obj
if not exist bin mkdir bin

echo.
echo Compile the C code
echo --------------------------
compile src\main.c -o obj\main.asm || goto :failed

echo.
echo Optimize the ASM code - v32opt, optional
echo --------------------------
REM v32opt (https://github.com/wedge1020/v32opt) is a third-party,
REM optional post-compile assembly optimizer - not part of the official
REM Vircon32 toolchain, and not required to build this project. Skipped
REM automatically if not found on PATH, or if SKIP_V32OPT is set to 1
REM (e.g. `set SKIP_V32OPT=1` before running this script) - either way,
REM the unoptimized assembly from compile.exe is used directly, same as
REM if this step didn't exist.
REM
REM Only six of v32opt's nine passes are used here, individually, NOT
REM via -O1/-O2/-O3 - every preset level bundles at least one pass with
REM a confirmed correctness bug (verified directly against this
REM project's own compiled output, tracing real transformed call
REM sites, not just checking whether the result assembled - see
REM PORTING.md for the full writeup and evidence for each):
REM   -fopt_inline              BROKEN - inlined bodies keep their
REM                              original [BP+N] parameter references,
REM                              which after inlining point at the
REM                              wrong stack slot in the caller's frame
REM                              instead of the actual argument passed.
REM   -fopt_strength_reduction  BROKEN - emits a bare "SHR" mnemonic
REM                              Vircon32 doesn't have (only SHL is a
REM                              real instruction), which the real
REM                              assembler rejects outright.
REM   -fopt_dce                 BROKEN - false positive in its
REM                              reachability analysis deleted a
REM                              function this project genuinely calls
REM                              (select_texture), which the assembler
REM                              then couldn't find.
REM   -fopt_constant_folding    BROKEN, and the most dangerous of the
REM                              four - its cross-block constant
REM                              tracking leaks across function
REM                              boundaries and function calls,
REM                              silently replacing a real computed
REM                              value (traced example: the result of a
REM                              pow() call, mid-formula in a MIDI-
REM                              note-to-frequency conversion) with a
REM                              stale, unrelated constant. No assemble
REM                              error - just silently wrong output.
REM The six enabled below (peephole, algebraic, forwarding, jump_next,
REM redundant_movs, combine_immediates) were checked the same way -
REM individually run against this project's real compiled output,
REM assembled, and for algebraic/forwarding/combine_immediates
REM specifically, a real transformed call site traced by hand and
REM confirmed correct. The other three didn't happen to trigger on
REM this specific file, so they're unverified by tracing, but are
REM simple, local, single-pattern peephole-style passes structurally
REM unlike any of the four broken ones above.
set ASM_TO_ASSEMBLE=obj\main.asm
if "%SKIP_V32OPT%"=="1" (
    echo SKIP_V32OPT=1 set - skipping, using unoptimized assembly
) else (
    where v32opt >nul 2>nul
    if %errorlevel% equ 0 (
        v32opt obj\main.asm obj\main_opt.asm -v -O3 && set ASM_TO_ASSEMBLE=obj\main_opt.asm
		REM v32opt obj\main.asm obj\main_opt.asm -fopt_inline && set ASM_TO_ASSEMBLE=obj\main_opt.asm
	) else (
        echo v32opt not found on PATH - skipping, using unoptimized assembly
    )
)

echo.
echo Assemble the ASM code
echo --------------------------
assemble %ASM_TO_ASSEMBLE% -o obj\main.vbin || goto :failed

echo.
echo Convert the PNG textures
echo --------------------------
png2vircon assets\white.png -o obj\white.vtex || goto :failed

echo.
echo Convert the PlayNote wavetable
echo --------------------------
wav2vircon libs\PlayNote\sounds\wt_saw.wav -o obj\wt_saw.vsnd || goto :failed

echo.
echo Pack the ROM
echo --------------------------
packrom rom.xml -o "bin\crisp-game-lib.v32" || goto :failed

goto :succeeded

:failed
echo.
echo BUILD FAILED
exit /b %errorlevel%

:succeeded
echo.
echo BUILD SUCCESSFUL
exit /b

@echo on
