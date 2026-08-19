# C++ Limit Order Book & Matching Engine

A C++ trading-system simulator that implements an in-memory limit order book and matching engine using price-time priority. The project supports limit and market orders, partial fills, order cancellation, multiple trading symbols, trade history, random order simulation, market-making simulation, and performance benchmarking.

## Overview

This project simulates the core order-matching component of a simplified electronic exchange.

Orders are submitted to the matching engine and routed to the appropriate order book. Buy and sell orders are matched according to **price-time priority**, and every successful match generates a trade.

The project is designed to explore the data structures and algorithms behind trading systems while keeping the implementation simple and easy to understand.

## Features

* Limit BUY and SELL orders
* Market BUY and SELL orders
* Price-time priority matching
* Partial order fills
* Multiple price levels
* Order cancellation
* Multiple trading symbols
* Executed trade history
* Interactive command-line interface
* Random order-flow simulation
* Configurable simulation parameters
* Basic market-making strategy
* Trading statistics
* PnL tracking
* Performance benchmarking
* CMake-based build system
* Unit testing

## Architecture

```text
                    Trading Simulator
                           |
                           v
                    Matching Engine
                           |
              +------------+------------+
              |                         |
              v                         v
        BTCUSD Order Book         ETHUSD Order Book
              |                         |
              +------------+------------+
                           |
                           v
                    Order Matching
                           |
                           v
                         Trades
                           |
                +----------+----------+
                |                     |
                v                     v
           Trade History         Statistics / PnL
```

The system separates the core matching logic from the command-line interface and simulation components.

## How Order Matching Works

The order book maintains two sides:

```text
Bids                          Asks

Highest price first            Lowest price first
     |                              |
     v                              v
  BUY orders                   SELL orders
```

For example:

```text
BUY  100 units @ 100
SELL 40  units @ 99
```

Since the BUY price is greater than or equal to the SELL price, the orders can match.

The resulting trade is:

```text
Price:    99
Quantity: 40
```

The remaining BUY order is:

```text
60 units @ 100
```

The matching engine continues matching while the incoming order still has remaining quantity and a valid opposing order is available.

## Price-Time Priority

The matching engine follows **price-time priority**.

For BUY orders:

> Higher price gets priority.

For SELL orders:

> Lower price gets priority.

If two orders have the same price:

> The order submitted earlier gets priority.

For example:

```text
BUY  10 @ 100
BUY  20 @ 100
BUY  15 @ 101
```

The `101` order has priority over both `100` orders.

Among the two `100` orders, the first submitted order is matched first.

## Data Structures

The order book uses ordered price levels with FIFO queues of orders.

Conceptually:

```text
Bids
  |
  +-- Price 101 -> Orders
  +-- Price 100 -> Orders
  +-- Price  99 -> Orders

Asks
  |
  +-- Price 102 -> Orders
  +-- Price 103 -> Orders
  +-- Price 104 -> Orders
```

Ordered price levels allow the matching engine to efficiently access the best available bid or ask, while FIFO ordering maintains time priority between orders at the same price.

Prices and quantities are represented using integer types rather than floating-point values to avoid precision problems during matching.

## Supported Order Types

### Limit Order

A limit order specifies the maximum price a buyer is willing to pay or the minimum price a seller is willing to accept.

Example:

```text
BUY BTCUSD LIMIT 100000 5
```

This means:

```text
Symbol:   BTCUSD
Side:     BUY
Type:     LIMIT
Price:    100000
Quantity: 5
```

### Market Order

A market order executes against the best available orders on the opposite side of the book.

For example:

```text
BUY BTCUSD MARKET 10
```

will consume available sell orders starting from the lowest ask.

## Partial Fills

Partial fills are supported.

Example:

```text
BUY  100 @ 100
SELL 40  @ 100
```

After matching:

```text
Trade:
40 @ 100

Remaining:
BUY 60 @ 100
```

An order can also be matched against multiple price levels until it is completely filled or no suitable opposing orders remain.

## Order Cancellation

Resting orders can be cancelled using their order ID.

Cancellation removes the remaining quantity from the order book without affecting already executed trades.

## Multiple Instruments

The simulator supports independent order books for multiple symbols.

Example:

```text
AAPL
BTCUSD
ETHUSD
```

Each symbol has its own bid and ask books, and orders from different symbols cannot match with each other.

## Interactive CLI

The application provides an interactive command-line interface:

```text
===== TRADING SIMULATOR =====
1. Add Order (Manual Command)
2. Cancel Order
3. Show Order Book
4. Show Executed Trades (History)
5. Show Statistics
6. Run Random Order Simulation
7. Run Market-Making Strategy
8. Exit
```

The application can display the current order book, executed trades, statistics, and simulation results.

## Random Order Simulation

The simulator can generate configurable synthetic order flow.

Parameters include:

