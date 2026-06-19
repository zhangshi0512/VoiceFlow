#include "inject/text_injector.h"

#include <windows.h>

#include <vector>

namespace voiceflow {

namespace {

INPUT MakeKeyInput(WORD vk, bool key_up) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    if (key_up) {
        input.ki.dwFlags = KEYEVENTF_KEYUP;
    }
    return input;
}

}  // namespace

bool TextInjector::TrySendInput(const std::wstring& text) {
    if (text.empty()) {
        return true;
    }

    std::vector<INPUT> inputs;
    inputs.reserve(text.size() * 2);

    for (wchar_t ch : text) {
        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = 0;
        down.ki.wScan = ch;
        down.ki.dwFlags = KEYEVENTF_UNICODE;

        INPUT up = down;
        up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

        inputs.push_back(down);
        inputs.push_back(up);
    }

    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    return sent == inputs.size();
}

bool TextInjector::TryClipboardPaste(const std::wstring& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }

    HGLOBAL previous = GetClipboardData(CF_UNICODETEXT);
    std::wstring backup;
    if (previous) {
        const wchar_t* locked = static_cast<const wchar_t*>(GlobalLock(previous));
        if (locked) {
            backup = locked;
            GlobalUnlock(previous);
        }
    }

    EmptyClipboard();

    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) {
        CloseClipboard();
        return false;
    }

    void* dest = GlobalLock(memory);
    if (!dest) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    memcpy(dest, text.c_str(), bytes);
    GlobalUnlock(memory);
    SetClipboardData(CF_UNICODETEXT, memory);

    INPUT inputs[4] = {
        MakeKeyInput(VK_CONTROL, false),
        MakeKeyInput(L'V', false),
        MakeKeyInput(L'V', true),
        MakeKeyInput(VK_CONTROL, true),
    };
    SendInput(4, inputs, sizeof(INPUT));

    EmptyClipboard();
    if (!backup.empty()) {
        const size_t restore_bytes = (backup.size() + 1) * sizeof(wchar_t);
        HGLOBAL restore_mem = GlobalAlloc(GMEM_MOVEABLE, restore_bytes);
        if (restore_mem) {
            void* restore_dest = GlobalLock(restore_mem);
            if (restore_dest) {
                memcpy(restore_dest, backup.c_str(), restore_bytes);
                GlobalUnlock(restore_mem);
                SetClipboardData(CF_UNICODETEXT, restore_mem);
            } else {
                GlobalFree(restore_mem);
            }
        }
    }

    CloseClipboard();
    return true;
}

bool TextInjector::Inject(const std::wstring& text) {
    if (text.empty()) {
        return true;
    }
    if (TrySendInput(text)) {
        return true;
    }
    return TryClipboardPaste(text);
}

}  // namespace voiceflow
