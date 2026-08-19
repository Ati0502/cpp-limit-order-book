#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include "Exchange.hpp"
#include "OrderGenerator.hpp"
#include "MarketMaker.hpp"
#include <random>

// Forward declaration of Simulation and MarketMaker helpers if we implement them later
void runSimulationMenu(Exchange& exchange);
void runMarketMakerMenu(Exchange& exchange);

void displayMainMenu() {
    std::cout << "\n===== TRADING SIMULATOR =====\n";
    std::cout << "1. Add Order (Manual Command)\n";
    std::cout << "2. Cancel Order\n";
    std::cout << "3. Show Order Book\n";
    std::cout << "4. Show Executed Trades (History)\n";
    std::cout << "5. Show Statistics\n";
    std::cout << "6. Run Random Order Simulation\n";
    std::cout << "7. Run Market-Making Strategy\n";
    std::cout << "8. Exit\n";
    std::cout << "=============================\n";
    std::cout << "Enter choice (1-8): ";
}

void handleAddOrder(Exchange& exchange) {
    std::cout << "\nEnter order details in one of the following formats:\n";
    std::cout << "  LIMIT:  [BUY/SELL] [SYMBOL] LIMIT [PRICE] [QUANTITY]\n";
    std::cout << "  MARKET: [BUY/SELL] [SYMBOL] MARKET [QUANTITY]\n";
    std::cout << "Example: BUY BTCUSD LIMIT 100000 5\n";
    std::cout << ">> ";
    
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) {
        std::getline(std::cin, line); // Handle leftover newline if any
    }

    std::stringstream ss(line);
    std::string sideStr, symbol, typeStr;
    ss >> sideStr >> symbol >> typeStr;

    if (sideStr.empty() || symbol.empty() || typeStr.empty()) {
        std::cout << "Error: Invalid command format.\n";
        return;
    }

    // Convert strings to uppercase
    std::transform(sideStr.begin(), sideStr.end(), sideStr.begin(), ::toupper);
    std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::toupper);
    std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

    Side side;
    if (sideStr == "BUY") {
        side = Side::Buy;
    } else if (sideStr == "SELL") {
        side = Side::Sell;
    } else {
        std::cout << "Error: Invalid side (must be BUY or SELL).\n";
        return;
    }

    OrderType type;
    if (typeStr == "LIMIT") {
        type = OrderType::Limit;
    } else if (typeStr == "MARKET") {
        type = OrderType::Market;
    } else {
        std::cout << "Error: Invalid order type (must be LIMIT or MARKET).\n";
        return;
    }

    Price price = 0;
    Quantity qty = 0;

    if (type == OrderType::Limit) {
        ss >> price >> qty;
        if (ss.fail()) {
            std::cout << "Error: Could not parse price and quantity.\n";
            return;
        }
    } else {
        ss >> qty;
        if (ss.fail()) {
            std::cout << "Error: Could not parse quantity.\n";
            return;
        }
    }

    Order order;
    order.id = 0; // Let the engine assign a unique ID
    order.symbol = symbol;
    order.side = side;
    order.type = type;
    order.price = price;
    order.originalQuantity = qty;
    order.remainingQuantity = qty;
    order.sequence = 0;

    auto result = exchange.submitOrder(order);
    if (!result.success) {
        std::cout << "Order Submission Failed: " << result.errorMessage << "\n";
        return;
    }

    std::cout << "\nORDER SUBMITTED SUCCESSFULLY\n";
    std::cout << "Assigned Order ID: " << result.assignedId << "\n";
    std::cout << "Symbol: " << symbol << " | Side: " << sideStr << " | Type: " << typeStr;
    if (type == OrderType::Limit) {
        std::cout << " | Price: " << price;
    }
    std::cout << " | Quantity: " << qty << "\n";

    if (!result.trades.empty()) {
        std::cout << "\n>>> " << result.trades.size() << " TRADE(S) EXECUTED:\n";
        std::cout << std::left << std::setw(10) << "Trade ID" 
                  << std::setw(10) << "Price" 
                  << std::setw(10) << "Qty" 
                  << std::setw(12) << "Buy Order" 
                  << std::setw(12) << "Sell Order" << "\n";
        std::cout << "--------------------------------------------------------\n";
        for (const auto& trade : result.trades) {
            std::cout << std::left << std::setw(10) << trade.tradeId
                      << std::setw(10) << trade.price
                      << std::setw(10) << trade.quantity
                      << std::setw(12) << trade.buyOrderId
                      << std::setw(12) << trade.sellOrderId << "\n";
        }
    }
}

void handleCancelOrder(Exchange& exchange) {
    OrderId id;
    std::cout << "Enter Order ID to cancel: ";
    if (!(std::cin >> id)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Error: Invalid Order ID input.\n";
        return;
    }

    bool success = exchange.cancelOrder(id);
    if (success) {
        std::cout << "Order ID " << id << " cancelled successfully.\n";
    } else {
        std::cout << "Error: Order ID " << id << " not found or already fully executed.\n";
    }
}

