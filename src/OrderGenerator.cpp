#include "OrderGenerator.hpp"
#include <random>
#include <iostream>
#include <chrono>

std::vector<Order> OrderGenerator::generateOrders(const Config& config) {
    std::vector<Order> orders;
    orders.reserve(config.numOrders);

    std::mt19937 rng(config.seed);
    std::uniform_real_distribution<double> realDist(0.0, 1.0);
    std::uniform_int_distribution<long long> priceDist(config.minPrice, config.maxPrice);
    std::uniform_int_distribution<long long> qtyDist(config.minQty, config.maxQty);
    std::uniform_int_distribution<size_t> symbolDist(0, config.symbols.size() - 1);

    for (int i = 0; i < config.numOrders; ++i) {
        Order order;
        order.id = 0; // Exchange will assign ID
        order.symbol = config.symbols[symbolDist(rng)];
        order.side = (realDist(rng) < config.buyRatio) ? Side::Buy : Side::Sell;
        order.type = (realDist(rng) < config.limitRatio) ? OrderType::Limit : OrderType::Market;
        order.price = (order.type == OrderType::Limit) ? priceDist(rng) : 0;
        
        Quantity q = qtyDist(rng);
        order.originalQuantity = q;
        order.remainingQuantity = q;
        order.sequence = 0;

        orders.push_back(order);
    }

    return orders;
}

void OrderGenerator::runSimulation(Exchange& exchange, const Config& config, bool verbose) {
    auto orders = generateOrders(config);
    
    std::cout << "\nStarting simulation with " << orders.size() << " orders...\n";
    auto startTime = std::chrono::steady_clock::now();
    
    long long tradesBefore = exchange.getEngine().getTradeHistory().size();
    long long volumeTraded = 0;

    for (auto& order : orders) {
        auto result = exchange.submitOrder(order);
        if (result.success && verbose) {
            std::cout << "Submitted: " << sideToString(order.side) 
                      << " " << order.symbol 
                      << " " << orderTypeToString(order.type);
            if (order.type == OrderType::Limit) std::cout << " @ " << order.price;
            std::cout << " Qty: " << order.originalQuantity 
                      << " -> Order ID: " << result.assignedId 
                      << " Executed Trades: " << result.trades.size() << "\n";
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    long long tradesAfter = exchange.getEngine().getTradeHistory().size();
    long long newTrades = tradesAfter - tradesBefore;

    // Calculate volume of new trades
    const auto& history = exchange.getEngine().getTradeHistory();
    for (size_t i = tradesBefore; i < history.size(); ++i) {
        volumeTraded += history[i].quantity;
    }

    std::cout << "Simulation completed in " << duration << " ms.\n";
    std::cout << "Orders processed: " << orders.size() << "\n";
    std::cout << "New trades executed: " << newTrades << "\n";
    std::cout << "Volume traded: " << volumeTraded << "\n";
    if (duration > 0) {
        std::cout << "Throughput: " << (orders.size() * 1000.0 / duration) << " orders/sec\n";
    }
}
