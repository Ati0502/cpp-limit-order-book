#include <gtest/gtest.h>
#include "MatchingEngine.hpp"

TEST(MatchingEngineTests, ValidationTest) {
    MatchingEngine engine;

    // Invalid Quantity
    auto res1 = engine.submitOrder({0, "BTCUSD", Side::Buy, OrderType::Limit, 100, 0, 0, 0});
    EXPECT_FALSE(res1.success);
    EXPECT_EQ(res1.errorMessage, "Quantity must be positive.");

    // Invalid Price
    auto res2 = engine.submitOrder({0, "BTCUSD", Side::Buy, OrderType::Limit, -5, 10, 10, 0});
    EXPECT_FALSE(res2.success);
    EXPECT_EQ(res2.errorMessage, "Limit order price must be positive.");

    // Empty symbol
    auto res3 = engine.submitOrder({0, "", Side::Buy, OrderType::Limit, 100, 10, 10, 0});
    EXPECT_FALSE(res3.success);
    EXPECT_EQ(res3.errorMessage, "Symbol cannot be empty.");

    // Duplicate ID
    auto res4 = engine.submitOrder({42, "BTCUSD", Side::Buy, OrderType::Limit, 100, 10, 10, 0});
    EXPECT_TRUE(res4.success);
    EXPECT_EQ(res4.assignedId, 42);

    auto res5 = engine.submitOrder({42, "BTCUSD", Side::Buy, OrderType::Limit, 100, 10, 10, 0});
    EXPECT_FALSE(res5.success);
    EXPECT_EQ(res5.errorMessage, "Duplicate Order ID: 42");
}

TEST(MatchingEngineTests, RoutingAndMatchingSymbols) {
    MatchingEngine engine;

    // Put buy order in BTCUSD, sell order in ETHUSD at same price. They should NOT match.
    auto res1 = engine.submitOrder({0, "BTCUSD", Side::Buy, OrderType::Limit, 1000, 10, 10, 0});
    auto res2 = engine.submitOrder({0, "ETHUSD", Side::Sell, OrderType::Limit, 1000, 10, 10, 0});

    EXPECT_TRUE(res1.trades.empty());
    EXPECT_TRUE(res2.trades.empty());

    EXPECT_NE(engine.getOrderBook("BTCUSD"), nullptr);
    EXPECT_NE(engine.getOrderBook("ETHUSD"), nullptr);

    // Cross the BTCUSD order
    auto res3 = engine.submitOrder({0, "BTCUSD", Side::Sell, OrderType::Limit, 1000, 10, 10, 0});
    EXPECT_EQ(res3.trades.size(), 1);
    EXPECT_EQ(res3.trades[0].symbol, "BTCUSD");

    // Cross the ETHUSD order
    auto res4 = engine.submitOrder({0, "ETHUSD", Side::Buy, OrderType::Limit, 1000, 10, 10, 0});
    EXPECT_EQ(res4.trades.size(), 1);
    EXPECT_EQ(res4.trades[0].symbol, "ETHUSD");
}

TEST(MatchingEngineTests, OrderCancellationTest) {
    MatchingEngine engine;

    auto res1 = engine.submitOrder({100, "BTCUSD", Side::Buy, OrderType::Limit, 500, 10, 10, 0});
    EXPECT_TRUE(res1.success);

    // Cancel order
    EXPECT_TRUE(engine.cancelOrder(100));

    // Try to cancel again
    EXPECT_FALSE(engine.cancelOrder(100));
}

TEST(MatchingEngineTests, TradeHistoryTest) {
    MatchingEngine engine;

    engine.submitOrder({1, "BTCUSD", Side::Buy, OrderType::Limit, 100, 10, 10, 0});
    engine.submitOrder({2, "BTCUSD", Side::Sell, OrderType::Limit, 100, 10, 10, 0});

    const auto& history = engine.getTradeHistory();
    ASSERT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].buyOrderId, 1);
    EXPECT_EQ(history[0].sellOrderId, 2);
}
