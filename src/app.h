#pragma once

#include <windows.h>

#include <functional>
#include <string>

namespace voiceflow {

enum class HotkeyMode { PushToTalk, Toggle };

enum class Language { English, Chinese };

struct Settings {
    HotkeyMode hotkey_mode = HotkeyMode::PushToTalk;
    Language language = Language::English;
    bool prefer_gpu = true;
    bool force_cpu = false;
};

class VoiceFlowApp {
 public:
    VoiceFlowApp();
    ~VoiceFlowApp();

    int Run(HINSTANCE instance);

    void StartDictation();
    void StopDictation();

 private:
    bool CreateMessageWindow(HINSTANCE instance);
    void RegisterWindowClass(HINSTANCE instance);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    void OnCreate();
    void OnDestroy();
    void OnHotkey(WPARAM wparam, LPARAM lparam);
    void OnTranscript(const std::wstring& text);
    void OnSetupRequired();

    void HandleTrayCommand(UINT command);
    void ApplySettings();

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    Settings settings_{};
    bool dictation_active_ = false;

#ifdef VOICEFLOW_USE_PYTHON_ASR
    class PythonAsrSession* session_ = nullptr;
#elif defined(VOICEFLOW_HAS_MOONSHINE)
    class MoonshineSession* session_ = nullptr;
#endif
};

}  // namespace voiceflow
