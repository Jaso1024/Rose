#pragma once

#include <string>
#include <vector>

struct TranscriptionResult {
    std::string text;
    float avg_logprob;
    float no_speech_prob;
    float score;
};

namespace textscore {

float score(float avg_logprob, float no_speech_prob);

inline float score(const TranscriptionResult& r) {
    return score(r.avg_logprob, r.no_speech_prob);
}

const TranscriptionResult& select_best(const std::vector<TranscriptionResult>& results);

} // namespace textscore

