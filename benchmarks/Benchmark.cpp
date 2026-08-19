#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include "Exchange.hpp"
#include "OrderGenerator.hpp"

void runBenchmark(int numOrders) {
    Exchange exchange;
    
    // Generate orders
    OrderGenerator::Config config;
    config.numOrders = numOrders;
    config.symbols = {"BTCUSD", "ETHUSD", "AAPL", "MSFT"};
    config.buyRatio = 0.5;
    config.limitRatio = 0.9; // 90% Limit, 10% Market
    config.minPrice = 9900;
    config.maxPrice = 10100;
    config.minQty = 1;
    config.maxQty = 100;
    config.seed = 42;
    
    auto orders = OrderGenerator::generateOrders(config);
    
    std::vector<long long> latenciesNs;
    latenciesNs.reserve(numOrders);

    long long initialTrades = exchange.getEngine().getTradeHistory().size();
    
    auto startTimeTotal = std::chrono::steady_clock::now();
    
    for (auto& order : orders) {
        auto startTimeOrder = std::chrono::steady_clock::now();
        
        exchange.submitOrder(order);
        
        auto endTimeOrder = std::chrono::steady_clock::now();
        auto orderLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(endTimeOrder - startTimeOrder).count();
        latenciesNs.push_back(orderLatency);
    }
    
    auto endTimeTotal = std::chrono::steady_clock::now();
    auto totalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTimeTotal - startTimeTotal).count();
    
    long long finalTrades = exchange.getEngine().getTradeHistory().size();
    long long totalTrades = finalTrades - initialTrades;

    // Calculate metrics
    double totalTimeSec = totalTimeMs / 1000.0;
    double throughput = totalTimeSec > 0 ? (numOrders / totalTimeSec) : 0;
    
    // Sort latencies to find percentiles
    std::sort(latenciesNs.begin(), latenciesNs.end());
    
    long long sumLatency = std::accumulate(latenciesNs.begin(), latenciesNs.end(), 0LL);
    double avgLatencyUs = (sumLatency / (double)numOrders) / 1000.0;
    
    double p95LatencyUs = latenciesNs[static_cast<size_t>(numOrders * 0.95)] / 1000.0;
    double p99LatencyUs = latenciesNs[static_cast<size_t>(numOrders * 0.99)] / 1000.0;

    std::cout << "\n================= BENCHMARK RESULT (" << numOrders << " orders) =================\n";
    std::cout << "Total Orders:          " << numOrders << "\n";
    std::cout << "Total Trades Executed: " << totalTrades << "\n";
    std::cout << "Total Runtime:         " << totalTimeMs << " ms (" << totalTimeSec << " seconds)\n";
    std::cout << "Throughput:            " << std::fixed << std::setprecision(2) << throughput << " orders/sec\n";
    std::cout << "Average Latency:       " << avgLatencyUs << " us\n";
    std::cout << "P95 Latency:           " << p95LatencyUs << " us\n";
    std::cout << "P99 Latency:           " << p99LatencyUs << " us\n";
    std::cout << "========================================================================\n";
}

int main() {
    std::cout << "Running Matching Engine Benchmarks...\n";
    runBenchmark(10000);
    runBenchmark(100000);
    runBenchmark(1000000);
    return 0;
}
