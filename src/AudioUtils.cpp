#include "AudioUtils.h"

#include <algorithm>
#include <cmath>

namespace audio {

std::vector<float> trim_silence(const std::vector<float>& audio,
                                int sample_rate,
                                float rms_threshold) {
    if (audio.empty()) return audio;
    const int w = std::max(1, sample_rate / 50);
    auto rms_ok = [&](int idx){
        const int a = std::max(0, idx - w/2);
        const int b = std::min<int>(audio.size(), idx + w/2);
        double e = 0.0; int n = 0;
        for (int i = a; i < b; ++i) { e += audio[i]*audio[i]; ++n; }
        const float rms = n ? std::sqrt(e / n) : 0.0f;
        return rms > rms_threshold;
    };
    int L = 0, R = static_cast<int>(audio.size());
    while (L < R && !rms_ok(L)) ++L;
    while (R > L && !rms_ok(R - 1)) --R;
    if (L <= 0 && R >= static_cast<int>(audio.size())) return audio;
    return std::vector<float>(audio.begin() + L, audio.begin() + R);
}

std::vector<float> apply_high_pass_filter(const std::vector<float>& audio,
                                          int sample_rate,
                                          float cutoff_hz) {
    if (audio.size() < 2) return audio;
    std::vector<float> filtered(audio.size());
    const float rc = 1.0f / (2.0f * static_cast<float>(M_PI) * cutoff_hz);
    const float dt = 1.0f / static_cast<float>(sample_rate);
    const float alpha = rc / (rc + dt);

    filtered[0] = audio[0];
    for (size_t i = 1; i < audio.size(); ++i) {
        filtered[i] = alpha * (filtered[i - 1] + audio[i] - audio[i - 1]);
    }
    return filtered;
}

std::vector<float> remove_noise(const std::vector<float>& audio,
                                size_t window_size,
                                float floor_factor,
                                float attenuation) {
    if (audio.size() < window_size * 2) return audio;
    std::vector<float> denoised = audio;

    float max_abs = 0.0f;
    const size_t N = std::min(window_size, audio.size());
    for (size_t i = 0; i < N; ++i) {
        max_abs = std::max(max_abs, std::abs(audio[i]));
    }
    const float noise_floor = max_abs * floor_factor;

    for (float& v : denoised) {
        if (std::abs(v) < noise_floor) v *= attenuation;
    }
    return denoised;
}

std::vector<float> normalize(const std::vector<float>& audio,
                             float min_amp,
                             float target_amp) {
    std::vector<float> out = audio;
    float max_abs = 0.0f;
    for (float s : audio) max_abs = std::max(max_abs, std::abs(s));
    if (max_abs > min_amp) {
        const float scale = std::min(target_amp / max_abs, 1.0f);
        for (float& s : out) s *= scale;
    }
    return out;
}

bool detect_voice_activity(const std::vector<float>& audio,
                           float min_energy,
                           float zcr_min,
                           float zcr_max) {
    if (audio.empty()) return false;
    float energy = 0.0f;
    for (float s : audio) energy += s * s;
    energy /= audio.size();
    float zero_cross = 0.0f;
    for (size_t i = 1; i < audio.size(); ++i) {
        if ((audio[i - 1] >= 0) != (audio[i] >= 0)) zero_cross += 1.0f;
    }
    const float zcr = zero_cross / audio.size();
    return energy > min_energy && zcr > zcr_min && zcr < zcr_max;
}

std::vector<float> preprocess(const std::vector<float>& audio,
                              int sample_rate,
                              float hp_cutoff_hz,
                              size_t noise_win,
                              float noise_floor_factor,
                              float noise_attenuation,
                              float norm_min_amp,
                              float norm_target_amp) {
    std::vector<float> out = trim_silence(audio, sample_rate, norm_min_amp);
    out = apply_high_pass_filter(out, sample_rate, hp_cutoff_hz);
    out = remove_noise(out, noise_win, noise_floor_factor, noise_attenuation);
    out = normalize(out, norm_min_amp, norm_target_amp);
    return out;
}

} // namespace audio
