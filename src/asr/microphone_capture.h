#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace voiceflow {

class MicrophoneCapture {
 public:
    MicrophoneCapture();
    ~MicrophoneCapture();

    bool Initialize();
    void Start();
    void Stop();
    void SetAudioCallback(std::function<void(const std::vector<float>&, int32_t)> callback);

 private:
    void CaptureLoop();

    struct ComHolder;
    ComHolder* com_ = nullptr;
    void* device_enumerator_ = nullptr;
    void* device_ = nullptr;
    void* audio_client_ = nullptr;
    void* capture_client_ = nullptr;
    uint32_t buffer_frame_count_ = 0;
    bool is_capturing_ = false;
    void* capture_thread_ = nullptr;
    std::function<void(const std::vector<float>&, int32_t)> audio_callback_;
    int32_t sample_rate_ = 16000;
    uint32_t actual_sample_rate_ = 0;
    uint16_t actual_channels_ = 0;
};

}  // namespace voiceflow
