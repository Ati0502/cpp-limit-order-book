#include "OrderBook.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>

OrderBook::OrderBook(const std::string& symbol) : symbol(symbol) {}

std::vector<Trade> OrderBook::addOrder(Order order, TradeId& nextTradeId) {
    order.sequence = ++lastSequence;
    order.remainingQuantity = order.originalQuantity;

    // Match order against the opposite book
    std::vector<Trade> trades = matchOrder(order, nextTradeId);

    // If remaining quantity > 0 and it's a LIMIT order, insert it into the book
    if (order.remainingQuantity > 0 && order.type == OrderType::Limit) {
        if (order.side == Side::Buy) {
            bids[order.price].push_back(order);
            orderLookup[order.id] = {order.price, Side::Buy};
        } else {
            asks[order.price].push_back(order);
            orderLookup[order.id] = {order.price, Side::Sell};
        }
    }

    return trades;
}

std::vector<Trade> OrderBook::matchOrder(Order& order, TradeId& nextTradeId) {
    std::vector<Trade> trades;

    if (order.side == Side::Buy) {
        while (order.remainingQuantity > 0 && !asks.empty()) {
            auto bestAskIt = asks.begin();
            Price askPrice = bestAskIt->first;

            // For LIMIT orders, check if price crosses
            if (order.type == OrderType::Limit && order.price < askPrice) {
                break;
            }

            auto& orderDeque = bestAskIt->second;
            while (order.remainingQuantity > 0 && !orderDeque.empty()) {
                Order& restingOrder = orderDeque.front();
                Quantity matchQty = std::min(order.remainingQuantity, restingOrder.remainingQuantity);

                Trade trade;
                trade.tradeId = nextTradeId++;
                trade.symbol = symbol;
                trade.price = restingOrder.price; // Resting price (passive execution)
                trade.quantity = matchQty;
                trade.buyOrderId = order.id;
                trade.sellOrderId = restingOrder.id;
                trade.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()
                ).count();

                trades.push_back(trade);

                order.remainingQuantity -= matchQty;
                restingOrder.remainingQuantity -= matchQty;

                if (restingOrder.remainingQuantity == 0) {
                    orderLookup.erase(restingOrder.id);
                    orderDeque.pop_front();
                }
            }

            if (orderDeque.empty()) {
                asks.erase(bestAskIt);
            }
        }
    } else { // Sell order matching against bids
        while (order.remainingQuantity > 0 && !bids.empty()) {
            auto bestBidIt = bids.begin();
            Price bidPrice = bestBidIt->first;

            // For LIMIT orders, check if price crosses
            if (order.type == OrderType::Limit && order.price > bidPrice) {
                break;
            }

            auto& orderDeque = bestBidIt->second;
            while (order.remainingQuantity > 0 && !orderDeque.empty()) {
                Order& restingOrder = orderDeque.front();
                Quantity matchQty = std::min(order.remainingQuantity, restingOrder.remainingQuantity);

                Trade trade;
                trade.tradeId = nextTradeId++;
                trade.symbol = symbol;
                trade.price = restingOrder.price; // Resting price (passive execution)
                trade.quantity = matchQty;
                trade.buyOrderId = restingOrder.id;
                trade.sellOrderId = order.id;
                trade.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()
                ).count();

                trades.push_back(trade);

                order.remainingQuantity -= matchQty;
                restingOrder.remainingQuantity -= matchQty;

                if (restingOrder.remainingQuantity == 0) {
                    orderLookup.erase(restingOrder.id);
                    orderDeque.pop_front();
                }
            }

            if (orderDeque.empty()) {
                bids.erase(bestBidIt);
            }
        }
    }

    return trades;
}

bool OrderBook::cancelOrder(OrderId id) {
    auto lookupIt = orderLookup.find(id);
    if (lookupIt == orderLookup.end()) {
        return false;
    }

    Price price = lookupIt->second.price;
    Side side = lookupIt->second.side;
    orderLookup.erase(lookupIt);

    if (side == Side::Buy) {
        auto bidsIt = bids.find(price);
        if (bidsIt != bids.end()) {
            auto& orderDeque = bidsIt->second;
            auto it = std::find_if(orderDeque.begin(), orderDeque.end(), [id](const Order& o) {
                return o.id == id;
            });
            if (it != orderDeque.end()) {
                orderDeque.erase(it);
            }
            if (orderDeque.empty()) {
                bids.erase(bidsIt);
            }
        }
    } else {
        auto asksIt = asks.find(price);
        if (asksIt != asks.end()) {
            auto& orderDeque = asksIt->second;
            auto it = std::find_if(orderDeque.begin(), orderDeque.end(), [id](const Order& o) {
                return o.id == id;
            });
            if (it != orderDeque.end()) {
                orderDeque.erase(it);
            }
            if (orderDeque.empty()) {
                asks.erase(asksIt);
            }
        }
    }

    return true;
}

void OrderBook::display() const {
    std::cout << "\n==================== " << symbol << " ====================\n";
    std::cout << "                      ASKS\n\n";
    std::cout << std::left << std::setw(15) << "Price" << std::setw(15) << "Quantity" << "\n";
    std::cout << "---------------------------------------------\n";
    
    // We print asks in descending order (highest ask down to lowest ask/best ask) for standard visual stack
    std::vector<std::pair<Price, Quantity>> askLevels;
    for (const auto& [price, deque] : asks) {
        Quantity totalQty = 0;
        for (const auto& order : deque) {
            totalQty += order.remainingQuantity;
        }
        askLevels.push_back({price, totalQty});
    }
    
    for (auto it = askLevels.rbegin(); it != askLevels.rend(); ++it) {
        std::cout << std::left << std::setw(15) << it->first << std::setw(15) << it->second << "\n";
    }

    Price bestAsk = asks.empty() ? 0 : asks.begin()->first;
    Price bestBid = bids.empty() ? 0 : bids.begin()->first;
    long long spread = (bestAsk > 0 && bestBid > 0) ? (bestAsk - bestBid) : 0;

    std::cout << "---------------------------------------------\n";
    if (bestAsk > 0) std::cout << "Best Ask: " << bestAsk << "\n";
    else std::cout << "Best Ask: N/A\n";
    
    if (bestAsk > 0 && bestBid > 0) std::cout << "Spread:   " << spread << "\n";
    else std::cout << "Spread:   N/A\n";
    
    if (bestBid > 0) std::cout << "Best Bid: " << bestBid << "\n";
    else std::cout << "Best Bid: N/A\n";
    std::cout << "---------------------------------------------\n";

    std::cout << "\n                      BIDS\n\n";
    std::cout << std::left << std::setw(15) << "Price" << std::setw(15) << "Quantity" << "\n";
    std::cout << "---------------------------------------------\n";

    for (const auto& [price, deque] : bids) {
        Quantity totalQty = 0;
        for (const auto& order : deque) {
            totalQty += order.remainingQuantity;
        }
        std::cout << std::left << std::setw(15) << price << std::setw(15) << totalQty << "\n";
    }
    std::cout << "=============================================\n\n";
}
