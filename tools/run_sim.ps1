# Build and run the Little World Arcade host simulator.
#
# The simulator compiles the real arcade_ui + arcade_core + generated fonts
# against desktop LVGL 9.5 with a 1024x600 memory display, drives a scripted
# pointer through the whole UI and writes screenshots to reports/screenshots.
# It catches crashes and layout mistakes without touching the board.
#
# Usage: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run_sim.ps1 [-SkipBuild]
#        [-PillowPython <python.exe with Pillow>]

[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [string]$PillowPython = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $root 'build/host_sim'
$shotDir = Join-Path $root 'reports/screenshots'

# --- MSVC environment -------------------------------------------------------
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw "vswhere.exe not found: $vswhere" }
$install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'No Visual Studio installation with the C++ toolchain was found.' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$dump = cmd /c "`"$vcvars`" >nul 2>&1 && set"
foreach ($line in $dump) {
    if ($line -match '^([^=]+)=(.*)$') {
        Set-Item -Path ("env:" + $Matches[1]) -Value $Matches[2]
    }
}

# --- ESP-IDF env (for cmake/ninja on PATH) ----------------------------------
. (Join-Path $PSScriptRoot 'activate_env.ps1') | Out-Null

if (-not $SkipBuild) {
    if (-not (Test-Path (Join-Path $buildDir 'build.ninja'))) {
        Write-Host 'configuring host simulator...'
        & cmake -S (Join-Path $root 'host_sim') -B $buildDir -G Ninja -DCMAKE_BUILD_TYPE=Release | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "cmake configure failed ($LASTEXITCODE)" }
    }
    Write-Host 'building host simulator...'
    & ninja -C $buildDir | Out-Null
    if ($LASTEXITCODE -ne 0) {
        & ninja -C $buildDir | Select-String -Pattern 'error|LNK' | ForEach-Object { $_.Line }
        throw "simulator build failed ($LASTEXITCODE)"
    }
}

New-Item -ItemType Directory -Force -Path $shotDir | Out-Null
Get-ChildItem $shotDir -File | Remove-Item -Force
Push-Location $root
try {
    & (Join-Path $buildDir 'arcade_sim.exe')
    $code = $LASTEXITCODE
}
finally {
    Pop-Location
}
if ($code -ne 0) { throw "simulator run failed ($code)" }

# --- BMP -> PNG (optional, needs Pillow) ------------------------------------
if ($PillowPython -eq '') {
    foreach ($candidate in @('python', (Join-Path $env:ProgramFiles 'Python311\python.exe'))) {
        if (Get-Command $candidate -ErrorAction SilentlyContinue) {
            & $candidate -c 'import PIL' 2>$null
            if ($LASTEXITCODE -eq 0) { $PillowPython = $candidate; break }
        }
    }
}
if ($PillowPython -ne '') {
    & $PillowPython (Join-Path $PSScriptRoot 'bmp_to_png.py') $shotDir
    if ($LASTEXITCODE -ne 0) { Write-Warning 'BMP to PNG conversion reported an error' }
} else {
    Write-Warning 'Pillow not found; screenshots stay as .bmp (pass -PillowPython <python.exe>)'
}

Get-ChildItem $shotDir | Select-Object Name, Length | Format-Table -AutoSize
Write-Output "screenshots: $shotDir"
