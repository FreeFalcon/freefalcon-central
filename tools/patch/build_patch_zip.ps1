<#
    Build the FFViper drop-in patch ZIP -- the asset attached to a GitHub release.

    This exists because the ZIP used to be assembled by hand, and the campaign's Add Package
    window very nearly shipped inert: its three art\ data files were not in the previous ZIP, and
    nothing would have said so. A missing file has to be an error, not a silent omission, which is
    the whole point of the manifest below.

    The MSI (src\installer\build_patch.cmd) is a separate, self-contained installer. This is the
    plain unzip-over-your-install route, and the two ship different things -- keep both in step.

    Usage:
        pwsh -File tools\patch\build_patch_zip.ps1 -Name FFViper-campaign-planning-patch
        pwsh -File tools\patch\build_patch_zip.ps1 -Name ... -GameDir D:\FreeFalcon6
#>
[CmdletBinding()]
param(
    # Base name of the archive; ".zip" is appended.
    [Parameter(Mandatory = $true)][string] $Name,

    # Third-party redistributables too large or too licence-encumbered to track live here.
    [string] $GameDir = 'C:\FreeFalcon6',

    # Release build output. FFViper.exe and the DLLs its post-build step copies come from here,
    # never from GameDir -- shipping the game dir's exe is how a stale build reaches users.
    [string] $BuildDir = 'Falcon4___x64_Release',

    [string] $OutDir = 'dist'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Repo root is two levels up from tools\patch.
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $repo

# ---------------------------------------------------------------------------------------------
# The manifest. Each entry is: path inside the ZIP = source.
#
#   build:  produced by the Release build. Must be current -- checked below.
#   repo:   tracked in this repository, so the ZIP is reproducible from a clone.
#   game:   third-party redistributable not tracked here (see .gitignore's OpenAL note for why
#           binaries like this stay out). The only entries that need a populated install.
# ---------------------------------------------------------------------------------------------
$manifest = [ordered]@{
    'FFViper.exe'                                = "build:FFViper.exe"
    'OpenAL32.dll'                               = "build:OpenAL32.dll"
    'openxr_loader.dll'                          = "build:openxr_loader.dll"
    'dxcompiler.dll'                             = "build:dxcompiler.dll"
    'dxil.dll'                                   = "build:dxil.dll"
    'nvtt30205.dll'                              = "game:nvtt30205.dll"

    # Artscout - 2026: the campaign's Add Package window. PACKAGE_WIN is already loaded by
    # CMN_SCF.LST, but the FF4 skin it draws with is named by no image list, so without these it
    # opens with no background and the menu item does nothing. They were absent from the previous
    # ZIP; that is what this manifest is for.
    'art/cp_uiskin.lst'                          = "repo:src/installer/res/art/cp_uiskin.lst"
    'art/cp_pkg_scf.lst'                         = "repo:src/installer/res/art/cp_pkg_scf.lst"
    'art/resource/uiskin_ff4.irc'                = "repo:src/installer/res/art/resource/uiskin_ff4.irc"

    'registry/FreeFalcon6-registry.reg'          = "repo:tools/registry/FreeFalcon6-registry.reg"
    'registry/install-registry.bat'              = "repo:tools/registry/install-registry.bat"

    'README-PATCH.txt'                           = "repo:tools/patch/README-PATCH.txt"
    'THIRD-PARTY-NOTICES.txt'                    = "repo:tools/patch/THIRD-PARTY-NOTICES.txt"
    'licenses/FreeFalcon-BSD-2-Clause.md'        = "repo:LICENSE.md"
    'licenses/OpenAL-Soft-LGPL-2.1-COPYING.txt'  = "repo:src/extlibs/openal/COPYING"
    'licenses/OpenXR-SDK-LICENSE-Apache-2.0.txt' = "repo:tools/patch/licenses/OpenXR-SDK-LICENSE-Apache-2.0.txt"
}

function Resolve-Entry {
    param([string] $Spec)
    $kind, $rel = $Spec -split ':', 2
    switch ($kind) {
        'build' { return (Join-Path $BuildDir $rel) }
        'repo'  { return (Join-Path $repo     $rel) }
        'game'  { return (Join-Path $GameDir  $rel) }
        default { throw "manifest entry has unknown source kind '$kind'" }
    }
}

# --- Resolve everything BEFORE writing anything, and report every problem at once. ------------
$resolved = [ordered]@{}
$missing = @()
foreach ($zipPath in $manifest.Keys) {
    $src = Resolve-Entry $manifest[$zipPath]
    if (Test-Path -LiteralPath $src -PathType Leaf) { $resolved[$zipPath] = $src }
    else { $missing += "  {0,-44} <- {1}" -f $zipPath, $src }
}

if ($missing.Count -gt 0) {
    Write-Host "`nMissing $($missing.Count) file(s); nothing was written:`n" -ForegroundColor Red
    $missing | ForEach-Object { Write-Host $_ -ForegroundColor Red }
    Write-Host "`nA 'build:' miss means the Release build has not run. A 'game:' miss means"
    Write-Host "-GameDir does not point at a populated FreeFalcon 6 install.`n"
    exit 1
}

# --- Sanity on the build, because a stale exe is the failure nobody notices. -------------------
$exe = Get-Item (Join-Path $BuildDir 'FFViper.exe')
$newestSource = Get-ChildItem -Path (Join-Path $repo 'src') -Recurse -File -Include *.cpp, *.h |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1

if ($newestSource -and $newestSource.LastWriteTime -gt $exe.LastWriteTime) {
    Write-Host "FFViper.exe is older than $($newestSource.Name). Rebuild before releasing." -ForegroundColor Red
    exit 1
}

$head  = (git rev-parse --short HEAD).Trim()
$dirty = (git status --porcelain)
if ($dirty) { Write-Host "Working tree is dirty; the ZIP will not match $head exactly." -ForegroundColor Yellow }

# --- Stage and compress. -----------------------------------------------------------------------
$stage = Join-Path ([System.IO.Path]::GetTempPath()) ("ffviper-patch-" + [guid]::NewGuid().ToString('N'))
try {
    foreach ($zipPath in $resolved.Keys) {
        $dest = Join-Path $stage $zipPath
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $dest) | Out-Null
        Copy-Item -LiteralPath $resolved[$zipPath] -Destination $dest
    }

    New-Item -ItemType Directory -Force -Path (Join-Path $repo $OutDir) | Out-Null
    $zip = Join-Path $repo (Join-Path $OutDir "$Name.zip")
    if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip }

    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal

    Write-Host "`n$Name.zip  (HEAD $head)`n"
    foreach ($zipPath in $resolved.Keys) {
        $len = (Get-Item -LiteralPath $resolved[$zipPath]).Length
        Write-Host ("  {0,-44} {1,12:N0}" -f $zipPath, $len)
    }
    Write-Host ("`n  {0,-44} {1,12:N0}" -f 'archive', (Get-Item -LiteralPath $zip).Length)
    Write-Host "`n-> $zip`n"
}
finally {
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
}
