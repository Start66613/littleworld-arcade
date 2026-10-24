# Little World Arcade - read-only full-flash backup helper (chunked, resumable).
#
# This script ONLY reads the ESP32-P4 external flash. It contains no write,
# erase, OTA or eFuse path: the command whitelist below rejects anything else.
#
# Long single reads over this CH343 USB-serial link stall or corrupt, so the
# 32 MiB region is read in 4 MiB chunks with a hard per-chunk timeout, retries,
# a baud-rate fallback, and resume support (existing chunks are reused).
# Two independent passes must produce identical SHA-256 hashes.
#
# Usage (only after the user approved this specific operation):
#   pwsh -NoProfile -ExecutionPolicy Bypass -File tools/backup_flash.ps1 `
#        -Port COM5 -Authorized [-Baud 460800] [-WorkDir <dir>]

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Port,
    [int]$Baud = 460800,
    [int]$ChunkBytes = 4194304,
    [string]$WorkDir = '',
    [string]$OutRoot = 'D:\Espressif\LittleWorld\private\backups\LittleWorld',
    [switch]$Authorized
)

$ErrorActionPreference = 'Continue'
$flashSize = [int64]33554432
$script:currentBaud = $Baud
$script:esptool = $null

if (-not $Authorized) {
    Write-Output 'REFUSED: this touches the physical board.'
    Write-Output 'Re-run with -Authorized only after the user approved this specific backup.'
    exit 2
}

$allowedCommands = @('flash_id', 'read_flash')

