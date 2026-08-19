#pragma once
#include "MatchingEngine.hpp"

class Exchange {
public:
    Exchange() = default;

    MatchingEngine::SubmissionResult submitOrder(const Order& order);
    bool cancelOrder(OrderId id);
    bool cancelOrder(const std::string& symbol, OrderId id);

    MatchingEngine& getEngine() { return engine; }
    const MatchingEngine& getEngine() const { return engine; }

private:
    MatchingEngine engine;
};
