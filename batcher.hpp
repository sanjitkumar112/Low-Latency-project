#pragma once

#include <vector>
#include <chrono>
#include <functional>
#include "order.hpp"

class Batcher {
private:
    std::vector<Order> batch_buffer_;
    size_t batch_size_threshold_;
    std::chrono::microseconds timeout_;
    std::chrono::high_resolution_clock::time_point first_order_time_;
    bool has_orders_;
    
    // Callback function to send batch to downstream (network layer)
    std::function<void(const std::vector<Order>&, uint64_t)> send_callback_;
    
public:
    Batcher(size_t batch_size, std::chrono::microseconds timeout, 
            std::function<void(const std::vector<Order>&, uint64_t)> send_callback)
        : batch_size_threshold_(batch_size), timeout_(timeout), 
          has_orders_(false), send_callback_(send_callback) {}
    
    // Add order to batch
    void add_order(const Order& order) {
        if (!has_orders_) {
            first_order_time_ = std::chrono::high_resolution_clock::now();
            has_orders_ = true;
        }
        
        batch_buffer_.push_back(order);
        
        // Check if batch size threshold reached
        if (batch_buffer_.size() >= batch_size_threshold_) {
            flush_batch();
        }
    }
    
    // Check if timeout has expired and flush if needed
    bool check_timeout() {
        if (!has_orders_ || batch_buffer_.empty()) {
            return false;
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - first_order_time_);
        
        if (elapsed >= timeout_) {
            flush_batch();
            return true;
        }
        
        return false;
    }
    
    // Force flush current batch
    void force_flush() {
        if (!batch_buffer_.empty()) {
            flush_batch();
        }
    }
    
    // Get current batch size
    size_t get_current_batch_size() const {
        return batch_buffer_.size();
    }
    
    // Check if batch has any orders
    bool has_orders() const {
        return has_orders_;
    }
    
private:
    void flush_batch() {
        if (batch_buffer_.empty()) {
            return;
        }
        
        // Calculate latency from first order to batch send
        auto now = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(now - first_order_time_);
        
        // Send batch to downstream
        if (send_callback_) {
            send_callback_(batch_buffer_, latency.count());
        }
        
        // Clear batch buffer
        batch_buffer_.clear();
        has_orders_ = false;
    }
}; 