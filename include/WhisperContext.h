#pragma once

#include <memory>
#include <string>

struct whisper_context;
struct whisper_state;

class WhisperContext {
public:
    WhisperContext() = default;
    ~WhisperContext() = default;

    WhisperContext(const WhisperContext&) = delete;
    WhisperContext& operator=(const WhisperContext&) = delete;

    bool initialize(const std::string& model_path, bool use_gpu);
    bool valid() const { return static_cast<bool>(ctx_); }
    whisper_context* get() const { return ctx_.get(); }
    void reset();

    class State {
    public:
        State() = default;
        explicit State(whisper_state* s) : state_(s, &State::deleter) {}
        whisper_state* get() const { return state_.get(); }
        explicit operator bool() const { return static_cast<bool>(state_); }

    private:
        static void deleter(whisper_state* s);
        std::unique_ptr<whisper_state, void(*)(whisper_state*)> state_{nullptr, &State::deleter};
    };

    State createState() const;

private:
    static void context_deleter(whisper_context*);
    std::unique_ptr<whisper_context, void(*)(whisper_context*)> ctx_{nullptr, &WhisperContext::context_deleter};
};
