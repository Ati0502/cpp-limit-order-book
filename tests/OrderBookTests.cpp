#include <gtest/gtest.h>
#include "OrderBook.hpp"

TEST(OrderBookTests, BasicInsertionAndSorting) {
    OrderBook book("AAPL");
    TradeId tradeId = 1;

    // Insert bids
    book.addOrder({1, "AAPL", Side::Buy, OrderType::Limit, 100, 10, 10, 0}, tradeId);
    book.addOrder({2, "AAPL", Side::Buy, OrderType::Limit, 105, 5, 5, 0}, tradeId);
    book.addOrder({3, "AAPL", Side::Buy, OrderType::Limit, 98, 20, 20, 0}, tradeId);

    // Bids should be sorted descending: 105, 100, 98
    const auto& bids = book.getBids();
    auto it = bids.begin();
    ASSERT_NE(it, bids.end());
    EXPECT_EQ(it->first, 105);
    
    ++it;
    ASSERT_NE(it, bids.end());
    EXPECT_EQ(it->first, 100);

    ++it;
    ASSERT_NE(it, bids.end());
    EXPECT_EQ(it->first, 98);
}

TEST(OrderBookTests, ExactMatch) {
    OrderBook book("BTCUSD");
    TradeId tradeId = 1;

    // BUY 100 @ 100
    auto trades1 = book.addOrder({1, "BTCUSD", Side::Buy, OrderType::Limit, 100, 100, 100, 0}, tradeId);
    EXPECT_TRUE(trades1.empty());

    // SELL 100 @ 100
    auto trades2 = book.addOrder({2, "BTCUSD", Side::Sell, OrderType::Limit, 100, 100, 100, 0}, tradeId);
    ASSERT_EQ(trades2.size(), 1);
    
    EXPECT_EQ(trades2[0].price, 100);
    EXPECT_EQ(trades2[0].quantity, 100);
    EXPECT_EQ(trades2[0].buyOrderId, 1);
    EXPECT_EQ(trades2[0].sellOrderId, 2);

    EXPECT_TRUE(book.getBids().empty());
    EXPECT_TRUE(book.getAsks().empty());
}

TEST(OrderBookTests, PartialFill) {
    OrderBook book("BTCUSD");
    TradeId tradeId = 1;

    // BUY 100 @ 100
    book.addOrder({1, "BTCUSD", Side::Buy, OrderType::Limit, 100, 100, 100, 0}, tradeId);

    // SELL 40 @ 100
    auto trades = book.addOrder({2, "BTCUSD", Side::Sell, OrderType::Limit, 100, 40, 40, 0}, tradeId);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 40);
    EXPECT_EQ(trades[0].price, 100);

    // The BUY order should have 60 remaining
    const auto& bids = book.getBids();
    ASSERT_FALSE(bids.empty());
    EXPECT_EQ(bids.begin()->first, 100);
    EXPECT_EQ(bids.begin()->second.front().remainingQuantity, 60);
}

TEST(OrderBookTests, PricePriority) {
    OrderBook book("BTCUSD");
    TradeId tradeId = 1;

    // Sell offers at different price levels
    book.addOrder({1, "BTCUSD", Side::Sell, OrderType::Limit, 102, 10, 10, 0}, tradeId);
    book.addOrder({2, "BTCUSD", Side::Sell, OrderType::Limit, 101, 10, 10, 0}, tradeId);

    // BUY 15 @ 102
    // It should match the lowest ask (101) first, and then the remaining 5 at 102.
    auto trades = book.addOrder({3, "BTCUSD", Side::Buy, OrderType::Limit, 102, 15, 15, 0}, tradeId);
    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].price, 101);
    EXPECT_EQ(trades[0].quantity, 10);

    EXPECT_EQ(trades[1].price, 102);
    EXPECT_EQ(trades[1].quantity, 5);
}

TEST(OrderBookTests, TimePriority) {
    OrderBook book("BTCUSD");
    TradeId tradeId = 1;

    // First Buy Order at 100
    book.addOrder({1, "BTCUSD", Side::Buy, OrderType::Limit, 100, 10, 10, 0}, tradeId);
    // Second Buy Order at 100
    book.addOrder({2, "BTCUSD", Side::Buy, OrderType::Limit, 100, 15, 15, 0}, tradeId);

    // Sell Order 10 @ 100 should match against the first order (1)
    auto trades = book.addOrder({3, "BTCUSD", Side::Sell, OrderType::Limit, 100, 10, 10, 0}, tradeId);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].buyOrderId, 1);
    EXPECT_EQ(trades[0].quantity, 10);

    // Check that order 1 is fully filled and order 2 is resting
    const auto& bids = book.getBids();
    ASSERT_FALSE(bids.empty());
    EXPECT_EQ(bids.begin()->second.size(), 1);
    EXPECT_EQ(bids.begin()->second.front().id, 2);
}

TEST(OrderBookTests, Cancellation) {
    OrderBook book("BTCUSD");
    TradeId tradeId = 1;

    book.addOrder({1, "BTCUSD", Side::Buy, OrderType::Limit, 100, 10, 10, 0}, tradeId);
    book.addOrder({2, "BTCUSD", Side::Buy, OrderType::Limit, 100, 15, 15, 0}, tradeId);
    book.addOrder({3, "BTCUSD", Side::Buy, OrderType::Limit, 100, 20, 20, 0}, tradeId);

    // Cancel first order (1)
    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_FALSE(book.hasOrder(1));

    // Cancel middle order (2)
    EXPECT_TRUE(book.cancelOrder(2));
    EXPECT_FALSE(book.hasOrder(2));

    // Cancel last order (3)
    EXPECT_TRUE(book.cancelOrder(3));
    EXPECT_FALSE(book.hasOrder(3));

    // Cancel nonexistent
    EXPECT_FALSE(book.cancelOrder(999));

    // Verify bids is empty now
    EXPECT_TRUE(book.getBids().empty());
}

TEST(OrderBookTests, MarketOrder) {
    OrderBook book("BTCUSD");
    TradeId tradeId = 1;

    book.addOrder({1, "BTCUSD", Side::Sell, OrderType::Limit, 100, 10, 10, 0}, tradeId);
    book.addOrder({2, "BTCUSD", Side::Sell, OrderType::Limit, 101, 15, 15, 0}, tradeId);

    // Market Buy of 20
    // Should match 10 @ 100, and 10 @ 101. The remaining market order quantity is discarded.
    auto trades = book.addOrder({3, "BTCUSD", Side::Buy, OrderType::Market, 0, 20, 20, 0}, tradeId);
    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].quantity, 10);
    EXPECT_EQ(trades[1].price, 101);
    EXPECT_EQ(trades[1].quantity, 10);

    // Verify remaining ask at 101 is 5
    const auto& asks = book.getAsks();
    EXPECT_EQ(asks.begin()->first, 101);
    EXPECT_EQ(asks.begin()->second.front().remainingQuantity, 5);
}
