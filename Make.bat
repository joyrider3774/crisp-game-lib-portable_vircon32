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
echo Assemble the ASM code
echo --------------------------
assemble obj\main.asm -o obj\main.vbin || goto :failed

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