void handleShowOrderBook(Exchange& exchange) {
    auto symbols = exchange.getEngine().getSymbols();
    if (symbols.empty()) {
        std::cout << "No active order books in the system. Submit an order first!\n";
        return;
    }

    std::cout << "Active Symbols: ";
    for (size_t i = 0; i < symbols.size(); ++i) {
        std::cout << symbols[i] << (i == symbols.size() - 1 ? "" : ", ");
    }
    std::cout << "\nEnter symbol to display (or press enter for all): ";
    
    std::string symbol;
    std::getline(std::cin, symbol);
    if (symbol.empty()) {
        std::getline(std::cin, symbol); // Handle leftover newline
    }
    std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

    if (symbol.empty()) {
        for (const auto& sym : symbols) {
            auto* book = exchange.getEngine().getOrderBook(sym);
            if (book) book->display();
        }
    } else {
        auto* book = exchange.getEngine().getOrderBook(symbol);
        if (book) {
            book->display();
        } else {
            std::cout << "Error: Symbol " << symbol << " not found.\n";
        }
    }
}

void handleShowTrades(Exchange& exchange) {
    const auto& history = exchange.getEngine().getTradeHistory();
    if (history.empty()) {
        std::cout << "No trades executed yet.\n";
        return;
    }

    std::cout << "\n==================== TRADE HISTORY ====================\n";
    std::cout << std::left << std::setw(10) << "Trade ID" 
              << std::setw(10) << "Symbol"
              << std::setw(10) << "Price" 
              << std::setw(10) << "Quantity" 
              << std::setw(12) << "Buy Order" 
              << std::setw(12) << "Sell Order" << "\n";
    std::cout << "-------------------------------------------------------\n";
    for (const auto& trade : history) {
        std::cout << std::left << std::setw(10) << trade.tradeId
                  << std::setw(10) << trade.symbol
                  << std::setw(10) << trade.price
                  << std::setw(10) << trade.quantity
                  << std::setw(12) << trade.buyOrderId
                  << std::setw(12) << trade.sellOrderId << "\n";
    }
    std::cout << "=======================================================\n\n";
}

void handleShowStatistics(Exchange& exchange) {
    const auto& history = exchange.getEngine().getTradeHistory();
    long long totalTrades = history.size();
    long long totalVolume = 0;
    
    // We can count active orders
    long long activeOrders = 0;
    auto symbols = exchange.getEngine().getSymbols();
    for (const auto& sym : symbols) {
        const auto* book = exchange.getEngine().getOrderBook(sym);
        if (book) {
            for (const auto& [price, deque] : book->getBids()) {
                activeOrders += deque.size();
            }
            for (const auto& [price, deque] : book->getAsks()) {
                activeOrders += deque.size();
            }
        }
    }

    for (const auto& trade : history) {
        totalVolume += trade.quantity;
    }

    std::cout << "\n================= EXCHANGE STATISTICS =================\n";
    std::cout << "Total Active Symbols:    " << symbols.size() << "\n";
    std::cout << "Total Resting Orders:    " << activeOrders << "\n";
    std::cout << "Total Executed Trades:   " << totalTrades << "\n";
    std::cout << "Total Traded Volume:     " << totalVolume << "\n";
    std::cout << "=======================================================\n\n";
}

int main() {
    Exchange exchange;
    
    // Pre-create some symbols for convenience
    exchange.getEngine().getOrCreateOrderBook("BTCUSD");
    exchange.getEngine().getOrCreateOrderBook("ETHUSD");
    exchange.getEngine().getOrCreateOrderBook("AAPL");

    std::string choiceStr;
    while (true) {
        displayMainMenu();
        if (!std::getline(std::cin, choiceStr)) {
            break;
        }

        if (choiceStr.empty()) continue;
        int choice = 0;
        try {
            choice = std::stoi(choiceStr);
        } catch (...) {
            std::cout << "Invalid choice. Please enter a number between 1 and 8.\n";
            continue;
        }

        if (choice == 8) {
            std::cout << "Exiting Simulator. Goodbye!\n";
            break;
        }

        switch (choice) {
            case 1:
                handleAddOrder(exchange);
                break;
            case 2:
                handleCancelOrder(exchange);
                std::cin.ignore(10000, '\n'); // Clear trailing newlines
                break;
            case 3:
                handleShowOrderBook(exchange);
                break;
            case 4:
                handleShowTrades(exchange);
                break;
            case 5:
                handleShowStatistics(exchange);
                break;
            case 6:
                runSimulationMenu(exchange);
                break;
            case 7:
                runMarketMakerMenu(exchange);
                break;
            default:
                std::cout << "Invalid choice. Please enter a number between 1 and 8.\n";
                break;
        }
    }
    return 0;
}

