#include "AudioRecorder.h"
#include "Settings.h"
#include "Constants.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <atomic>

namespace { std::atomic<bool> g_pa_initialized{false}; }

AudioRecorder::AudioRecorder() : stream(nullptr), recording(false) {}

AudioRecorder::~AudioRecorder() {
    if (stream) {
        Pa_CloseStream(stream);
    }
    if (g_pa_initialized.load(std::memory_order_relaxed)) {
        Pa_Terminate();
        g_pa_initialized.store(false, std::memory_order_relaxed);
    }
}

bool AudioRecorder::initialize(int deviceId) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        return false;
    }
    g_pa_initialized.store(true, std::memory_order_relaxed);

    PaStreamParameters inputParams;
    std::memset(&inputParams, 0, sizeof inputParams);

    if (deviceId < 0) {
        deviceId = Settings::getInstance().getDeviceId();
    }

    if (deviceId < 0 || deviceId >= Pa_GetDeviceCount()) {
        inputParams.device = Pa_GetDefaultInputDevice();
    } else {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(deviceId);
        if (info && info->maxInputChannels > 0) {
            inputParams.device = deviceId;
        } else {
            inputParams.device = Pa_GetDefaultInputDevice();
        }
    }

    if (inputParams.device == paNoDevice) {
        return false;
    }

    inputParams.channelCount = channels;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&stream,
                        &inputParams,
                        nullptr,
                        sampleRate,
                        framesPerBuffer,
                        paClipOff,
                        audioCallback,
                        this);

    if (err != paNoError) return false;

    capacity_ = static_cast<size_t>(sampleRate * constants::kMaxRecordingSeconds * channels);
    ringBuffer_.assign(capacity_, 0.0f);
    write_index_.store(0, std::memory_order_relaxed);
    total_written_.store(0, std::memory_order_relaxed);
    return true;
}

void AudioRecorder::startRecording() {
    if (!recording) {
        write_index_.store(0, std::memory_order_relaxed);
        total_written_.store(0, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lk(prepare_mutex_);
            last_capture_.clear();
        }
        recording = true;
        if (PaError err = Pa_StartStream(stream); err != paNoError) {
            recording = false;
            std::cerr << "[rose] pa start failed: " << err << "\n";
        }
    }
}

void AudioRecorder::stopRecording() {
    if (recording) {
        recording = false;
        if (PaError err = Pa_StopStream(stream); err != paNoError) {
            std::cerr << "[rose] pa stop failed: " << err << "\n";
        }
        const size_t total = total_written_.load(std::memory_order_relaxed);
        const size_t count = std::min(capacity_, total);
        const size_t wi = write_index_.load(std::memory_order_relaxed);
        if (count > 0 && capacity_ > 0) {
            std::vector<float> tmp;
            tmp.resize(count);
            const size_t start = (wi + capacity_ - (count % capacity_)) % capacity_;
            if (start + count <= capacity_) {
                std::memcpy(tmp.data(), ringBuffer_.data() + start, count * sizeof(float));
            } else {
                const size_t first = capacity_ - start;
                std::memcpy(tmp.data(), ringBuffer_.data() + start, first * sizeof(float));
                std::memcpy(tmp.data() + first, ringBuffer_.data(), (count - first) * sizeof(float));
            }
            {
                std::lock_guard<std::mutex> lk(prepare_mutex_);
                last_capture_.swap(tmp);
            }
        } else {
            std::lock_guard<std::mutex> lk(prepare_mutex_);
            last_capture_.clear();
        }
    }
}

std::vector<float> AudioRecorder::getAudioData() {
    std::vector<float> data;
    {
        std::lock_guard<std::mutex> lk(prepare_mutex_);
        data.swap(last_capture_);
    }

    float maxAmp = 0.0f;
    for (float sample : data) {
        float abs = std::abs(sample);
        if (abs > maxAmp) maxAmp = abs;
    }

    if (maxAmp > 0.0f && maxAmp < constants::kAutoGainThreshold) {
        float scale = constants::kAutoGainTarget / maxAmp;
        for (float& sample : data) sample *= scale;
    }
    for (float& sample : data) {
        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
    }

    return data;
}

int AudioRecorder::audioCallback(const void* input, void* output,
                                 unsigned long frameCount,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void* userData) {
    (void)output;
    (void)timeInfo;
    (void)statusFlags;

    AudioRecorder* recorder = static_cast<AudioRecorder*>(userData);
    if (!recorder->recording) {
        return paContinue;
    }

    if (!input) return paContinue;
    const float* inputData = static_cast<const float*>(input);
    const size_t n = static_cast<size_t>(frameCount) * static_cast<size_t>(channels);

    if (recorder->capacity_ == 0 || recorder->ringBuffer_.empty()) {
        return paContinue;
    }

    size_t wi = recorder->write_index_.load(std::memory_order_relaxed);
    const size_t cap = recorder->capacity_;

    size_t first = std::min(n, cap - (wi % cap));
    std::memcpy(recorder->ringBuffer_.data() + (wi % cap), inputData, first * sizeof(float));
    if (first < n) {
        std::memcpy(recorder->ringBuffer_.data(), inputData + first, (n - first) * sizeof(float));
    }
    recorder->write_index_.store((wi + n) % cap, std::memory_order_relaxed);
    recorder->total_written_.fetch_add(n, std::memory_order_relaxed);

    return paContinue;
}

std::vector<AudioDevice> AudioRecorder::getInputDevices() {
    std::vector<AudioDevice> devices;

    bool didInit = false;
    if (!g_pa_initialized.load(std::memory_order_relaxed)) {
        if (Pa_Initialize() == paNoError) {
            didInit = true;
        }
    }

    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0) {
            AudioDevice device;
            device.id = i;
            device.name = info->name;
            device.maxInputChannels = info->maxInputChannels;
            devices.push_back(device);
        }
    }

    if (didInit) Pa_Terminate();
    return devices;
}
