#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include <signal.h>
#include "ring_buffer.hpp"
#include "batcher.hpp"

enum class NetworkType { TCP, UDP, SHM };

void init_tcp_simulator(double drop_rate, int base_delay_ms, int max_retries, bool enable_congestion_control);
bool tcp_send_orders(const std::vector<Order>& orders, uint64_t batch_latency_us);
struct TCPStats { int active_connections; int dropped_packets; int retransmissions; int base_delay_ms; double drop_rate; };
TCPStats get_tcp_stats();

void init_udp_simulator(double drop_rate, int base_delay_us, bool enable_jitter);
bool udp_send_orders(const std::vector<Order>& orders, uint64_t batch_latency_us);
struct UDPStats { int packets_sent; int packets_dropped; double avg_delay_us; double actual_drop_rate; int base_delay_us; double configured_drop_rate; };
UDPStats get_udp_stats();

void init_shm_simulator(bool enable_noise, int noise_range_ns);
bool shm_send_orders(const std::vector<Order>& orders, uint64_t batch_latency_us);
struct SHMStats { int messages_sent; double avg_delay_ns; uint64_t min_delay_ns; uint64_t max_delay_ns; bool noise_enabled; int noise_range_ns; };
SHMStats get_shm_stats();

std::atomic<bool> g_shutdown_requested{false};
std::atomic<bool> g_running{false};

struct RuntimeConfig {
    int num_producers = 2;
    int num_consumers = 3;
    size_t buffer_size = 1024;
    size_t batch_size = 10;
    int orders_per_second = 10000;
    int runtime_seconds = 60;
    bool enable_batching = true;
    bool enable_network_simulation = true;
    NetworkType network_type = NetworkType::TCP;
};

struct Stats {
    std::atomic<uint64_t> orders_produced{0};
    std::atomic<uint64_t> orders_consumed{0};
    std::atomic<uint64_t> batches_sent{0};
    std::atomic<uint64_t> total_latency_us{0};
};

std::unique_ptr<OrderRingBuffer> g_buffer;
std::unique_ptr<Batcher> g_batcher;
Stats g_stats;
RuntimeConfig g_config;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Initiating shutdown...\n";
    g_shutdown_requested = true;
}

bool simulate_network_send(const std::vector<Order>& batch, uint64_t latency_us) {
    bool success = true;
    if (g_config.enable_network_simulation) {
        switch (g_config.network_type) {
            case NetworkType::TCP:
                success = tcp_send_orders(batch, latency_us);
                break;
            case NetworkType::UDP:
                success = udp_send_orders(batch, latency_us);
                break;
            case NetworkType::SHM:
                success = shm_send_orders(batch, latency_us);
                break;
        }
    }
    if (success) {
        g_stats.batches_sent++;
        g_stats.total_latency_us += latency_us;
        g_stats.orders_consumed += batch.size();
    }
    return success;
}

