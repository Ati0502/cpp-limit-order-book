#pragma once
#include "Exchange.hpp"
#include "Types.hpp"
#include "Trade.hpp"
#include <string>

class MarketMaker {
public:
    MarketMaker(Exchange& exchange, const std::string& symbol, double initialCash = 1000000.0);

    // Run one step of the market-making strategy
    // Calculates mid-price, cancels previous quotes, and places new buy/sell quotes.
    void updateQuotes(Price halfSpread, Quantity quoteSize, Price defaultPrice = 10000);

    // Call this whenever a trade occurs in the exchange to see if the MM was filled
    void processTrade(const Trade& trade);

    // Display MM status
    void displayStatus(Price midPrice) const;

    // Getters for PnL & stats
    double getCash() const { return cash; }
    Quantity getInventory() const { return inventory; }
    double getRealizedPnL() const { return realizedPnL; }
    double getUnrealizedPnL(Price midPrice) const;
    double getTotalPnL(Price midPrice) const;
    long long getNumTrades() const { return numTrades; }
    long long getTradedVolume() const { return tradedVolume; }

    OrderId getLastBuyOrderId() const { return lastBuyOrderId; }
    OrderId getLastSellOrderId() const { return lastSellOrderId; }

private:
    Exchange& exchange;
    std::string symbol;

    double cash;
    double initialCash;
    Quantity inventory = 0;
    double avgCost = 0.0; // Running average cost of long position, or short position

    double realizedPnL = 0.0;
    long long numTrades = 0;
    long long tradedVolume = 0;

    OrderId lastBuyOrderId = 0;
    OrderId lastSellOrderId = 0;

    // Internal helper to update position when a trade matches our order
    void updatePosition(Side side, Price price, Quantity qty);
};
