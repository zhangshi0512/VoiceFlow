#include "hotkey/global_hotkey.h"

#include "app_messages.h"

namespace voiceflow {

namespace {

constexpr int kActionStart = 1;
constexpr int kActionStop = 2;

bool IsTargetKey(DWORD vk) {
    return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_SHIFT ||
           vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_SPACE;
}

void UpdateModifier(bool* flag, DWORD vk, bool down) {
    if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL) {
        *flag = down;
    }
    if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT) {
        *flag = down;
    }
}

GlobalHotkey* g_hotkey_instance = nullptr;

}  // namespace

bool GlobalHotkey::Install(HWND target_window, HotkeyMode mode, HotkeyHandler handler, void* user_data) {
    target_window_ = target_window;
    mode_ = mode;
    handler_ = handler;
    user_data_ = user_data;
    g_hotkey_instance = this;

    hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(nullptr), 0);
    return hook_ != nullptr;
}

void GlobalHotkey::Uninstall() {
    if (hook_) {
        UnhookWindowsHookEx(hook_);
        hook_ = nullptr;
    }
    if (g_hotkey_instance == this) {
        g_hotkey_instance = nullptr;
    }
}

void GlobalHotkey::SetMode(HotkeyMode mode) {
    mode_ = mode;
    toggle_recording_ = false;
    chord_active_ = false;
}

LRESULT CALLBACK GlobalHotkey::LowLevelKeyboardProc(int code, WPARAM wparam, LPARAM lparam) {
    if (code != HC_ACTION || !g_hotkey_instance) {
        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    const auto* event = reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam);
    const bool key_down = wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN;
    const bool key_up = wparam == WM_KEYUP || wparam == WM_SYSKEYUP;
    const DWORD vk = event->vkCode;

    if (!IsTargetKey(vk)) {
        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    auto* self = g_hotkey_instance;
    if (key_down || key_up) {
        UpdateModifier(&self->ctrl_down_, vk, key_down);
        UpdateModifier(&self->shift_down_, vk, key_down);
        if (vk == VK_SPACE) {
            self->space_down_ = key_down;
        }
    }

    const bool chord_now = self->ctrl_down_ && self->shift_down_ && self->space_down_;

    if (self->mode_ == HotkeyMode::PushToTalk) {
        if (chord_now && !self->chord_active_) {
            self->chord_active_ = true;
            if (self->handler_) {
                self->handler_(kActionStart, 0, self->user_data_);
            }
        } else if (!chord_now && self->chord_active_) {
            self->chord_active_ = false;
            if (self->handler_) {
                self->handler_(kActionStop, 0, self->user_data_);
            }
        }
    } else {
        if (chord_now && !self->chord_active_) {
            self->chord_active_ = true;
            self->toggle_recording_ = !self->toggle_recording_;
            if (self->handler_) {
                self->handler_(self->toggle_recording_ ? kActionStart : kActionStop, 0, self->user_data_);
            }
        } else if (!chord_now && self->chord_active_) {
            self->chord_active_ = false;
        }
    }

    return CallNextHookEx(nullptr, code, wparam, lparam);
}

}  // namespace voiceflow
