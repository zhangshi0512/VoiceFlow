#include "asr/microphone_capture.h"

#include <audioclient.h>
#include <mmdeviceapi.h>

#include <atomic>
#include <thread>

namespace voiceflow {

struct MicrophoneCapture::ComHolder {
    ComHolder() {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    }
    ~ComHolder() {
        CoUninitialize();
    }
};

MicrophoneCapture::MicrophoneCapture() {
    com_ = new ComHolder();
}

MicrophoneCapture::~MicrophoneCapture() {
    Stop();
    if (capture_client_) {
        static_cast<IAudioCaptureClient*>(capture_client_)->Release();
        capture_client_ = nullptr;
    }
    if (audio_client_) {
        static_cast<IAudioClient*>(audio_client_)->Release();
        audio_client_ = nullptr;
    }
    if (device_) {
        static_cast<IMMDevice*>(device_)->Release();
        device_ = nullptr;
    }
    if (device_enumerator_) {
        static_cast<IMMDeviceEnumerator*>(device_enumerator_)->Release();
        device_enumerator_ = nullptr;
    }
    delete com_;
    com_ = nullptr;
}

bool MicrophoneCapture::Initialize() {
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), &device_enumerator_);
    if (FAILED(hr)) {
        return false;
    }

    hr = static_cast<IMMDeviceEnumerator*>(device_enumerator_)
             ->GetDefaultAudioEndpoint(eCapture, eConsole, reinterpret_cast<IMMDevice**>(&device_));
    if (FAILED(hr)) {
        return false;
    }

    hr = static_cast<IMMDevice*>(device_)
             ->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audio_client_));
    if (FAILED(hr)) {
        return false;
    }

    WAVEFORMATEX* mix_format = nullptr;
    hr = static_cast<IAudioClient*>(audio_client_)->GetMixFormat(&mix_format);
    if (FAILED(hr)) {
        return false;
    }

    WAVEFORMATEX desired = {};
    desired.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    desired.nChannels = 1;
    desired.nSamplesPerSec = sample_rate_;
    desired.wBitsPerSample = 32;
    desired.nBlockAlign = desired.nChannels * (desired.wBitsPerSample / 8);
    desired.nAvgBytesPerSec = desired.nSamplesPerSec * desired.nBlockAlign;

    hr = static_cast<IAudioClient*>(audio_client_)
             ->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, &desired, nullptr);
    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
        hr = static_cast<IAudioClient*>(audio_client_)
                 ->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, mix_format, nullptr);
        if (FAILED(hr)) {
            CoTaskMemFree(mix_format);
            return false;
        }
        actual_sample_rate_ = mix_format->nSamplesPerSec;
        actual_channels_ = mix_format->nChannels;
    } else if (FAILED(hr)) {
        CoTaskMemFree(mix_format);
        return false;
    } else {
        actual_sample_rate_ = sample_rate_;
        actual_channels_ = 1;
    }
    CoTaskMemFree(mix_format);

    hr = static_cast<IAudioClient*>(audio_client_)
             ->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture_client_));
    if (FAILED(hr)) {
        return false;
    }

    hr = static_cast<IAudioClient*>(audio_client_)->GetBufferSize(&buffer_frame_count_);
    return SUCCEEDED(hr);
}

void MicrophoneCapture::SetAudioCallback(std::function<void(const std::vector<float>&, int32_t)> callback) {
    audio_callback_ = std::move(callback);
}

void MicrophoneCapture::Start() {
    if (is_capturing_) {
        return;
    }
    if (FAILED(static_cast<IAudioClient*>(audio_client_)->Start())) {
        return;
    }
    is_capturing_ = true;
    auto* thread = new std::thread(&MicrophoneCapture::CaptureLoop, this);
    capture_thread_ = thread;
}

void MicrophoneCapture::Stop() {
    if (!is_capturing_) {
        return;
    }
    is_capturing_ = false;
    if (capture_thread_) {
        auto* thread = static_cast<std::thread*>(capture_thread_);
        if (thread->joinable()) {
            thread->join();
        }
        delete thread;
        capture_thread_ = nullptr;
    }
    if (audio_client_) {
        static_cast<IAudioClient*>(audio_client_)->Stop();
    }
}

void MicrophoneCapture::CaptureLoop() {
    while (is_capturing_) {
        Sleep(static_cast<DWORD>((1000.0 * buffer_frame_count_) / (2 * actual_sample_rate_)));

        UINT32 frames_available = 0;
        BYTE* data = nullptr;
        DWORD flags = 0;
        HRESULT hr = static_cast<IAudioCaptureClient*>(capture_client_)
                         ->GetBuffer(&data, &frames_available, &flags, nullptr, nullptr);
        if (FAILED(hr) || frames_available == 0) {
            if (SUCCEEDED(hr)) {
                static_cast<IAudioCaptureClient*>(capture_client_)->ReleaseBuffer(frames_available);
            }
            continue;
        }
        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            static_cast<IAudioCaptureClient*>(capture_client_)->ReleaseBuffer(frames_available);
            continue;
        }

        std::vector<float> audio;
        auto* float_data = reinterpret_cast<float*>(data);
        if (actual_sample_rate_ == static_cast<uint32_t>(sample_rate_) && actual_channels_ == 1) {
            audio.assign(float_data, float_data + frames_available);
        } else {
            for (UINT32 i = 0; i < frames_available; ++i) {
                float sum = 0.0f;
                for (UINT16 ch = 0; ch < actual_channels_; ++ch) {
                    sum += float_data[i * actual_channels_ + ch];
                }
                audio.push_back(sum / actual_channels_);
            }
            if (actual_sample_rate_ != static_cast<uint32_t>(sample_rate_)) {
                const double ratio =
                    static_cast<double>(sample_rate_) / static_cast<double>(actual_sample_rate_);
                std::vector<float> resampled;
                resampled.reserve(audio.size());
                for (size_t i = 0; i < audio.size(); ++i) {
                    const size_t index = static_cast<size_t>(i * ratio);
                    if (index >= resampled.size()) {
                        resampled.push_back(audio[i]);
                    } else {
                        resampled[index] = audio[i];
                    }
                }
                audio = std::move(resampled);
            }
        }

        if (audio_callback_ && !audio.empty()) {
            audio_callback_(audio, sample_rate_);
        }

        static_cast<IAudioCaptureClient*>(capture_client_)->ReleaseBuffer(frames_available);
    }
}

}  // namespace voiceflow
