#include "setup/component_cache.h"

#include "util/paths.h"

#include <filesystem>

namespace voiceflow {

namespace {

bool HasModelArtifacts(const std::wstring& dir) {
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) {
        return false;
    }
    bool has_encoder = false;
    bool has_decoder = false;
    for (const auto& entry : fs::directory_iterator(dir)) {
        const auto name = entry.path().filename().wstring();
        if (name.find(L"encoder") != std::wstring::npos) {
            has_encoder = true;
        }
        if (name.find(L"decoder") != std::wstring::npos) {
            has_decoder = true;
        }
    }
    return has_encoder && has_decoder;
}

}  // namespace

bool EnsureDataDirectories() {
    namespace fs = std::filesystem;
    try {
        fs::create_directories(GetVoiceFlowDataRoot());
        fs::create_directories(GetRuntimeDir());
        fs::create_directories(GetModelsDir(L"en"));
        fs::create_directories(GetModelsDir(L"zh"));
        return true;
    } catch (...) {
        return false;
    }
}

bool HasRuntime() {
    namespace fs = std::filesystem;
    const fs::path runtime_dll = fs::path(GetRuntimeDir()) / L"onnxruntime.dll";
    if (fs::exists(runtime_dll)) {
        return true;
    }

    // Developer fallback: Moonshine DLL next to the executable.
    wchar_t module_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, module_path, MAX_PATH);
    const fs::path exe_dir = fs::path(module_path).parent_path();
    return fs::exists(exe_dir / L"onnxruntime.dll");
}

bool HasLanguageModel(const wchar_t* language_code) {
    return HasModelArtifacts(GetModelsDir(language_code));
}

bool IsSetupComplete() {
    return HasRuntime() && HasLanguageModel(L"en") && HasLanguageModel(L"zh");
}

std::wstring ResolveModelPath(const wchar_t* language_code) {
    const std::wstring custom = GetModelsDir(language_code);
    if (HasModelArtifacts(custom)) {
        return custom;
    }

    wchar_t module_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, module_path, MAX_PATH);
    const std::filesystem::path bundled =
        std::filesystem::path(module_path).parent_path() / L"models" / language_code;
    if (HasModelArtifacts(bundled.wstring())) {
        return bundled.wstring();
    }
    return custom;
}

}  // namespace voiceflow
