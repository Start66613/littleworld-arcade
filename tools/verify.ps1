# Run every local check for Little World Arcade and print one summary.
#
#   1. font coverage   - every Chinese character used by the UI exists in the subset fonts
#   2. host unit tests - arcade_core rules compiled with MSVC and executed
#   3. image headers   - the built images target esp32p4 and accept chip revision v3.2
#
# Exit code 0 only when all three pass. This does not touch the board.
#
# Usage: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/verify.ps1

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root 'build'
$failures = @()
$results = @()

# python (esptool / check scripts) only resolves correctly inside the pinned env.
. (Join-Path $PSScriptRoot 'activate_env.ps1') | Out-Null

Write-Output '=== 1/3 font coverage ==='
& python (Join-Path $PSScriptRoot 'check_fonts.py')
if ($LASTEXITCODE -ne 0) { $failures += 'font coverage'; $results += 'font coverage  : FAIL' }
else { $results += 'font coverage  : PASS' }

Write-Output ''
Write-Output '=== 2/3 host unit tests ==='
& (Join-Path $PSScriptRoot 'run_host_tests.ps1')
if ($LASTEXITCODE -ne 0) { $failures += 'host tests'; $results += 'host tests     : FAIL' }
else { $results += 'host tests     : PASS' }

Write-Output ''
Write-Output '=== 3/3 image headers ==='
$app = Join-Path $build 'little_world_arcade.bin'
$boot = Join-Path $build 'bootloader/bootloader.bin'
$table = Join-Path $build 'partition_table/partition-table.bin'
if (-not (Test-Path $app)) {
    Write-Output 'build artefacts missing; run tools/build.ps1 first'
    $failures += 'image headers'
    $results += 'image headers  : SKIPPED (no build)'
} else {
    & python (Join-Path $PSScriptRoot 'image_check.py') --expect-chip esp32p4 --expect-rev 302 --json (Join-Path $root 'reports/image-headers.json') $app $boot $table
    if ($LASTEXITCODE -ne 0) { $failures += 'image headers'; $results += 'image headers  : FAIL' }
    else { $results += 'image headers  : PASS' }
}

Write-Output ''
Write-Output '================ SUMMARY ================'
$results | ForEach-Object { Write-Output $_ }
if ($failures.Count -gt 0) {
    Write-Output ('VERIFY FAILED: ' + ($failures -join ', '))
    exit 1
}
Write-Output 'VERIFY PASSED (local checks only; board not touched)'
exit 0
