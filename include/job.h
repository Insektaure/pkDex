#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

#include <switch.h>

// A long operation's state, written by the worker and read by the screen that
// shows it. Everything but the message is atomic; the message has a lock.
struct Job {
    std::atomic<int>      stage{0};
    std::atomic<float>    progress{-1.0f};   // 0..1, or -1 when unknown
    std::atomic<uint64_t> done{0};
    std::atomic<uint64_t> total{0};
    std::atomic<bool>     cancel{false};
    std::atomic<bool>     finished{false};
    std::atomic<bool>     ok{false};

    void reset() {
        stage = 0; progress = -1.0f; done = 0; total = 0;
        cancel = false; finished = false; ok = false;
        std::lock_guard<std::mutex> l(m_);
        message_.clear();
    }
    void finish(bool success, const std::string& msg = std::string()) {
        {
            std::lock_guard<std::mutex> l(m_);
            message_ = msg;
        }
        ok = success;
        finished = true;
    }
    std::string message() {
        std::lock_guard<std::mutex> l(m_);
        return message_;
    }

private:
    std::mutex m_;
    std::string message_;
};

// One background thread at a time, on libnx's own threads so the stack can be
// big enough for an mbedTLS handshake.
class Worker {
public:
    ~Worker() { join(); }

    bool busy() const { return running_.load(); }

    // False when the thread could not be started or one is still running.
    bool start(std::function<void()> fn, size_t stack = 512 * 1024) {
        if (running_.load()) return false;
        join();
        fn_ = std::move(fn);
        running_ = true;
        // Below the main thread's priority: the work must never cost a frame.
        Result rc = threadCreate(&thread_, entry, this, nullptr, stack, 0x3B, -2);
        if (R_SUCCEEDED(rc)) {
            rc = threadStart(&thread_);
            if (R_FAILED(rc)) threadClose(&thread_);
        }
        if (R_FAILED(rc)) {
            running_ = false;
            return false;
        }
        live_ = true;
        return true;
    }

    void join() {
        if (!live_) return;
        threadWaitForExit(&thread_);
        threadClose(&thread_);
        live_ = false;
    }

private:
    static void entry(void* arg) {
        auto* self = static_cast<Worker*>(arg);
        self->fn_();
        self->running_ = false;
    }

    std::function<void()> fn_;
    Thread thread_{};
    bool live_ = false;
    std::atomic<bool> running_{false};
};
