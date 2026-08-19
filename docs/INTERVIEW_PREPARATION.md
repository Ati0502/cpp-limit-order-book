# Interview Preparation Guide

This guide is designed to help university students prepare for software engineering interviews at quantitative trading firms (e.g., Jane Street, Optiver, Citadel, Teesta Investment) using this project as a talking point.

---

## Pitching the Project

### 20-Second Pitch (Elevator Pitch)
> "I built a high-performance C++20 limit order book and matching engine that processes over 600,000 orders per second with an average latency of 1.5 microseconds. It implements price-time priority, handles limit and market orders, supports partial fills, multiple instruments, order cancellation, and includes a mock market-making bot with inventory-based PnL tracking."

### 1-Minute Pitch
> "I designed and built a simulated trading exchange in C++20. The system uses a sorted map of deques to implement strict price-time priority for limit and market orders, ensuring FIFO execution. To support fast cancellations, I implemented a hash-map lookup index to locate resting orders in $O(1)$ time. 
> 
> The project compiles out-of-the-box using CMake and has comprehensive unit test coverage with GoogleTest. I also built a simulation framework that generates realistic randomized order flows, and a mock market-making bot that quotes spreads, manages inventory risk, and calculates realized/unrealized PnL. In Release mode, the single-threaded matching loop achieves sub-microsecond latencies."

### 5-Minute Pitch
> "My project is an educational electronic exchange written in modern C++. It contains several components:
> 
> 1. **Data Structures**: The core `OrderBook` uses `std::map` to store price levels (descending for bids, ascending for asks). At each price level, orders are stored in `std::deque` to guarantee FIFO priority. I used integer-based prices (`long long`) to eliminate floating-point precision issues.
> 2. **Matching Engine**: Routes orders to symbol-specific books, manages trade and order ID generation, validates submissions, and keeps a transaction history.
> 3. **Order Cancellation**: Supported in $O(1)$ lookup time using an auxiliary `unordered_map` that maps order IDs to their price level and book side, followed by an $O(N)$ removal from the deque.
> 4. **Trading Simulation & Bot**: I wrote an `OrderGenerator` that produces deterministic randomized streams of orders. I also created a `MarketMaker` bot that runs update cycles quoting bids/asks around the mid-price, processes trades to adjust inventory, maintains position average cost, and tracks realized and unrealized PnL.
> 5. **Benchmarking**: I compiled the engine in Release mode and ran throughput tests. For 1 million orders, the engine achieves a throughput of over 600k orders/second with a P99 latency of 4.8 microseconds.
> 
> This project shows that I understand C++ memory layout, STL container selection, basic algorithmic trading logic, and the importance of benchmarking and clean OOP design."

---

## 25 Interview Questions & Answers

### 1. Why did you choose `std::map` for prices and `std::deque` for order queues?
- **Simple Answer**: `std::map` keeps prices sorted automatically so we can easily find the best bid/ask in $O(1)$ time. `std::deque` is used because orders at the same price must be matched FIFO, and `std::deque` supports $O(1)$ push to the back and pop from the front.
- **Deep Answer**: `std::map` is a node-based red-black tree. Insertions and deletions take $O(\log L)$ where $L$ is the number of price levels. While this keeps prices sorted, it has poor cache locality. `std::deque` is a chunk-based container that avoids the vector reallocation copy-cost and offers better cache performance than a linked list (`std::list`), though elements are still scattered in memory blocks.

### 2. Why did you use integer types for prices instead of double/float?
- **Simple Answer**: Floating-point types have precision errors in binary representations (e.g., $0.1 + 0.2 \ne 0.3$), which would cause comparison bugs when matching orders.
- **Deep Answer**: IEEE 754 floating-point numbers cannot represent base-10 fractions exactly. In trading, price crossing rules must be exact. Representing prices as integers in "ticks" or cents (fixed-point arithmetic) ensures exact comparisons and is the industry standard for matching engine cores.

### 3. What is price-time priority?
- **Simple Answer**: The order with the best price (highest bid or lowest ask) is matched first. If multiple orders have the same price, the order that arrived first (earliest time/sequence) is matched first.
- **Deep Answer**: Price-time priority is the standard queue model for electronic exchanges. Price is the first key; arrival sequence is the second key. This encourages price discovery and early liquidity provisioning.

### 4. How does a market order differ from a limit order in your engine?
- **Simple Answer**: Limit orders have a maximum/minimum execution price and rest in the book if unfilled. Market orders execute immediately at the best available prices, and any unfilled quantity is discarded.
- **Deep Answer**: Market orders do not post to the order book. In `matchOrder`, market orders skip the crossing price check and match directly against any resting liquidity. If the opposite side is empty, the market order's remaining quantity is immediately cancelled.

