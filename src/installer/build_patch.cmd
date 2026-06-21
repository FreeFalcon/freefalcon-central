@echo off
REM ---------------------------------------------------------------------------
REM Build the FFViper patch MSI for FreeFalcon 6 (modern WiX / "wix build").
REM
REM One-time prerequisite (install the WiX UI extension for this machine):
REM     wix extension add -g WixToolset.UI.wixext
REM
REM GAMEDIR must point at the FreeFalcon 6 folder that holds the up-to-date
REM assets (FFViper.exe, FFEmu.hlsl, art\, config\ ...). Build a fresh Release
REM first and copy FFViper.exe there, plus FFEmu.hlsl into GAMEDIR\shaders\.
REM ---------------------------------------------------------------------------
setlocal
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
