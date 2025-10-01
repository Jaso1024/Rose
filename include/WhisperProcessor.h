#pragma once

#include <string>
#include <vector>
#include "TextScoring.h"
#include "WhisperContext.h"

class WhisperProcessor {
public:
    WhisperProcessor();
    ~WhisperProcessor();

    bool initialize(const std::string& modelPath);
    std::string transcribe(const std::vector<float>& audioData);
    void unload();

private:
    std::vector<float> preprocessAudio(const std::vector<float>& audioData);
    std::vector<float> removeNoise(const std::vector<float>& audioData);
    std::vector<float> normalizeAudio(const std::vector<float>& audioData);
    std::vector<float> applyHighPassFilter(const std::vector<float>& audioData);
    bool detectVoiceActivity(const std::vector<float>& audioData);
    TranscriptionResult runTranscription(const std::vector<float>& audioData, float temperature);
    TranscriptionResult selectBestResult(const std::vector<TranscriptionResult>& results);

    WhisperContext context;
};
