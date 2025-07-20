#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Forward declaration
struct Order;

// TCP statistics structure
struct TCPStats {
    int active_connections;
    int dropped_packets;
    int retransmissions;
    int base_delay_ms;
    double drop_rate;
};

class TCPSimulator {
private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> drop_dist_;
    std::uniform_real_distribution<double> delay_dist_;
    std::uniform_int_distribution<int> retry_delay_dist_;
    
    // Congestion control simulation
    std::atomic<int> active_connections_{0};
    std::atomic<int> dropped_packets_{0};
    std::atomic<int> retransmissions_{0};
    
    // Configuration
    double drop_rate_;
    int base_delay_ms_;
    int max_retries_;
    bool enable_congestion_control_;
    
    mutable std::mutex stats_mutex_;
    
public:
    TCPSimulator(double drop_rate = 0.02, int base_delay_ms = 5, 
                 int max_retries = 3, bool enable_congestion_control = true)
        : rng_(std::random_device{}()),
          drop_dist_(0.0, 1.0),
          delay_dist_(0.8, 1.2),  // 20% variance
          retry_delay_dist_(base_delay_ms * 2, base_delay_ms * 4),
          drop_rate_(drop_rate),
          base_delay_ms_(base_delay_ms),
          max_retries_(max_retries),
          enable_congestion_control_(enable_congestion_control) {}
    
    // Simulate TCP-like reliable transmission
    bool send_reliable(const std::vector<Order>& orders, uint64_t batch_latency_us) {
        (void)orders; // Suppress unused parameter warning
        (void)batch_latency_us; // Suppress unused parameter warning
        
        active_connections_++;
        
        // Calculate congestion delay
        int congestion_delay = 0;
        if (enable_congestion_control_) {
            int active = active_connections_.load();
            if (active > 10) {
                congestion_delay = (active - 10) * 2; // 2ms per extra connection
            }
        }
        
        // Simulate network delay
        double delay_multiplier = delay_dist_(rng_);
        int total_delay = static_cast<int>(base_delay_ms_ * delay_multiplier) + congestion_delay;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(total_delay));
        
        // Simulate packet drops and retransmissions
        int retries = 0;
        while (retries <= max_retries_) {
            if (drop_dist_(rng_) >= drop_rate_) {
                // Success - packet delivered
                active_connections_--;
                return true;
            } else {
                // Packet dropped
                dropped_packets_++;
                retries++;
                
                if (retries <= max_retries_) {
                    retransmissions_++;
                    
                    // Exponential backoff for retries
                    int retry_delay = retry_delay_dist_(rng_) * retries;
                    std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay));
                    
                    // Log retransmission
                    std::cout << "TCP: Retransmitting batch (attempt " << retries 
                              << "/" << max_retries_ << "), delay: " << retry_delay << "ms\n";
                }
            }
        }
        
        // All retries exhausted
        active_connections_--;
        std::cout << "TCP: Failed to deliver batch after " << max_retries_ << " retries\n";
        return false;
    }
    
    // Get current statistics
    struct TCPStats get_stats() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return {
            active_connections_.load(),
            dropped_packets_.load(),
            retransmissions_.load(),
            base_delay_ms_,
            drop_rate_
        };
    }
    
    // Reset statistics
    void reset_stats() {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        dropped_packets_ = 0;
        retransmissions_ = 0;
    }
    
    // Update configuration
    void set_drop_rate(double rate) { drop_rate_ = rate; }
    void set_base_delay(int delay_ms) { base_delay_ms_ = delay_ms; }
    void set_max_retries(int retries) { max_retries_ = retries; }
    void set_congestion_control(bool enabled) { enable_congestion_control_ = enabled; }
};

// Global TCP simulator instance
static std::unique_ptr<TCPSimulator> g_tcp_sim;

// Initialize TCP simulator
void init_tcp_simulator(double drop_rate, int base_delay_ms, int max_retries, bool enable_congestion_control) {
    g_tcp_sim = std::make_unique<TCPSimulator>(drop_rate, base_delay_ms, max_retries, enable_congestion_control);
    std::cout << "TCP Simulator initialized: drop_rate=" << drop_rate 
              << ", base_delay=" << base_delay_ms << "ms, max_retries=" << max_retries << "\n";
}

// Send orders via TCP simulation
bool tcp_send_orders(const std::vector<Order>& orders, uint64_t batch_latency_us) {
    if (!g_tcp_sim) {
        std::cerr << "TCP Simulator not initialized\n";
        return false;
    }
    
    return g_tcp_sim->send_reliable(orders, batch_latency_us);
}

// Get TCP statistics
struct TCPStats get_tcp_stats() {
    if (!g_tcp_sim) {
        return {0, 0, 0, 0, 0.0};
    }
    return g_tcp_sim->get_stats();
}

// Reset TCP statistics
void reset_tcp_stats() {
    if (g_tcp_sim) {
        g_tcp_sim->reset_stats();
    }
}

// Update TCP configuration
void update_tcp_config(double drop_rate, int base_delay_ms, int max_retries, bool enable_congestion_control) {
    if (g_tcp_sim) {
        g_tcp_sim->set_drop_rate(drop_rate);
        g_tcp_sim->set_base_delay(base_delay_ms);
        g_tcp_sim->set_max_retries(max_retries);
        g_tcp_sim->set_congestion_control(enable_congestion_control);
    }
} 