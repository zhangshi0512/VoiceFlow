#include "app.h"

#include "app_messages.h"
#include "config/settings.h"
#include "hotkey/global_hotkey.h"
#include "inject/text_injector.h"
#include "setup/component_cache.h"
#include "setup/first_run_wizard.h"
#include "tray/tray_icon.h"
#include "ui/settings_dialog.h"
#include "util/single_instance.h"

#ifdef VOICEFLOW_USE_PYTHON_ASR
#include "asr/python_asr_bridge.h"
#elif defined(VOICEFLOW_HAS_MOONSHINE)
#include "asr/moonshine_session.h"
#endif

#include <memory>
#include <string>

namespace voiceflow {

namespace {

constexpr wchar_t kWindowClassName[] = L"VoiceFlowHiddenWindow";
VoiceFlowApp* g_app = nullptr;
std::unique_ptr<TrayIcon> g_tray;
std::unique_ptr<GlobalHotkey> g_hotkey;
std::unique_ptr<TextInjector> g_injector;

void OnHotkeyAction(WPARAM action, LPARAM, void* user_data) {
    auto* app = static_cast<VoiceFlowApp*>(user_data);
    if (!app) {
        return;
    }
    if (action == 1) {
        app->StartDictation();
    } else if (action == 2) {
        app->StopDictation();
    }
}

}  // namespace

VoiceFlowApp::VoiceFlowApp() = default;

VoiceFlowApp::~VoiceFlowApp() {
#ifdef VOICEFLOW_USE_PYTHON_ASR
    delete session_;
#elif defined(VOICEFLOW_HAS_MOONSHINE)
    delete session_;
#endif
    session_ = nullptr;
}

void VoiceFlowApp::RegisterWindowClass(HINSTANCE instance) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kWindowClassName;
    RegisterClassExW(&wc);
}

bool VoiceFlowApp::CreateMessageWindow(HINSTANCE instance) {
    instance_ = instance;
    RegisterWindowClass(instance);
    hwnd_ = CreateWindowExW(0, kWindowClassName, L"VoiceFlow", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance,
                              this);
    return hwnd_ != nullptr;
}

