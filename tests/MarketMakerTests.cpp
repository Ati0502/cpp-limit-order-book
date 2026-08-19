#include <gtest/gtest.h>
#include "MarketMaker.hpp"

TEST(MarketMakerTests, PortfolioTrackingLong) {
    Exchange exchange;
    MarketMaker mm(exchange, "BTCUSD", 100000.0);

    // Initial state
    EXPECT_DOUBLE_EQ(mm.getCash(), 100000.0);
    EXPECT_EQ(mm.getInventory(), 0);
    EXPECT_DOUBLE_EQ(mm.getRealizedPnL(), 0.0);

    // Process a Buy Trade: Buy 2 units at $100
    // This is as if the MM's order got filled.
    Trade trade1{1, "BTCUSD", 100, 2, 999, 888, 0};
    // Pretend the MM's buy order ID was 999
    // To do this, we can manually trigger updatePosition through processTrade
    // wait, processTrade checks if trade.buyOrderId == lastBuyOrderId.
    // In our test, we can use the MM public processTrade but we need to set lastBuyOrderId.
    // Since lastBuyOrderId is private, let's see how we can set it or test it.
    // Ah, updatePosition is private, but processTrade is public!
    // Since processTrade matches lastBuyOrderId or lastSellOrderId, we can check if we can simulate it.
    // Wait! Can we trigger updateQuotes to set lastBuyOrderId? Yes, or we can just test the public updateQuotes flow by actually placing orders and executing them!
    // That is a much better integration test!
    
    // Let's configure the exchange and run updateQuotes
    // Since the book is empty, updateQuotes will quote around defaultPrice = 100
    // Let's call updateQuotes with spread 10, quoteSize 5.
    // Mid price = 100.
    // Buy quote should be 100 - 5 = 95. Sell quote should be 100 + 5 = 105.
    mm.updateQuotes(5, 5, 100);

    OrderId buyId = mm.getLastBuyOrderId();
    OrderId sellId = mm.getLastSellOrderId();
    EXPECT_GT(buyId, 0);
    EXPECT_GT(sellId, 0);

    // Now, let's match the MM's buy quote by submitting a crossed Sell Order to the exchange!
    // Sell 3 units @ 95. This matches the MM's resting BUY quote of 5 units @ 95.
    Order crossSell{0, "BTCUSD", Side::Sell, OrderType::Limit, 95, 3, 3, 0};
    auto res = exchange.submitOrder(crossSell);
    
    // This submission will generate a trade where MM buy order matches.
    ASSERT_EQ(res.trades.size(), 1);
    
    // The MM should process the trade
    mm.processTrade(res.trades[0]);

    // Cash: initial 100000 - 3 * 95 = 99715
    EXPECT_DOUBLE_EQ(mm.getCash(), 99715.0);
    EXPECT_EQ(mm.getInventory(), 3);
    EXPECT_DOUBLE_EQ(mm.getRealizedPnL(), 0.0);
    // Unrealized PnL: midPrice is still 95 (last match) or let's check mid:
    // midPrice = 95 (best bid is 95, no asks yet because the sell ask was fully filled).
    // Let's query unrealized at mid=100: 3 * (100 - 95) = 15
    EXPECT_DOUBLE_EQ(mm.getUnrealizedPnL(100), 15.0);
    EXPECT_DOUBLE_EQ(mm.getTotalPnL(100), 15.0);
}

TEST(MarketMakerTests, PortfolioTrackingShortAndPnL) {
    Exchange exchange;
    MarketMaker mm(exchange, "BTCUSD", 100000.0);

    mm.updateQuotes(5, 5, 100); // Buy 95, Sell 105
    OrderId sellId = mm.getLastSellOrderId();

    // Match the MM's sell quote: Buy 5 units @ 105
    Order crossBuy{0, "BTCUSD", Side::Buy, OrderType::Limit, 105, 5, 5, 0};
    auto res = exchange.submitOrder(crossBuy);
    ASSERT_EQ(res.trades.size(), 1);

    mm.processTrade(res.trades[0]);

    // Cash: 100000 + 5 * 105 = 100525
    EXPECT_DOUBLE_EQ(mm.getCash(), 100525.0);
    EXPECT_EQ(mm.getInventory(), -5);
    EXPECT_DOUBLE_EQ(mm.getRealizedPnL(), 0.0);
    // Unrealized PnL at mid=100: abs(-5) * (105 - 100) = 25
    EXPECT_DOUBLE_EQ(mm.getUnrealizedPnL(100), 25.0);

    // Now let's cover the short position!
    // Update quotes. Bids is now empty (old MM bid cancelled, and new bid placed).
    // Let's execute a buy against the MM's new buy quote to close the short.
    // MM quotes again. Mid price is 105.
    // MM buy quote: 105 - 5 = 100.
    mm.updateQuotes(5, 5, 105);
    
    // Match MM's buy quote of 5 @ 100
    Order crossSell{0, "BTCUSD", Side::Sell, OrderType::Limit, 100, 5, 5, 0};
    auto res2 = exchange.submitOrder(crossSell);
    ASSERT_EQ(res2.trades.size(), 1);

    mm.processTrade(res2.trades[0]);

    // We covered 5 units at 100 (avgCost was 105)
    // Realized PnL: 5 * (105 - 100) = 25
    // Cash: 100525 - 5 * 100 = 100025. Cash includes initial 100000 + 25 realized.
    EXPECT_EQ(mm.getInventory(), 0);
    EXPECT_DOUBLE_EQ(mm.getRealizedPnL(), 25.0);
    EXPECT_DOUBLE_EQ(mm.getCash(), 100025.0);
    EXPECT_DOUBLE_EQ(mm.getTotalPnL(100), 25.0);
}
