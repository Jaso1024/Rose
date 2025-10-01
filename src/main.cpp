#include "AudioRecorder.h"
#include "WhisperProcessor.h"
#include "HotkeyMonitor.h"
#include "ClipboardManager.h"
#include "MenuBarUI.h"
#include "Settings.h"
#include "DispatchQueue.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <algorithm>
#include <cctype>
#include "Constants.h"

class App {
public:
    App() : running(true), processingQueue("com.rose.processing") {}

    bool initialize() {
        Settings::getInstance().load();
        Settings::getInstance().setOnChangeCallback([this]{ onSettingsChange(); });

        if (!audioRecorder.initialize()) {
            std::cerr << "[rose] audio init failed\n";
            return false;
        }
        std::cout << "[rose] audio ready\n";

        modelReady = false;

        if (!hotkeyMonitor.initialize([this]() { toggleRecording(); })) {
            std::cerr << "[rose] hotkey unavailable\n";
        }

        if (!menuBar.initialize([this]() { quit(); },
                               [this]() { onSettingsChange(); },
                               [this]() { toggleRecording(); })) {
            std::cerr << "[rose] menubar init failed\n";
            return false;
        }

        hotkeyMonitor.start();
        std::cout << "[rose] init ok\n";
        return true;
    }

    bool ensureModelLoaded() {
        if (modelReady.load(std::memory_order_relaxed)) return true;
        const std::string path = Settings::getInstance().getModelPath();
        if (whisperProcessor.initialize(path)) {
            modelReady.store(true, std::memory_order_relaxed);
            std::cout << "[rose] model: " << Settings::getInstance().getModelName() << "\n";
            return true;
        }
        for (const auto& fb : constants::ModelFallbacks()) {
            if (whisperProcessor.initialize(fb)) {
                modelReady.store(true, std::memory_order_relaxed);
                std::cout << "[rose] model: fallback " << fb << "\n";
                return true;
            }
        }
        std::cerr << "[rose] model missing (place a ggml in models/)\n";
        return false;
    }

    void run() {
        std::string hotkey = Settings::getInstance().getHotkey();
        std::string displayHotkey = hotkey;
        std::replace(displayHotkey.begin(), displayHotkey.end(), '+', ' ');
        for (auto& c : displayHotkey) c = std::toupper(c);
        std::cout << "[rose] started (" << displayHotkey << ")\n";
        menuBar.run();
    }

private:
    void toggleRecording() {
        DispatchQueue::main_async([this]{
            if (audioRecorder.isRecording()) stopRecording(); else startRecording();
        });
    }

    void startRecording() {
        std::cout << "[rose] rec start\n";
        audioRecorder.startRecording();
        menuBar.setRecordingState(true);
        cancelScheduledUnload();
        preloadModelAsync();
    }

    void stopRecording() {
        std::cout << "[rose] rec stop\n";
        audioRecorder.stopRecording();
        menuBar.setRecordingState(false);
        cancelScheduledUnload();
        processingQueue.async([this]{ processAudio(); });
    }

    void processAudio() {
        std::vector<float> audioData = audioRecorder.getAudioData();
        if (audioData.empty()) {
            std::cout << "No audio data captured\n";
            return;
        }

        std::cout << "[rose] samples: " << audioData.size() << "\n";

        if (!ensureModelLoaded()) return;
        std::string transcription = whisperProcessor.transcribe(audioData);

        if (!transcription.empty()) {
            std::cout << "[rose] text: " << transcription << "\n";

            ClipboardManager::copyToClipboard(transcription);
            std::cout << "[rose] copied\n";
        } else {
            std::cout << "[rose] empty\n";
        }
        scheduleModelUnload();
    }

    void onSettingsChange() {
        std::cout << "[rose] settings changed\n";
        modelReady.store(false, std::memory_order_relaxed);
        hotkeyMonitor.update();
        menuBar.updateMenu();
    }

    void quit() {
        running = false;
        hotkeyMonitor.stop();
        Settings::getInstance().save();
        exit(0);
    }

    AudioRecorder audioRecorder;
    WhisperProcessor whisperProcessor;
    HotkeyMonitor hotkeyMonitor;
    MenuBarUI menuBar;
    std::atomic<bool> running;
    std::atomic<bool> modelReady{false};
    std::atomic<bool> modelLoading{false};
    DispatchQueue processingQueue;

    void preloadModelAsync() {
        if (modelReady.load(std::memory_order_relaxed)) return;
        bool expected = false;
        if (!modelLoading.compare_exchange_strong(expected, true, std::memory_order_relaxed)) return;
        processingQueue.async([this]{
            (void)ensureModelLoaded();
            modelLoading.store(false, std::memory_order_relaxed);
        });
    }

    void unloadModel() {
        whisperProcessor.unload();
        modelReady.store(false, std::memory_order_relaxed);
    }

    void scheduleModelUnload() {
        int seconds = Settings::getInstance().getModelRetainSeconds();
        if (seconds < constants::kRetainSecondsMin) seconds = constants::kRetainSecondsMin;
        if (seconds > constants::kRetainSecondsMax) seconds = constants::kRetainSecondsMax;
        int gen = ++unloadGeneration;
        dispatch_time_t when = dispatch_time(DISPATCH_TIME_NOW, (int64_t)seconds * NSEC_PER_SEC);
        dispatch_after(when, dispatch_get_main_queue(), ^{
            if (unloadGeneration.load() == gen) {
                processingQueue.async([this]{ unloadModel(); });
            }
        });
    }

    void cancelScheduledUnload() {
        ++unloadGeneration;
    }

    std::atomic<int> unloadGeneration{0};
};

int main() {
    App app;
    if (!app.initialize()) {
        std::cerr << "[rose] init failed\n";
        return 1;
    }
    app.run();
    return 0;
}
