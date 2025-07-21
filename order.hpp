#pragma once

#include <cstdint>
#include <cstring>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

enum class OrderType : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderStatus : uint8_t {
    PENDING = 0,
    FILLED = 1,
    CANCELLED = 2,
    REJECTED = 3
};

struct Order {
    uint64_t order_id;
    uint64_t timestamp_ns;
    char symbol[16];
    uint32_t quantity;
    uint32_t price_cents;
    OrderType type;
    OrderStatus status;
    uint16_t reserved;
    Order() : order_id(0), timestamp_ns(0), quantity(0), price_cents(0), 
              type(OrderType::BUY), status(OrderStatus::PENDING), reserved(0) {
        std::memset(symbol, 0, sizeof(symbol));
    }
    Order(uint64_t id, const std::string& sym, OrderType t, double price, uint32_t qty)
        : order_id(id), quantity(qty), type(t), status(OrderStatus::PENDING), reserved(0) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        std::strncpy(symbol, sym.c_str(), sizeof(symbol) - 1);
        symbol[sizeof(symbol) - 1] = '\0';
        price_cents = static_cast<uint32_t>(price * 100.0 + 0.5);
    }
    Order(const Order& other) {
        std::memcpy(this, &other, sizeof(Order));
    }
    Order& operator=(const Order& other) {
        if (this != &other) {
            std::memcpy(this, &other, sizeof(Order));
        }
        return *this;
    }
    double get_price() const {
        return static_cast<double>(price_cents) / 100.0;
    }
    void set_price(double price) {
        price_cents = static_cast<uint32_t>(price * 100.0 + 0.5);
    }
    std::string get_symbol() const {
        return std::string(symbol);
    }
    void set_symbol(const std::string& sym) {
        std::strncpy(symbol, sym.c_str(), sizeof(symbol) - 1);
        symbol[sizeof(symbol) - 1] = '\0';
    }
    std::chrono::high_resolution_clock::time_point get_timestamp() const {
        return std::chrono::high_resolution_clock::time_point(
            std::chrono::nanoseconds(timestamp_ns));
    }
    void set_timestamp(const std::chrono::high_resolution_clock::time_point& tp) {
        auto duration = tp.time_since_epoch();
        timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }
    bool is_valid() const {
        return order_id != 0 && quantity > 0 && price_cents > 0 && 
               symbol[0] != '\0' && reserved == 0;
    }
    bool is_buy() const {
        return type == OrderType::BUY;
    }
    bool is_sell() const {
        return type == OrderType::SELL;
    }
    bool is_pending() const {
        return status == OrderStatus::PENDING;
    }
    bool is_filled() const {
        return status == OrderStatus::FILLED;
    }
    bool is_cancelled() const {
        return status == OrderStatus::CANCELLED;
    }
    bool is_rejected() const {
        return status == OrderStatus::REJECTED;
    }
    void print(std::ostream& os = std::cout) const {
        os << "Order[" << order_id << "] "
           << symbol << " "
           << (is_buy() ? "BUY" : "SELL") << " "
           << quantity << "@"
           << std::fixed << std::setprecision(2) << get_price()
           << " Status:" << get_status_string()
           << " Time:" << timestamp_ns;
    }
    std::string to_string() const {
        std::ostringstream oss;
        print(oss);
        return oss.str();
    }
    std::string get_status_string() const {
        switch (status) {
            case OrderStatus::PENDING: return "PENDING";
            case OrderStatus::FILLED: return "FILLED";
            case OrderStatus::CANCELLED: return "CANCELLED";
            case OrderStatus::REJECTED: return "REJECTED";
            default: return "UNKNOWN";
        }
    }
    std::string get_type_string() const {
        return is_buy() ? "BUY" : "SELL";
    }
    void serialize(char* buffer) const {
        std::memcpy(buffer, this, sizeof(Order));
    }
    void deserialize(const char* buffer) {
        std::memcpy(this, buffer, sizeof(Order));
    }
    static constexpr size_t serialized_size() {
        return sizeof(Order);
    }
    uint64_t get_value_cents() const {
        return static_cast<uint64_t>(quantity) * price_cents;
    }
    double get_value() const {
        return static_cast<double>(get_value_cents()) / 100.0;
    }
    bool operator<(const Order& other) const {
        return order_id < other.order_id;
    }
    bool operator==(const Order& other) const {
        return order_id == other.order_id;
    }
    bool operator!=(const Order& other) const {
        return order_id != other.order_id;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Order& order) {
    order.print(os);
    return os;
}

namespace OrderUtils {
    inline Order create_buy_order(uint64_t id, const std::string& symbol, double price, uint32_t quantity) {
        return Order(id, symbol, OrderType::BUY, price, quantity);
    }
    inline Order create_sell_order(uint64_t id, const std::string& symbol, double price, uint32_t quantity) {
        return Order(id, symbol, OrderType::SELL, price, quantity);
    }
    inline Order create_order_now(uint64_t id, const std::string& symbol, OrderType type, double price, uint32_t quantity) {
        Order order(id, symbol, type, price, quantity);
        order.set_timestamp(std::chrono::high_resolution_clock::now());
        return order;
    }
    inline bool validate_order(const Order& order) {
        return order.is_valid() && 
               order.get_price() > 0.0 && 
               order.quantity > 0 && 
               !order.get_symbol().empty();
    }
}
