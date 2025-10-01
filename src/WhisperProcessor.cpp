#include "WhisperProcessor.h"
#include "Settings.h"
#include "Constants.h"
#include "AudioUtils.h"
#include "TextScoring.h"
#include "WhisperContext.h"
#include "whisper.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <future>
#include <limits>
#include <iostream>

WhisperProcessor::WhisperProcessor() = default;

WhisperProcessor::~WhisperProcessor() = default;

bool WhisperProcessor::initialize(const std::string& modelPath) {
    return context.initialize(modelPath, constants::kUseGPU);
}

std::vector<float> WhisperProcessor::applyHighPassFilter(const std::vector<float>& audioData) {
    return audio::apply_high_pass_filter(audioData, constants::kSampleRate, constants::kHighPassCutoffHz);
}

std::vector<float> WhisperProcessor::removeNoise(const std::vector<float>& audioData) {
    return audio::remove_noise(audioData, constants::kNoiseWindowSize, constants::kNoiseFloorFactor, constants::kNoiseAttenuation);
}

std::vector<float> WhisperProcessor::normalizeAudio(const std::vector<float>& audioData) {
    return audio::normalize(audioData, constants::kNormalizeMinAmp, constants::kNormalizeTargetAmp);
}

bool WhisperProcessor::detectVoiceActivity(const std::vector<float>& audioData) {
    return audio::detect_voice_activity(audioData, constants::kVADMinEnergy, constants::kVADZcrMin, constants::kVADZcrMax);
}

std::vector<float> WhisperProcessor::preprocessAudio(const std::vector<float>& audioData) {
    return audio::preprocess(audioData,
                             constants::kSampleRate,
                             constants::kHighPassCutoffHz,
                             constants::kNoiseWindowSize,
                             constants::kNoiseFloorFactor,
                             constants::kNoiseAttenuation,
                             constants::kNormalizeMinAmp,
                             constants::kNormalizeTargetAmp);
}

TranscriptionResult WhisperProcessor::runTranscription(
    const std::vector<float>& audioData, float temperature) {

    TranscriptionResult result;
    result.text = "";
    result.avg_logprob = -std::numeric_limits<float>::infinity();
    result.no_speech_prob = 0.0f;
    result.score = 0.0f;

    if (!context.valid() || audioData.empty()) {
        return result;
    }

    auto state = context.createState();
    if (!state) return result;

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_special = false;
    params.print_realtime = false;
    params.print_timestamps = false;
    params.single_segment = false;
    params.no_context = false;
    const std::string lang = Settings::getInstance().getLanguage();
    if (lang == "auto" || lang.empty()) {
        params.detect_language = true;
        params.language = nullptr;
    } else {
        params.detect_language = false;
        params.language = lang.c_str();
    }
    params.n_threads = constants::kWhisperThreads;
    params.temperature = temperature;
    params.suppress_blank = true;
    params.suppress_nst = true;
    params.max_initial_ts = constants::kWhisperMaxInitialTs;
    params.entropy_thold = constants::kWhisperEntropyThold;
    params.logprob_thold = constants::kWhisperLogprobThold;
    params.greedy.best_of = constants::kWhisperGreedyBestOf;

    if (whisper_full_with_state(context.get(), state.get(), params, audioData.data(), audioData.size()) == 0) {
        const int n_segments = whisper_full_n_segments_from_state(state.get());

        float total_logprob = 0.0f;
        int total_tokens = 0;

        for (int i = 0; i < n_segments; ++i) {
            const char* text = whisper_full_get_segment_text_from_state(state.get(), i);
            if (text && text[0]) {
                if (!result.text.empty() && result.text.back() != ' ') {
                    result.text += " ";
                }
                result.text += text;
            }

            result.no_speech_prob = std::max(result.no_speech_prob,
                whisper_full_get_segment_no_speech_prob_from_state(state.get(), i));

            const int n_tokens = whisper_full_n_tokens_from_state(state.get(), i);
            for (int j = 0; j < n_tokens; ++j) {
                float token_prob = whisper_full_get_token_p_from_state(state.get(), i, j);
                if (token_prob > 0) {
                    total_logprob += std::log(token_prob);
                    total_tokens++;
                }
            }
        }

        if (total_tokens > 0) {
            result.avg_logprob = total_logprob / total_tokens;
        }

        result.score = textscore::score(result.avg_logprob, result.no_speech_prob);
    }

    return result;
}

