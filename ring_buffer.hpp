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
    std::atomic<size_t> head_;  // Producer position
    std::atomic<size_t> tail_;  // Consumer position
    
    // Cache line size to avoid false sharing
    static constexpr size_t CACHE_LINE_SIZE = 64;
    
    // Padding to ensure head and tail are on different cache lines
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head_padding_[CACHE_LINE_SIZE / sizeof(size_t)];
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail_padding_[CACHE_LINE_SIZE / sizeof(size_t)];

public:
    explicit LockFreeRingBuffer(size_t capacity) 
        : capacity_(capacity), head_(0), tail_(0) {
        // Ensure capacity is a power of 2 for efficient modulo operation
        assert((capacity & (capacity - 1)) == 0 && "Capacity must be a power of 2");
        buffer_ = std::make_unique<T[]>(capacity);
    }
    
    ~LockFreeRingBuffer() = default;
    
    // Delete copy constructor and assignment
    LockFreeRingBuffer(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer& operator=(const LockFreeRingBuffer&) = delete;
    
    // Try to push an item to the buffer (producer)
    bool try_push(const T& item) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        size_t next_head = (current_head + 1) & (capacity_ - 1);
        size_t current_tail = tail_.load(std::memory_order_acquire);
        
        // Check if buffer is full
        if (next_head == current_tail) {
            return false;
        }
        
        // Copy the item
        buffer_[current_head] = item;
        
        // Update head atomically
        head_.store(next_head, std::memory_order_release);
        return true;
    }
    
    // Try to pop an item from the buffer (consumer)
    bool try_pop(T& item) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t current_head = head_.load(std::memory_order_acquire);
        
        // Check if buffer is empty
        if (current_tail == current_head) {
            return false;
        }
        
        // Copy the item
        item = buffer_[current_tail];
        
        // Update tail atomically
        size_t next_tail = (current_tail + 1) & (capacity_ - 1);
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    // Check if buffer is empty
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    // Check if buffer is full
    bool full() const {
        size_t next_head = (head_.load(std::memory_order_relaxed) + 1) & (capacity_ - 1);
        return next_head == tail_.load(std::memory_order_acquire);
    }
    
    // Get current size (approximate)
    size_t size() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & (capacity_ - 1);
    }
    
    // Get capacity
    size_t capacity() const {
        return capacity_;
    }
    
    // Clear the buffer (not thread-safe, use with caution)
    void clear() {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }
};

// Specialized ring buffer for Order type with additional functionality
class OrderRingBuffer : public LockFreeRingBuffer<Order> {
public:
    explicit OrderRingBuffer(size_t capacity) : LockFreeRingBuffer<Order>(capacity) {}
    
    // Push order with timeout
    bool push_with_timeout(const Order& order, std::chrono::milliseconds timeout) {
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start + timeout;
        
        while (std::chrono::high_resolution_clock::now() < end) {
            if (try_push(order)) {
                return true;
            }
            // Small delay to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        return false;
    }
    
    // Pop order with timeout
    bool pop_with_timeout(Order& order, std::chrono::milliseconds timeout) {
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start + timeout;
        
        while (std::chrono::high_resolution_clock::now() < end) {
            if (try_pop(order)) {
                return true;
            }
            // Small delay to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        return false;
    }
};
