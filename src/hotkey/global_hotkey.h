#pragma once

#include "app.h"

#include <windows.h>

namespace voiceflow {

class GlobalHotkey {
 public:
    using HotkeyHandler = void (*)(WPARAM action, LPARAM extra, void* user_data);

    bool Install(HWND target_window, HotkeyMode mode, HotkeyHandler handler, void* user_data);
    void Uninstall();
    void SetMode(HotkeyMode mode);

 private:
    static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wparam, LPARAM lparam);

    HWND target_window_ = nullptr;
    HHOOK hook_ = nullptr;
    HotkeyMode mode_ = HotkeyMode::PushToTalk;
    HotkeyHandler handler_ = nullptr;
    void* user_data_ = nullptr;

    bool ctrl_down_ = false;
    bool shift_down_ = false;
    bool space_down_ = false;
    bool chord_active_ = false;
    bool toggle_recording_ = false;
};

}  // namespace voiceflow