function Invoke-EsptoolTimed {
    param([string[]]$Arguments, [string]$Command, [int]$TimeoutSeconds)
    if ($allowedCommands -notcontains $Command) {
        throw "command '$Command' is not on the read-only whitelist"
    }
    $stdout = Join-Path $workDir 'esptool.stdout.txt'
    $stderr = Join-Path $workDir 'esptool.stderr.txt'
    if (Test-Path $stdout) { Remove-Item $stdout -Force }
    if (Test-Path $stderr) { Remove-Item $stderr -Force }
    $proc = Start-Process -FilePath $script:esptool -ArgumentList $Arguments -NoNewWindow -PassThru `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $lastSize = -1
    $lastChange = Get-Date
    while (-not $proc.HasExited) {
        $now = Get-Date
        if ($now -gt $deadline) {
            try { $proc.Kill() } catch { }
            Write-Host ("    TIMEOUT after {0}s" -f $TimeoutSeconds)
            return $false
        }
        $size = if (Test-Path $stdout) { (Get-Item $stdout).Length } else { 0 }
        if ($size -ne $lastSize) {
            $lastSize = $size
            $lastChange = $now
        } elseif (($now - $lastChange).TotalSeconds -gt 90) {
            try { $proc.Kill() } catch { }
            Write-Host "    STALLED (no progress for 90s), killed"
            return $false
        }
        Start-Sleep -Milliseconds 700
    }
    $proc.WaitForExit()
    # NOTE: PowerShell 5.1 Start-Process -PassThru does not populate ExitCode in this
    # environment (verified), so success is decided by the caller via the output file.
    # A non-zero exit code is still honoured when Windows does report one.
    if ($null -ne $proc.ExitCode -and $proc.ExitCode -ne 0) {
        Write-Host ("    esptool exit {0}" -f $proc.ExitCode)
        if (Test-Path $stderr) {
            Get-Content $stderr -Tail 3 | ForEach-Object { Write-Host ("      " + $_) }
        }
        return $false
    }
    if ((Test-Path $stderr) -and (Get-Item $stderr).Length -gt 0) {
        Get-Content $stderr -Tail 2 | ForEach-Object { Write-Host ("      stderr: " + $_) }
    }
    return $true
}

# A single ROM session reliably transfers about 3.5 MiB before the link stalls,
# independent of baud rate, so every session is capped well below that.
$script:maxPerSession = 2097152

function Read-Segment {
    param([int64]$Address, [int64]$Size, [string]$File)
    for ($attempt = 1; $attempt -le 4; $attempt++) {
        if ($attempt -eq 4) {
            $script:currentBaud = 230400
            Write-Host "    falling back to 230400 baud"
        }
        if ($attempt -gt 1) {
            # A killed read can leave the stub running at a non-default baud; force a
            # clean ROM session before trying the data again.
            Write-Host "    recovering link (flash_id reset)"
            [void](Invoke-EsptoolTimed -Arguments @(
                '--chip', 'esp32p4', '--port', $Port, '--baud', '115200',
                '--before', 'default_reset', '--after', 'hard_reset', 'flash_id'
            ) -Command 'flash_id' -TimeoutSeconds 40)
            Start-Sleep -Seconds 1
        }

        $expected = [int]($Size * 10 / $script:currentBaud)
        $timeout = [int]($expected * 2 + 60)
        if (Test-Path -LiteralPath $File) { Remove-Item -LiteralPath $File -Force }

        $sw = [Diagnostics.Stopwatch]::StartNew()
        $ok = Invoke-EsptoolTimed -Arguments @(
            '--chip', 'esp32p4', '--port', $Port, '--baud', "$($script:currentBaud)",
            '--before', 'default_reset', '--after', 'hard_reset',
            'read_flash', '--flash_size', '32MB',
            ("0x{0:X}" -f $Address), ("0x{0:X}" -f $Size), $File
        ) -Command 'read_flash' -TimeoutSeconds $timeout
        $sw.Stop()

        if ($ok -and (Test-Path -LiteralPath $File) -and ((Get-Item -LiteralPath $File).Length -eq $Size)) {
            $rate = if ($sw.Elapsed.TotalSeconds -gt 0) { ($Size / 1KB) / $sw.Elapsed.TotalSeconds } else { 0 }
            Write-Host ("    ok  {0:N1}s  {1:N1} KB/s" -f $sw.Elapsed.TotalSeconds, $rate)
            return $true
        }
        Write-Host ("    attempt {0} failed after {1:N1}s" -f $attempt, $sw.Elapsed.TotalSeconds)
        Start-Sleep -Seconds 2
    }
    return $false
}

function Read-Chunk {
    param([int64]$Address, [int64]$Size, [string]$File)
    if ((Test-Path -LiteralPath $File) -and ((Get-Item -LiteralPath $File).Length -eq $Size)) {
        Write-Host "    already present, reusing"
        return $true
    }

    $parts = [int][Math]::Ceiling($Size / $script:maxPerSession)
    $partFiles = @()
    for ($p = 0; $p -lt $parts; $p++) {
        $partAddr = $Address + [int64]$p * $script:maxPerSession
        $partSize = [Math]::Min([int64]$script:maxPerSession, $Address + $Size - $partAddr)
        $partFile = "$File.part{0:D2}" -f $p
        $partFiles += $partFile
        if ($parts -gt 1) {
            Write-Host ("    part {0}/{1}  addr=0x{2:X8} size=0x{3:X}" -f ($p + 1), $parts, $partAddr, $partSize)
        }
        if (-not (Read-Segment -Address $partAddr -Size $partSize -File $partFile)) {
            foreach ($f in $partFiles) { if (Test-Path $f) { Remove-Item $f -Force } }
            return $false
        }
    }

    if ($parts -eq 1) {
        Move-Item -LiteralPath $partFiles[0] -Destination $File -Force
        return $true
    }
    if (Test-Path -LiteralPath $File) { Remove-Item -LiteralPath $File -Force }
    $out = [System.IO.File]::Create($File)
    try {
        foreach ($partFile in $partFiles) {
            $in = [System.IO.File]::OpenRead($partFile)
            try { $in.CopyTo($out) } finally { $in.Dispose() }
        }
    }
    finally {
        $out.Dispose()
    }
    foreach ($partFile in $partFiles) { Remove-Item -LiteralPath $partFile -Force }
    return ((Get-Item -LiteralPath $File).Length -eq $Size)
}

function Read-ChunkedPass {
    param([string]$FinalFile)
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
        if (-not (Read-Chunk -Address $addr -Size $size -File $chunkFile)) {
            Write-Host "  CHUNK FAILED PERMANENTLY"
            return $false
        }
    }

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
$script:esptool = $esptoolCommand.Source

if ($WorkDir -eq '') {
    $WorkDir = Join-Path $OutRoot (Get-Date -Format 'yyyyMMdd-HHmmss')
}
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null
$workDir = (Resolve-Path -LiteralPath $WorkDir).Path
$fileA = Join-Path $workDir 'p4-full-A.bin'
$fileB = Join-Path $workDir 'p4-full-B.bin'

Write-Host "port=$Port baud=$Baud chunk=$ChunkBytes"
Write-Host "output=$workDir"
Write-Host ''
Write-Host '--- step 1: chip identification (read only) ---'
$idOut = & $script:esptool --chip esp32p4 --port $Port --baud $Baud --before default_reset --after hard_reset flash_id 2>&1
$idOut | Set-Content -LiteralPath (Join-Path $workDir 'flash_id.txt') -Encoding UTF8
$idOut | Write-Host

$started = Get-Date
Write-Host ''
Write-Host '--- step 2: pass A (chunked read) ---'
if (-not (Read-ChunkedPass -FinalFile $fileA)) {
    Write-Host 'BACKUP FAILED during pass A.'
    exit 1
}
Write-Host ''
Write-Host '--- step 3: pass B (independent second read) ---'
if (-not (Read-ChunkedPass -FinalFile $fileB)) {
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
