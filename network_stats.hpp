#pragma once

struct TCPStats {
    int dropped_packets = 0;
    int retransmissions = 0;
    int base_delay_ms = 0;
    double drop_rate = 0.0;
};

struct UDPStats {
    int packets_sent = 0;
    int packets_dropped = 0;
    int base_delay_us = 0;
    double drop_rate = 0.0;
};

struct SHMStats {
    int messages_sent = 0;
    int noise_range_ns = 0;
};

TCPStats get_tcp_stats();
UDPStats get_udp_stats();
SHMStats get_shm_stats(); 