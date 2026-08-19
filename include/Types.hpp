#pragma once
#include <string>

using OrderId = long long;
using TradeId = long long;
using Price = long long;
using Quantity = long long;

enum class Side {
    Buy,
    Sell
};

enum class OrderType {
    Limit,
    Market
};

inline std::string sideToString(Side side) {
    return side == Side::Buy ? "BUY" : "SELL";
}

inline std::string orderTypeToString(OrderType type) {
    return type == OrderType::Limit ? "LIMIT" : "MARKET";
}
