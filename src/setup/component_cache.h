#pragma once

#include <string>

namespace voiceflow {

bool HasRuntime();
bool HasLanguageModel(const wchar_t* language_code);
bool IsSetupComplete();
bool EnsureDataDirectories();

std::wstring ResolveModelPath(const wchar_t* language_code);

}  // namespace voiceflow
