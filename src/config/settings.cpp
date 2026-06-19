#include "config/settings.h"

#include "util/paths.h"

#include <windows.h>

#include <filesystem>

namespace voiceflow {

namespace {

HotkeyMode ParseHotkeyMode(const std::wstring& value) {
    if (value == L"toggle") {
        return HotkeyMode::Toggle;
    }
    return HotkeyMode::PushToTalk;
}

Language ParseLanguage(const std::wstring& value) {
    if (value == L"zh") {
        return Language::Chinese;
    }
    return Language::English;
}

std::wstring HotkeyModeToString(HotkeyMode mode) {
    return mode == HotkeyMode::Toggle ? L"toggle" : L"ptt";
}

std::wstring LanguageToString(Language language) {
    return language == Language::Chinese ? L"zh" : L"en";
}

}  // namespace

void LoadSettings(Settings& settings) {
    const std::wstring path = GetSettingsPath();
    if (!std::filesystem::exists(path)) {
        return;
    }

    wchar_t buffer[64] = {};
    GetPrivateProfileStringW(L"general", L"hotkey_mode", L"ptt", buffer, 64, path.c_str());
    settings.hotkey_mode = ParseHotkeyMode(buffer);

    GetPrivateProfileStringW(L"general", L"language", L"en", buffer, 64, path.c_str());
    settings.language = ParseLanguage(buffer);

    settings.prefer_gpu = GetPrivateProfileIntW(L"asr", L"prefer_gpu", 1, path.c_str()) != 0;
    settings.force_cpu = GetPrivateProfileIntW(L"asr", L"force_cpu", 0, path.c_str()) != 0;
}

void SaveSettings(const Settings& settings) {
    const std::wstring path = GetSettingsPath();
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());

    WritePrivateProfileStringW(L"general", L"hotkey_mode", HotkeyModeToString(settings.hotkey_mode).c_str(),
                               path.c_str());
    WritePrivateProfileStringW(L"general", L"language", LanguageToString(settings.language).c_str(), path.c_str());
    WritePrivateProfileStringW(L"asr", L"prefer_gpu", settings.prefer_gpu ? L"1" : L"0", path.c_str());
    WritePrivateProfileStringW(L"asr", L"force_cpu", settings.force_cpu ? L"1" : L"0", path.c_str());
}

}  // namespace voiceflow
