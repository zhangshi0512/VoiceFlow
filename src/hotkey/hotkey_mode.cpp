#include "hotkey/hotkey_mode.h"

#include "app.h"

namespace voiceflow {

const wchar_t* HotkeyModeLabel(HotkeyMode mode) {
    return mode == HotkeyMode::Toggle ? L"Toggle" : L"Push-to-talk";
}

}  // namespace voiceflow
