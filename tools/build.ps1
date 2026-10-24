# Build the Little World Arcade firmware for the Waveshare ESP32-P4-WIFI6-Touch-LCD-7B.
#
# Usage:
#   pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1            # incremental
#   pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 -Clean     # sdkconfig + build
#   pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 -ImageInfo # also dump image headers
#
# The build directory lives outside firmware/ (D:\LittleWorld-Arcade\build) so the
# source tree stays clean and C: is never touched.

[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$ImageInfo
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$firmware = Join-Path $root 'firmware'
$buildDir = Join-Path $root 'build'
$logDir = Join-Path $root 'reports/logs'
New-Item -ItemType Directory -Force -Path $logDir, $buildDir | Out-Null

. (Join-Path $PSScriptRoot 'activate_env.ps1') | Out-Null
if (-not $env:IDF_PATH) { throw 'ESP-IDF activation failed.' }

$idfPy = Join-Path $env:IDF_PATH 'tools/idf.py'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logFile = Join-Path $logDir ("build-$stamp.log")

Push-Location $firmware
try {
    if ($Clean) {
        Write-Output 'cleaning sdkconfig and build directory...'
        foreach ($path in @((Join-Path $firmware 'sdkconfig'), (Join-Path $firmware 'sdkconfig.old'))) {
            if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
        }
        if (Test-Path -LiteralPath $buildDir) {
            Remove-Item -LiteralPath $buildDir -Recurse -Force
        }
        New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
        & python $idfPy -B $buildDir set-target esp32p4 *>&1 | Tee-Object -FilePath $logFile
        if ($LASTEXITCODE -ne 0) { throw "set-target failed ($LASTEXITCODE)" }
    } elseif (-not (Test-Path -LiteralPath (Join-Path $firmware 'sdkconfig'))) {
        & python $idfPy -B $buildDir set-target esp32p4 *>&1 | Tee-Object -FilePath $logFile
        if ($LASTEXITCODE -ne 0) { throw "set-target failed ($LASTEXITCODE)" }
    }

    & python $idfPy -B $buildDir build *>&1 | Tee-Object -FilePath $logFile -Append
    if ($LASTEXITCODE -ne 0) { throw "build failed ($LASTEXITCODE)" }
}
finally {
    Pop-Location
}

Write-Output ''
Write-Output '--- firmware artefacts ---'
$artifacts = @(
    (Join-Path $buildDir 'little_world_arcade.bin'),
    (Join-Path $buildDir 'bootloader/bootloader.bin'),
    (Join-Path $buildDir 'partition_table/partition-table.bin')
)
foreach ($file in $artifacts) {
    if (Test-Path -LiteralPath $file) {
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file).Hash.ToLower()
        $size = (Get-Item -LiteralPath $file).Length
        Write-Output ("{0,-58} {1,9} bytes  sha256={2}" -f (Split-Path $file -Leaf), $size, $hash)
    } else {
        Write-Output ("MISSING: {0}" -f $file)
    }
}

if ($ImageInfo) {
    Write-Output ''
    Write-Output '--- image headers (esptool image_info) ---'
    foreach ($file in @((Join-Path $buildDir 'little_world_arcade.bin'), (Join-Path $buildDir 'bootloader/bootloader.bin'))) {
        if (Test-Path -LiteralPath $file) {
            & python -m esptool --chip esp32p4 image_info $file 2>&1 | Select-String -Pattern 'Chip ID|Chip revision|Flash size|Entry point|Segments|Checksum' | ForEach-Object { $_.Line.Trim() }
        }
    }
}

Write-Output ''
Write-Output "log: $logFile"
Write-Output 'LOCAL BUILD PASSED. BOARD TEST: NOT PERFORMED.'
