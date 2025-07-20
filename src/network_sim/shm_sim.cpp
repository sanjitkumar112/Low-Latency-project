#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

// Forward declaration
struct Order;

// SHM statistics structure
struct SHMStats {
    int messages_sent;
    double avg_delay_ns;
    uint64_t min_delay_ns;
    uint64_t max_delay_ns;
    bool noise_enabled;
    int noise_range_ns;
};

class SHMSimulator {
private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> noise_dist_;
    
    // Statistics
    std::atomic<int> messages_sent_{0};
    std::atomic<int> total_delay_ns_{0};
    std::atomic<uint64_t> min_delay_ns_{UINT64_MAX};
    std::atomic<uint64_t> max_delay_ns_{0};
    
    // Configuration
    bool enable_noise_;
    int noise_range_ns_;
    
    mutable std::mutex stats_mutex_;
    
public:
    SHMSimulator(bool enable_noise = true, int noise_range_ns = 100)
        : rng_(std::random_device{}()),
          noise_dist_(-noise_range_ns, noise_range_ns),
          enable_noise_(enable_noise),
          noise_range_ns_(noise_range_ns) {}
    
    // Simulate shared memory delivery (near-zero transmission time)
    bool send_instant(const std::vector<Order>& orders, uint64_t batch_latency_us) {
        (void)orders; // Suppress unused parameter warning
        (void)batch_latency_us; // Suppress unused parameter warning
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        messages_sent_++;
        
        // Simulate minimal IPC overhead
        int noise_ns = 0;
        if (enable_noise_) {
            noise_ns = noise_dist_(rng_);
        }
        
        // Add tiny delay to simulate IPC context switch
        if (noise_ns > 0) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(noise_ns));
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto actual_delay = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        uint64_t delay_ns = actual_delay.count();
        
        // Update statistics
        total_delay_ns_ += delay_ns;
        
        uint64_t current_min = min_delay_ns_.load();
        while (delay_ns < current_min && !min_delay_ns_.compare_exchange_weak(current_min, delay_ns)) {
            // Retry if CAS failed
        }
        
        uint64_t current_max = max_delay_ns_.load();
        while (delay_ns > current_max && !max_delay_ns_.compare_exchange_weak(current_max, delay_ns)) {
            // Retry if CAS failed
        }
        
        return true;
    }
    
    // Get current statistics
    struct SHMStats get_stats() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        int sent = messages_sent_.load();
        int total_delay = total_delay_ns_.load();
        uint64_t min_delay = min_delay_ns_.load();
        uint64_t max_delay = max_delay_ns_.load();
        
        double avg_delay_ns = sent > 0 ? static_cast<double>(total_delay) / sent : 0.0;
        
        return {
            sent,
            avg_delay_ns,
            min_delay == UINT64_MAX ? 0 : min_delay,
            max_delay,
            enable_noise_,
            noise_range_ns_
        };
    }
    
    // Reset statistics
    void reset_stats() {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        messages_sent_ = 0;
        total_delay_ns_ = 0;
        min_delay_ns_ = UINT64_MAX;
        max_delay_ns_ = 0;
    }
    
    // Update configuration
    void set_noise(bool enabled) { enable_noise_ = enabled; }
    void set_noise_range(int range_ns) { 
        noise_range_ns_ = range_ns; 
        noise_dist_ = std::uniform_int_distribution<int>(-range_ns, range_ns);
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

// Get SHM statistics
struct SHMStats get_shm_stats() {
    if (!g_shm_sim) {
        return {0, 0.0, 0, 0, false, 0};
    }
    return g_shm_sim->get_stats();
}

// Reset SHM statistics
void reset_shm_stats() {
    if (g_shm_sim) {
        g_shm_sim->reset_stats();
    }
}

// Update SHM configuration
void update_shm_config(bool enable_noise, int noise_range_ns) {
    if (g_shm_sim) {
        g_shm_sim->set_noise(enable_noise);
        g_shm_sim->set_noise_range(noise_range_ns);
    }
} 