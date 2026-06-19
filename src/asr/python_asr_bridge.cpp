#include "asr/python_asr_bridge.h"

#include "config/settings.h"
#include "util/paths.h"

#include <windows.h>

#include <filesystem>
#include <string>

namespace voiceflow {

namespace {

std::wstring LanguageToCode(Language language) {
    return language == Language::Chinese ? L"zh" : L"en";
}

std::wstring ResolveWorkerScript() {
    wchar_t module_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, module_path, MAX_PATH);
    const std::filesystem::path script =
        std::filesystem::path(module_path).parent_path() / L"packaging" / L"voiceflow_asr_worker.py";
    if (std::filesystem::exists(script)) {
        return script.wstring();
    }

    // Developer builds keep the script at repo/packaging.
    const std::filesystem::path dev_script =
        std::filesystem::path(module_path).parent_path().parent_path().parent_path() / L"packaging" /
        L"voiceflow_asr_worker.py";
    return dev_script.wstring();
}

}  // namespace

PythonAsrSession::PythonAsrSession(TranscriptHandler handler) : handler_(std::move(handler)) {}

PythonAsrSession::~PythonAsrSession() {
    if (process_handle_) {
        SendCommand(L"QUIT");
        WaitForSingleObject(static_cast<HANDLE>(process_handle_), 2000);
        CloseHandle(static_cast<HANDLE>(process_handle_));
        process_handle_ = nullptr;
    }
    if (stdin_write_) {
        CloseHandle(static_cast<HANDLE>(stdin_write_));
        stdin_write_ = nullptr;
    }
    if (stdout_read_) {
        CloseHandle(static_cast<HANDLE>(stdout_read_));
        stdout_read_ = nullptr;
    }
}

bool PythonAsrSession::EnsureWorker() {
    if (process_handle_) {
        return true;
    }

    const std::wstring script = ResolveWorkerScript();
    if (script.empty() || !std::filesystem::exists(script)) {
        return false;
    }

    SECURITY_ATTRIBUTES sa = {sizeof(sa), nullptr, TRUE};
    HANDLE stdin_read = nullptr;
    HANDLE stdout_write = nullptr;
    HANDLE stdin_write_handle = nullptr;
    HANDLE stdout_read_handle = nullptr;

    if (!CreatePipe(&stdin_read, &stdin_write_handle, &sa, 0)) {
        return false;
    }
    if (!CreatePipe(&stdout_read_handle, &stdout_write, &sa, 0)) {
        CloseHandle(stdin_read);
        CloseHandle(stdin_write_handle);
        return false;
    }

    SetHandleInformation(stdin_write_handle, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stdout_read_handle, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {sizeof(si)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdin_read;
    si.hStdOutput = stdout_write;
    si.hStdError = stdout_write;

    std::wstring command = L"python \"" + script + L"\"";
    std::wstring mutable_command = command;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, mutable_command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        CloseHandle(stdin_read);
        CloseHandle(stdin_write_handle);
        CloseHandle(stdout_read_handle);
        CloseHandle(stdout_write);
        return false;
    }

    CloseHandle(stdin_read);
    CloseHandle(stdout_write);
    CloseHandle(pi.hThread);

    process_handle_ = pi.hProcess;
    stdin_write_ = stdin_write_handle;
    stdout_read_ = stdout_read_handle;
    return true;
}

bool PythonAsrSession::SendCommand(const std::wstring& command) {
    if (!stdin_write_) {
        return false;
    }
    std::string utf8;
    const int size = WideCharToMultiByte(CP_UTF8, 0, command.c_str(), static_cast<int>(command.size()), nullptr, 0,
                                         nullptr, nullptr);
    utf8.assign(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, command.c_str(), static_cast<int>(command.size()), utf8.data(), size, nullptr,
                        nullptr);
    utf8.push_back('\n');

    DWORD written = 0;
    return WriteFile(static_cast<HANDLE>(stdin_write_), utf8.data(), static_cast<DWORD>(utf8.size()), &written,
                     nullptr) == TRUE;
}

bool PythonAsrSession::ReadFinalTranscript(std::wstring& text) {
    if (!stdout_read_) {
        return false;
    }

    std::string buffer;
    char chunk[256] = {};
    DWORD read = 0;
    while (ReadFile(static_cast<HANDLE>(stdout_read_), chunk, sizeof(chunk) - 1, &read, nullptr) && read > 0) {
        chunk[read] = '\0';
        buffer.append(chunk, read);
        const size_t newline = buffer.find('\n');
        if (newline != std::string::npos) {
            const std::string line = buffer.substr(0, newline);
            const std::string prefix = "FINAL:";
            if (line.rfind(prefix, 0) == 0) {
                const std::string payload = line.substr(prefix.size());
                const int wide_size =
                    MultiByteToWideChar(CP_UTF8, 0, payload.c_str(), static_cast<int>(payload.size()), nullptr, 0);
                text.assign(wide_size, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, payload.c_str(), static_cast<int>(payload.size()), text.data(),
                                    wide_size);
                return true;
            }
            if (line.rfind("ERROR:", 0) == 0) {
                return false;
            }
            buffer.erase(0, newline + 1);
        }
    }
    return false;
}

bool PythonAsrSession::Initialize(Language language, bool force_cpu) {
    language_ = language;
    force_cpu_ = force_cpu;
    if (!EnsureWorker()) {
        return false;
    }
    std::wstring init = L"INIT lang=" + LanguageToCode(language) + L" force_cpu=" + (force_cpu ? L"1" : L"0");
    return SendCommand(init);
}

void PythonAsrSession::Start() {
    if (!EnsureWorker()) {
        return;
    }
    SendCommand(L"START");
}

void PythonAsrSession::Stop() {
    if (!EnsureWorker()) {
        return;
    }
    SendCommand(L"STOP");
    std::wstring text;
    if (ReadFinalTranscript(text) && handler_) {
        handler_(text);
    }
}

}  // namespace voiceflow
