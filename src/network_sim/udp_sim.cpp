#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include "network_stats.hpp"

// Forward declaration
struct Order;

class UDPSimulator {
private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> drop_dist_;
    std::uniform_real_distribution<double> delay_dist_;
    
    // Configuration
    double drop_rate_;
    int base_delay_us_;
    bool enable_jitter_;
    int packets_sent_ = 0;
    int packets_dropped_ = 0;
    
public:
    UDPSimulator(double drop_rate = 0.02, int base_delay_us = 1000, bool enable_jitter = true)
        : rng_(std::random_device{}()), drop_dist_(0.0, 1.0), delay_dist_(0.5, 1.5),
          drop_rate_(drop_rate), base_delay_us_(base_delay_us), enable_jitter_(enable_jitter) {}
    
    // Simulate UDP-like fast but lossy transmission
    bool send_fast(const std::vector<Order>& orders, uint64_t) {
        (void)orders;
        packets_sent_++;
        if (drop_dist_(rng_) < drop_rate_) { packets_dropped_++; return false; }
        int delay_us = base_delay_us_;
        if (enable_jitter_) delay_us = static_cast<int>(base_delay_us_ * delay_dist_(rng_));
        std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
        return true;
    }
    UDPStats get_stats() const {
        return {packets_sent_, packets_dropped_, base_delay_us_, drop_rate_};
    }
};

// Global UDP simulator instance
static std::unique_ptr<UDPSimulator> g_udp_sim;

// Initialize UDP simulator
void init_udp_simulator(double drop_rate, int base_delay_us, bool enable_jitter) {
    g_udp_sim = std::make_unique<UDPSimulator>(drop_rate, base_delay_us, enable_jitter);
}

// Send orders via UDP simulation
bool udp_send_orders(const std::vector<Order>& orders, uint64_t batch_latency_us) {
    if (!g_udp_sim) return false;
    return g_udp_sim->send_fast(orders, batch_latency_us);
}

UDPStats get_udp_stats() {
    if (!g_udp_sim) return {};
    return g_udp_sim->get_stats();
} 