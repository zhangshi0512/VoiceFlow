#include "setup/first_run_wizard.h"

#include "setup/component_cache.h"
#include "util/paths.h"

#include <shellapi.h>

#include <filesystem>
#include <string>

namespace voiceflow {

namespace {

bool CopyDeveloperRuntime() {
    wchar_t module_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, module_path, MAX_PATH);
    const std::filesystem::path exe_dir = std::filesystem::path(module_path).parent_path();
    const std::filesystem::path source_dll = exe_dir / L"onnxruntime.dll";
    if (!std::filesystem::exists(source_dll)) {
        return false;
    }

    EnsureDataDirectories();
    const std::filesystem::path target_dll = std::filesystem::path(GetRuntimeDir()) / L"onnxruntime.dll";
    std::error_code ec;
    std::filesystem::copy_file(source_dll, target_dll, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
}

bool LaunchModelDownload() {
    wchar_t module_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, module_path, MAX_PATH);
    const std::filesystem::path script =
        std::filesystem::path(module_path).parent_path() / L"packaging" / L"download_models.ps1";
    if (!std::filesystem::exists(script)) {
        return false;
    }

    const std::wstring ps_command = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" +
                                    script.wstring() + L"\"";
    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    std::wstring mutable_command = ps_command;
    if (!CreateProcessW(nullptr, mutable_command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exit_code == 0;
}

}  // namespace

bool RunFirstRunWizard(HWND owner) {
    EnsureDataDirectories();

    if (!HasRuntime()) {
        const int choice = MessageBoxW(
            owner,
            L"VoiceFlow needs to prepare the speech runtime on first launch.\n\n"
            L"Click Yes to copy the bundled runtime, or No to cancel.",
            L"VoiceFlow Setup",
            MB_YESNO | MB_ICONINFORMATION);
        if (choice != IDYES || !CopyDeveloperRuntime()) {
            MessageBoxW(owner,
                        L"Speech runtime is missing.\n\n"
                        L"Build with Moonshine dependencies or place onnxruntime.dll in:\n"
                        L"%LOCALAPPDATA%\\VoiceFlow\\runtime",
                        L"VoiceFlow Setup",
                        MB_OK | MB_ICONWARNING);
            return false;
        }
    }

    if (!HasLanguageModel(L"en") || !HasLanguageModel(L"zh")) {
        const int choice = MessageBoxW(
            owner,
            L"VoiceFlow needs English and Chinese speech models.\n\n"
            L"Download now? This requires Python and moonshine-voice.",
            L"VoiceFlow Setup",
            MB_YESNO | MB_ICONQUESTION);
        if (choice == IDYES) {
            LaunchModelDownload();
        }
    }

    if (!IsSetupComplete()) {
        MessageBoxW(owner,
                    L"Setup is incomplete.\n\n"
                    L"Install models with:\n"
                    L"  pip install moonshine-voice\n"
                    L"  python -m moonshine_voice.download --language en\n"
                    L"  python -m moonshine_voice.download --language zh\n\n"
                    L"Then copy model folders into %LOCALAPPDATA%\\VoiceFlow\\models\\en and \\zh",
                    L"VoiceFlow Setup",
                    MB_OK | MB_ICONWARNING);
        return false;
    }

    return true;
}

}  // namespace voiceflow
