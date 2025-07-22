#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include "network_stats.hpp"

// Forward declaration
struct Order;

class TCPSimulator {
private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> drop_dist_;
    std::uniform_real_distribution<double> delay_dist_;
    double drop_rate_;
    int base_delay_ms_;
    int max_retries_;
    int dropped_packets_ = 0;
    int retransmissions_ = 0;
public:
    TCPSimulator(double drop_rate = 0.02, int base_delay_ms = 5, int max_retries = 3)
        : rng_(std::random_device{}()), drop_dist_(0.0, 1.0), delay_dist_(0.8, 1.2),
          drop_rate_(drop_rate), base_delay_ms_(base_delay_ms), max_retries_(max_retries) {}
    bool send_reliable(const std::vector<Order>& orders, uint64_t) {
        (void)orders;
        int retries = 0;
        while (retries <= max_retries_) {
            double delay = base_delay_ms_ * delay_dist_(rng_);
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay)));
            if (drop_dist_(rng_) >= drop_rate_) return true;
            dropped_packets_++;
            if (retries < max_retries_) retransmissions_++;
            retries++;
        }
        return false;
    }
    TCPStats get_stats() const {
        return {dropped_packets_, retransmissions_, base_delay_ms_, drop_rate_};
    }
};

// Global TCP simulator instance
static std::unique_ptr<TCPSimulator> g_tcp_sim;

// Initialize TCP simulator
void init_tcp_simulator(double drop_rate, int base_delay_ms, int max_retries, bool) {
    g_tcp_sim = std::make_unique<TCPSimulator>(drop_rate, base_delay_ms, max_retries);
}

// Send orders via TCP simulation
bool tcp_send_orders(const std::vector<Order>& orders, uint64_t batch_latency_us) {
    if (!g_tcp_sim) return false;
    return g_tcp_sim->send_reliable(orders, batch_latency_us);
}

TCPStats get_tcp_stats() {
    if (!g_tcp_sim) return {};
    return g_tcp_sim->get_stats();
} 