#pragma once

#include <atomic>
#include <memory>
#include <cstdint>
#include <cassert>
#include "order.hpp"

class OrderRingBuffer {
    std::unique_ptr<Order[]> buffer_;
    const size_t capacity_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
public:
    explicit OrderRingBuffer(size_t capacity)
        : capacity_(capacity), head_(0), tail_(0) {
        assert((capacity & (capacity - 1)) == 0 && "Capacity must be a power of 2");
        buffer_ = std::make_unique<Order[]>(capacity);
    }
    bool try_push(const Order& item) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next = (head + 1) & (capacity_ - 1);
        size_t tail = tail_.load(std::memory_order_acquire);
        if (next == tail) return false;
        buffer_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }
    bool try_pop(Order& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);
        if (tail == head) return false;
        item = buffer_[tail];
        size_t next = (tail + 1) & (capacity_ - 1);
        tail_.store(next, std::memory_order_release);
        return true;
    }
    bool empty() const { return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire); }
    bool full() const { size_t next = (head_.load(std::memory_order_relaxed) + 1) & (capacity_ - 1); return next == tail_.load(std::memory_order_acquire); }
    size_t size() const { size_t h = head_.load(std::memory_order_acquire); size_t t = tail_.load(std::memory_order_acquire); return (h - t) & (capacity_ - 1); }
    size_t capacity() const { return capacity_; }
    void clear() { head_.store(0, std::memory_order_relaxed); tail_.store(0, std::memory_order_relaxed); }
};
