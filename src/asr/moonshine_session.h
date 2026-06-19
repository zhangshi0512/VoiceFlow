#pragma once

#include "app.h"

#include <functional>
#include <string>

namespace voiceflow {

class MoonshineSession {
 public:
    using TranscriptHandler = std::function<void(const std::wstring&)>;

    explicit MoonshineSession(TranscriptHandler handler);
    ~MoonshineSession();

    bool Initialize(Language language, bool force_cpu);
    void Start();
    void Stop();

 private:
    struct Impl;
    Impl* impl_ = nullptr;
    TranscriptHandler handler_;
};

}  // namespace voiceflow