* Number of orders
* BUY/SELL ratio
* LIMIT/MARKET ratio
* Minimum price
* Maximum price
* Random seed
* Verbose submission logging

Example:

```text
Orders:       100000
BUY/SELL:     50/50
LIMIT/MARKET: 80/20
Seed:         42
```

A deterministic seed can be used to reproduce a simulation.

## Market-Making Simulation

The project includes a simple market-making strategy for educational purposes.

The strategy observes the current market and places buy and sell quotes around the estimated mid-price.

Example:

```text
Best Bid:  9995
Best Ask:  10005

Mid Price: 10000

Market Maker:
BUY  @ 9998
SELL @ 10002
```

The simulator can track inventory, trades and PnL generated by the strategy.

This is a simplified simulation and is **not intended to represent a production trading strategy or guarantee profitability**.

## Performance Benchmarking

The project includes performance measurement for the matching engine and simulation.

The benchmark measures metrics such as:

* Orders processed
* Trades executed
* Total execution time
* Throughput

Example output:

```text
Simulation completed in 189 ms.
Orders processed: 108
New trades executed: 83
Volume traded: 1064
Throughput: 571.429 orders/sec
```

Benchmark results depend on the machine, compiler, build configuration and workload, so results should not be interpreted as general hardware-independent performance claims.

## Complexity

For an order book implemented using ordered price levels:

* Finding/inserting a price level: approximately `O(log P)`
* Accessing the best price level: approximately `O(1)` through the ordered container's boundary
* Processing matching: depends on the number of orders/price levels consumed
* Order cancellation: depends on the lookup structure and removal strategy

Here `P` represents the number of distinct price levels.

## Project Structure

```text
cpp-limit-order-book/
│
├── CMakeLists.txt
├── README.md
├── .gitignore
│
├── include/
│   ├── Types.hpp
│   ├── Order.hpp
│   ├── Trade.hpp
│   ├── OrderBook.hpp
│   ├── MatchingEngine.hpp
│   ├── Exchange.hpp
│   ├── OrderGenerator.hpp
│   └── MarketMaker.hpp
│
├── src/
│   ├── OrderBook.cpp
│   ├── MatchingEngine.cpp
│   ├── Exchange.cpp
│   ├── OrderGenerator.cpp
│   └── MarketMaker.cpp
│
├── tests/
│
├── benchmarks/
│
├── docs/
│
└── main.cpp
```

The exact directory contents may vary slightly depending on the current implementation.

## Requirements

* C++20 compatible compiler
* CMake
* Git
* Windows, Linux, or another supported development environment

## Build

Clone the repository:

```bash
git clone https://github.com/Ati0502/cpp-limit-order-book.git
cd cpp-limit-order-book
```

Create the build directory:

```bash
cmake -S . -B build
```

Build the project:

```bash
cmake --build build
```

For a Release build:

```bash
cmake --build build --config Release
```

## Run

After building, run the generated trading simulator executable.

On Windows, the executable may be located under:

```text
build/
```

or a configuration-specific directory such as:

```text
build/Debug/
build/Release/
```

The exact location depends on the CMake generator being used.

## Testing

If tests are configured with CTest:

```bash
ctest --test-dir build
```

Individual tests can also be built and executed according to the generated CMake configuration.

## Example Workflow

A typical simulation can follow this flow:

```text
1. Start the simulator
        |
2. Select a symbol
        |
3. Submit BUY/SELL orders
        |
4. Matching engine checks the opposite book
        |
5. Compatible orders are matched
        |
6. Trades are generated
        |
7. Remaining quantities stay in the book
        |
8. View order book / trades / statistics
```

## Design Decisions

### Why an ordered price structure?

The matching engine needs to quickly identify the best available bid and ask. Keeping price levels ordered makes this natural.

### Why FIFO queues?

Orders at the same price follow time priority. A queue naturally represents this behavior.

### Why integer prices?

Floating-point arithmetic can introduce precision issues. Integer-based price representation keeps matching deterministic.

### Why start with a single-threaded matching engine?

A single-threaded core makes order processing deterministic and easier to reason about. It also provides a useful baseline before considering concurrency or further performance optimization.

## Future Improvements

Possible extensions include:

* More advanced order types
* Persistent order/trade storage
* Web-based trading dashboard
* Network-based market-data simulation
* More sophisticated market-making strategies
* Advanced performance profiling
* Concurrency experiments
* Alternative order-book data structures
* More detailed latency analysis

## Disclaimer

This project is an educational trading-system simulator.

It does not connect to real exchanges, does not execute real trades, and does not represent production-grade HFT infrastructure.

The purpose of the project is to explore:

* C++ programming
* Data structures
* Order matching
* Trading-system concepts
* Simulation
* Performance measurement

## Author

**Atish Paul**

Built as a C++ systems/trading project to explore the implementation of a simplified electronic exchange and limit order book.
