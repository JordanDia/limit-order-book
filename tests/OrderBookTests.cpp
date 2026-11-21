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

TEST(OrderBook, BuyAtBestAskFullyFills)
{
    OrderBook book;
    AddResult sell = book.AddLimit(Side::Sell, 16000, 100u);
    AddResult buy = book.AddLimit(Side::Buy, 16000, 100u);

    EXPECT_TRUE(sell.trades.empty());

    ASSERT_EQ(buy.trades.size(), 1u);

    const Trade& t = buy.trades[0];
    EXPECT_EQ(t.buyId, 2u);
    EXPECT_EQ(t.sellId, 1u);
    EXPECT_EQ(t.price, 16000);
    EXPECT_EQ(t.quantity, 100u);
}

TEST(OrderBook, TradeHappensAtRestingPrice)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 10000, 100u);
    AddResult buy = book.AddLimit(Side::Buy, 12000, 100u);


    ASSERT_EQ(buy.trades.size(), 1u);
    EXPECT_EQ(buy.trades[0].price, 10000);
}

TEST(OrderBook, PartialFillLeavesRemainderResting)
{
    OrderBook book;
    AddResult buy = book.AddLimit(Side::Buy, 12000, 50u);
    AddResult sell = book.AddLimit(Side::Sell, 10000, 100u);
    
    ASSERT_EQ(sell.trades.size(), 1u);
    EXPECT_EQ(sell.trades[0].price, 12000);
    
    const Order* sellOrder = book.GetOrder(sell.id.value());
    ASSERT_NE(sellOrder, nullptr);
    EXPECT_EQ(sellOrder->quantity, 50u);

    EXPECT_EQ(book.GetOrder(buy.id.value()), nullptr); // filled, not in book
}

TEST(OrderBook, PartialFillOfRestingOrder)
{
    OrderBook book;
    AddResult buy = book.AddLimit(Side::Buy, 12000, 100u);
    AddResult sell = book.AddLimit(Side::Sell, 10000, 50u);
    
    ASSERT_EQ(sell.trades.size(), 1u);
    EXPECT_EQ(sell.trades[0].price, 12000);
    
    const Order* buyOrder = book.GetOrder(buy.id.value());
    ASSERT_NE(buyOrder, nullptr);
    EXPECT_EQ(buyOrder->quantity, 50u);

    EXPECT_EQ(book.GetOrder(sell.id.value()), nullptr); // filled, not in book
}

TEST(OrderBook, FifoWithinLevel)
{
    OrderBook book;
    AddResult s1 = book.AddLimit(Side::Sell, 10000, 100u);
    AddResult s2 = book.AddLimit(Side::Sell, 10000, 50u);
    AddResult b1 = book.AddLimit(Side::Buy, 10000, 150u);

    ASSERT_EQ(b1.trades.size(), 2u);

    EXPECT_EQ(b1.trades[0].sellId, s1.id.value());
    EXPECT_EQ(b1.trades[0].quantity, 100u);

    EXPECT_EQ(b1.trades[1].sellId, s2.id.value());
    EXPECT_EQ(b1.trades[1].quantity, 50u);
}

TEST(OrderBook, SweepsMultipleLevels)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 16000, 100u);
    book.AddLimit(Side::Sell, 16100, 50u);
    book.AddLimit(Side::Sell, 16200, 50u);
    AddResult b1 = book.AddLimit(Side::Buy, 16100, 200u);

    ASSERT_EQ(b1.trades.size(), 2u);

    EXPECT_EQ(b1.trades[0].price, 16000);
    EXPECT_EQ(b1.trades[1].price, 16100);
    EXPECT_EQ(book.VolumeAt(Side::Sell, 16200), 50u);
    EXPECT_EQ(book.BestBid(), 16100);
    EXPECT_EQ(book.BestAsk(), 16200);
}

TEST(OrderBook, CancelRemovesOrder)
{
    OrderBook book;
    AddResult buy = book.AddLimit(Side::Buy, 12000, 50u);
    bool cancelResult = book.Cancel(buy.id.value());

    EXPECT_TRUE(cancelResult);
    EXPECT_EQ(book.VolumeAt(Side::Buy, 12000), 0);
    EXPECT_FALSE(book.BestBid().has_value());
    EXPECT_EQ(book.GetOrder(buy.id.value()), nullptr);
}

TEST(OrderBook, CancelUnknownIdReturnsFalse)
{
    OrderBook book;
    AddResult buy = book.AddLimit(Side::Buy, 12000, 50u);
    bool cancelResult = book.Cancel(buy.id.value() + 5);

    EXPECT_FALSE(cancelResult);
}

