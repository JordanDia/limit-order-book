#pragma once
#include <cstdint>
#include <optional>
#include <map>
#include <list>
#include <vector>
#include <functional>
#include <unordered_map>

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

struct AddResult
{
    std::optional<OrderId> id;
    std::vector<Trade> trades;
};

struct Order
{
    OrderId id;
    Side side;
    Price price;
    Quantity quantity;
};

struct OrderLocation
{
    Side side;
    Price price;
    std::list<Order>::iterator it;
};

class OrderBook {
public:
    AddResult AddLimit(Side side, Price price, Quantity qty);
    AddResult AddMarket(Side side, Quantity qty);
    bool Cancel(OrderId id);

    std::optional<Price> BestBid() const;
    std::optional<Price> BestAsk() const;
    Quantity VolumeAt(Side side, Price price) const;
    const Order* GetOrder(OrderId id) const;

private:
    template<typename OppositeBook>
    void Match(Order& incoming, OppositeBook& opposite, std::vector<Trade>& trades);

private:
    OrderId m_nextId = 1;
    std::map<Price, std::list<Order>, std::greater<Price>> m_bids;
    std::map<Price, std::list<Order>> m_asks;

    std::unordered_map<OrderId, OrderLocation> m_orderIndex;
};