#pragma once

#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <portaudio.h>
#include "Constants.h"

struct AudioDevice {
    int id;
    std::string name;
    int maxInputChannels;
};

class AudioRecorder {
public:
    AudioRecorder();
    ~AudioRecorder();

    bool initialize(int deviceId = -1);
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording.load(); }
    std::vector<float> getAudioData();

    static std::vector<AudioDevice> getInputDevices();

private:
    static int audioCallback(const void* input, void* output,
                           unsigned long frameCount,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void* userData);

    PaStream* stream;
    std::vector<float> ringBuffer_;
    std::atomic<size_t> write_index_{0};
    std::atomic<size_t> total_written_{0};
    std::atomic<bool> recording;
    size_t capacity_ { 0 };
    std::vector<float> last_capture_;
    std::mutex prepare_mutex_;
    static constexpr int sampleRate = constants::kSampleRate;
    static constexpr int channels = constants::kChannels;
    static constexpr int framesPerBuffer = constants::kFramesPerBuffer;
};
