# Code Walkthrough

This document guides you through the source files of the C++ Limit Order Book & Matching Engine, explaining their purpose, key functions, and how they connect.

---

## Directory Overview

- **`include/`**: Header files defining classes and data types.
- **`src/`**: Implementation files containing the business logic.
- **`tests/`**: Unit tests verifying correctness using GoogleTest.
- **`benchmarks/`**: Performance measurement script.
- **`main.cpp`**: Interactive CLI wrapper.

---

## File Walkthrough

### 1. [`Types.hpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/include/Types.hpp)
- **Purpose**: Defines system-wide aliases and enums.
- **Key Definitions**:
  - `OrderId`, `TradeId`, `Price`, `Quantity` as `long long` integers. Using integers avoids the floating-point inaccuracies associated with `float` or `double` (e.g., $0.1 + 0.2 \ne 0.3$).
  - `Side` enum (`Buy`, `Sell`).
  - `OrderType` enum (`Limit`, `Market`).

### 2. [`Order.hpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/include/Order.hpp) & [`Trade.hpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/include/Trade.hpp)
- **Purpose**: Simple data containers for orders and trades.
- **Key Concepts**:
  - `Order` tracks `originalQuantity` and `remainingQuantity`. The matching engine decrements `remainingQuantity` as matches occur.
  - `Trade` stores execution metadata (price, quantity, buyer/seller IDs, timestamp).

### 3. [`OrderBook.hpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/include/OrderBook.hpp) & [`OrderBook.cpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/src/OrderBook.cpp)
- **Purpose**: Manages buy and sell queues for a single trading symbol.
- **Key Functions**:
  - `addOrder(Order order, TradeId& nextTradeId)`: Coordinates the matching process. If any quantity remains after matching, limit orders are added to the book.
  - `matchOrder(...)`: Symmetrically matches buy orders against asks, and sell orders against bids. Performs crossing checks and generates `Trade` objects.
  - `cancelOrder(OrderId id)`: Locates the order, removes it from the book, and cleans up the price levels if they become empty.
  - `display()`: Generates a terminal-based representation of the book, showing bids, asks, best bid/ask, and the spread.

### 4. [`MatchingEngine.hpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/include/MatchingEngine.hpp) & [`MatchingEngine.cpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/src/MatchingEngine.cpp)
- **Purpose**: Manages multiple order books and handles validation and routing.
- **Key Functions**:
  - `submitOrder(Order order)`: Validates input (positive quantity, valid limit price), routes to the correct book, collects executed trades, and removes fully matched order IDs from active tracking.
  - `cancelOrder(OrderId id)`: Searches across books to cancel the order.
  - `validateOrder(...)`: Returns descriptive error messages for invalid submissions.

### 5. [`Exchange.hpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/include/Exchange.hpp) & [`Exchange.cpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/src/Exchange.cpp)
- **Purpose**: High-level system wrapper (Facade pattern) that encapsulates the `MatchingEngine`.

### 6. [`OrderGenerator.hpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/include/OrderGenerator.hpp) & [`OrderGenerator.cpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/src/OrderGenerator.cpp)
- **Purpose**: Generates reproducible streams of random orders using `std::mt19937` for testing.

### 7. [`MarketMaker.hpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/include/MarketMaker.hpp) & [`MarketMaker.cpp`](file:///c:/Users/atish/OneDrive/Desktop/c++%20order%20booking/src/MarketMaker.cpp)
- **Purpose**: Simulates a basic market-making bot, quoting limit orders around the mid-price and tracking position PnL.

---

## Order Flow Lifecycle Diagram

```text
User enters order via CLI
        ↓
    main.cpp (Parses command)
        ↓
    Exchange::submitOrder(order)
        ↓
    MatchingEngine::submitOrder(order)
        ├── Validates order parameters (validateOrder)
        └── Routes to symbol's OrderBook (getOrCreateOrderBook)
                 ↓
             OrderBook::addOrder(order)
                 ├── Matches order against opposite book (matchOrder)
                 │     ├── Generates Trades
                 │     └── Decrements remaining quantities
                 └── Rests remaining quantity in book if LIMIT
                         ↓
                     MatchingEngine collects Trades
                         ├── Adds to central tradeHistory
                         └── Cleans up filled Order IDs from active tracking
```
