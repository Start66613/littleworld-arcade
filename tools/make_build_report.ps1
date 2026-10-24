# Regenerate reports/BUILD_REPORT.md from the current build artefacts.
#
# The script itself is ASCII-only on purpose: Windows PowerShell 5.1 reads
# non-BOM .ps1 files using the ANSI code page. The Chinese template lives in
# tools/templates/build_report.md.tmpl and is read/written as UTF-8.
#
# Usage: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/make_build_report.ps1

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root 'build'
$reportPath = Join-Path $root 'reports/BUILD_REPORT.md'
$headerJson = Join-Path $root 'reports/image-headers.json'
$templatePath = Join-Path $PSScriptRoot 'templates/build_report.md.tmpl'
$utf8 = New-Object System.Text.UTF8Encoding($false)

if (-not (Test-Path -LiteralPath $templatePath)) { throw "missing template: $templatePath" }

$artefacts = @(
    @{ Label = 'application'; Path = (Join-Path $build 'little_world_arcade.bin') },
    @{ Label = 'bootloader'; Path = (Join-Path $build 'bootloader/bootloader.bin') },
    @{ Label = 'partition table'; Path = (Join-Path $build 'partition_table/partition-table.bin') }
)

$rows = @()
foreach ($item in $artefacts) {
    if (-not (Test-Path -LiteralPath $item.Path)) { throw "missing artefact: $($item.Path)" }
    $info = Get-Item -LiteralPath $item.Path
    $hash = (Get-FileHash -LiteralPath $item.Path -Algorithm SHA256).Hash.ToLower()
    $rows += '| `' + $info.Name + '` | ' + $info.Length + ' B | `' + $hash + '` |'
}

$headers = ''
if (Test-Path -LiteralPath $headerJson) {
    $data = Get-Content -LiteralPath $headerJson -Raw -Encoding UTF8 | ConvertFrom-Json
    foreach ($entry in $data) {
        if ($entry.kind -eq 'partition_table') { continue }
        $minMajor = [math]::Floor($entry.min_chip_rev_full / 100)
        $minMinor = $entry.min_chip_rev_full % 100
        $maxMajor = [math]::Floor($entry.max_chip_rev_full / 100)
        $maxMinor = $entry.max_chip_rev_full % 100
        $headers += ('- `{0}`: chip **{1}**, revision **v{2}.{3} .. v{4}.{5}**, flash {6}' -f `
                $entry.name, $entry.chip, $minMajor, $minMinor, $maxMajor, $maxMinor, $entry.flash_size) + "`n"
    }
    foreach ($entry in $data) {
        if ($entry.kind -ne 'partition_table') { continue }
        $headers += "`nPartition table:`n"
        foreach ($part in $entry.entries) {
            $headers += ('  - partition `{0}` offset 0x{1:X6} size 0x{2:X6}' -f $part.name, $part.offset, $part.size) + "`n"
        }
    }
}

Push-Location $root
try {
    $commit = (git rev-parse --short HEAD 2>$null)
    $dirtyCount = (git status --porcelain 2>$null | Measure-Object).Count
}
finally {
    Pop-Location
}
$dirty = ''
if ($dirtyCount -gt 0) {
    $dirty = ' (working tree has ' + $dirtyCount + ' uncommitted change(s))'
}

$template = [System.IO.File]::ReadAllText($templatePath, [System.Text.Encoding]::UTF8)
$content = $template.Replace('{{DATE}}', (Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')).
    Replace('{{COMMIT}}', $commit).
    Replace('{{DIRTY}}', $dirty).
    Replace('{{ARTEFACTS}}', ($rows -join "`n")).
    Replace('{{HEADERS}}', $headers.TrimEnd())

[System.IO.File]::WriteAllText($reportPath, $content, $utf8)
Write-Output "wrote $reportPath"
