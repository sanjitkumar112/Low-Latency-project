#pragma once

#include <cstdint>
#include <cstring>
#include <chrono>
#include <string>

enum class OrderType : uint8_t {
    BUY = 0,
    SELL = 1
};

struct Order {
    uint64_t order_id;
    uint64_t timestamp_ns;
    char symbol[16];
    uint32_t quantity;
    uint32_t price_cents;
    OrderType type;
    Order() : order_id(0), timestamp_ns(0), quantity(0), price_cents(0), type(OrderType::BUY) {
        std::memset(symbol, 0, sizeof(symbol));
    }
    Order(uint64_t id, const std::string& sym, OrderType t, double price, uint32_t qty)
        : order_id(id), quantity(qty), type(t) {
        auto now = std::chrono::high_resolution_clock::now();
        timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        std::strncpy(symbol, sym.c_str(), sizeof(symbol) - 1);
        symbol[sizeof(symbol) - 1] = '\0';
        price_cents = static_cast<uint32_t>(price * 100.0 + 0.5);
    }
};
