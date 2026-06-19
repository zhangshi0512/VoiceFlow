#pragma once

#include <shlobj.h>
#include <windows.h>

#include <filesystem>
#include <string>

namespace voiceflow {

std::wstring GetLocalAppDataPath();
std::wstring GetVoiceFlowDataRoot();
std::wstring GetSettingsPath();
std::wstring GetRuntimeDir();
std::wstring GetModelsDir(const wchar_t* language_code);

}  // namespace voiceflow
