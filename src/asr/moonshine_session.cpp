#include "asr/moonshine_session.h"

#include "asr/moonshine_include.h"

#include "asr/microphone_capture.h"
#include "setup/component_cache.h"

#include <memory>
#include <mutex>
#include <string>

namespace voiceflow {

namespace {

moonshine::ModelArch ModelArchForLanguage(Language language) {
    return language == Language::Chinese ? moonshine::ModelArch::BASE : moonshine::ModelArch::TINY_STREAMING;
}

const wchar_t* LanguageCode(Language language) {
    return language == Language::Chinese ? L"zh" : L"en";
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int size =
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size, nullptr,
                        nullptr);
    return result;
}

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

}  // namespace

class SessionListener : public moonshine::TranscriptEventListener {
 public:
    explicit SessionListener(std::function<void(const std::wstring&)> on_complete)
        : on_complete_(std::move(on_complete)) {}

    void onLineCompleted(const moonshine::LineCompleted& event) override {
        std::lock_guard<std::mutex> lock(mutex_);
        last_line_ = Utf8ToWide(event.line.text);
    }

    std::wstring TakeLastLine() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::wstring line = last_line_;
        last_line_.clear();
        return line;
    }

 private:
    std::function<void(const std::wstring&)> on_complete_;
    std::mutex mutex_;
    std::wstring last_line_;
};

struct MoonshineSession::Impl {
    std::unique_ptr<moonshine::Transcriber> transcriber;
    std::unique_ptr<MicrophoneCapture> microphone;
    std::unique_ptr<SessionListener> listener;
    Language language = Language::English;
};

MoonshineSession::MoonshineSession(TranscriptHandler handler) : handler_(std::move(handler)) {
    impl_ = new Impl();
}

MoonshineSession::~MoonshineSession() {
    Stop();
    delete impl_;
    impl_ = nullptr;
}

bool MoonshineSession::Initialize(Language language, bool force_cpu) {
    (void)force_cpu;  // GPU EP selection hooks into Moonshine runtime packaging in a later step.

    impl_->language = language;
    const std::wstring model_path = ResolveModelPath(LanguageCode(language));
    if (model_path.empty()) {
        return false;
    }

    try {
        impl_->listener = std::make_unique<SessionListener>(handler_);
        impl_->transcriber = std::make_unique<moonshine::Transcriber>(
            WideToUtf8(model_path), ModelArchForLanguage(language), 0.5f);
        impl_->transcriber->addListener(impl_->listener.get());

        impl_->microphone = std::make_unique<MicrophoneCapture>();
        if (!impl_->microphone->Initialize()) {
            return false;
        }

        impl_->microphone->SetAudioCallback([this](const std::vector<float>& audio, int32_t sample_rate) {
            if (!impl_->transcriber) {
                return;
            }
            impl_->transcriber->addAudio(audio, sample_rate);
        });
        return true;
    } catch (const moonshine::MoonshineException&) {
        return false;
    }
}

void MoonshineSession::Start() {
    if (!impl_->transcriber || !impl_->microphone) {
        return;
    }
    impl_->transcriber->start();
    impl_->microphone->Start();
}

void MoonshineSession::Stop() {
    if (impl_->microphone) {
        impl_->microphone->Stop();
    }
    if (impl_->transcriber) {
        impl_->transcriber->updateTranscription();
        impl_->transcriber->stop();
    }
    if (impl_->listener) {
        const std::wstring line = impl_->listener->TakeLastLine();
        if (!line.empty() && handler_) {
            handler_(line);
        }
    }
}

}  // namespace voiceflow
