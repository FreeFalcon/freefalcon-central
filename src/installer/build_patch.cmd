@echo off
REM ---------------------------------------------------------------------------
REM Build the FFViper patch MSI for FreeFalcon 6 (modern WiX / "wix build").
REM
REM One-time prerequisite (install the WiX UI extension for this machine):
REM     wix extension add -g WixToolset.UI.wixext
REM
REM GAMEDIR supplies only the BUILD OUTPUTS / runtime files: FFViper.exe, ffviper.cfg,
REM and the ST80 voice tools (st80conv.exe, st80w.dll). Build a fresh Release first and copy
REM FFViper.exe there. (FFEmu.hlsl is NOT shipped -- it is baked into the exe as an RCDATA
REM resource and compiled at runtime, so no external shader file is packaged.)
REM All hand-edited DATA assets (art\setup, art\fonts, art\uiskin, art\ckptart, art\main,
REM config\) now ship from this installer's own res\ tree -- the MSI is self-contained for
REM data and does NOT depend on the game dir for those (only GAMEDIR for the exe/cfg/st80).
REM The big ~500MB falcon_pcm.tlk PCM bank is NO LONGER shipped; instead st80conv.exe + st80w.dll
REM let the user transcode the native voices locally from their own falcon.tlk.
REM ---------------------------------------------------------------------------
setlocal
REM run from this script's folder so the relative res\ source paths in the .wxs resolve
cd /d "%~dp0"
set GAMEDIR=G:\Games\FreeFalcon6

wix build FFViperPatch.wxs -d GameDir="%GAMEDIR%" -ext WixToolset.UI.wixext -o FFViperPatch.msi
if errorlevel 1 (
  echo.
  echo Build FAILED. If it complains about the UI extension, run once:
  echo     wix extension add -g WixToolset.UI.wixext
  exit /b 1
)
echo.
endlocal
