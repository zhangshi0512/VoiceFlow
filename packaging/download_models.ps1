# Downloads Moonshine EN/ZH models into %LOCALAPPDATA%\VoiceFlow\models\
$ErrorActionPreference = "Stop"

$ModelsRoot = Join-Path $env:LOCALAPPDATA "VoiceFlow\models"
New-Item -ItemType Directory -Force -Path (Join-Path $ModelsRoot "en") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $ModelsRoot "zh") | Out-Null

function Ensure-MoonshineVoice {
    python -c "import moonshine_voice" 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Installing moonshine-voice..."
        python -m pip install --upgrade moonshine-voice
    }
}

Ensure-MoonshineVoice

$env:MOONSHINE_VOICE_CACHE = Join-Path $env:LOCALAPPDATA "VoiceFlow\moonshine_cache"

Write-Host "Downloading English model..."
python -m moonshine_voice.download --language en

Write-Host "Downloading Chinese model..."
python -m moonshine_voice.download --language zh

function Copy-Model($Language, $ArchFolder) {
    $cacheRoot = Join-Path $env:MOONSHINE_VOICE_CACHE "download.moonshine.ai\model"
    if (-not (Test-Path $cacheRoot)) {
        throw "Moonshine cache not found at $cacheRoot"
    }

    $match = Get-ChildItem -Path $cacheRoot -Recurse -Directory |
        Where-Object { $_.FullName -match $ArchFolder } |
        Select-Object -First 1

    if (-not $match) {
        throw "Could not locate model folder matching '$ArchFolder' for $Language"
    }

    $target = Join-Path $ModelsRoot $Language
    Write-Host "Copying $Language model from $($match.FullName) to $target"
    Copy-Item -Path (Join-Path $match.FullName '*') -Destination $target -Recurse -Force
}

Copy-Model "en" "tiny.*streaming|tiny-streaming"
Copy-Model "zh" "mandarin|zh"

Write-Host "Models installed under $ModelsRoot"
