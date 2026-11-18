#include <gtest/gtest.h>
#include "orderbook/OrderBook.hpp"

TEST(OrderBook, EmptyBookHasNoPrices)
{
    OrderBook book;
    EXPECT_FALSE(book.BestBid().has_value());
    EXPECT_FALSE(book.BestAsk().has_value());
    EXPECT_EQ(book.VolumeAt(Side::Buy, 15000), 0u);
}

TEST(OrderBook, OneBuyLimitRests)
{
    OrderBook book;
    AddResult result = book.AddLimit(Side::Buy, 15000, 1u);

    EXPECT_EQ(book.BestBid(), 15000);
    EXPECT_EQ(result.trades.size(), 0u);
    EXPECT_EQ(result.id, 1);

}

TEST(OrderBook, TwoBuysAtDifferentPrices)
{
    OrderBook book;
    book.AddLimit(Side::Buy, 14000, 1u);
    book.AddLimit(Side::Buy, 15000, 1u);

    EXPECT_EQ(book.BestBid(), 15000);

    book.AddLimit(Side::Sell, 17000, 1u);
    book.AddLimit(Side::Sell, 16000, 1u);

    EXPECT_EQ(book.BestAsk(), 16000);
}

TEST(OrderBook, VolumeAtEqualsTotalRestingShares)
{
    OrderBook book;
    book.AddLimit(Side::Buy, 15000, 100u);
    book.AddLimit(Side::Buy, 15000, 300u);

    EXPECT_EQ(book.VolumeAt(Side::Buy, 15000), 400u);
}

TEST(OrderBook, BuyBelowBestAskDoesNotTrade)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 16000, 100u);
    book.AddLimit(Side::Buy, 14000, 100u);

    EXPECT_EQ(book.VolumeAt(Side::Sell, 16000), 100);
    EXPECT_EQ(book.VolumeAt(Side::Buy, 14000), 100);
}

TEST(OrderBook, IdsKeepGoingUp)
{
    OrderBook book;

    OrderId currentId = 1;
    for (std::size_t i = 0; i < 10; i++)
    {
        Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
        Price price = 10000;
        if (side == Side::Sell)
            price = 12000;
        AddResult result = book.AddLimit(side, price, 100u);

        EXPECT_EQ(result.id, currentId);
        currentId++;
    }
}

TEST(OrderBook, VolumeAtEmptyLevelReturnsZeroOnNonEmptyBook)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 16000, 100u);

    EXPECT_EQ(book.VolumeAt(Side::Sell, 15000), 0);
    EXPECT_EQ(book.VolumeAt(Side::Buy, 15000), 0);
}

TEST(OrderBook, QueryDoesNotChangeBook)
{
    OrderBook book;
    book.AddLimit(Side::Buy, 15000, 100u);
    book.VolumeAt(Side::Buy, 16000);

    book.AddLimit(Side::Sell, 18000, 100u);
    book.VolumeAt(Side::Sell, 17000);

    EXPECT_EQ(book.BestBid(), 15000);
    EXPECT_EQ(book.BestAsk(), 18000);



}