### 5. How does order cancellation work, and what is its time complexity?
- **Simple Answer**: We use a hash map to find the order's price and side in $O(1)$ time, then search the corresponding deque and remove it.
- **Deep Answer**: The `orderLookup` is a `std::unordered_map<OrderId, OrderLocation>`. Finding the price level and side is $O(1)$. Erasing the order from the `std::deque` requires a linear search inside the deque, which is $O(N)$ where $N$ is the number of orders resting at that specific price level. In practice, deques at a single price level are small, so this is very fast.

### 6. Why not use `std::vector` for order queues?
- **Simple Answer**: Removing an order from the front of a `std::vector` is an $O(M)$ operation because all other elements must shift left.
- **Deep Answer**: `std::vector` stores elements contiguously. While this has excellent cache locality, calling `erase(vector.begin())` is linear in the number of elements in the vector because it requires moving all remaining elements. `std::deque` is a double-ended queue that supports $O(1)$ removals from the front without shifting elements.

### 7. Why not use `std::unordered_map` for price levels?
- **Simple Answer**: Hash maps are unsorted. Finding the best bid or ask would require iterating through all keys, taking $O(L)$ time instead of $O(1)$.
- **Deep Answer**: Although hash maps provide $O(1)$ insertions and lookups, a matching engine constantly needs to know the minimum and maximum price keys to check for crosses. Iterating the map to find the minimum/maximum is $O(L)$ where $L$ is the number of active price levels, which degrades matching throughput.

### 8. What is the time complexity of adding a limit order?
- **Simple Answer**: Logarithmic $O(\log L)$ if it doesn't match and needs to be inserted into the map.
- **Deep Answer**: If the order crosses and matches $K$ resting orders, matching is $O(K)$. If any quantity remains and it must rest, inserting it into the `std::map` takes $O(\log L)$ where $L$ is the number of price levels. Storing the lookup in `std::unordered_map` is $O(1)$ on average.

### 9. How would you optimize the cancellation performance from $O(N)$?
- **Simple Answer**: We can store iterator positions or pointers directly, or use a doubly-linked list for the order queue.
- **Deep Answer**: Instead of storing orders directly in a deque, we can store orders in a doubly-linked list (`std::list`). The lookup hash map can store the list's iterator. Erasing a node in a doubly-linked list using an iterator is $O(1)$, which eliminates the linear search.

### 10. Why is the core matching engine single-threaded?
- **Simple Answer**: It keeps order matching deterministic, avoids race conditions, and prevents lock contention which increases latency.
- **Deep Answer**: Matching is a sequence-dependent process. If multiple threads were matching concurrently on the same book, you would need complex locks (like spinlocks or mutexes). Thread synchronization overhead and context switches often exceed the execution time of a single-threaded matching loop.

### 11. How would you scale this to handle millions of symbols?
- **Simple Answer**: We can shard the symbols across multiple matching engine threads.
- **Deep Answer**: Since orders for `BTCUSD` never match against `ETHUSD`, we can run a thread pool where each thread manages a subset of symbols. This allows parallel processing across CPU cores without requiring locks within the matching logic.

### 12. What is the difference between realized and unrealized PnL?
- **Simple Answer**: Realized PnL is the profit or loss locked in when a position is closed. Unrealized PnL is the paper profit or loss based on current market prices.
- **Deep Answer**: Realized PnL represents cash changes from completed round-trips. Unrealized PnL represents the current valuation of inventory marked-to-market against the mid-price relative to its acquisition average cost.

### 13. How does your market maker calculate average cost?
- **Simple Answer**: It uses a running average. If we buy more while long, we compute the weighted average price. If we sell to close, the average cost stays the same.
- **Deep Answer**: It tracks long and short positions separately. If we are long $I$ units at average cost $C$ and buy $Q$ more at price $P$, the new cost is $(I \times C + Q \times P) / (I + Q)$. If we sell to close part of the long, the average cost is unchanged, and we lock in realized PnL.

### 14. What is latency, and how did you measure it?
- **Simple Answer**: Latency is the time it takes to process one order. I measured it using `std::chrono::steady_clock`.
- **Deep Answer**: Latency is the round-trip time of a submission. I captured the steady clock time before and after `submitOrder` and recorded the elapsed nanoseconds. I then sorted the latencies to extract the median, P95, and P99 tail latencies.

### 15. What are P95 and P99 latencies, and why do they matter more than average latency?
- **Simple Answer**: P95 and P99 represent the tail latencies—meaning 95% and 99% of orders were processed faster than this time. They matter because trading systems must have consistent, predictable speeds.
- **Deep Answer**: Average latency can hide temporary spikes (caused by GC, cache misses, page faults, or context switches). In high-frequency trading, a single slow order (the "tail") can result in executing at stale prices and losing money. Tail latency profiling helps identify these systemic issues.

