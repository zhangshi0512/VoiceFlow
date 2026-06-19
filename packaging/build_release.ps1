param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "build"

if (-not (Test-Path (Join-Path $Root "third_party\moonshine-voice-windows-x86_64\include\moonshine-cpp.h"))) {
    Write-Host "Fetching Moonshine dependencies..."
    & (Join-Path $PSScriptRoot "fetch_deps.ps1")
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Push-Location $BuildDir

cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config $Configuration
if ($LASTEXITCODE -ne 0) {
    Pop-Location
    throw "CMake build failed with exit code $LASTEXITCODE"
}

Pop-Location

$OutDir = Join-Path $BuildDir "$Configuration"
$ExePath = Join-Path $OutDir "VoiceFlow.exe"
if (-not (Test-Path $ExePath)) {
    throw "Build failed: $ExePath was not produced"
}
Write-Host "Built: $ExePath"
