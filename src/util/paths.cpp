#include "util/paths.h"

#include <knownfolders.h>
#include <shlobj.h>

namespace voiceflow {

std::wstring GetLocalAppDataPath() {
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
        return L"";
    }
    std::wstring result(path);
    CoTaskMemFree(path);
    return result;
}

std::wstring GetVoiceFlowDataRoot() {
    return GetLocalAppDataPath() + L"\\VoiceFlow";
}

std::wstring GetSettingsPath() {
    return GetVoiceFlowDataRoot() + L"\\settings.ini";
}

std::wstring GetRuntimeDir() {
    return GetVoiceFlowDataRoot() + L"\\runtime";
}

std::wstring GetModelsDir(const wchar_t* language_code) {
    return GetVoiceFlowDataRoot() + L"\\models\\" + language_code;
}

}  // namespace voiceflow
