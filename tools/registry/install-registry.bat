@echo off
setlocal EnableExtensions
rem ---------------------------------------------------------------------------
rem  FreeFalcon 6 registry setup / migration
rem
rem  This build reads its data paths from the Falcon "4.1" key instead of "4.0",
rem  so FreeFalcon and a stock Falcon 4.0 / GOG install can coexist -- each owns
rem  its own key instead of fighting over 4.0.
rem
rem  If you ALREADY had FreeFalcon working, your paths are under 4.0 and this
rem  script copies them to 4.1 for you (nothing under 4.0 is changed or removed).
rem
rem  Usage: put next to FFViper.exe, right-click -> "Run as administrator".
rem         install-registry.bat            migrate, or create from this folder
rem         install-registry.bat /force     overwrite 4.1 paths with this folder
rem
rem  /reg:32 targets the 32-bit registry view (WOW6432Node) -- the game opens the
rem  key with KEY_WOW64_32KEY even though the exe is 64-bit. Do not remove it.
rem ---------------------------------------------------------------------------

set "BASE=%~dp0"
if "%BASE:~-1%"=="\" set "BASE=%BASE:~0,-1%"
set "OLD=HKLM\SOFTWARE\MicroProse\Falcon\4.0"
set "NEW=HKLM\SOFTWARE\MicroProse\Falcon\4.1"
set "FORCE="
if /i "%~1"=="/force" set "FORCE=1"

net session >nul 2>&1
if errorlevel 1 (
  echo ERROR: must be run as administrator ^(writes to HKEY_LOCAL_MACHINE^).
  echo Right-click install-registry.bat -^> "Run as administrator".
  pause & exit /b 1
)

echo FreeFalcon folder detected as:
echo     %BASE%
echo.
if not exist "%BASE%\FFViper.exe" (
  echo WARNING: FFViper.exe not found here. This script should sit in your
  echo          FreeFalcon folder. Continuing anyway.
  echo.
)

reg query "%NEW%" /v baseDir /reg:32 >nul 2>&1
if not errorlevel 1 if not defined FORCE (
  echo The 4.1 key already exists:
  echo.
  reg query "%NEW%" /reg:32
  echo.
  echo Nothing changed. Re-run with /force to reset the paths to this folder.
  pause & exit /b 0
)

if defined FORCE goto :writefresh

rem --- migrate an existing FreeFalcon install from 4.0 -----------------------
reg query "%OLD%" /v baseDir /reg:32 >nul 2>&1
if errorlevel 1 goto :writefresh

for /f "tokens=2,*" %%A in ('reg query "%OLD%" /v baseDir /reg:32 ^| findstr /i baseDir') do set "OLDBASE=%%B"
echo Found an existing Falcon 4.0 key pointing at:
echo     %OLDBASE%
echo.
if /i "%OLDBASE%"=="%BASE%" (
  echo That is this same folder, so it is your existing FreeFalcon install.
) else (
  echo NOTE: that is a DIFFERENT folder from this one. If it is a stock Falcon
  echo       4.0 / GOG install rather than FreeFalcon, press Ctrl+C now and run
  echo       this script again with /force to write this folder's paths instead.
)
echo.
echo Copying 4.0 -^> 4.1 ^(4.0 is left untouched^)...
reg copy "%OLD%" "%NEW%" /s /f /reg:32 >nul || goto :fail
goto :done

:writefresh
echo Writing 4.1 keys from this folder...
reg add "%NEW%" /v baseDir    /t REG_SZ /d "%BASE%"                  /reg:32 /f >nul || goto :fail
reg add "%NEW%" /v misctexDir /t REG_SZ /d "%BASE%\terrdata\misctex" /reg:32 /f >nul || goto :fail
reg add "%NEW%" /v objectDir  /t REG_SZ /d "%BASE%\terrdata\objects" /reg:32 /f >nul || goto :fail
reg add "%NEW%" /v theaterDir /t REG_SZ /d "%BASE%\terrdata\korea"   /reg:32 /f >nul || goto :fail
reg add "%NEW%" /v movieDir   /t REG_SZ /d "%BASE%"                  /reg:32 /f >nul || goto :fail
reg add "%NEW%" /v curTheater /t REG_SZ /d "Korea"                   /reg:32 /f >nul || goto :fail
reg add "%NEW%" /v FFver      /t REG_SZ /d "6.0"                     /reg:32 /f >nul || goto :fail
reg add "%NEW%\MPR" /v MPRDetect3Dx /t REG_DWORD /d 1 /reg:32 /f >nul || goto :fail
reg add "%NEW%\MPR" /v MPRDetectCPU /t REG_DWORD /d 1 /reg:32 /f >nul || goto :fail
reg add "%NEW%\MPR" /v MPRDetectMMX /t REG_DWORD /d 1 /reg:32 /f >nul || goto :fail
reg add "%NEW%\MPR" /v MPRDetectXMM /t REG_DWORD /d 1 /reg:32 /f >nul || goto :fail

:done
echo.
echo Done. The 4.1 key now contains:
echo.
reg query "%NEW%" /reg:32
echo.
echo Your Falcon 4.0 key was not modified.
pause & exit /b 0

:fail
echo.
echo FAILED to write the registry. Are you running as administrator?
pause & exit /b 1
