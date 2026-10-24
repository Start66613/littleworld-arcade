# Little World Arcade - read-only full-flash backup helper (chunked, retrying).
#
# This script ONLY reads the ESP32-P4 external flash. It contains no write,
# erase, OTA or eFuse path: the command whitelist below rejects anything else.
#
# Long single reads on this USB-serial link have proven unreliable, so the 32 MiB
# region is read in 4 MiB chunks with per-chunk retries, twice, and the two
# assembled images must match byte for byte.
#
# Usage (only after the user approved this exact operation):
#   pwsh -NoProfile -ExecutionPolicy Bypass -File tools/backup_flash.ps1 `
#        -Port COM5 -Authorized [-Baud 460800]
#
# -Baud defaults to 460800. Higher rates produced corrupted transfers on this
# CH343 link; lower rates are slower but more robust.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Port,
    [int]$Baud = 460800,
    [int]$ChunkBytes = 4194304,
    [string]$OutRoot = 'D:\Espressif\LittleWorld\private\backups\LittleWorld',
    [switch]$Authorized
)

$ErrorActionPreference = 'Continue'
$root = Split-Path $PSScriptRoot -Parent
$flashSize = [int64]33554432

if (-not $Authorized) {
    Write-Output 'REFUSED: this touches the physical board.'
    Write-Output 'Re-run with -Authorized only after the user approved this specific backup.'
    exit 2
}

$allowedCommands = @('flash_id', 'read_flash')

function Invoke-Esptool {
    param([string[]]$Arguments, [string]$Command)
    if ($allowedCommands -notcontains $Command) {
        throw "command '$Command' is not on the read-only whitelist"
    }
    & esptool.exe @Arguments
    return ($LASTEXITCODE -eq 0)
}

function Read-Region {
    param([string]$OutFile, [int]$BaudRate, [int]$Attempts = 3)
    if (Test-Path -LiteralPath $OutFile) { Remove-Item -LiteralPath $OutFile -Force }
    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        Write-Host ("    attempt {0}/{1} @ {2} baud" -f $attempt, $Attempts, $BaudRate)
        $ok = Invoke-Esptool -Arguments @(
            '--chip', 'esp32p4', '--port', $Port, '--baud', "$BaudRate",
            '--before', 'default_reset', '--after', 'hard_reset',
            'read_flash', '--flash_size', '32MB', '--no-progress',
            '0x0', "0x$('{0:X}' -f $flashSize)", $OutFile
        ) -Command 'read_flash'
        if ($ok -and (Test-Path -LiteralPath $OutFile) -and ((Get-Item -LiteralPath $OutFile).Length -eq $flashSize)) {
            return $true
        }
        Write-Host "    incomplete or failed; retrying"
    }
    return $false
}

function Read-ChunkedPass {
    param([string]$FinalFile, [int]$BaudRate)
    $chunkDir = Join-Path $workDir ("chunks-" + [System.IO.Path]::GetFileNameWithoutExtension($FinalFile))
    New-Item -ItemType Directory -Force -Path $chunkDir | Out-Null
    $count = [int][Math]::Ceiling($flashSize / $ChunkBytes)
    $chunkFiles = @()

    for ($i = 0; $i -lt $count; $i++) {
        $addr = [int64]$i * $ChunkBytes
        $size = [Math]::Min([int64]$ChunkBytes, $flashSize - $addr)
        $chunkFile = Join-Path $chunkDir ("chunk-{0:D2}.bin" -f $i)
        $chunkFiles += $chunkFile
        Write-Host ("  chunk {0}/{1}  offset=0x{2:X8}  size=0x{3:X}" -f ($i + 1), $count, $addr, $size)

        $done = $false
        for ($attempt = 1; $attempt -le 3 -and -not $done; $attempt++) {
            if (Test-Path -LiteralPath $chunkFile) { Remove-Item -LiteralPath $chunkFile -Force }
            $sw = [Diagnostics.Stopwatch]::StartNew()
            $ok = Invoke-Esptool -Arguments @(
                '--chip', 'esp32p4', '--port', $Port, '--baud', "$BaudRate",
                '--before', 'default_reset', '--after', 'hard_reset',
                'read_flash', '--flash_size', '32MB', '--no-progress',
                ("0x{0:X}" -f $addr), ("0x{0:X}" -f $size), $chunkFile
            ) -Command 'read_flash'
            $sw.Stop()
            if ($ok -and (Test-Path -LiteralPath $chunkFile) -and ((Get-Item -LiteralPath $chunkFile).Length -eq $size)) {
                $rate = if ($sw.Elapsed.TotalSeconds -gt 0) { ($size / 1KB) / $sw.Elapsed.TotalSeconds } else { 0 }
                Write-Host ("    ok  {0:N1}s  {1:N1} KB/s" -f $sw.Elapsed.TotalSeconds, $rate)
                $done = $true
            } else {
                Write-Host ("    failed after {0:N1}s (attempt {1}/3)" -f $sw.Elapsed.TotalSeconds, $attempt)
            }
        }
        if (-not $done) {
            Write-Host "  chunk failed three times; dropping baud for this chunk"
            $sw = [Diagnostics.Stopwatch]::StartNew()
            $ok = Invoke-Esptool -Arguments @(
                '--chip', 'esp32p4', '--port', $Port, '--baud', '230400',
                '--before', 'default_reset', '--after', 'hard_reset',
                'read_flash', '--flash_size', '32MB', '--no-progress',
                ("0x{0:X}" -f $addr), ("0x{0:X}" -f $size), $chunkFile
            ) -Command 'read_flash'
            $sw.Stop()
            if (-not ($ok -and (Test-Path -LiteralPath $chunkFile) -and ((Get-Item -LiteralPath $chunkFile).Length -eq $size))) {
                Write-Host "  CHUNK FAILED PERMANENTLY"
                return $false
            }
            Write-Host ("    ok at 230400  {0:N1}s" -f $sw.Elapsed.TotalSeconds)
        }
    }

    # Assemble.
    if (Test-Path -LiteralPath $FinalFile) { Remove-Item -LiteralPath $FinalFile -Force }
    $out = [System.IO.File]::Create($FinalFile)
    try {
        foreach ($chunkFile in $chunkFiles) {
            $in = [System.IO.File]::OpenRead($chunkFile)
            try { $in.CopyTo($out) } finally { $in.Dispose() }
        }
    }
    finally {
        $out.Dispose()
    }
    if ((Get-Item -LiteralPath $FinalFile).Length -ne $flashSize) {
        Write-Host "  assembled file has the wrong size"
        return $false
    }
    Remove-Item -LiteralPath $chunkDir -Recurse -Force
    return $true
}

