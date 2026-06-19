#pragma once

#include <windows.h>

namespace voiceflow {

bool AcquireSingleInstance(const wchar_t* mutex_name);

}  // namespace voiceflow
