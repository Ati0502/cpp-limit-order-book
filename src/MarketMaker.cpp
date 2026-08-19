#include "MarketMaker.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

MarketMaker::MarketMaker(Exchange& exchange, const std::string& symbol, double initialCash)
    : exchange(exchange), symbol(symbol), cash(initialCash), initialCash(initialCash) {}

void MarketMaker::updateQuotes(Price halfSpread, Quantity quoteSize, Price defaultPrice) {
    // 1. Cancel previous quotes first if they are still resting
    if (lastBuyOrderId > 0) {
        exchange.cancelOrder(symbol, lastBuyOrderId);
        lastBuyOrderId = 0;
    }
    if (lastSellOrderId > 0) {
        exchange.cancelOrder(symbol, lastSellOrderId);
        lastSellOrderId = 0;
    }

    // 2. Observe the order book to calculate mid-price
    const auto* book = exchange.getEngine().getOrderBook(symbol);
    Price bestBid = 0;
    Price bestAsk = 0;

    if (book) {
        if (!book->getBids().empty()) {
            bestBid = book->getBids().begin()->first;
        }
        if (!book->getAsks().empty()) {
            bestAsk = book->getAsks().begin()->first;
        }
    }

    Price midPrice = defaultPrice;
    if (bestBid > 0 && bestAsk > 0) {
        midPrice = (bestBid + bestAsk) / 2;
    } else if (bestBid > 0) {
        midPrice = bestBid;
    } else if (bestAsk > 0) {
        midPrice = bestAsk;
    }

    Price buyPrice = midPrice - halfSpread;
    Price sellPrice = midPrice + halfSpread;

    if (buyPrice <= 0) {
        buyPrice = 1; // Price must be positive
    }

    // 3. Place new quotes
    Order buyOrder;
    buyOrder.id = 0;
    buyOrder.symbol = symbol;
    buyOrder.side = Side::Buy;
    buyOrder.type = OrderType::Limit;
    buyOrder.price = buyPrice;
    buyOrder.originalQuantity = quoteSize;
    buyOrder.remainingQuantity = quoteSize;
    buyOrder.sequence = 0;

    auto buyResult = exchange.submitOrder(buyOrder);
    if (buyResult.success) {
        lastBuyOrderId = buyResult.assignedId;
        // Process any immediate fills that occurred upon submission
        for (const auto& trade : buyResult.trades) {
            processTrade(trade);
        }
    }

    Order sellOrder;
    sellOrder.id = 0;
    sellOrder.symbol = symbol;
    sellOrder.side = Side::Sell;
    sellOrder.type = OrderType::Limit;
    sellOrder.price = sellPrice;
    sellOrder.originalQuantity = quoteSize;
    sellOrder.remainingQuantity = quoteSize;
    sellOrder.sequence = 0;

    auto sellResult = exchange.submitOrder(sellOrder);
    if (sellResult.success) {
        lastSellOrderId = sellResult.assignedId;
        // Process any immediate fills that occurred upon submission
        for (const auto& trade : sellResult.trades) {
            processTrade(trade);
        }
    }
}

void MarketMaker::processTrade(const Trade& trade) {
    if (trade.symbol != symbol) return;

    if (trade.buyOrderId == lastBuyOrderId) {
        updatePosition(Side::Buy, trade.price, trade.quantity);
    } else if (trade.sellOrderId == lastSellOrderId) {
        updatePosition(Side::Sell, trade.price, trade.quantity);
    }
}

void MarketMaker::updatePosition(Side side, Price price, Quantity qty) {
    numTrades++;
    tradedVolume += qty;

    if (side == Side::Buy) {
        if (inventory >= 0) {
            // Long adding to long
            double totalCost = (inventory * avgCost) + (qty * price);
            inventory += qty;
            avgCost = totalCost / inventory;
        } else {
            // Long covering short position
            Quantity absInv = std::abs(inventory);
            if (qty <= absInv) {
                realizedPnL += qty * (avgCost - price);
                inventory += qty;
                if (inventory == 0) avgCost = 0.0;
            } else {
                Quantity coverQty = absInv;
                Quantity longQty = qty - absInv;
                realizedPnL += coverQty * (avgCost - price);
                inventory = longQty;
                avgCost = price;
            }
        }
        cash -= (qty * price);
    } else { // Sell
        if (inventory <= 0) {
            // Short adding to short
            Quantity absInv = std::abs(inventory);
            double totalCost = (absInv * avgCost) + (qty * price);
            inventory -= qty;
            avgCost = totalCost / std::abs(inventory);
        } else {
            // Short closing long position
            if (qty <= inventory) {
                realizedPnL += qty * (price - avgCost);
                inventory -= qty;
                if (inventory == 0) avgCost = 0.0;
            } else {
                Quantity closeQty = inventory;
                Quantity shortQty = qty - inventory;
                realizedPnL += closeQty * (price - avgCost);
                inventory = -shortQty;
                avgCost = price;
            }
        }
        cash += (qty * price);
    }
}

double MarketMaker::getUnrealizedPnL(Price midPrice) const {
    if (inventory > 0) {
        return inventory * (midPrice - avgCost);
    } else if (inventory < 0) {
        return std::abs(inventory) * (avgCost - midPrice);
    }
    return 0.0;
}

double MarketMaker::getTotalPnL(Price midPrice) const {
    return realizedPnL + getUnrealizedPnL(midPrice);
}

void MarketMaker::displayStatus(Price midPrice) const {
    std::cout << "\n================= MARKET MAKER STATUS =================\n";
    std::cout << "Symbol:                " << symbol << "\n";
    std::cout << "Cash:                  $" << std::fixed << std::setprecision(2) << cash << "\n";
    std::cout << "Inventory:             " << inventory << " units\n";
    std::cout << "Average Cost:          $" << avgCost << "\n";
    std::cout << "Mid Price:             $" << midPrice << "\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << "Realized PnL:          $" << realizedPnL << "\n";
    std::cout << "Unrealized PnL:        $" << getUnrealizedPnL(midPrice) << "\n";
    std::cout << "Total PnL:             $" << getTotalPnL(midPrice) << "\n";
    std::cout << "MM Trades Executed:    " << numTrades << "\n";
    std::cout << "MM Traded Volume:      " << tradedVolume << " units\n";
    std::cout << "=======================================================\n\n";
}
