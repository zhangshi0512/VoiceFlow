#include "tray/tray_icon.h"

#include "app_messages.h"

#include <shellapi.h>

namespace voiceflow {

namespace {
constexpr UINT kTrayIconId = 1;
}  // namespace

bool TrayIcon::Create(HWND owner, HINSTANCE instance, CommandHandler handler) {
    owner_ = owner;
    instance_ = instance;
    handler_ = std::move(handler);

    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = owner_;
    nid.uID = kTrayIconId;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = TRAY_CALLBACK_MESSAGE;
    nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"VoiceFlow");

    created_ = Shell_NotifyIconW(NIM_ADD, &nid) == TRUE;
    return created_;
}

void TrayIcon::Destroy() {
    if (!created_) {
        return;
    }
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = owner_;
    nid.uID = kTrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    created_ = false;
}

void TrayIcon::SetRecording(bool recording) {
    recording_ = recording;
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = owner_;
    nid.uID = kTrayIconId;
    nid.uFlags = NIF_TIP;
    wcscpy_s(nid.szTip, recording ? L"VoiceFlow (listening)" : L"VoiceFlow");
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrayIcon::ShowBalloon(const std::wstring& title, const std::wstring& message) {
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = owner_;
    nid.uID = kTrayIconId;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nid.szInfo, message.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrayIcon::ShowContextMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, TRAY_CMD_LANGUAGE_EN, L"Language: English");
    AppendMenuW(menu, MF_STRING, TRAY_CMD_LANGUAGE_ZH, L"Language: Chinese");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, TRAY_CMD_SETTINGS, L"Settings...");
    AppendMenuW(menu, MF_STRING, TRAY_CMD_EXIT, L"Exit");

    POINT cursor = {};
    GetCursorPos(&cursor);
    SetForegroundWindow(owner_);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, owner_, nullptr);
    DestroyMenu(menu);
}

}  // namespace voiceflow
