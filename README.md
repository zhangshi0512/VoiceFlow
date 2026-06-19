# VoiceFlow

Local speech dictation for Windows. Hold **Ctrl+Shift+Space** (push-to-talk) or toggle the same chord to dictate into the focused application.

## Features (MVP)

- System tray resident app (no browser)
- Push-to-talk and toggle hotkey modes
- Final-sentence injection into the focused window
- English and Chinese via Moonshine Voice (local inference)
- First-run setup for runtime + language models

## Prerequisites

- Windows 11 x64
- [Visual Studio 2022](https://visualstudio.microsoft.com/) with **Desktop development with C++**
- CMake 3.20+
- Python 3.10+ (for first-run model download script)

## Build

```powershell
powershell -ExecutionPolicy Bypass -File packaging\build_release.ps1
```

By default the build uses the **Python ASR worker** (`moonshine-voice` subprocess), which avoids MSVC/Moonshine static-library version mismatches on older Visual Studio installs.

To use the native **Moonshine C++** backend instead (requires a recent VS 2022 toolset):

```powershell
cmake -S . -B build -DVOICEFLOW_USE_PYTHON_ASR=OFF
cmake --build build --config Release
```

Output: `build\Release\VoiceFlow.exe` (plus `build\Release\packaging\voiceflow_asr_worker.py`)

## First-run model setup

```powershell
pip install moonshine-voice
powershell -ExecutionPolicy Bypass -File packaging\download_models.ps1
```

Models are copied to `%LOCALAPPDATA%\VoiceFlow\models\en` and `\zh`.

## Usage

1. Launch `VoiceFlow.exe`
2. Complete first-run setup if prompted
3. Focus any text field
4. Hold **Ctrl+Shift+Space**, speak, release — text is injected on release
5. Or switch to **Toggle** mode in the tray settings dialog

## Project layout

See [doc/DESIGN_DOC.md](doc/DESIGN_DOC.md) for architecture and product decisions.

## License

MIT — see [LICENSE](LICENSE).
