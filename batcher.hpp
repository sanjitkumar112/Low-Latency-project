#pragma once

#include <vector>
#include <chrono>
#include <functional>
#include "order.hpp"

class Batcher {
    std::vector<Order> buffer_;
    size_t batch_size_;
    std::chrono::microseconds timeout_;
    std::chrono::high_resolution_clock::time_point first_time_;
    bool started_ = false;
    std::function<void(const std::vector<Order>&, uint64_t)> send_;
public:
    Batcher(size_t batch_size, std::chrono::microseconds timeout, std::function<void(const std::vector<Order>&, uint64_t)> send)
        : batch_size_(batch_size), timeout_(timeout), send_(send) {}
    void add_order(const Order& o) {
        if (!started_) { first_time_ = std::chrono::high_resolution_clock::now(); started_ = true; }
        buffer_.push_back(o);
        if (buffer_.size() >= batch_size_) flush();
    }
    bool check_timeout() {
        if (!started_ || buffer_.empty()) return false;
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::microseconds>(now - first_time_) >= timeout_) {
            flush();
            return true;
        }
        return false;
    }
    void force_flush() { if (!buffer_.empty()) flush(); }
private:
    void flush() {
        if (buffer_.empty()) return;
        auto now = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(now - first_time_);
        if (send_) send_(buffer_, latency.count());
        buffer_.clear();
        started_ = false;
    }
}; 