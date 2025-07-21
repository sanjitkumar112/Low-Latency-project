#pragma once

#include <atomic>
#include <memory>
#include <cstdint>
#include <cassert>
#include <thread>
#include <chrono>
#include "order.hpp"

template<typename T>
class LockFreeRingBuffer {
private:
    std::unique_ptr<T[]> buffer_;
    const size_t capacity_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
    static constexpr size_t CACHE_LINE_SIZE = 64;
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head_padding_[CACHE_LINE_SIZE / sizeof(size_t)];
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail_padding_[CACHE_LINE_SIZE / sizeof(size_t)];
public:
    explicit LockFreeRingBuffer(size_t capacity) 
        : capacity_(capacity), head_(0), tail_(0) {
        assert((capacity & (capacity - 1)) == 0 && "Capacity must be a power of 2");
        buffer_ = std::make_unique<T[]>(capacity);
    }
    ~LockFreeRingBuffer() = default;
    LockFreeRingBuffer(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer& operator=(const LockFreeRingBuffer&) = delete;
    bool try_push(const T& item) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        size_t next_head = (current_head + 1) & (capacity_ - 1);
        size_t current_tail = tail_.load(std::memory_order_acquire);
        if (next_head == current_tail) {
            return false;
        }
        buffer_[current_head] = item;
        head_.store(next_head, std::memory_order_release);
        return true;
    }
    bool try_pop(T& item) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t current_head = head_.load(std::memory_order_acquire);
        if (current_tail == current_head) {
            return false;
        }
        item = buffer_[current_tail];
        size_t next_tail = (current_tail + 1) & (capacity_ - 1);
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    bool full() const {
        size_t next_head = (head_.load(std::memory_order_relaxed) + 1) & (capacity_ - 1);
        return next_head == tail_.load(std::memory_order_acquire);
    }
    size_t size() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & (capacity_ - 1);
    }
    size_t capacity() const {
        return capacity_;
    }
    void clear() {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }
};

class OrderRingBuffer : public LockFreeRingBuffer<Order> {
public:
    explicit OrderRingBuffer(size_t capacity) : LockFreeRingBuffer<Order>(capacity) {}
    bool push_with_timeout(const Order& order, std::chrono::milliseconds timeout) {
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start + timeout;
        while (std::chrono::high_resolution_clock::now() < end) {
            if (try_push(order)) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        return false;
    }
    bool pop_with_timeout(Order& order, std::chrono::milliseconds timeout) {
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start + timeout;
        while (std::chrono::high_resolution_clock::now() < end) {
            if (try_pop(order)) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        return false;
    }
};
