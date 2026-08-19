#pragma once
#include "Types.hpp"
#include <string>

struct Order {
    OrderId id;
    std::string symbol;
    Side side;
    OrderType type;
    Price price; // For LIMIT orders. Ignored or 0 for MARKET orders.
    Quantity originalQuantity;
    Quantity remainingQuantity;
    long long sequence; // Sequence number inside the OrderBook (for FIFO tie-breaking)
};
