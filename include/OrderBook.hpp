#pragma once
#include "Types.hpp"
#include "Order.hpp"
#include "Trade.hpp"
#include <map>
#include <deque>
#include <vector>
#include <unordered_map>
#include <string>

class OrderBook {
public:
    explicit OrderBook(const std::string& symbol);

    // Adds a limit or market order to the book, matches it, and returns executed trades.
    std::vector<Trade> addOrder(Order order, TradeId& nextTradeId);

    // Cancels an order by ID. Returns true if successful, false if not found.
    bool cancelOrder(OrderId id);

    // Getters for book state (useful for CLI/display and testing)
    const std::map<Price, std::deque<Order>, std::greater<Price>>& getBids() const { return bids; }
    const std::map<Price, std::deque<Order>, std::less<Price>>& getAsks() const { return asks; }
    const std::string& getSymbol() const { return symbol; }

    // Helper to print the order book to the terminal
    void display() const;

    // Helper to check if order exists in lookup
    bool hasOrder(OrderId id) const { return orderLookup.find(id) != orderLookup.end(); }

private:
    std::string symbol;

    // Bids sorted highest price first
    std::map<Price, std::deque<Order>, std::greater<Price>> bids;

    // Asks sorted lowest price first
    std::map<Price, std::deque<Order>, std::less<Price>> asks;

    // Lookup index for quick order location (O(1) search for cancellation)
    struct OrderLocation {
        Price price;
        Side side;
    };
    std::unordered_map<OrderId, OrderLocation> orderLookup;

    long long lastSequence = 0;

    // Internal helper to match a buy order against sell book (asks)
    std::vector<Trade> matchOrder(Order& order, TradeId& nextTradeId);
};
