#include "orderbook/OrderBook.hpp"
#include <algorithm>
#include <limits>

AddResult OrderBook::AddLimit(Side side, Price price, Quantity qty)
{
    AddResult result { m_nextId, {} };
    
    Order order { m_nextId, side, price, qty };
    
    MatchIncoming(order, result.trades);

    if (order.quantity > 0)
    {
        auto& level = (side == Side::Buy) ? m_bids[price] : m_asks[price];
        level.push_back(order);
        m_orderIndex[m_nextId] = {side, price, std::prev(level.end()) };
    }
    
    m_nextId++;
    return result;
}

AddResult OrderBook::AddMarket(Side side, Quantity qty)
{
    AddResult result { m_nextId, {} };
    Price price = (side == Side::Buy) ? std::numeric_limits<Price>::max() : std::numeric_limits<Price>::min();
    Order order { m_nextId, side, price, qty };

    MatchIncoming(order, result.trades);

    m_nextId++;
    return result;
}

bool OrderBook::Cancel(OrderId id)
{
    auto found = m_orderIndex.find(id);
    if (found == m_orderIndex.end())
        return false;

    OrderLocation& loc = found->second;

    auto removeFrom = [&](auto& book)
    {
        auto levelIt = book.find(loc.price);
        auto& level = levelIt->second;

        level.erase(loc.it);

        if (level.empty())
            book.erase(levelIt);
    };

    if (loc.side == Side::Buy)
        removeFrom(m_bids);
    else
        removeFrom(m_asks);

    m_orderIndex.erase(found);
    return true;
}

std::optional<Price> OrderBook::BestBid() const
{
    auto beginIt = m_bids.begin();

    if (beginIt != m_bids.end())
        return beginIt->first;

    return std::nullopt;
}

std::optional<Price> OrderBook::BestAsk() const
{
    auto beginIt = m_asks.begin();

    if (beginIt != m_asks.end())
        return beginIt->first;

    return std::nullopt;
}

Quantity OrderBook::VolumeAt(Side side, Price price) const
{
    Quantity total = 0;
    if (side == Side::Buy)
    {
        auto level = m_bids.find(price);

        if (level == m_bids.end())
            return 0;

        for (const auto& order : level->second)
        {
            total += order.quantity;
        }
    }
    else
    {
        auto level = m_asks.find(price);

        if (level == m_asks.end())
            return 0;

        for (const auto& order : level->second)
        {
            total += order.quantity;
        }
    }
    return total;
}

const Order* OrderBook::GetOrder(OrderId id) const
{
    auto found = m_orderIndex.find(id);
    if (found == m_orderIndex.end())
        return nullptr;
        
    return &*found->second.it;
}

template <typename OppositeBook>
void OrderBook::Match(Order& incoming, OppositeBook& opposite, std::vector<Trade>& trades)
{
    while (incoming.quantity > 0 && !opposite.empty())
    {
        Price bestPrice = opposite.begin()->first;
        bool crosses = (incoming.side == Side::Buy) ? incoming.price >= bestPrice : incoming.price <= bestPrice;
        if (!crosses)
            break;

        auto& level = opposite.begin()->second;
        Order& resting = level.front();

        Quantity fill = std::min(incoming.quantity, resting.quantity);

        OrderId buyId = (incoming.side == Side::Buy) ? incoming.id : resting.id;
        OrderId sellId = (incoming.side == Side::Sell) ? incoming.id : resting.id;

        Trade trade { buyId, sellId, resting.price, fill };
        trades.push_back(trade);

        incoming.quantity -= fill;
        resting.quantity -= fill;

        if (resting.quantity == 0)
        {
            m_orderIndex.erase(resting.id);
            level.pop_front();
        }
        
        if (level.empty())
            opposite.erase(opposite.begin());
    }
}

void OrderBook::MatchIncoming(Order& incoming, std::vector<Trade>& trades)
{
    if (incoming.side == Side::Buy)
        Match(incoming, m_asks, trades);
    else
        Match(incoming, m_bids, trades);
}