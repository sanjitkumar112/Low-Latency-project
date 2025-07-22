#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include <signal.h>
#include "ring_buffer.hpp"
#include "batcher.hpp"
#include <iomanip>
#include "network_stats.hpp"

// Network simulation headers
void init_tcp_simulator(double, int, int, bool);
bool tcp_send_orders(const std::vector<Order>&, uint64_t);
void init_udp_simulator(double, int, bool);
bool udp_send_orders(const std::vector<Order>&, uint64_t);
void init_shm_simulator(bool, int);
bool shm_send_orders(const std::vector<Order>&, uint64_t);

enum class NetworkType { TCP, UDP, SHM };
struct Config {
    int producers = 2;
    int consumers = 2;
    size_t buffer_size = 1024;
    size_t batch_size = 10;
    int runtime_seconds = 30;
    NetworkType net_type = NetworkType::TCP; // Change this to UDP or SHM as desired
};

std::atomic<bool> running{true};
std::unique_ptr<OrderRingBuffer> buffer;
std::unique_ptr<Batcher> batcher;
std::atomic<uint64_t> produced{0}, consumed{0};
uint64_t batches_sent = 0;
uint64_t total_batch_latency_us = 0;

void signal_handler(int) { running = false; }

void producer(int id) {
    std::mt19937 gen(id);
    std::uniform_real_distribution<> price(100, 200);
    std::uniform_int_distribution<> qty(1, 1000);
    std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT"};
    uint64_t order_id = id * 1000000;
    while (running) {
        Order o(order_id++, symbols[gen() % symbols.size()], OrderType::BUY, price(gen), qty(gen));
        if (buffer->try_push(o)) produced++;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void consumer() {
    while (running) {
        Order o;
        if (buffer->try_pop(o)) {
            batcher->add_order(o);
            consumed++;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

int main() {
    signal(SIGINT, signal_handler);
    Config cfg;
    buffer = std::make_unique<OrderRingBuffer>(cfg.buffer_size);

    // Initialize network simulation
    switch (cfg.net_type) {
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

    // Batcher sends batches through the selected network simulation
    batcher = std::make_unique<Batcher>(cfg.batch_size, std::chrono::microseconds(1000),
        [&cfg](const std::vector<Order>& batch, uint64_t latency_us) {
            switch (cfg.net_type) {
                case NetworkType::TCP:
                    tcp_send_orders(batch, latency_us);
                    break;
                case NetworkType::UDP:
                    udp_send_orders(batch, latency_us);
                    break;
                case NetworkType::SHM:
                    shm_send_orders(batch, latency_us);
                    break;
            }
            batches_sent++;
            total_batch_latency_us += latency_us;
        });

    std::vector<std::thread> threads;
    for (int i = 0; i < cfg.producers; ++i) threads.emplace_back(producer, i);
    for (int i = 0; i < cfg.consumers; ++i) threads.emplace_back(consumer);
    std::cout << "System running for " << cfg.runtime_seconds << " seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(cfg.runtime_seconds));
    running = false;
    for (auto& t : threads) t.join();

    std::cout << "\n=== Final Statistics ===\n";
    std::cout << "Total orders produced: " << produced << "\n";
    std::cout << "Total orders consumed: " << consumed << "\n";
    std::cout << "Total batches sent: " << batches_sent << "\n";
    double avg_batch_latency = batches_sent ? (double)total_batch_latency_us / batches_sent : 0.0;
    std::cout << "Average batch latency: " << std::fixed << std::setprecision(2) << avg_batch_latency << "\u03bcs\n";

    switch (cfg.net_type) {
        case NetworkType::TCP: {
            auto stats = get_tcp_stats();
            std::cout << "\n=== TCP Network Statistics ===\n";
            std::cout << "Dropped packets: " << stats.dropped_packets << "\n";
            std::cout << "Retransmissions: " << stats.retransmissions << "\n";
            std::cout << "Base delay: " << stats.base_delay_ms << "ms\n";
            std::cout << "Drop rate: " << stats.drop_rate << "\n";
            break;
        }
        case NetworkType::UDP: {
            auto stats = get_udp_stats();
            std::cout << "\n=== UDP Network Statistics ===\n";
            std::cout << "Packets sent: " << stats.packets_sent << "\n";
            std::cout << "Packets dropped: " << stats.packets_dropped << "\n";
            std::cout << "Base delay: " << stats.base_delay_us << "\u03bcs\n";
            std::cout << "Drop rate: " << stats.drop_rate << "\n";
            break;
        }
        case NetworkType::SHM: {
            auto stats = get_shm_stats();
            std::cout << "\n=== SHM Network Statistics ===\n";
            std::cout << "Messages sent: " << stats.messages_sent << "\n";
            std::cout << "Noise range: " << stats.noise_range_ns << "ns\n";
            break;
        }
    }
    std::cout << "==============================\n";
    return 0;
}
