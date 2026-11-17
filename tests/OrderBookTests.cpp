#include <gtest/gtest.h>
#include "orderbook/OrderBook.hpp"

TEST(OrderBook, EmptyBookHasNoPrices)
{
    OrderBook book;
    EXPECT_FALSE(book.BestBid().has_value());
    EXPECT_FALSE(book.BestAsk().has_value());
    EXPECT_EQ(book.VolumeAt(Side::Buy, 15000), 0u);
}