void runSimulationMenu(Exchange& exchange) {
    OrderGenerator::Config config;
    std::string input;

    std::cout << "\n--- RANDOM ORDER SIMULATION SETUP ---\n";
    std::cout << "Enter number of orders [1000]: ";
    std::getline(std::cin, input);
    if (!input.empty()) config.numOrders = std::stoi(input);

    std::cout << "Enter BUY/SELL ratio (0.0 to 1.0) [0.5]: ";
    std::getline(std::cin, input);
    if (!input.empty()) config.buyRatio = std::stod(input);

    std::cout << "Enter LIMIT/MARKET ratio (0.0 to 1.0) [0.8]: ";
    std::getline(std::cin, input);
    if (!input.empty()) config.limitRatio = std::stod(input);

    std::cout << "Enter min price [9900]: ";
    std::getline(std::cin, input);
    if (!input.empty()) config.minPrice = std::stoll(input);

    std::cout << "Enter max price [10100]: ";
    std::getline(std::cin, input);
    if (!input.empty()) config.maxPrice = std::stoll(input);

    std::cout << "Enter random seed [42]: ";
    std::getline(std::cin, input);
    if (!input.empty()) config.seed = std::stoul(input);

    std::cout << "Verbose log of submissions? (y/n) [n]: ";
    std::getline(std::cin, input);
    bool verbose = (input == "y" || input == "Y");

    OrderGenerator::runSimulation(exchange, config, verbose);
}

void runMarketMakerMenu(Exchange& exchange) {
    std::string symbol = "BTCUSD";
    Price halfSpread = 5;
    Quantity quoteSize = 10;
    int cycles = 10;
    Price defaultPrice = 10000;
    
    std::string input;
    std::cout << "\n--- EDUCATIONAL MARKET MAKER BOT ---\n";
    std::cout << "Enter symbol to trade [BTCUSD]: ";
    std::getline(std::cin, input);
    if (!input.empty()) symbol = input;
    
    std::cout << "Enter half spread [5]: ";
    std::getline(std::cin, input);
    if (!input.empty()) halfSpread = std::stoll(input);

    std::cout << "Enter quote size [10]: ";
    std::getline(std::cin, input);
    if (!input.empty()) quoteSize = std::stoll(input);

    std::cout << "Enter simulation cycles (steps) [10]: ";
    std::getline(std::cin, input);
    if (!input.empty()) cycles = std::stoi(input);

    MarketMaker mm(exchange, symbol);

    std::cout << "\nRunning " << cycles << " cycles of Market Making on " << symbol << "...\n";
    
    std::mt19937 rng(1337); // Deterministic seed for consistency
    std::uniform_real_distribution<double> realDist(0.0, 1.0);

    for (int c = 1; c <= cycles; ++c) {
        std::cout << "\n------------------- Cycle " << c << " / " << cycles << " -------------------\n";
        
        // 1. MM places new quotes
        mm.updateQuotes(halfSpread, quoteSize, defaultPrice);
        
        // Let's find the current mid-price
        const auto* book = exchange.getEngine().getOrderBook(symbol);
        Price bestBid = 0, bestAsk = 0;
        if (book) {
            if (!book->getBids().empty()) bestBid = book->getBids().begin()->first;
            if (!book->getAsks().empty()) bestAsk = book->getAsks().begin()->first;
        }
        Price mid = defaultPrice;
        if (bestBid > 0 && bestAsk > 0) mid = (bestBid + bestAsk) / 2;
        else if (bestBid > 0) mid = bestBid;
        else if (bestAsk > 0) mid = bestAsk;

        // 2. Generate some retail orders to trade against MM
        std::uniform_int_distribution<long long> priceOffsetDist(-halfSpread * 2, halfSpread * 2);
        std::uniform_int_distribution<long long> qtyDist(1, quoteSize);
        
        long long tradeIndexStart = exchange.getEngine().getTradeHistory().size();

        for (int i = 0; i < 5; ++i) {
            Order retailOrder;
            retailOrder.id = 0;
            retailOrder.symbol = symbol;
            retailOrder.side = (realDist(rng) < 0.5) ? Side::Buy : Side::Sell;
            retailOrder.type = (realDist(rng) < 0.3) ? OrderType::Market : OrderType::Limit;
            
            retailOrder.price = mid + priceOffsetDist(rng);
            Quantity q = qtyDist(rng);
            retailOrder.originalQuantity = q;
            retailOrder.remainingQuantity = q;
            retailOrder.sequence = 0;

            auto result = exchange.submitOrder(retailOrder);
            if (result.success) {
                for (const auto& trade : result.trades) {
                    mm.processTrade(trade);
                }
            }
        }

        long long tradeIndexEnd = exchange.getEngine().getTradeHistory().size();
        std::cout << "Executed " << (tradeIndexEnd - tradeIndexStart) << " new trade(s) in this cycle.\n";

        // 3. Display MM portfolio status
        mm.displayStatus(mid);
    }
}
