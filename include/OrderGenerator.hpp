#pragma once
#include "Exchange.hpp"
#include <vector>
#include <string>

class OrderGenerator {
public:
    struct Config {
        int numOrders = 1000;
        std::vector<std::string> symbols = {"BTCUSD"};
        double buyRatio = 0.5;      // BUY / (BUY + SELL)
        double limitRatio = 0.8;    // LIMIT / (LIMIT + MARKET)
        Price minPrice = 9900;
        Price maxPrice = 10100;
        Quantity minQty = 1;
        Quantity maxQty = 50;
        unsigned int seed = 42;
    };

    // Generates a batch of randomized orders
    static std::vector<Order> generateOrders(const Config& config);

    // Runs a simulation on the exchange using the config
    static void runSimulation(Exchange& exchange, const Config& config, bool verbose = false);
};
