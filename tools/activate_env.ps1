# Dot-source this file to activate the pinned ESP-IDF v5.5.5 environment in the current process.
# It never installs, upgrades or writes into the ESP-IDF / tools directories.
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$lockPath = Join-Path $PSScriptRoot 'env.lock.json'
if (-not (Test-Path -LiteralPath $lockPath)) { throw "Environment lock missing: $lockPath" }
$lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json

foreach ($name in 'idf_path', 'idf_tools_path', 'python_env', 'python_base', 'component_cache') {
    $value = $lock.$name
    if (-not (Test-Path -LiteralPath $value)) { throw "Environment lock path does not exist ($name): $value" }
}
if ($lock.idf_version -ne '5.5.5' -or $lock.target -ne 'esp32p4') { throw 'Unexpected environment baseline.' }

$env:IDF_PATH = $lock.idf_path
$env:IDF_TOOLS_PATH = $lock.idf_tools_path
$env:IDF_PYTHON_ENV_PATH = $lock.python_env
$env:IDF_COMPONENT_CACHE_PATH = $lock.component_cache
$env:TEMP = Join-Path $lock.tools_root 'temp'
$env:TMP = $env:TEMP
$env:PIP_CACHE_DIR = Join-Path $lock.tools_root 'cache/pip'
$env:XDG_CACHE_HOME = Join-Path $lock.tools_root 'cache/xdg'
$env:PYTHONUTF8 = '1'
$env:PYTHONIOENCODING = 'utf-8'
$env:GIT_TERMINAL_PROMPT = '0'
$env:GCM_INTERACTIVE = 'Never'

# Reuse the Windows system proxy if one is configured (same approach as the formal project).
if (-not $env:HTTPS_PROXY) {
    $destination = [uri]'https://github.com'
    $proxyUri = [Net.WebRequest]::GetSystemWebProxy().GetProxy($destination)
    if ($proxyUri.AbsoluteUri -ne $destination.AbsoluteUri) {
        $env:HTTPS_PROXY = $proxyUri.AbsoluteUri
        $env:HTTP_PROXY = $proxyUri.AbsoluteUri
    }
}

$exportLines = @(& $lock.python_base (Join-Path $env:IDF_PATH 'tools/idf_tools.py') export --format key-value)
if ($LASTEXITCODE -ne 0) { throw 'Official ESP-IDF environment export failed.' }
foreach ($line in $exportLines) {
    if ($line -match '^([A-Z][A-Z0-9_]*)=(.*)$') {
        $variableName = $Matches[1]
        $variableValue = $Matches[2].Replace('%PATH%', $env:PATH).Replace('$PATH', $env:PATH)
        [Environment]::SetEnvironmentVariable($variableName, $variableValue, 'Process')
    }
}
$env:PATH = (Join-Path $lock.python_env 'Scripts') + ';' + $env:PATH + ';' + (Join-Path $env:IDF_PATH 'tools')

# Project defaults: base + chip-revision profile (v3.x family).
$env:SDKCONFIG_DEFAULTS = 'sdkconfig.defaults;sdkconfig.defaults.rev3_x'
Write-Output "Little World Arcade: ESP-IDF $($lock.idf_version) active (target $($lock.target), profile $($lock.compilation_profile)). No device action performed."
