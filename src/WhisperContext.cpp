#include "WhisperContext.h"
#include "whisper.h"

bool WhisperContext::initialize(const std::string& model_path, bool use_gpu) {
    ctx_.reset();
    whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = use_gpu;
    whisper_context* raw = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    if (!raw) return false;
    ctx_.reset(raw);
    return true;
}

WhisperContext::State WhisperContext::createState() const {
    if (!ctx_) return State{};
    whisper_state* s = whisper_init_state(ctx_.get());
    return State{s};
}

void WhisperContext::context_deleter(whisper_context* ctx) {
    if (ctx) whisper_free(ctx);
}

void WhisperContext::State::deleter(whisper_state* s) {
    if (s) whisper_free_state(s);
}

void WhisperContext::reset() {
    ctx_.reset();
}
