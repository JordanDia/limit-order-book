# Limit Order Book

A limit order book and matching engine in C++20, tested with GoogleTest.

An exchange keeps one order book per stock: every buy order (bid) and sell order (ask) that hasn't traded yet, sorted by price. When a new order comes in, the matching engine checks whether it crosses the other side of the book. If it does, trades happen; if anything is left over, a limit order rests in the book and waits.

Matching follows **price-time priority**: the best price trades first, and among orders at the same price, the one that arrived first trades first.

## Features

- **Limit orders**: trade against the opposite side while the price crosses, then rest the remainder.
- **Market orders**: trade against the opposite side with no price limit. Anything left once that side is empty is discarded, never rested.
- **Cancel** a resting order by id.
- **Queries**: best bid, best ask, total volume at a price, and lookup of a resting order by id.
- **Validation**: a quantity of 0 or a limit price of 0 or below is rejected without using up an id.
- Trades always execute at the **resting** order's price.

Prices are integers in ticks (`15000` means $150.00), which avoids floating-point rounding.

## API

```cpp
AddResult AddLimit(Side side, Price price, Quantity qty);
AddResult AddMarket(Side side, Quantity qty);
bool Cancel(OrderId id);

std::optional<Price> BestBid() const;
std::optional<Price> BestAsk() const;
Quantity VolumeAt(Side side, Price price) const;
const Order* GetOrder(OrderId id) const;
```

`AddResult` holds the new order's id (`std::nullopt` if it was rejected) and the list of trades it caused.

## Design

```cpp
std::map<Price, std::list<Order>, std::greater<Price>> m_bids;  // highest price first
std::map<Price, std::list<Order>>                      m_asks;  // lowest price first
std::unordered_map<OrderId, OrderLocation>             m_orderIndex;
```

- **One `std::map` per side, keyed by price.** The comparators are opposite, so `begin()` is always the best price on both sides. Bids sort highest first, asks lowest first.
- **A `std::list<Order>` per price level** is the FIFO queue for time priority. New orders go on the back, and matching takes from the front.
- **An index from order id to its location** (side, price, and an iterator into the level's list). A `std::list` iterator stays valid when other elements are inserted or erased, so the index can point straight at an order and Cancel can remove it without searching the queue.
- **One matching loop.** `Match` is a template over the opposite book's map type, so the same code serves buys (walking the asks) and sells (walking the bids). A market order reuses it with a price that always crosses: `numeric_limits<Price>::max()` for a buy, `min()` for a sell. Trades use the resting order's price, so that sentinel never appears in a trade.

## Cost of each operation

P is the number of price levels on a side, F the number of fills an incoming order makes, and N the number of orders resting at one price.

| Operation | Cost | Why |
|---|---|---|
| `AddLimit` that rests without trading | O(log P) | Find or create the price level in the map; the list append and index insert are O(1) |
| `AddLimit` / `AddMarket` that trades | O(log P + F) | Each fill is O(1) at the front of the best level; removing an emptied level from the front of the map is amortized O(1) |
| `Cancel` | O(log P) | O(1) to find the order through the index and erase it from its list, plus a map lookup to find its level |
| `BestBid` / `BestAsk` | O(1) | `begin()` of the map |
| `VolumeAt` | O(log P + N) | Find the level, then sum its orders |
| `GetOrder` | O(1) average | Hash lookup in the index |

## Build and test

Requires CMake 3.20+ and a C++20 compiler. GoogleTest is downloaded automatically by CMake.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite covers resting orders, matching across levels, FIFO within a level, partial fills, cancels, market orders, validation, and a full multi-step scenario (`FullWorkedExample`) that checks every trade and the book after each step.

## Possible improvements

- Store the map iterator for each order's level in the index to make `Cancel` O(1).
- Keep a running total per level to make `VolumeAt` O(log P).
- Replace `std::list` nodes with a preallocated pool, and try a flat price-indexed array for the levels, to cut allocations and cache misses.
- Randomized tests against a simple reference implementation, and a throughput/latency benchmark.
