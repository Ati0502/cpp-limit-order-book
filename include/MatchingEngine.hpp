#pragma once
#include "Types.hpp"
#include "Order.hpp"
#include "Trade.hpp"
#include "OrderBook.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <optional>

class MatchingEngine {
public:
    MatchingEngine() = default;

    // Submits an order. Returns the list of executed trades.
    // If validation fails, returns std::nullopt (or throws, but optional is cleaner)
    // with a output message or status code. Let's return a struct with status.
    struct SubmissionResult {
        bool success;
        std::string errorMessage;
        std::vector<Trade> trades;
        OrderId assignedId;
    };
    SubmissionResult submitOrder(Order order);

    // Cancels an order by ID.
    bool cancelOrder(std::string symbol, OrderId id);
    
    // Cancels an order searching across all order books if symbol is not known.
    bool cancelOrder(OrderId id);

    // Get order book for a symbol
    OrderBook* getOrderBook(const std::string& symbol);
    const OrderBook* getOrderBook(const std::string& symbol) const;
    
    // Checks if order book exists for a symbol, and creates it if not.
    OrderBook& getOrCreateOrderBook(const std::string& symbol);

    // Retrieve trade history
    const std::vector<Trade>& getTradeHistory() const { return tradeHistory; }

    // Generates a unique order ID
    OrderId generateNextOrderId() { return nextOrderId++; }

    // Retrieve all active symbols
    std::vector<std::string> getSymbols() const;

private:
    std::unordered_map<std::string, OrderBook> books;
    std::vector<Trade> tradeHistory;
    std::unordered_set<OrderId> activeOrderIds;
    
    OrderId nextOrderId = 1;
    TradeId nextTradeId = 1;

    bool validateOrder(const Order& order, std::string& errorMsg) const;
};
