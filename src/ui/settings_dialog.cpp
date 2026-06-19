#include "ui/settings_dialog.h"

#include "config/settings.h"

namespace voiceflow {

namespace {

constexpr int IDC_MODE_PTT = 2001;
constexpr int IDC_MODE_TOGGLE = 2002;
constexpr int IDC_LANG_EN = 2003;
constexpr int IDC_LANG_ZH = 2004;
constexpr int IDC_FORCE_CPU = 2005;
constexpr int IDC_OK = 2006;

INT_PTR CALLBACK SettingsDialogProc(HWND dialog, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* settings = reinterpret_cast<Settings*>(GetWindowLongPtrW(dialog, GWLP_USERDATA));

    switch (msg) {
        case WM_INITDIALOG: {
            settings = reinterpret_cast<Settings*>(lparam);
            SetWindowLongPtrW(dialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));
            CheckRadioButton(dialog, IDC_MODE_PTT, IDC_MODE_TOGGLE,
                             settings->hotkey_mode == HotkeyMode::PushToTalk ? IDC_MODE_PTT : IDC_MODE_TOGGLE);
            CheckRadioButton(dialog, IDC_LANG_EN, IDC_LANG_ZH,
                             settings->language == Language::English ? IDC_LANG_EN : IDC_LANG_ZH);
            CheckDlgButton(dialog, IDC_FORCE_CPU, settings->force_cpu ? BST_CHECKED : BST_UNCHECKED);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case IDC_OK:
                case IDOK: {
                    settings->hotkey_mode =
                        IsDlgButtonChecked(dialog, IDC_MODE_TOGGLE) == BST_CHECKED ? HotkeyMode::Toggle
                                                                                   : HotkeyMode::PushToTalk;
                    settings->language = IsDlgButtonChecked(dialog, IDC_LANG_ZH) == BST_CHECKED ? Language::Chinese
                                                                                                : Language::English;
                    settings->force_cpu = IsDlgButtonChecked(dialog, IDC_FORCE_CPU) == BST_CHECKED;
                    SaveSettings(*settings);
                    EndDialog(dialog, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(dialog, IDCANCEL);
                    return TRUE;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return FALSE;
}

}  // namespace

bool ShowSettingsDialog(HWND owner, Settings& settings) {
    return DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101), owner, SettingsDialogProc,
                           reinterpret_cast<LPARAM>(&settings)) == IDOK;
}

}  // namespace voiceflow
