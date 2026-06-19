#include "util/single_instance.h"

namespace voiceflow {

bool AcquireSingleInstance(const wchar_t* mutex_name) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, mutex_name);
    if (!mutex) {
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return false;
    }
    return true;
}

}  // namespace voiceflow
