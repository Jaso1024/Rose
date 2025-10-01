#pragma once

#include <string>
#include <utility>
#include <vector>

namespace constants {

inline constexpr int kSampleRate = 16000;
inline constexpr int kChannels = 1;
inline constexpr int kFramesPerBuffer = 2048;
inline constexpr int kMaxRecordingSeconds = 30;

inline constexpr float kAutoGainThreshold = 0.5f;
inline constexpr float kAutoGainTarget = 0.8f;

inline constexpr float kHighPassCutoffHz = 80.0f;
inline constexpr size_t kNoiseWindowSize = 256;
inline constexpr float kNoiseFloorFactor = 0.3f;
inline constexpr float kNoiseAttenuation = 0.1f;
inline constexpr float kNormalizeMinAmp = 0.001f;
inline constexpr float kNormalizeTargetAmp = 0.95f;

inline constexpr float kVADMinEnergy = 0.001f;
inline constexpr float kVADZcrMin = 0.02f;
inline constexpr float kVADZcrMax = 0.5f;

inline constexpr int kWhisperThreads = 2;
inline constexpr int kWhisperGreedyBestOf = 1;
inline constexpr float kNoSpeechProbThreshold = 0.6f;
inline constexpr bool kUseGPU = true;
inline constexpr float kWhisperMaxInitialTs = 1.0f;
inline constexpr float kWhisperEntropyThold = 2.4f;
inline constexpr float kWhisperLogprobThold = -1.0f;

// Enable extra debug logs for troubleshooting (prints preprocessing + VAD stats)
inline constexpr bool kDebugLogging = false;

inline constexpr int kBestOfNMin = 1;
inline constexpr int kBestOfNDefault = 5;
inline constexpr int kBestOfNMax = 10;

inline constexpr int kRetainSecondsMin = 0;
inline constexpr int kRetainSecondsDefault = 10;
inline constexpr int kRetainSecondsMax = 120;

inline const std::string kDefaultHotkey = "cmd+shift+space";

inline const std::vector<std::pair<std::string, std::string>>& HotkeyOptions() {
    static const std::vector<std::pair<std::string, std::string>> opts = {
        {"cmd+shift+space", "⌘⇧Space"},
        {"cmd+option+space", "⌘⌥Space"},
        {"option+space", "⌥Space"},
        {"cmd+shift+r", "⌘⇧R"},
        {"cmd+option+r", "⌘⌥R"},
    };
    return opts;
}

inline const std::vector<float>& Temperatures() {
    static const std::vector<float> temps = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 1.2f, 1.4f, 1.6f, 1.8f};
    return temps;
}

inline const std::vector<std::string>& ModelFallbacks() {
    static const std::vector<std::string> models = {
        "models/ggml-tiny.en.bin",
        "models/ggml-base.en.bin",
        "models/ggml-small.en.bin",
        "models/ggml-medium.en.bin",
        "models/ggml-large-v3.bin",
    };
    return models;
}

inline const std::vector<std::pair<std::string, std::string>>& LanguageOptions() {
    static const std::vector<std::pair<std::string, std::string>> langs = {
        {"auto", "Auto Detect"},
        {"en", "English"},
        {"es", "Spanish"},
        {"fr", "French"},
        {"de", "German"},
        {"it", "Italian"},
        {"pt", "Portuguese"},
        {"nl", "Dutch"},
        {"ru", "Russian"},
        {"zh", "Chinese"},
        {"ja", "Japanese"},
        {"ko", "Korean"},
    };
    return langs;
}

} // namespace constants
