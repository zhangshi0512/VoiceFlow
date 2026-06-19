#pragma once

#include <windows.h>

#include <functional>
#include <string>

namespace voiceflow {

class TrayIcon {
 public:
    using CommandHandler = std::function<void(UINT command)>;

    bool Create(HWND owner, HINSTANCE instance, CommandHandler handler);
    void Destroy();
    void SetRecording(bool recording);
    void ShowBalloon(const std::wstring& title, const std::wstring& message);
    void ShowContextMenu();

 private:

    HWND owner_ = nullptr;
    HINSTANCE instance_ = nullptr;
    CommandHandler handler_;
    bool created_ = false;
    bool recording_ = false;
};

}  // namespace voiceflow
