#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "Constants.h"
#include "AudioUtils.h"
#include "TextScoring.h"

using std::vector;

static void test_text_scoring() {
    TranscriptionResult a{"hello", -0.5f, 0.2f, 0.0f};
    TranscriptionResult b{"world", -0.2f, 0.8f, 0.0f};
    TranscriptionResult c{"test", -1.0f, 0.1f, 0.0f};

    a.score = textscore::score(a);
    b.score = textscore::score(b);
    c.score = textscore::score(c);

    // a: -0.5 * 0.8 = -0.4
    // b: -0.2 * 0.2 = -0.04
    // c: -1.0 * 0.9 = -0.9
    assert(std::abs(a.score + 0.4f) < 1e-6);
    assert(std::abs(b.score + 0.04f) < 1e-6);
    assert(std::abs(c.score + 0.9f) < 1e-6);

    vector<TranscriptionResult> v{a, b, c};
    const auto& best = textscore::select_best(v);
    if (best.text != "world") {
        std::cerr << "select_best failed" << std::endl;
        std::abort();
    }
}

static void test_audio_preprocessing() {
    // Build a short buffer with small DC offset + low-frequency drift + a burst tone
    const int N = constants::kSampleRate / 100; // 10ms at 16kHz
    vector<float> x(N, 0.01f);
    for (int i = 0; i < N; ++i) {
        x[i] += 0.02f * std::sin(2.0f * static_cast<float>(M_PI) * (50.0f / constants::kSampleRate) * i);
    }
    // Add a short louder region
    for (int i = N / 4; i < N / 2; ++i) {
        x[i] += 0.3f * std::sin(2.0f * static_cast<float>(M_PI) * (1000.0f / constants::kSampleRate) * i);
    }

    auto hp = audio::apply_high_pass_filter(x, constants::kSampleRate, constants::kHighPassCutoffHz);
    // DC offset should be reduced
    float mean_before = 0.0f, mean_after = 0.0f;
    for (float v : x) mean_before += v;
    for (float v : hp) mean_after += v;
    mean_before /= x.size();
    mean_after /= hp.size();
    if (!(std::abs(mean_after) < std::abs(mean_before))) {
        std::cerr << "high-pass filter did not reduce DC" << std::endl;
        std::abort();
    }

    auto denoised = audio::remove_noise(hp, constants::kNoiseWindowSize, constants::kNoiseFloorFactor, constants::kNoiseAttenuation);
    // Samples below floor should be attenuated; compare a quiet sample index
    float q_before = hp[1];
    float q_after = denoised[1];
    if (std::abs(q_before) < 0.05f) {
        if (!(std::abs(q_after) <= std::abs(q_before))) {
            std::cerr << "noise removal did not attenuate quiet sample" << std::endl;
            std::abort();
        }
    }

    auto norm = audio::normalize(denoised, constants::kNormalizeMinAmp, constants::kNormalizeTargetAmp);
    float max_abs = 0.0f;
    for (float v : norm) max_abs = std::max(max_abs, std::abs(v));
    if (!(max_abs <= constants::kNormalizeTargetAmp + 1e-3f)) {
        std::cerr << "normalize exceeded target amplitude" << std::endl;
        std::abort();
    }

    // VAD should detect presence of activity in this buffer
    if (!audio::detect_voice_activity(norm, constants::kVADMinEnergy, constants::kVADZcrMin, constants::kVADZcrMax)) {
        std::cerr << "VAD failed on voiced buffer" << std::endl;
        std::abort();
    }

    // Silence should not be detected as speech
    vector<float> silence(512, 0.0f);
    if (audio::detect_voice_activity(silence, constants::kVADMinEnergy, constants::kVADZcrMin, constants::kVADZcrMax)) {
        std::cerr << "VAD false positive on silence" << std::endl;
        std::abort();
    }
}

int main() {
    test_text_scoring();
    test_audio_preprocessing();
    std::cout << "All tests passed\n";
    return 0;
}