### 16. Why did you use `std::chrono::steady_clock` instead of `system_clock`?
- **Simple Answer**: `system_clock` represents wall-clock time and can jump backward if NTP sync occurs. `steady_clock` is monotonic and only goes forward, making it perfect for intervals.
- **Deep Answer**: `system_clock` is susceptible to system time adjustments, timezone changes, and leap seconds. `steady_clock` is a monotonic clock that measures elapsed ticks from a fixed starting point, ensuring that time differences are always positive and accurate.

### 17. How does logging affect matching engine latency?
- **Simple Answer**: Disk and console I/O are very slow and can increase latency from microseconds to milliseconds.
- **Deep Answer**: Writing to `std::cout` or writing logs to disk involves system calls, kernel-space transitions, and blocking operations. In our benchmark, we disabled console output during the matching loop to measure the true CPU speed of the engine. Production engines use asynchronous ring-buffer loggers to offload I/O from the critical path.

### 18. What is cache locality, and how does it relate to your project?
- **Simple Answer**: Cache locality means storing data close together in memory so the CPU can load it into its fast cache memory.
- **Deep Answer**: The CPU fetches memory in 64-byte cache lines. A `std::vector` or `std::deque` stores data contiguously, which has good cache locality. `std::map` stores nodes on the heap, which causes pointer-chasing and CPU cache misses. Using contiguous array structures instead of trees dramatically speeds up code execution.

### 19. What is a "cross" in an order book?
- **Simple Answer**: When a buy price is greater than or equal to the best sell price.
- **Deep Answer**: A cross represents a mismatch in supply and demand where a buyer is willing to pay more than what a seller is asking. The matching engine resolves this immediately by executing a trade and clearing the crossed orders.

### 20. Why do you execute trades at the resting order's price instead of the incoming order's price?
- **Simple Answer**: The resting order was there first, so it set the price. The incoming order is the aggressor and accepts the resting price.
- **Deep Answer**: Executing at the passive (resting) order's price is the standard rule on double-auction exchanges. It rewards liquidity providers by giving them their quoted price and charges liquidity takers the spread.

### 21. If multiple threads were to submit orders, how would you synchronize access?
- **Simple Answer**: You could use a mutex lock, but a better way is to use an asynchronous lock-free queue to serialize orders to a single thread.
- **Deep Answer**: Using a `std::mutex` around the `submitOrder` method would protect the structures from race conditions, but thread contention would cause thread blockages and high latency. A better architecture is the Disruptor pattern: threads write orders to a lock-free ring buffer, and a single dedicated CPU thread reads from the buffer and matches orders sequentially.

### 22. What happens if a market order cannot be fully filled?
- **Simple Answer**: The matched portion is executed, and the remaining quantity is cancelled.
- **Deep Answer**: In our simulator, the unmatched balance of a market order is simply discarded because market orders represent immediate liquidity takers and should not rest in the book.

### 23. What could cause a memory leak in your matching engine?
- **Simple Answer**: Using raw pointers and forgetting to `delete` them. I used stack-allocated objects and standard container values (RAII) to avoid this.
- **Deep Answer**: By relying on modern C++ RAII principles—using standard containers like `std::map` and `std::deque` and copying values rather than dynamic allocations (`new`/`delete`)—memory is automatically reclaimed when orders are removed from the books or when containers go out of scope.

### 24. What compiler flags are critical when compiling trading systems?
- **Simple Answer**: Optimization flags like `-O3` and target architecture flags like `-march=native`.
- **Deep Answer**: Compiling with `-O3` enables aggressive loop unrolling, vectorization, and inlining. `-march=native` allows the compiler to use instruction sets specific to the host CPU (like AVX2/AVX-512), which can make a huge difference in mathematical computations and data moves.

### 25. How would you make this system production-ready?
- **Simple Answer**: Add networking (TCP/FIX protocol), persistent database logging for recovery, and optimize memory allocations.
- **Deep Answer**:
  1. **Protocol Ingestion**: Implement a FIX (Financial Information eXchange) or binary parser (like SBE) using Epoll/IOCP to handle TCP connections.
  2. **Persistence**: Use a fast write-ahead log (WAL) on SSD to persist orders before matching so the system can recover from crashes.
  3. **Zero-Copy Memory**: Pre-allocate all memory buffers and use lock-free circular queues.
  4. **Hardware Acceleration**: Use Kernel Bypass (Solarflare Onload) to minimize network latency, or move the matching engine onto FPGA (Verilog/VHDL) for sub-nanosecond matching.