int VoiceFlowApp::Run(HINSTANCE instance) {
    if (!AcquireSingleInstance(L"VoiceFlow_SingleInstance")) {
        MessageBoxW(nullptr, L"VoiceFlow is already running.", L"VoiceFlow", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    g_app = this;
    if (!CreateMessageWindow(instance)) {
        return 1;
    }

    ShowWindow(hwnd_, SW_HIDE);
    UpdateWindow(hwnd_);
    OnCreate();

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_app = nullptr;
    return static_cast<int>(msg.wParam);
}

void VoiceFlowApp::OnCreate() {
    LoadSettings(settings_);
    EnsureDataDirectories();

#if !defined(VOICEFLOW_USE_PYTHON_ASR)
    if (!IsSetupComplete()) {
        PostMessageW(hwnd_, WM_VOICEFLOW_SETUP_REQUIRED, 0, 0);
    }
#endif

    g_tray = std::make_unique<TrayIcon>();
    g_tray->Create(hwnd_, instance_, [this](UINT command) { HandleTrayCommand(command); });

    g_hotkey = std::make_unique<GlobalHotkey>();
    g_hotkey->Install(hwnd_, settings_.hotkey_mode, OnHotkeyAction, this);

    g_injector = std::make_unique<TextInjector>();

#ifdef VOICEFLOW_USE_PYTHON_ASR
    session_ = new PythonAsrSession([this](const std::wstring& text) { OnTranscript(text); });
    session_->Initialize(settings_.language, settings_.force_cpu);
#elif defined(VOICEFLOW_HAS_MOONSHINE)
    session_ = new MoonshineSession([this](const std::wstring& text) { OnTranscript(text); });
    session_->Initialize(settings_.language, settings_.force_cpu);
#endif

    g_tray->ShowBalloon(L"VoiceFlow", L"Running in the system tray. Use Ctrl+Shift+Space to dictate.");
}

void VoiceFlowApp::OnDestroy() {
    if (g_hotkey) {
        g_hotkey->Uninstall();
        g_hotkey.reset();
    }
    if (g_tray) {
        g_tray->Destroy();
        g_tray.reset();
    }
}

void VoiceFlowApp::ApplySettings() {
    SaveSettings(settings_);
    if (g_hotkey) {
        g_hotkey->SetMode(settings_.hotkey_mode);
    }
#ifdef VOICEFLOW_USE_PYTHON_ASR
    if (session_) {
        session_->Initialize(settings_.language, settings_.force_cpu);
    }
#elif defined(VOICEFLOW_HAS_MOONSHINE)
    if (session_) {
        session_->Initialize(settings_.language, settings_.force_cpu);
    }
#endif
}

void VoiceFlowApp::StartDictation() {
    if (dictation_active_) {
        return;
    }
#if !defined(VOICEFLOW_USE_PYTHON_ASR) && !defined(VOICEFLOW_HAS_MOONSHINE)
    MessageBoxW(hwnd_, L"VoiceFlow was built without ASR support.", L"VoiceFlow", MB_OK | MB_ICONWARNING);
    return;
#else
#if !defined(VOICEFLOW_USE_PYTHON_ASR)
    if (!IsSetupComplete()) {
        OnSetupRequired();
        return;
    }
#endif
    if (!session_ || !session_->Initialize(settings_.language, settings_.force_cpu)) {
        g_tray->ShowBalloon(L"VoiceFlow", L"Failed to initialize speech engine.");
        return;
    }
    dictation_active_ = true;
    session_->Start();
    if (g_tray) {
        g_tray->SetRecording(true);
    }
#endif
}

void VoiceFlowApp::StopDictation() {
    if (!dictation_active_) {
        return;
    }
#if defined(VOICEFLOW_USE_PYTHON_ASR) || defined(VOICEFLOW_HAS_MOONSHINE)
    if (session_) {
        session_->Stop();
    }
#endif
    dictation_active_ = false;
    if (g_tray) {
        g_tray->SetRecording(false);
    }
}

void VoiceFlowApp::OnTranscript(const std::wstring& text) {
    if (g_injector && !text.empty()) {
        g_injector->Inject(text);
    }
}

void VoiceFlowApp::OnSetupRequired() {
    if (!RunFirstRunWizard(hwnd_)) {
        g_tray->ShowBalloon(L"VoiceFlow", L"Setup incomplete. Open Settings after installing models.");
    }
}

void VoiceFlowApp::HandleTrayCommand(UINT command) {
    switch (command) {
        case TRAY_CMD_EXIT:
            DestroyWindow(hwnd_);
            break;
        case TRAY_CMD_SETTINGS:
            if (ShowSettingsDialog(hwnd_, settings_)) {
                ApplySettings();
            }
            break;
        case TRAY_CMD_LANGUAGE_EN:
            settings_.language = Language::English;
            ApplySettings();
            g_tray->ShowBalloon(L"VoiceFlow", L"Language set to English.");
            break;
        case TRAY_CMD_LANGUAGE_ZH:
            settings_.language = Language::Chinese;
            ApplySettings();
            g_tray->ShowBalloon(L"VoiceFlow", L"Language set to Chinese.");
            break;
        default:
            break;
    }
}

LRESULT CALLBACK VoiceFlowApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }

    auto* app = reinterpret_cast<VoiceFlowApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (msg == TRAY_CALLBACK_MESSAGE && app && g_tray) {
        switch (LOWORD(lparam)) {
            case WM_RBUTTONUP:
                g_tray->ShowContextMenu();
                return 0;
            case WM_LBUTTONDBLCLK:
                app->HandleTrayCommand(TRAY_CMD_SETTINGS);
                return 0;
            default:
                break;
        }
    }

    if (!app) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    switch (msg) {
        case WM_VOICEFLOW_SETUP_REQUIRED:
            app->OnSetupRequired();
            return 0;
        case WM_DESTROY:
            app->OnDestroy();
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

}  // namespace voiceflow
