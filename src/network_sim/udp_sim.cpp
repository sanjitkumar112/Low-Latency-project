#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

// Forward declaration
struct Order;

// UDP statistics structure
struct UDPStats {
    int packets_sent;
    int packets_dropped;
    double avg_delay_us;
    double actual_drop_rate;
    int base_delay_us;
    double configured_drop_rate;
};

class UDPSimulator {
private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> drop_dist_;
    std::uniform_real_distribution<double> delay_dist_;
    
    // Statistics
    std::atomic<int> packets_sent_{0};
    std::atomic<int> packets_dropped_{0};
    std::atomic<int> total_delay_us_{0};
    
    // Configuration
    double drop_rate_;
    int base_delay_us_;
    bool enable_jitter_;
    
    mutable std::mutex stats_mutex_;
    
public:
    UDPSimulator(double drop_rate = 0.02, int base_delay_us = 1000, bool enable_jitter = true)
        : rng_(std::random_device{}()),
          drop_dist_(0.0, 1.0),
          delay_dist_(0.5, 1.5),  // 50% variance for jitter
          drop_rate_(drop_rate),
          base_delay_us_(base_delay_us),
          enable_jitter_(enable_jitter) {}
    
    // Simulate UDP-like fast but lossy transmission
    bool send_fast(const std::vector<Order>& orders, uint64_t batch_latency_us) {
        (void)orders; // Suppress unused parameter warning
        (void)batch_latency_us; // Suppress unused parameter warning
        
        packets_sent_++;
        
        // Simulate packet drop (no retransmission)
        if (drop_dist_(rng_) < drop_rate_) {
            packets_dropped_++;
            std::cout << "UDP: Packet dropped (drop rate: " << drop_rate_ << ")\n";
            return false;
        }
        
        // Simulate network delay with jitter
        int delay_us = base_delay_us_;
        if (enable_jitter_) {
            double jitter_multiplier = delay_dist_(rng_);
            delay_us = static_cast<int>(base_delay_us_ * jitter_multiplier);
        }
        
        // Add some microsecond noise for realism
        std::uniform_int_distribution<int> noise_dist(-50, 50);
        delay_us += noise_dist(rng_);
        delay_us = std::max(1, delay_us); // Ensure positive delay
        
        std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
        total_delay_us_ += delay_us;
        
        return true;
    }
    
    // Get current statistics
    struct UDPStats get_stats() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        int sent = packets_sent_.load();
        int dropped = packets_dropped_.load();
        int total_delay = total_delay_us_.load();
        
        double avg_delay_us = (sent - dropped) > 0 ? static_cast<double>(total_delay) / (sent - dropped) : 0.0;
        double actual_drop_rate = sent > 0 ? static_cast<double>(dropped) / sent : 0.0;
        
        return {
            sent,
            dropped,
            avg_delay_us,
            actual_drop_rate,
            base_delay_us_,
            drop_rate_
        };
    }
    
    // Reset statistics
    void reset_stats() {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        packets_sent_ = 0;
        packets_dropped_ = 0;
        total_delay_us_ = 0;
    }
    
    // Update configuration
    void set_drop_rate(double rate) { drop_rate_ = rate; }
    void set_base_delay(int delay_us) { base_delay_us_ = delay_us; }
    void set_jitter(bool enabled) { enable_jitter_ = enabled; }
};

// Global UDP simulator instance
static std::unique_ptr<UDPSimulator> g_udp_sim;

// Initialize UDP simulator
void init_udp_simulator(double drop_rate, int base_delay_us, bool enable_jitter) {
    g_udp_sim = std::make_unique<UDPSimulator>(drop_rate, base_delay_us, enable_jitter);
    std::cout << "UDP Simulator initialized: drop_rate=" << drop_rate 
              << ", base_delay=" << base_delay_us << "μs, jitter=" << (enable_jitter ? "enabled" : "disabled") << "\n";
}

// Send orders via UDP simulation
bool udp_send_orders(const std::vector<Order>& orders, uint64_t batch_latency_us) {
    if (!g_udp_sim) {
        std::cerr << "UDP Simulator not initialized\n";
        return false;
    }
    
    return g_udp_sim->send_fast(orders, batch_latency_us);
}

// Get UDP statistics
struct UDPStats get_udp_stats() {
    if (!g_udp_sim) {
        return {0, 0, 0.0, 0.0, 0, 0.0};
    }
    return g_udp_sim->get_stats();
}

// Reset UDP statistics
void reset_udp_stats() {
    if (g_udp_sim) {
        g_udp_sim->reset_stats();
    }
}

// Update UDP configuration
void update_udp_config(double drop_rate, int base_delay_us, bool enable_jitter) {
    if (g_udp_sim) {
        g_udp_sim->set_drop_rate(drop_rate);
        g_udp_sim->set_base_delay(base_delay_us);
        g_udp_sim->set_jitter(enable_jitter);
    }
} 