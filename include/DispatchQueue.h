#pragma once

#include <dispatch/dispatch.h>
#include <functional>
#include <string>

class DispatchQueue {
public:
    explicit DispatchQueue(const char* label)
        : q_(dispatch_queue_create(label, DISPATCH_QUEUE_SERIAL)) {}
    explicit DispatchQueue(const std::string& label)
        : DispatchQueue(label.c_str()) {}
    ~DispatchQueue() { if (q_) dispatch_release(q_); }

    DispatchQueue(const DispatchQueue&) = delete;
    DispatchQueue& operator=(const DispatchQueue&) = delete;

    void async(std::function<void()> fn) const {
        auto heap_fn = new std::function<void()>(std::move(fn));
        dispatch_async_f(q_, heap_fn, [](void* ctx){
            auto f = static_cast<std::function<void()>*>(ctx);
            (*f)();
            delete f;
        });
    }

    static void main_async(std::function<void()> fn) {
        auto heap_fn = new std::function<void()>(std::move(fn));
        dispatch_async_f(dispatch_get_main_queue(), heap_fn, [](void* ctx){
            auto f = static_cast<std::function<void()>*>(ctx);
            (*f)();
            delete f;
        });
    }

    dispatch_queue_t native() const { return q_; }

private:
    dispatch_queue_t q_ { nullptr };
};
