#include "app.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    voiceflow::VoiceFlowApp app;
    return app.Run(instance);
}
