#include "TextScoring.h"

#include <algorithm>

namespace textscore {

float score(float avg_logprob, float no_speech_prob) {
    return avg_logprob * (1.0f - no_speech_prob);
}

const TranscriptionResult& select_best(const std::vector<TranscriptionResult>& results) {
    if (results.empty()) {
        static const TranscriptionResult kEmpty{"", -1e9f, 1.0f, -1e9f};
        return kEmpty;
    }
    return *std::max_element(results.begin(), results.end(), [](const TranscriptionResult& a, const TranscriptionResult& b) {
        return a.score < b.score;
    });
}

} // namespace textscore

