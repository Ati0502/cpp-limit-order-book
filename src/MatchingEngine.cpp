#include "MatchingEngine.hpp"
#include <algorithm>

bool MatchingEngine::validateOrder(const Order& order, std::string& errorMsg) const {
    if (order.originalQuantity <= 0) {
        errorMsg = "Quantity must be positive.";
        return false;
    }
    if (order.type == OrderType::Limit && order.price <= 0) {
        errorMsg = "Limit order price must be positive.";
        return false;
    }
    if (order.symbol.empty()) {
        errorMsg = "Symbol cannot be empty.";
        return false;
    }
    if (order.id > 0 && activeOrderIds.find(order.id) != activeOrderIds.end()) {
        errorMsg = "Duplicate Order ID: " + std::to_string(order.id);
        return false;
    }
    return true;
}

MatchingEngine::SubmissionResult MatchingEngine::submitOrder(Order order) {
    SubmissionResult result;
    result.success = false;
    result.assignedId = 0;

    std::string errorMsg;
    if (!validateOrder(order, errorMsg)) {
        result.errorMessage = errorMsg;
        return result;
    }

    if (order.id <= 0) {
        order.id = generateNextOrderId();
    }
    result.assignedId = order.id;

    // Track active order ID
    activeOrderIds.insert(order.id);

    OrderBook& book = getOrCreateOrderBook(order.symbol);
    
    // Execute trade matching
    result.trades = book.addOrder(order, nextTradeId);
    
    // Record executed trades in history
    for (const auto& trade : result.trades) {
        tradeHistory.push_back(trade);
    }

    // Clean up activeOrderIds: if orders are fully filled (no longer resting in the book)
    // we remove them from activeOrderIds.
    // 1. Check incoming order
    if (order.type == OrderType::Market || !book.hasOrder(order.id)) {
        activeOrderIds.erase(order.id);
    }
    
    // 2. Check resting orders that matched in the trades
    for (const auto& trade : result.trades) {
        OrderId restingId = (order.id == trade.buyOrderId) ? trade.sellOrderId : trade.buyOrderId;
        if (!book.hasOrder(restingId)) {
            activeOrderIds.erase(restingId);
        }
    }

    result.success = true;
    return result;
}

bool MatchingEngine::cancelOrder(std::string symbol, OrderId id) {
    auto it = books.find(symbol);
    if (it != books.end()) {
        if (it->second.cancelOrder(id)) {
            activeOrderIds.erase(id);
            return true;
        }
    }
    return false;
}

bool MatchingEngine::cancelOrder(OrderId id) {
    for (auto& [symbol, book] : books) {
        if (book.cancelOrder(id)) {
            activeOrderIds.erase(id);
            return true;
        }
    }
    return false;
}

OrderBook* MatchingEngine::getOrderBook(const std::string& symbol) {
    auto it = books.find(symbol);
    if (it != books.end()) {
        return &(it->second);
    }
    return nullptr;
}

const OrderBook* MatchingEngine::getOrderBook(const std::string& symbol) const {
    auto it = books.find(symbol);
    if (it != books.end()) {
        return &(it->second);
    }
    return nullptr;
}

OrderBook& MatchingEngine::getOrCreateOrderBook(const std::string& symbol) {
    auto it = books.find(symbol);
    if (it == books.end()) {
        auto [insertedIt, success] = books.emplace(symbol, OrderBook(symbol));
        return insertedIt->second;
    }
    return it->second;
}

std::vector<std::string> MatchingEngine::getSymbols() const {
    std::vector<std::string> symbols;
    for (const auto& [symbol, _] : books) {
        symbols.push_back(symbol);
    }
    std::sort(symbols.begin(), symbols.end());
    return symbols;
}