. (Join-Path $PSScriptRoot 'activate_env.ps1') | Out-Null
$esptoolCommand = Get-Command esptool.exe -ErrorAction SilentlyContinue
if (-not $esptoolCommand) { throw 'esptool.exe is not on PATH after activation.' }

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$workDir = Join-Path $OutRoot $stamp
New-Item -ItemType Directory -Force -Path $workDir | Out-Null
$fileA = Join-Path $workDir 'p4-full-A.bin'
$fileB = Join-Path $workDir 'p4-full-B.bin'

Write-Host "port=$Port baud=$Baud chunk=$ChunkBytes"
Write-Host "output=$workDir"
Write-Host ''
Write-Host '--- step 1: chip identification (read only) ---'
$idOut = & esptool.exe --chip esp32p4 --port $Port --baud $Baud --before default_reset --after hard_reset flash_id 2>&1
$idOut | Set-Content -LiteralPath (Join-Path $workDir 'flash_id.txt') -Encoding UTF8
$idOut | Write-Host

$started = Get-Date
Write-Host ''
Write-Host '--- step 2: pass A (chunked read) ---'
if (-not (Read-ChunkedPass -FinalFile $fileA -BaudRate $Baud)) {
    Write-Host 'BACKUP FAILED during pass A.'
    exit 1
}
Write-Host ''
Write-Host '--- step 3: pass B (independent second read) ---'
if (-not (Read-ChunkedPass -FinalFile $fileB -BaudRate $Baud)) {
    Write-Host 'BACKUP FAILED during pass B; pass A alone is not a verified backup.'
    exit 1
}
$elapsed = (Get-Date) - $started

Write-Host ''
Write-Host '--- step 4: verification ---'
$sizeA = (Get-Item -LiteralPath $fileA).Length
$sizeB = (Get-Item -LiteralPath $fileB).Length
$hashA = (Get-FileHash -LiteralPath $fileA -Algorithm SHA256).Hash.ToLower()
$hashB = (Get-FileHash -LiteralPath $fileB -Algorithm SHA256).Hash.ToLower()
$ok = ($sizeA -eq $flashSize) -and ($sizeB -eq $flashSize) -and ($hashA -eq $hashB)

$manifest = [ordered]@{
    schema_version  = 1
    kind            = 'esp32p4-full-flash-backup'
    result          = if ($ok) { 'verified' } else { 'FAILED' }
    created         = (Get-Date -Format 'o')
    elapsed_seconds = [int]$elapsed.TotalSeconds
    port            = $Port
    baud            = $Baud
    chunk_bytes     = $ChunkBytes
    stub            = $true
    chip            = 'esp32p4'
    flash_bytes     = $flashSize
    files           = @(
        [ordered]@{ name = 'p4-full-A.bin'; bytes = $sizeA; sha256 = $hashA },
        [ordered]@{ name = 'p4-full-B.bin'; bytes = $sizeB; sha256 = $hashB }
    )
    note            = 'Private local backup (contains NVS / Wi-Fi credentials). Never commit or upload.'
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $workDir 'manifest.json') -Encoding UTF8

Write-Host ("A: {0} bytes  sha256={1}" -f $sizeA, $hashA)
Write-Host ("B: {0} bytes  sha256={1}" -f $sizeB, $hashB)
Write-Host ("elapsed: {0:N1} min" -f $elapsed.TotalMinutes)

if (-not $ok) {
    Write-Host 'BACKUP FAILED: size or hash mismatch. Do NOT treat this as a usable backup.'
    exit 1
}
Write-Host 'BACKUP VERIFIED (two independent reads agree).'
exit 0
