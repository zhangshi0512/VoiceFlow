#pragma once

#include "app.h"

#include <functional>
#include <string>

namespace voiceflow {

class PythonAsrSession {
 public:
    using TranscriptHandler = std::function<void(const std::wstring&)>;

    explicit PythonAsrSession(TranscriptHandler handler);
    ~PythonAsrSession();

    bool Initialize(Language language, bool force_cpu);
    void Start();
    void Stop();

 private:
    bool EnsureWorker();
    bool SendCommand(const std::wstring& command);
    bool ReadFinalTranscript(std::wstring& text);

    TranscriptHandler handler_;
    void* process_handle_ = nullptr;
    void* stdin_write_ = nullptr;
    void* stdout_read_ = nullptr;
    Language language_ = Language::English;
    bool force_cpu_ = false;
};

}  // namespace voiceflow
