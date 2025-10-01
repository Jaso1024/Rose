#pragma once

#include <vector>

namespace audio {

std::vector<float> apply_high_pass_filter(const std::vector<float>& audio,
                                          int sample_rate,
                                          float cutoff_hz);

std::vector<float> remove_noise(const std::vector<float>& audio,
                                size_t window_size,
                                float floor_factor,
                                float attenuation);

std::vector<float> normalize(const std::vector<float>& audio,
                             float min_amp,
                             float target_amp);

bool detect_voice_activity(const std::vector<float>& audio,
                           float min_energy,
                           float zcr_min,
                           float zcr_max);

std::vector<float> trim_silence(const std::vector<float>& audio,
                                int sample_rate,
                                float rms_threshold);

std::vector<float> preprocess(const std::vector<float>& audio,
                              int sample_rate,
                              float hp_cutoff_hz,
                              size_t noise_win,
                              float noise_floor_factor,
                              float noise_attenuation,
                              float norm_min_amp,
                              float norm_target_amp);

} // namespace audio
