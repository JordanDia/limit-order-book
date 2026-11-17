#include "orderbook/OrderBook.hpp"

AddResult OrderBook::AddLimit(Side side, Price price, Quantity qty)
{
    return AddResult();
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
    return std::optional<Price>();
}

std::optional<Price> OrderBook::BestAsk() const
{
    return std::optional<Price>();
}

Quantity OrderBook::VolumeAt(Side side, Price price) const
{
    return Quantity();
}
