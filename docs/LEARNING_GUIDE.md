# Learning Guide: C++ Limit Order Book & Matching Engine

Welcome to the C++ Limit Order Book & Matching Engine learning guide. This project is designed to help you understand the core concepts of electronic trading, exchange matching engines, and basic algorithmic trading through a simple, clean, and highly readable C++ implementation.

This guide is structured as a 2-day curriculum.

---

## Day 1: Core Exchange Concepts & Matching Algorithms

### 1. What is an Exchange?
An electronic financial exchange is a marketplace that brings together buyers and sellers of assets (e.g., stocks, cryptocurrencies, derivatives). The exchange's primary responsibility is to accept buy and sell orders, match them according to deterministic rules, and broadcast executed trades.

### 2. What is an Order?
An order is an instruction sent by a trader to buy or sell a specific quantity of a security at a specific price (or at the best available price). In our codebase, an order is represented by a simple struct (`Order`) containing its ID, the side (Buy/Sell), the type (Limit/Market), price, quantity, and sequence number.

### 3. What is a Bid? What is an Ask?
- **Bid**: An order to **buy** a security. It represents the highest price the buyer is willing to pay.
- **Ask** (or Offer): An order to **sell** a security. It represents the lowest price the seller is willing to accept.

### 4. What is a Limit Order?
A limit order is an order to buy or sell a security at a **specific price or better**.
- A buy limit order will only execute at the limit price or lower.
- A sell limit order will only execute at the limit price or higher.
If a limit order cannot be matched immediately upon arrival, it is added to the order book as a **resting order**.

### 5. What is a Market Order?
A market order is an order to buy or sell a security **immediately at the best available prices** currently resting in the order book.
- Market orders prioritize speed of execution over price.
- If a market order cannot be fully matched because the book is empty, the remaining quantity is discarded (not placed in the order book).

### 6. What is an Order Book?
An order book is a structured list of resting buy and sell orders, organized by price levels. It represents the outstanding supply (asks) and demand (bids) for a single trading symbol.

### 7. What is Price-Time Priority?
Matching engines use **Price-Time Priority (FIFO)** to determine which resting orders get filled first:
1. **Price Priority**: Bids at higher prices and asks at lower prices always execute before others.
2. **Time Priority**: When multiple orders exist at the same price level, the order that arrived first (earliest sequence number) executes first.

### 8. Why use `std::map`?
We use `std::map` to store price levels in the order book:
- Bids use `std::map<Price, ..., std::greater<Price>>` to keep the highest price level at the front.
- Asks use `std::map<Price, ..., std::less<Price>>` to keep the lowest price level at the front.
This makes finding the best bid and best ask an $O(1)$ operation (accessing `begin()`) and keeps the book sorted in logarithmic time $O(\log N)$ on insertion.

### 9. Why use `std::deque`?
At each price level, we use a `std::deque<Order>` because:
- Orders at the same price must maintain FIFO priority (time priority).
- `std::deque` allows efficient $O(1)$ push to the back (when adding new resting orders) and pop from the front (when fully matching resting orders).

### 10. How does Matching Work?
When a new order arrives, the engine compares it to the best resting order on the opposite side of the book. 
- If prices cross (Buy Limit Price $\ge$ Best Ask Price, or Sell Limit Price $\le$ Best Bid Price), a trade is executed at the resting order's price.
- The engine continues matching down the book until the new order is fully filled or prices no longer cross.

### 11. What is a Partial Fill?
If an incoming order's quantity is larger than the best opposite resting order, the resting order is fully executed, and the incoming order is **partially filled**. The engine continues matching the remaining quantity against the next available resting orders. If quantity still remains, it rests in the book (for limit orders) or is discarded (for market orders).

---

## Day 2: Advanced Features, Simulation & Performance

### 12. How does Cancellation Work?
Traders can cancel outstanding resting orders. In our engine:
- We maintain an `orderLookup` index (`std::unordered_map<OrderId, OrderLocation>`).
- This tells us the price level and side of the order in $O(1)$ time.
- We then navigate to the correct deque and erase the order. If the deque becomes empty, we remove the price level.

### 13. How does the Exchange manage Multiple Symbols?
The `MatchingEngine` class holds a map of symbols to `OrderBook` instances:
- `std::unordered_map<std::string, OrderBook> books;`
- Orders are validated and routed to their respective books using the symbol name. Orders for different symbols never match against each other.

### 14. How does the Simulator generate orders?
The `OrderGenerator` uses a pseudo-random number generator (`std::mt19937`) with a deterministic seed to generate realistic sequences of orders. This allows reproducible benchmarks and realistic simulation of order flows.

### 15. How does the Market Maker work?
A market maker (MM) provides liquidity by quoting both buy and sell limit orders around the mid-price:
1. It calculates the **mid-price** between the best bid and best ask.
2. It cancels its previous quotes.
3. It places a new BUY quote below the mid-price and a new SELL quote above the mid-price.
4. It captures the spread (the difference between its buy and sell prices) when other traders match against its quotes.

### 16. How is PnL calculated?
The market maker tracks:
- **Cash**: The running ledger balance updated on executions.
- **Inventory**: The number of units owned (long position is positive, short position is negative).
- **Average Cost** (`avgCost`): The average price at which the current inventory was acquired.
- **Realized PnL**: Profit/loss locked in when closing out long or short positions.
- **Unrealized PnL**: Profit/loss based on marking the current inventory to the current market mid-price:
  - Long: $\text{Inventory} \times (\text{Mid Price} - \text{Average Cost})$
  - Short: $\text{Abs(Inventory)} \times (\text{Average Cost} - \text{Mid Price})$
- **Total PnL**: $\text{Realized PnL} + \text{Unrealized PnL}$.

### 17. How does Benchmarking work?
We measure the throughput and latency of the matching engine using `std::chrono::steady_clock`. The benchmark submits 10,000, 100,000, and 1,000,000 orders in Release mode and records the average, P95, and P99 latency per order.

### 18. What is Latency? What is Throughput?
- **Latency**: The time taken to process a single order submission (measured in microseconds or nanoseconds).
- **Throughput**: The number of orders the engine can process per second.

### 19. What could be Optimized?
If you want to scale this engine further:
1. **Memory Allocations**: Avoid copying `Order` objects and use custom block allocators or `std::pmr` to avoid frequent heap allocations.
2. **Flat Data Structures**: Standard `std::map` uses node-based red-black trees, which can cause cache misses. A flat vector or contiguous price ladder improves cache locality.
3. **Optimized Lookups**: For cancellations, traversing a `std::deque` is $O(N)$. Storing iterators or using a doubly-linked list of order nodes at each price level allows $O(1)$ cancellation.
