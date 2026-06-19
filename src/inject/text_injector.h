#pragma once

#include <string>

namespace voiceflow {

class TextInjector {
 public:
    bool Inject(const std::wstring& text);

 private:
    bool TrySendInput(const std::wstring& text);
    bool TryClipboardPaste(const std::wstring& text);
};

}  // namespace voiceflow
