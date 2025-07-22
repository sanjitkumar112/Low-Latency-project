#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include "network_stats.hpp"

// Forward declaration
struct Order;

class SHMSimulator {
private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> noise_dist_;
    bool enable_noise_;
    int noise_range_ns_;
    int messages_sent_ = 0;
public:
    SHMSimulator(bool enable_noise = true, int noise_range_ns = 100)
        : rng_(std::random_device{}()), noise_dist_(-noise_range_ns, noise_range_ns),
          enable_noise_(enable_noise), noise_range_ns_(noise_range_ns) {}

    bool send_instant(const std::vector<Order>& orders, uint64_t) {
        (void)orders;
        messages_sent_++;
        int noise_ns = enable_noise_ ? noise_dist_(rng_) : 0;
        if (noise_ns > 0) std::this_thread::sleep_for(std::chrono::nanoseconds(noise_ns));
        return true;
    }
    SHMStats get_stats() const {
        return {messages_sent_, noise_range_ns_};
    }
};

// Global SHM simulator instance
static std::unique_ptr<SHMSimulator> g_shm_sim;

// Initialize SHM simulator
void init_shm_simulator(bool enable_noise, int noise_range_ns) {
    g_shm_sim = std::make_unique<SHMSimulator>(enable_noise, noise_range_ns);
    std::cout << "SHM Simulator initialized: noise=" << (enable_noise ? "enabled" : "disabled") 
              << ", noise_range=" << noise_range_ns << "ns\n";
}

// Send orders via SHM simulation
bool shm_send_orders(const std::vector<Order>& orders, uint64_t batch_latency_us) {
    if (!g_shm_sim) {
        std::cerr << "SHM Simulator not initialized\n";
        return false;
    }
    
    return g_shm_sim->send_instant(orders, batch_latency_us);
}

SHMStats get_shm_stats() {
    if (!g_shm_sim) return {};
    return g_shm_sim->get_stats();
}