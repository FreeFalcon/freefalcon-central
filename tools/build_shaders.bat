@echo off
REM Artscout - 2026: Windows half of the shader build -- same ffshaders.manifest
REM and same DXC -Fh/-Vn output as tools/build_shaders.sh.
REM
REM   build_shaders.bat <shader_dir> <out_dir>
REM
REM Uses the Microsoft.Direct3D.DXC NuGet dxc.exe already vendored under
REM packages\ -- it emits BOTH DXIL and SPIR-V, unlike the Windows SDK copy,
REM which has no SPIR-V backend.
setlocal enabledelayedexpansion

set "SRC_DIR=%~1"
set "OUT_DIR=%~2"
if "%SRC_DIR%"=="" set "SRC_DIR=%~dp0..\src\graphics\shaders"
if "%OUT_DIR%"=="" set "OUT_DIR=%SRC_DIR%\generated"

set "DXC="
for /f "delims=" %%D in ('dir /b /o-n "%~dp0..\packages\Microsoft.Direct3D.DXC.*" 2^>nul') do (
    if not defined DXC if exist "%~dp0..\packages\%%D\build\native\bin\x64\dxc.exe" (
        set "DXC=%~dp0..\packages\%%D\build\native\bin\x64\dxc.exe"
    )
)
if not defined DXC if exist "%VULKAN_SDK%\Bin\dxc.exe" set "DXC=%VULKAN_SDK%\Bin\dxc.exe"
if not defined DXC for %%X in (dxc.exe) do if not "%%~$PATH:X"=="" set "DXC=%%~$PATH:X"

if not defined DXC (
    echo build_shaders: dxc.exe not found. Restore the Microsoft.Direct3D.DXC
    echo                NuGet package ^(packages\^) or install the Vulkan SDK.
    exit /b 1
)

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

for /f "usebackq eol=# tokens=1-6 delims=|" %%a in ("%SRC_DIR%\ffshaders.manifest") do (
    set "SRC=%%a"
    set "ENTRY=%%b"
    set "PROFILE=%%c"
    set "TARGET=%%d"
    set "DEFINES=%%e"
    set "VAR=%%f"
    call :trim SRC
    call :trim ENTRY
    call :trim PROFILE
    call :trim TARGET
    call :trim DEFINES
    call :trim VAR

    if not "!SRC!"=="" (
        set "FLAGS="
        REM vulkan1.2 == SPIR-V 1.5, matching VulkanBackend's apiVersion; a 1.3
        REM target emits SPIR-V 1.6, which a 1.2 device rejects.
        if "!TARGET!"=="spirv" set "FLAGS=-spirv -fspv-target-env=vulkan1.2"
        if not "!DEFINES!"=="-" if not "!DEFINES!"=="" (
            for %%D in ("!DEFINES:,=" "!") do set "FLAGS=!FLAGS! -D %%~D=1"
        )

        echo dxc !SRC! [!ENTRY!/!PROFILE!/!TARGET!] -^> !VAR!.h
        "!DXC!" -T !PROFILE! -E !ENTRY! -Fh "%OUT_DIR%\!VAR!.h" -Vn !VAR! !FLAGS! "%SRC_DIR%\!SRC!"
        if errorlevel 1 (
            echo build_shaders: FAILED on !SRC! !ENTRY! !TARGET!
            exit /b 1
        )
    )
)

echo build_shaders: done -^> "%OUT_DIR%"
endlocal
exit /b 0

:trim
REM Manifest columns are space-padded; strip both ends of the named variable.
setlocal enabledelayedexpansion
call set "V=%%%~1%%"
for /f "tokens=* delims= " %%T in ("!V!") do set "V=%%T"
:trim_tail
if "!V:~-1!"==" " set "V=!V:~0,-1!" & goto :trim_tail
endlocal & set "%~1=%V%"
exit /b 0
