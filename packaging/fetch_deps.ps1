# Downloads Moonshine Voice Windows x64 libraries into third_party/
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$ThirdParty = Join-Path $Root "third_party"
$MoonshineDir = Join-Path $ThirdParty "moonshine-voice-windows-x86_64"
$Archive = Join-Path $ThirdParty "moonshine-voice-windows-x86_64.tar.gz"
$Url = "https://github.com/moonshine-ai/moonshine/releases/latest/download/moonshine-voice-windows-x86_64.tar.gz"

New-Item -ItemType Directory -Force -Path $ThirdParty | Out-Null

if (Test-Path $MoonshineDir) {
    Write-Host "Moonshine libs already present at $MoonshineDir"
    exit 0
}

Write-Host "Downloading Moonshine Voice Windows libraries..."
Invoke-WebRequest -Uri $Url -OutFile $Archive -UseBasicParsing

Write-Host "Extracting..."
tar -xzf $Archive -C $ThirdParty
Remove-Item $Archive

if (-not (Test-Path (Join-Path $MoonshineDir "include\moonshine-cpp.h"))) {
    throw "Extraction failed: moonshine-cpp.h not found"
}

Write-Host "Done: $MoonshineDir"