TEST(OrderBook, CancelTwiceReturnsFalse)
{
    OrderBook book;
    AddResult buy = book.AddLimit(Side::Buy, 12000, 50u);
    bool c1 = book.Cancel(buy.id.value());
    bool c2 = book.Cancel(buy.id.value());

    EXPECT_TRUE(c1);
    EXPECT_FALSE(c2);
}

TEST(OrderBook, CancelFilledOrderReturnsFalse)
{
    OrderBook book;
    AddResult buy = book.AddLimit(Side::Buy, 12000, 50u);
    AddResult sell = book.AddLimit(Side::Sell, 11000, 100u);

    EXPECT_EQ(sell.trades.size(), 1u);

    bool c1 = book.Cancel(buy.id.value());
    
    EXPECT_FALSE(c1);
};

TEST(OrderBook, CancelLeavesOtherOrdersAtLevel)
{
    OrderBook book;
    AddResult b1 = book.AddLimit(Side::Buy, 12000, 50u);
    AddResult b2 = book.AddLimit(Side::Buy, 12000, 50u);

    book.Cancel(b1.id.value());
    
    EXPECT_EQ(book.VolumeAt(Side::Buy, 12000), 50u);
    EXPECT_EQ(book.BestBid(), 12000);
    EXPECT_EQ(book.GetOrder(b1.id.value()), nullptr);
    EXPECT_NE(book.GetOrder(b2.id.value()), nullptr);

    // b2 still matchable, so lets sell trade with it
    AddResult sell = book.AddLimit(Side::Sell, 12000, 50u);
    ASSERT_EQ(sell.trades.size(), 1u);
    EXPECT_EQ(sell.trades[0].buyId, b2.id.value());
}

TEST(OrderBook, CancelledOrderDoesNotMatch)
{
    OrderBook book;
    AddResult sell = book.AddLimit(Side::Sell, 12000, 50u);

    bool c1 = book.Cancel(sell.id.value());
    ASSERT_TRUE(c1);

    AddResult b2 = book.AddLimit(Side::Buy, 13000, 50u);

    EXPECT_TRUE(b2.trades.empty());
    EXPECT_EQ(book.BestBid(), 13000);
}

TEST(OrderBook, MarketBuyFillsAskInPriceOrder)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 10100, 100u);
    book.AddLimit(Side::Sell, 10200, 100u);
    AddResult marketBuy = book.AddMarket(Side::Buy, 200u);

    ASSERT_EQ(marketBuy.trades.size(), 2u);
    EXPECT_EQ(marketBuy.trades[0].price, 10100);
    EXPECT_EQ(marketBuy.trades[1].price, 10200);
}

TEST(OrderBook, MarketSellFillsBidsInPriceOrder)
{
    OrderBook book;
    book.AddLimit(Side::Buy, 10100, 100u);
    book.AddLimit(Side::Buy, 10200, 100u);
    AddResult marketSell = book.AddMarket(Side::Sell, 200u);

    ASSERT_EQ(marketSell.trades.size(), 2u);
    EXPECT_EQ(marketSell.trades[0].price, 10200);
    EXPECT_EQ(marketSell.trades[1].price, 10100);
}

TEST(OrderBook, MarketOrderLeftoverDoesNotRest)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 10000, 50u);
    AddResult marketBuy = book.AddMarket(Side::Buy, 80u);

    ASSERT_FALSE(marketBuy.trades.empty());
    EXPECT_EQ(marketBuy.trades[0].price, 10000);
    EXPECT_EQ(marketBuy.trades[0].quantity, 50u);

    EXPECT_FALSE(book.BestAsk().has_value());
    EXPECT_FALSE(book.BestBid().has_value());
}

TEST(OrderBook, MarketOrderIntoEmptySideMakesNoTrades)
{
    OrderBook book;
    AddResult marketBuy = book.AddMarket(Side::Buy, 200u);

    EXPECT_TRUE(marketBuy.trades.empty());
    EXPECT_FALSE(book.BestAsk().has_value());
    EXPECT_FALSE(book.BestBid().has_value());
}

TEST(OrderBook, MarketOrderGetsIdButDoesNotRest)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 10000, 50u);
    AddResult marketBuy = book.AddMarket(Side::Buy, 200u);
    AddResult sell = book.AddLimit(Side::Sell, 12000, 50u);

    EXPECT_EQ(marketBuy.id.value(), 2u);
    EXPECT_EQ(*sell.id, *marketBuy.id + 1);

    EXPECT_EQ(book.GetOrder(*marketBuy.id), nullptr);
    EXPECT_FALSE(book.Cancel(*marketBuy.id));
}

TEST(OrderBook, MarketOrderTradesAtRestingPrice)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 10000, 50u);
    AddResult marketBuy = book.AddMarket(Side::Buy, 100);

    ASSERT_EQ(marketBuy.trades.size(), 1u);
    EXPECT_EQ(marketBuy.trades[0].price, 10000);
}