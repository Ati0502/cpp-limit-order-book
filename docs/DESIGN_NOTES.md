# Design Notes

This document captures the architectural decisions and design choices made in this trading engine project, explaining the trade-offs between simplicity and system realism.

---

### 1. Why `std::map` for Price Levels?
Inside `OrderBook`, buy and sell price levels are managed using `std::map`.
- **Reason**: Bids must be sorted descending, and asks must be sorted ascending. `std::map` is a self-balancing binary search tree (usually a Red-Black tree) that keeps key elements sorted.
- **Complexity**:
  - Insertion of a new price level: $O(\log L)$, where $L$ is the number of active price levels.
  - Finding the best bid/ask: $O(1)$ (using `.begin()`).
- **Alternative**: A hash map (`std::unordered_map`) has $O(1)$ insertion but does not keep prices sorted. Sorting a hash map on every quote update would be too slow ($O(L \log L)$).

### 2. Why `std::deque` for Orders?
At each price level, we store orders in a `std::deque<Order>`.
- **Reason**: Standard price-time priority dictates that orders at the same price must be matched first-in, first-out (FIFO).
- **Complexity**:
  - Appending a new order: $O(1)$ (using `push_back`).
  - Removing a fully filled order from the front: $O(1)$ (using `pop_front`).
- **Alternative**: `std::vector` has $O(M)$ overhead to remove elements from the front (since it must shift all subsequent elements). Storing a doubly-linked list (`std::list`) is also possible, but `std::deque` has better cache locality because it allocates memory in chunks rather than individual nodes.

### 3. Why Integer Prices (`long long`)?
The engine uses `long long` for prices instead of `double` or `float`.
- **Reason**: Floating-point numbers cannot represent decimal values precisely in binary. For example, `0.1 + 0.2` might equal `0.30000000000000004`, leading to comparison bugs (e.g., `0.30000000000000004 > 0.3` which is true, preventing a matching cross).
- **Real-World Practice**: Standard exchanges use integer-based representation, expressing prices in "ticks" or cents (e.g., $100.00 is represented as `10000` cents, where the multiplier is determined by the minimum tick size of the instrument).

### 4. Why Single-Threaded Core?
The matching engine is strictly single-threaded.
- **Reason**: Electronic matching is fundamentally serial. The sequence in which orders arrive determines who gets priority. Having multiple threads competing to modify the same order book requires complex locking/synchronization, which increases latency (due to lock contention) and introduces non-determinism.
- **Real-World Practice**: Most matching engines in production (such as LMAX Disruptor architecture) run a single thread per matching engine matching loop, using ring buffers or lock-free queues to feed orders to the matching thread.

### 5. Why Separate `OrderBook` and `MatchingEngine`?
- **OrderBook**: Focuses entirely on the matching logic and sorting queue of a single instrument (e.g., BTCUSD). It knows nothing about other instruments or order validation.
- **MatchingEngine**: Acts as the router and validator. It manages the collection of order books, generates unique IDs, tracks trade history, and enforces trading rules. This separation of concerns simplifies testing and keeps the code highly modular.