TranscriptionResult WhisperProcessor::selectBestResult(
    const std::vector<TranscriptionResult>& results) {

    if (results.empty()) {
        return TranscriptionResult{"", -std::numeric_limits<float>::infinity(), 1.0f, 0.0f};
    }
    return textscore::select_best(results);
}

std::string WhisperProcessor::transcribe(const std::vector<float>& audioData) {
    if (!context.valid() || audioData.empty()) {
        return "";
    }

    std::vector<float> processed = preprocessAudio(audioData);

    bool sufficient_length = processed.size() >= static_cast<size_t>(constants::kSampleRate / 2);
    bool vad_ok = detectVoiceActivity(processed);

    if (constants::kDebugLogging) {
        // Compute simple energy/ZCR stats for visibility
        float energy = 0.0f;
        for (float s : processed) energy += s * s;
        energy = processed.empty() ? 0.0f : energy / processed.size();
        float zero_cross = 0.0f;
        for (size_t i = 1; i < processed.size(); ++i) {
            if ((processed[i - 1] >= 0) != (processed[i] >= 0)) zero_cross += 1.0f;
        }
        const float zcr = processed.empty() ? 0.0f : zero_cross / processed.size();
        std::cout << "[rose] preprocess: "
                  << (processed.size() / static_cast<float>(constants::kSampleRate)) * 1000.0f
                  << " ms, energy=" << energy
                  << ", zcr=" << zcr
                  << ", vad=" << (vad_ok ? "yes" : "no")
                  << ", len_ok=" << (sufficient_length ? "yes" : "no")
                  << "\n";
    }

    // If preprocessing thinks this is too short or not voiced, try a permissive fallback
    std::vector<float> to_transcribe;
    if (sufficient_length && vad_ok) {
        to_transcribe = std::move(processed);
    } else {
        // Fallback: only high-pass + normalize; skip silence trim + VAD gate
        auto hp = applyHighPassFilter(audioData);
        to_transcribe = normalizeAudio(hp);
        if (constants::kDebugLogging) {
            std::cout << "[rose] fallback enabled (permissive preprocessing)\n";
        }
        if (to_transcribe.size() < static_cast<size_t>(constants::kSampleRate / 2)) {
            return "";
        }
    }

    int bestOfN = Settings::getInstance().getBestOfN();
    std::vector<std::future<TranscriptionResult>> futures;
    const auto& temperatures = constants::Temperatures();
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    int cap = static_cast<int>(hw);
    if (constants::kUseGPU) cap = 2;
    const int max_tasks = std::min(bestOfN, std::min(static_cast<int>(temperatures.size()), cap));

    for (int i = 0; i < max_tasks; ++i) {
        futures.push_back(std::async(std::launch::async,
            [this, &to_transcribe, temp = temperatures[i]]() {
                return runTranscription(to_transcribe, temp);
            }));
    }

    std::vector<TranscriptionResult> results;
    for (auto& future : futures) {
        results.push_back(future.get());
    }

    TranscriptionResult best = selectBestResult(results);

    if (constants::kDebugLogging) {
        std::cout << "[rose] decode: avg_logprob=" << best.avg_logprob
                  << ", no_speech_prob=" << best.no_speech_prob
                  << ", text_len=" << best.text.size() << "\n";
    }

    if (best.no_speech_prob > constants::kNoSpeechProbThreshold && best.text.empty()) {
        return "";
    }

    return best.text;
}

void WhisperProcessor::unload() {
    context.reset();
}
