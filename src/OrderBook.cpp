#include "orderbook/OrderBook.hpp"

AddResult OrderBook::AddLimit(Side side, Price price, Quantity qty)
{
    AddResult result { m_nextId, {} };
    
    auto& level = (side == Side::Buy) ? m_bids[price] : m_asks[price];
    
    Order order { m_nextId, side, price, qty };

    level.push_back(order);
    
    m_nextId++;
    return result;
}

AddResult OrderBook::AddMarket(Side side, Quantity qty)
{
    return AddResult();
}

bool OrderBook::Cancel(OrderId id)
{
    return false;
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
