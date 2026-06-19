#pragma once

#include <windows.h>

namespace voiceflow {

constexpr UINT WM_VOICEFLOW_HOTKEY = WM_APP + 1;
constexpr UINT WM_VOICEFLOW_TRANSCRIPT = WM_APP + 2;
constexpr UINT WM_VOICEFLOW_SETUP_REQUIRED = WM_APP + 3;

constexpr int HOTKEY_ID_TOGGLE = 1;

constexpr UINT TRAY_CMD_EXIT = 1001;
constexpr UINT TRAY_CMD_SETTINGS = 1002;
constexpr UINT TRAY_CMD_LANGUAGE_EN = 1003;
constexpr UINT TRAY_CMD_LANGUAGE_ZH = 1004;

constexpr UINT TRAY_CALLBACK_MESSAGE = WM_USER + 50;

}  // namespace voiceflow