void producer_thread(int producer_id) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(100.0, 200.0);
    std::uniform_int_distribution<> qty_dist(1, 1000);
    std::uniform_int_distribution<> type_dist(0, 1);
    std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT", "AMZN", "TSLA"};
    uint64_t order_id_base = static_cast<uint64_t>(producer_id) * 1000000;
    uint64_t order_count = 0;
    while (!g_shutdown_requested && g_running) {
        Order order(
            order_id_base + order_count,
            symbols[gen() % symbols.size()],
            static_cast<OrderType>(type_dist(gen)),
            price_dist(gen),
            qty_dist(gen)
        );
        if (g_buffer->try_push(order)) {
            g_stats.orders_produced++;
        }
        order_count++;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void consumer_thread(int consumer_id) {
    (void)consumer_id;
    while (!g_shutdown_requested && g_running) {
        Order order;
        if (g_buffer->try_pop(order)) {
            if (g_config.enable_batching) {
                g_batcher->add_order(order);
                g_batcher->check_timeout();
            } else {
                simulate_network_send({order}, 0);
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

void telemetry_logger_thread() {
    auto last_stats_time = std::chrono::high_resolution_clock::now();
    while (!g_shutdown_requested && g_running) {
        auto now = std::chrono::high_resolution_clock::now();
        auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_time);
        if (time_since_last.count() >= 1) {
            double throughput = static_cast<double>(g_stats.orders_consumed.load()) / time_since_last.count();
            double avg_latency = g_stats.batches_sent.load() > 0 ? 
                static_cast<double>(g_stats.total_latency_us.load()) / g_stats.batches_sent.load() : 0.0;
            std::cout << "Stats: Produced=" << g_stats.orders_produced.load() 
                      << ", Consumed=" << g_stats.orders_consumed.load()
                      << ", Batches=" << g_stats.batches_sent.load()
                      << ", Throughput=" << std::fixed << std::setprecision(2) << throughput << " ops/sec"
                      << ", AvgLatency=" << avg_latency << "μs"
                      << ", Buffer=" << g_buffer->size() << "/" << g_buffer->capacity() << "\n";
            last_stats_time = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool initialize_system(const RuntimeConfig& config) {
    std::cout << "Initializing low-latency trading system...\n";
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    g_buffer = std::make_unique<OrderRingBuffer>(config.buffer_size);
    switch (config.network_type) {
        case NetworkType::TCP:
            init_tcp_simulator(0.02, 5, 3, true);
            break;
        case NetworkType::UDP:
            init_udp_simulator(0.02, 1000, true);
            break;
        case NetworkType::SHM:
            init_shm_simulator(true, 100);
            break;
    }
    if (config.enable_batching) {
        g_batcher = std::make_unique<Batcher>(
            config.batch_size,
            std::chrono::microseconds(1000),
            simulate_network_send
        );
    }
    g_config = config;
    std::cout << "System initialized successfully.\n";
    std::cout << "Configuration: Producers=" << config.num_producers 
              << ", Consumers=" << config.num_consumers
              << ", Buffer=" << config.buffer_size
              << ", Batch=" << config.batch_size
              << ", Rate=" << config.orders_per_second << " ops/sec"
              << ", Network=" << (config.network_type == NetworkType::TCP ? "TCP" : 
                                 config.network_type == NetworkType::UDP ? "UDP" : "SHM") << "\n";
    return true;
}

void shutdown_system() {
    std::cout << "\nInitiating system shutdown...\n";
    g_running = false;
    if (g_batcher) {
        g_batcher->force_flush();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "\n=== Final Statistics ===\n";
    std::cout << "Total orders produced: " << g_stats.orders_produced.load() << "\n";
    std::cout << "Total orders consumed: " << g_stats.orders_consumed.load() << "\n";
    std::cout << "Total batches sent: " << g_stats.batches_sent.load() << "\n";
    if (g_stats.batches_sent.load() > 0) {
        std::cout << "Average batch latency: " 
                  << static_cast<double>(g_stats.total_latency_us.load()) / g_stats.batches_sent.load() 
                  << "μs\n";
    }
    if (g_config.enable_network_simulation) {
        switch (g_config.network_type) {
            case NetworkType::TCP: {
                TCPStats stats = get_tcp_stats();
                std::cout << "\n=== TCP Network Statistics ===\n";
                std::cout << "Active connections: " << stats.active_connections << "\n";
                std::cout << "Dropped packets: " << stats.dropped_packets << "\n";
                std::cout << "Retransmissions: " << stats.retransmissions << "\n";
                std::cout << "Base delay: " << stats.base_delay_ms << "ms\n";
                std::cout << "Drop rate: " << stats.drop_rate << "\n";
                std::cout << "==============================\n";
                break;
            }
            case NetworkType::UDP: {
                UDPStats stats = get_udp_stats();
                std::cout << "\n=== UDP Network Statistics ===\n";
                std::cout << "Packets sent: " << stats.packets_sent << "\n";
                std::cout << "Packets dropped: " << stats.packets_dropped << "\n";
                std::cout << "Average delay: " << stats.avg_delay_us << "μs\n";
                std::cout << "Actual drop rate: " << stats.actual_drop_rate << "\n";
                std::cout << "Base delay: " << stats.base_delay_us << "μs\n";
                std::cout << "Configured drop rate: " << stats.configured_drop_rate << "\n";
                std::cout << "==============================\n";
                break;
            }
            case NetworkType::SHM: {
                SHMStats stats = get_shm_stats();
                std::cout << "\n=== SHM Network Statistics ===\n";
                std::cout << "Messages sent: " << stats.messages_sent << "\n";
                std::cout << "Average delay: " << stats.avg_delay_ns << "ns\n";
                std::cout << "Min delay: " << stats.min_delay_ns << "ns\n";
                std::cout << "Max delay: " << stats.max_delay_ns << "ns\n";
                std::cout << "Noise enabled: " << (stats.noise_enabled ? "yes" : "no") << "\n";
                std::cout << "Noise range: " << stats.noise_range_ns << "ns\n";
                std::cout << "==============================\n";
                break;
            }
        }
    }
    std::cout << "======================\n";
    std::cout << "System shutdown complete.\n";
}

int main(int argc, char* argv[]) {
    std::cout << "Low-Latency Trading System\n";
    std::cout << "==========================\n\n";
    RuntimeConfig config;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--producers" && i + 1 < argc) {
            config.num_producers = std::stoi(argv[++i]);
        } else if (arg == "--consumers" && i + 1 < argc) {
            config.num_consumers = std::stoi(argv[++i]);
        } else if (arg == "--buffer-size" && i + 1 < argc) {
            config.buffer_size = std::stoul(argv[++i]);
        } else if (arg == "--batch-size" && i + 1 < argc) {
            config.batch_size = std::stoi(argv[++i]);
        } else if (arg == "--rate" && i + 1 < argc) {
            config.orders_per_second = std::stoi(argv[++i]);
        } else if (arg == "--runtime" && i + 1 < argc) {
            config.runtime_seconds = std::stoi(argv[++i]);
        } else if (arg == "--no-batching") {
            config.enable_batching = false;
        } else if (arg == "--no-network") {
            config.enable_network_simulation = false;
        } else if (arg == "--network" && i + 1 < argc) {
            std::string net_type = argv[++i];
            if (net_type == "tcp") {
                config.network_type = NetworkType::TCP;
            } else if (net_type == "udp") {
                config.network_type = NetworkType::UDP;
            } else if (net_type == "shm") {
                config.network_type = NetworkType::SHM;
            } else {
                std::cerr << "Unknown network type: " << net_type << "\n";
                return 1;
            }
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --producers N     Number of producer threads (default: 2)\n";
            std::cout << "  --consumers N     Number of consumer threads (default: 3)\n";
            std::cout << "  --buffer-size N   Ring buffer size (default: 1024)\n";
            std::cout << "  --batch-size N    Batch size (default: 10)\n";
            std::cout << "  --rate N          Orders per second (default: 10000)\n";
            std::cout << "  --runtime N       Runtime in seconds (default: 60)\n";
            std::cout << "  --no-batching     Disable batching\n";
            std::cout << "  --no-network      Disable network simulation\n";
            std::cout << "  --network TYPE    Network type: tcp, udp, shm (default: tcp)\n";
            std::cout << "  --help            Show this help message\n";
            return 0;
        }
    }
    if (!initialize_system(config)) {
        std::cerr << "Failed to initialize system.\n";
        return 1;
    }
    g_running = true;
    std::vector<std::thread> threads;
    for (int i = 0; i < config.num_producers; ++i) {
        threads.emplace_back(producer_thread, i);
    }
    for (int i = 0; i < config.num_consumers; ++i) {
        threads.emplace_back(consumer_thread, i);
    }
    threads.emplace_back(telemetry_logger_thread);
    std::cout << "\nSystem started. Press Ctrl+C to stop.\n";
    std::cout << "Runtime: " << config.runtime_seconds << " seconds\n\n";
    auto start_time = std::chrono::high_resolution_clock::now();
    while (!g_shutdown_requested && g_running) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
        if (elapsed.count() >= config.runtime_seconds) {
            std::cout << "\nRuntime completed. Initiating shutdown...\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    g_running = false;
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    shutdown_system();
    return 0;
}
