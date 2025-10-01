cmake_minimum_required(VERSION 3.15)
project(Rose)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Needed for Objective-C sources in ggml-metal
enable_language(OBJC)
enable_language(OBJCXX)

find_package(Threads REQUIRED)

# PortAudio
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h
    HINTS /opt/homebrew/include /usr/local/include
)
find_library(PORTAUDIO_LIB portaudio
    HINTS /opt/homebrew/lib /usr/local/lib
)
if (NOT PORTAUDIO_LIB)
    message(FATAL_ERROR "PortAudio not found. Install via Homebrew: brew install portaudio")
endif()

# Enable Metal acceleration for whisper.cpp / ggml on Apple Silicon
set(GGML_METAL ON CACHE BOOL "Enable GGML Metal backend" FORCE)
set(GGML_METAL_EMBED_LIBRARY ON CACHE BOOL "Embed Metal library into binary" FORCE)
add_subdirectory(vendor/whisper.cpp)

set(SOURCES
    src/main.cpp
    src/AudioRecorder.cpp
    src/WhisperProcessor.cpp
    src/WhisperContext.cpp
    src/AudioUtils.cpp
    src/TextScoring.cpp
    src/HotkeyMonitor.cpp
    src/ClipboardManager.mm
    src/MenuBarUI.mm
    src/Settings.cpp
)

set_source_files_properties(src/ClipboardManager.mm PROPERTIES LANGUAGE OBJCXX)
set_source_files_properties(src/MenuBarUI.mm PROPERTIES LANGUAGE OBJCXX)
set_source_files_properties(src/ClipboardManager.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
set_source_files_properties(src/MenuBarUI.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")

add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/mac/Info.plist"
)

# When using the Xcode generator, apply strict sandbox entitlements (microphone only)
if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/mac/Rose.entitlements"
    )
endif()

# Ensure bundle binary directory exists before linking
add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# Copy models into the app bundle if present
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/models"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/models"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/models"
    COMMENT "Copying models into app bundle Resources"
)

# Copy icons/resources
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/mac/icon.svg"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/icon.svg"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    vendor/whisper.cpp/include
    ${PORTAUDIO_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    whisper
    Threads::Threads
    ${PORTAUDIO_LIB}
    "-framework Cocoa"
    "-framework Carbon"
    "-framework CoreGraphics"
    "-framework ApplicationServices"
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework AudioUnit"
    "-framework CoreServices"
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O3)

# Tests (simple, header-only style)
add_executable(rose_tests
    tests/test_main.cpp
    src/AudioUtils.cpp
    src/TextScoring.cpp
)
target_include_directories(rose_tests PRIVATE include)
target_compile_options(rose_tests PRIVATE -Wall -Wextra -O2)
