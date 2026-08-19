#pragma once
#include "Types.hpp"
#include <string>

struct Trade {
    TradeId tradeId;
    std::string symbol;
    Price price;
    Quantity quantity;
    OrderId buyOrderId;
    OrderId sellOrderId;
    long long timestamp; // Timestamp or sequence number of when the match occurred
};
