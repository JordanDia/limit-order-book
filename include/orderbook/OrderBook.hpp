#pragma once
#include <cstdint>
#include <optional>
#include <vector>

using OrderId   = std::uint64_t;
using Price     = std::int64_t;
using Quantity  = std::uint32_t;

enum class Side { Buy, Sell };

struct Trade
{
    OrderId buyId;
    OrderId sellId;
    Price price;
    Quantity quantity;
};

struct AddResult {
    std::optional<OrderId> id;
    std::vector<Trade> trades;
};

class OrderBook {
public:
    AddResult AddLimit(Side side, Price price, Quantity qty);
    AddResult AddMarket(Side side, Quantity qty);
    bool Cancel(OrderId id);

    std::optional<Price> BestBid() const;
    std::optional<Price> BestAsk() const;
    Quantity VolumeAt(Side side, Price price) const;
};