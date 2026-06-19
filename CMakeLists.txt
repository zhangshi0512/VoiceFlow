cmake_minimum_required(VERSION 3.20)
project(VoiceFlow VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MOONSHINE_ROOT "${CMAKE_SOURCE_DIR}/third_party/moonshine-voice-windows-x86_64")
set(MOONSHINE_HEADERS_PRESENT OFF)
if(EXISTS "${MOONSHINE_ROOT}/include/moonshine-cpp.h")
    set(MOONSHINE_HEADERS_PRESENT ON)
endif()

option(VOICEFLOW_USE_PYTHON_ASR "Use Python moonshine-voice worker for ASR" ON)

if(VOICEFLOW_USE_PYTHON_ASR)
    message(STATUS "ASR backend: Python moonshine-voice worker")
elseif(MOONSHINE_HEADERS_PRESENT)
    set(VOICEFLOW_HAS_MOONSHINE ON)
    message(STATUS "ASR backend: Moonshine C++ (${MOONSHINE_ROOT})")
else()
    message(WARNING "No ASR backend enabled. Enable VOICEFLOW_USE_PYTHON_ASR or fetch Moonshine SDK.")
endif()

set(VOICEFLOW_SOURCES
    src/main.cpp
    src/app.cpp
    src/util/single_instance.cpp
    src/util/paths.cpp
    src/config/settings.cpp
    src/tray/tray_icon.cpp
    src/hotkey/global_hotkey.cpp
    src/hotkey/hotkey_mode.cpp
    src/inject/text_injector.cpp
    src/setup/component_cache.cpp
    src/setup/first_run_wizard.cpp
    src/ui/settings_dialog.cpp
    src/resource.rc
)

if(VOICEFLOW_USE_PYTHON_ASR)
    list(APPEND VOICEFLOW_SOURCES src/asr/python_asr_bridge.cpp)
elseif(VOICEFLOW_HAS_MOONSHINE)
    list(APPEND VOICEFLOW_SOURCES
        src/asr/microphone_capture.cpp
        src/asr/moonshine_session.cpp
    )
endif()

add_executable(VoiceFlow WIN32 ${VOICEFLOW_SOURCES})

target_include_directories(VoiceFlow PRIVATE "${CMAKE_SOURCE_DIR}/src")

target_compile_definitions(VoiceFlow PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(VoiceFlow PRIVATE
    shell32
    ole32
    winhttp
    comctl32
    user32
    gdi32
    advapi32
)

if(VOICEFLOW_USE_PYTHON_ASR)
    target_compile_definitions(VoiceFlow PRIVATE VOICEFLOW_USE_PYTHON_ASR=1)
elseif(VOICEFLOW_HAS_MOONSHINE)
    target_include_directories(VoiceFlow PRIVATE "${MOONSHINE_ROOT}/include")
    target_link_directories(VoiceFlow PRIVATE "${MOONSHINE_ROOT}/lib")
    target_link_libraries(VoiceFlow PRIVATE
        moonshine
        bin-tokenizer
        moonshine-utils
        onnxruntime
        ort-utils
        mmdevapi
    )
    target_compile_definitions(VoiceFlow PRIVATE VOICEFLOW_HAS_MOONSHINE=1)

    add_custom_command(TARGET VoiceFlow POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MOONSHINE_ROOT}/lib/onnxruntime.dll"
            "$<TARGET_FILE_DIR:VoiceFlow>"
        COMMENT "Copying onnxruntime.dll"
    )
endif()

add_custom_command(TARGET VoiceFlow POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:VoiceFlow>/packaging"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/packaging/voiceflow_asr_worker.py"
        "$<TARGET_FILE_DIR:VoiceFlow>/packaging/voiceflow_asr_worker.py"
    COMMENT "Copying ASR worker script"
)
