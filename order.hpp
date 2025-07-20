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

// Cache-friendly Order structure with fixed-size fields
struct Order {
    // Fixed-size fields for cache efficiency
    uint64_t order_id;                    // 8 bytes
    uint64_t timestamp_ns;                // 8 bytes - nanoseconds since epoch
    char symbol[16];                      // 16 bytes - fixed-size symbol
    uint32_t quantity;                    // 4 bytes
    uint32_t price_cents;                 // 4 bytes - price in cents (avoids floating point)
    OrderType type;                       // 1 byte
    OrderStatus status;                   // 1 byte
    uint16_t reserved;                    // 2 bytes - padding for alignment
    
    // Total size: 44 bytes (cache-line friendly)
    
    // Default constructor
    Order() : order_id(0), timestamp_ns(0), quantity(0), price_cents(0), 
              type(OrderType::BUY), status(OrderStatus::PENDING), reserved(0) {
        std::memset(symbol, 0, sizeof(symbol));
    }
    
    // Constructor with parameters
    Order(uint64_t id, const std::string& sym, OrderType t, double price, uint32_t qty)
        : order_id(id), quantity(qty), type(t), status(OrderStatus::PENDING), reserved(0) {
        // Set timestamp
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        
        // Set symbol (truncate if too long)
        std::strncpy(symbol, sym.c_str(), sizeof(symbol) - 1);
        symbol[sizeof(symbol) - 1] = '\0';
        
        // Set price in cents (avoid floating point for performance)
        price_cents = static_cast<uint32_t>(price * 100.0 + 0.5);
    }
    
    // Copy constructor
    Order(const Order& other) {
        std::memcpy(this, &other, sizeof(Order));
    }
    
    // Assignment operator
    Order& operator=(const Order& other) {
        if (this != &other) {
            std::memcpy(this, &other, sizeof(Order));
        }
        return *this;
    }
    
    // Utility methods
    
    // Get price as double
    double get_price() const {
        return static_cast<double>(price_cents) / 100.0;
    }
    
    // Set price from double
    void set_price(double price) {
        price_cents = static_cast<uint32_t>(price * 100.0 + 0.5);
    }
    
    // Get symbol as string
    std::string get_symbol() const {
        return std::string(symbol);
    }
    
    // Set symbol
    void set_symbol(const std::string& sym) {
        std::strncpy(symbol, sym.c_str(), sizeof(symbol) - 1);
        symbol[sizeof(symbol) - 1] = '\0';
    }
    
    // Get timestamp as time_point
    std::chrono::high_resolution_clock::time_point get_timestamp() const {
        return std::chrono::high_resolution_clock::time_point(
            std::chrono::nanoseconds(timestamp_ns));
    }
    
    // Set timestamp from time_point
    void set_timestamp(const std::chrono::high_resolution_clock::time_point& tp) {
        auto duration = tp.time_since_epoch();
        timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }
    
    // Check if order is valid
    bool is_valid() const {
        return order_id != 0 && quantity > 0 && price_cents > 0 && 
               symbol[0] != '\0' && reserved == 0;
    }
    
    // Check if order is buy
    bool is_buy() const {
        return type == OrderType::BUY;
    }
    
    // Check if order is sell
    bool is_sell() const {
        return type == OrderType::SELL;
    }
    
    // Check if order is pending
    bool is_pending() const {
        return status == OrderStatus::PENDING;
    }
    
    // Check if order is filled
    bool is_filled() const {
        return status == OrderStatus::FILLED;
    }
    
    // Check if order is cancelled
    bool is_cancelled() const {
        return status == OrderStatus::CANCELLED;
    }
    
    // Check if order is rejected
    bool is_rejected() const {
        return status == OrderStatus::REJECTED;
    }
    
    // Print order to stream
    void print(std::ostream& os = std::cout) const {
        os << "Order[" << order_id << "] "
           << symbol << " "
           << (is_buy() ? "BUY" : "SELL") << " "
           << quantity << "@"
           << std::fixed << std::setprecision(2) << get_price()
           << " Status:" << get_status_string()
           << " Time:" << timestamp_ns;
    }
    
    // Convert to string representation
    std::string to_string() const {
        std::ostringstream oss;
        print(oss);
        return oss.str();
    }
    
    // Get status as string
    std::string get_status_string() const {
        switch (status) {
            case OrderStatus::PENDING: return "PENDING";
            case OrderStatus::FILLED: return "FILLED";
            case OrderStatus::CANCELLED: return "CANCELLED";
            case OrderStatus::REJECTED: return "REJECTED";
            default: return "UNKNOWN";
        }
    }
    
    // Get type as string
    std::string get_type_string() const {
        return is_buy() ? "BUY" : "SELL";
    }
    
    // Serialize to binary format (for network transmission)
    void serialize(char* buffer) const {
        std::memcpy(buffer, this, sizeof(Order));
    }
    
    // Deserialize from binary format
    void deserialize(const char* buffer) {
        std::memcpy(this, buffer, sizeof(Order));
    }
    
    // Get serialized size
    static constexpr size_t serialized_size() {
        return sizeof(Order);
    }
    
    // Calculate order value (quantity * price)
    uint64_t get_value_cents() const {
        return static_cast<uint64_t>(quantity) * price_cents;
    }
    
    // Calculate order value as double
    double get_value() const {
        return static_cast<double>(get_value_cents()) / 100.0;
    }
    
    // Comparison operators for sorting
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

// Stream operator for easy printing
inline std::ostream& operator<<(std::ostream& os, const Order& order) {
    order.print(os);
    return os;
}

// Utility functions for order creation
namespace OrderUtils {
    // Create a buy order
    inline Order create_buy_order(uint64_t id, const std::string& symbol, double price, uint32_t quantity) {
        return Order(id, symbol, OrderType::BUY, price, quantity);
    }
    
    // Create a sell order
    inline Order create_sell_order(uint64_t id, const std::string& symbol, double price, uint32_t quantity) {
        return Order(id, symbol, OrderType::SELL, price, quantity);
    }
    
    // Create order with current timestamp
    inline Order create_order_now(uint64_t id, const std::string& symbol, OrderType type, double price, uint32_t quantity) {
        Order order(id, symbol, type, price, quantity);
        order.set_timestamp(std::chrono::high_resolution_clock::now());
        return order;
    }
    
    // Validate order fields
    inline bool validate_order(const Order& order) {
        return order.is_valid() && 
               order.get_price() > 0.0 && 
               order.quantity > 0 && 
               !order.get_symbol().empty();
    }
}
