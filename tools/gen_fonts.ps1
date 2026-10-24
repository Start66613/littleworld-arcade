# Generate the Little World Arcade UI font from the Noto Sans SC subset OTF.
#
# Why: LVGL's built-in fonts have no CJK glyphs, and the CJK fonts bundled inside
# LVGL only cover a fixed character list that does not match our UI strings.
# We therefore scan our own sources for every non-ASCII character we actually use
# and rasterise exactly those glyphs, which keeps the firmware small and makes a
# missing glyph a build-time error instead of a box on the screen.
#
# Requirements (already installed locally): node + lv_font_conv, and the OTF files
# in tools/fonts/cache (git-ignored, see tools/fonts/README.md).
#
# Usage: pwsh -NoProfile -File tools/gen_fonts.ps1 [-Check]
#   -Check : only report whether the generated files are up to date (exit 1 if not)

[CmdletBinding()]
param(
    [switch]$Check
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$fontDir = Join-Path $projectRoot 'tools/fonts'
$outDir = Join-Path $projectRoot 'firmware/components/arcade_ui/fonts'
$regular = Join-Path $fontDir 'cache/NotoSansSC-Regular.otf'
$bold = Join-Path $fontDir 'cache/NotoSansSC-Bold.otf'

foreach ($p in @($regular, $bold)) {
    if (-not (Test-Path -LiteralPath $p)) { throw "Font source missing: $p (see tools/fonts/README.md)" }
}

# 1. Collect characters: every non-ASCII character used in the firmware sources,
#    plus an explicit baseline list (digits, punctuation, symbols used at runtime).
$chars = New-Object System.Text.StringBuilder
$sources = Get-ChildItem -Path (Join-Path $projectRoot 'firmware') -Recurse -File -Include *.c, *.h |
    Where-Object { $_.FullName -notmatch '[\\/](build|managed_components|fonts)[\\/]' }

foreach ($file in $sources) {
    $text = [System.IO.File]::ReadAllText($file.FullName, [System.Text.Encoding]::UTF8)
    foreach ($runes in $text.ToCharArray()) {
        if ([int]$runes -ge 0x20 -and [int]$runes -lt 0x7F) { continue }
        if ([int]$runes -lt 0x20) { continue }
        [void]$chars.Append($runes)
    }
}
$baseline = Join-Path $fontDir 'baseline_chars.txt'
if (Test-Path -LiteralPath $baseline) {
    $baseText = [System.IO.File]::ReadAllText($baseline, [System.Text.Encoding]::UTF8)
    [void]$chars.Append([string]($baseText -replace '\s', ''))
}

# Unique + sorted for a stable output file (important for reproducible builds).
$unique = ($chars.ToString().ToCharArray() | Sort-Object -Unique) -join ''
# Keep only printable non-ASCII BMP characters: ASCII comes from --range 0x20-0x7E,
# and dropping quotes/backslashes keeps the argument list free of shell quoting hazards.
$unique = ($unique.ToCharArray() | Where-Object { [int]$_ -ge 0x7F -and [int]$_ -le 0xFFFF }) -join ''
Write-Output ("glyph set: {0} non-ASCII characters" -f $unique.Length)

$jobs = @(
    @{ Name = 'arcade_font_22'; Size = 22; Bpp = 4; Font = $regular; Label = 'Regular 22 (CJK + Latin)'; Symbols = $unique },
    @{ Name = 'arcade_font_28'; Size = 28; Bpp = 4; Font = $bold; Label = 'Bold 28 (CJK + Latin)'; Symbols = $unique },
    # 44 px is only used for the Latin wordmark and the 2048 tile numbers, so it
    # deliberately carries no CJK glyphs (keeps ~200 KB out of the image).
    @{ Name = 'arcade_font_44'; Size = 44; Bpp = 4; Font = $bold; Label = 'Bold 44 (Latin + digits)'; Symbols = '' }
)

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
# Call node.exe with the JS entry point directly: going through the .cmd shim would
# route the UTF-8 glyph list through cmd.exe's code page and corrupt it.
$lvFontConvJs = Join-Path $fontDir 'node_modules/lv_font_conv/lv_font_conv.js'
if (-not (Test-Path -LiteralPath $lvFontConvJs)) { throw "lv_font_conv is not installed in $fontDir (run: npm install lv_font_conv)" }

$stale = @()
foreach ($job in $jobs) {
    $out = Join-Path $outDir ($job.Name + '.c')
    $target = $out
    if ($Check) {
        $target = Join-Path ([System.IO.Path]::GetTempPath()) ($job.Name + '.check.c')
    }
    $fontArgs = @(
        $lvFontConvJs,
        '--font', $job.Font,
        '--range', '0x20-0x7E'
    )
    if ($job.Symbols -ne '') {
        $fontArgs += @('--symbols', $job.Symbols)
    }
    $fontArgs += @(
        '--size', $job.Size,
        '--bpp', $job.Bpp,
        '--format', 'lvgl',
        '--no-compress',
        '--lv-include', 'lvgl.h',
        '--output', $target
    )
    if ($Check) {
        & node @fontArgs
        if ($LASTEXITCODE -ne 0) { throw "lv_font_conv failed for $($job.Name)" }
        if (-not (Test-Path $out) -or (Get-FileHash $out).Hash -ne (Get-FileHash $target).Hash) { $stale += $job.Name }
        Remove-Item -LiteralPath $target -Force
        continue
    }
    Write-Output ("generating {0} ({1}, {2}px)..." -f $job.Name, $job.Label, $job.Size)
    & node @fontArgs
    if ($LASTEXITCODE -ne 0) { throw "lv_font_conv failed for $($job.Name)" }
    $size = (Get-Item -LiteralPath $out).Length
    Write-Output ("  -> {0} ({1:N0} bytes)" -f $out, $size)
}

if ($Check) {
    if ($stale.Count -gt 0) {
        Write-Output ("STALE FONT OUTPUTS: " + ($stale -join ', '))
        exit 1
    }
    Write-Output 'font outputs are up to date'
}

Copy-Item -LiteralPath (Join-Path $fontDir 'cache/OFL.txt') -Destination (Join-Path $outDir 'OFL.txt') -Force
Write-Output 'done.'
