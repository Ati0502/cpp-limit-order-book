#include "Exchange.hpp"

MatchingEngine::SubmissionResult Exchange::submitOrder(const Order& order) {
    return engine.submitOrder(order);
}

bool Exchange::cancelOrder(OrderId id) {
    return engine.cancelOrder(id);
}

bool Exchange::cancelOrder(const std::string& symbol, OrderId id) {
    return engine.cancelOrder(symbol, id);
}
