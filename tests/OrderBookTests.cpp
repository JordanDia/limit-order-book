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

TEST(OrderBook, CancelBestBidMovesToNextLevel)
{
    OrderBook book;
    AddResult b1 = book.AddLimit(Side::Buy, 10000, 100u);

    EXPECT_EQ(book.BestBid(), 10000);

    book.AddLimit(Side::Buy, 9000, 100u);

    bool cancelSuccess = book.Cancel(*b1.id);
    EXPECT_TRUE(cancelSuccess);
    EXPECT_EQ(book.BestBid(), 9000);
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

TEST(OrderBook, ZeroQuantityLimitIsRejected)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 10000, 100u);
    AddResult buy = book.AddLimit(Side::Buy, 10000, 0u);

    EXPECT_FALSE(buy.id.has_value());
    EXPECT_TRUE(buy.trades.empty());
    EXPECT_EQ(book.VolumeAt(Side::Sell, 10000), 100u);
}

TEST(OrderBook, ZeroQuantityMarketIsRejected)
{
    OrderBook book;
    book.AddLimit(Side::Sell, 10000, 100u);
    AddResult marketBuy = book.AddMarket(Side::Buy, 0u);

    EXPECT_FALSE(marketBuy.id.has_value());
    EXPECT_TRUE(marketBuy.trades.empty());
    EXPECT_EQ(book.VolumeAt(Side::Sell, 10000), 100u);
}

TEST(OrderBook, NonPositiveLimitPriceIsRejected)
{
    OrderBook book;
    book.AddLimit(Side::Buy, 10000, 200u);
    AddResult bad1 = book.AddLimit(Side::Sell, -100, 100u);
    AddResult bad2 = book.AddLimit(Side::Sell, 0, 100u);
    AddResult validBuy = book.AddLimit(Side::Buy, 5000, 100u);

    EXPECT_FALSE(bad1.id.has_value());
    EXPECT_TRUE(bad1.trades.empty());
    
    EXPECT_FALSE(bad2.id.has_value());
    EXPECT_TRUE(bad2.trades.empty());

    EXPECT_EQ(*validBuy.id, 2u);
}
TEST(OrderBook, FullWorkedExample)
{
    OrderBook book;

    // Starting book
    OrderId b1 = *book.AddLimit(Side::Buy, 15000, 200u).id;
    OrderId b2 = *book.AddLimit(Side::Buy, 15000, 150u).id;
    OrderId b3 = *book.AddLimit(Side::Buy, 15000, 50u).id;
    book.AddLimit(Side::Buy, 14999, 100u);
    book.AddLimit(Side::Buy, 14998, 700u);

    OrderId a1 = *book.AddLimit(Side::Sell, 15001, 300u).id;
    OrderId a2 = *book.AddLimit(Side::Sell, 15002, 200u).id;
    book.AddLimit(Side::Sell, 15003, 500u);

    ASSERT_EQ(book.BestBid(), 15000);
    ASSERT_EQ(book.BestAsk(), 15001);
    ASSERT_EQ(book.VolumeAt(Side::Buy, 15000), 400u);

    // Step 1: buy limit 250 @ 150.00 rests at the back of the queue
    AddResult step1 = book.AddLimit(Side::Buy, 15000, 250u);
    OrderId b4 = *step1.id;

    EXPECT_TRUE(step1.trades.empty());
    EXPECT_EQ(book.VolumeAt(Side::Buy, 15000), 650u);
    EXPECT_EQ(book.BestBid(), 15000);

    // Step 2: buy limit 400 @ 150.02 sweeps A1 and part of A2
    AddResult step2 = book.AddLimit(Side::Buy, 15002, 400u);
    OrderId b5 = *step2.id;

    ASSERT_EQ(step2.trades.size(), 2u);
    EXPECT_EQ(step2.trades[0].buyId, b5);
    EXPECT_EQ(step2.trades[0].sellId, a1);
    EXPECT_EQ(step2.trades[0].price, 15001);
    EXPECT_EQ(step2.trades[0].quantity, 300u);

    EXPECT_EQ(step2.trades[1].buyId, b5);
    EXPECT_EQ(step2.trades[1].sellId, a2);
    EXPECT_EQ(step2.trades[1].price, 15002);
    EXPECT_EQ(step2.trades[1].quantity, 100u);

    EXPECT_EQ(book.VolumeAt(Side::Sell, 15001), 0u);
    EXPECT_EQ(book.VolumeAt(Side::Sell, 15002), 100u);
    EXPECT_EQ(book.BestAsk(), 15002);
    EXPECT_EQ(book.GetOrder(b5), nullptr);
    EXPECT_EQ(book.GetOrder(a1), nullptr);
    EXPECT_EQ(book.BestBid(), 15000);

    // Step 3: market sell 500 takes B1, B2, B3, then 100 of B4
    AddResult step3 = book.AddMarket(Side::Sell, 500u);
    OrderId marketSell = *step3.id;

    const OrderId expectedBuyIds[] = { b1, b2, b3, b4 };
    const Quantity expectedQuantities[] = { 200u, 150u, 50u, 100u };

    ASSERT_EQ(step3.trades.size(), 4u);
    for (size_t i = 0; i < step3.trades.size(); i++)
    {
        EXPECT_EQ(step3.trades[i].buyId, expectedBuyIds[i]);
        EXPECT_EQ(step3.trades[i].sellId, marketSell);
        EXPECT_EQ(step3.trades[i].price, 15000);
        EXPECT_EQ(step3.trades[i].quantity, expectedQuantities[i]);
    }

    ASSERT_NE(book.GetOrder(b4), nullptr);
    EXPECT_EQ(book.GetOrder(b4)->quantity, 150u);
    EXPECT_EQ(book.VolumeAt(Side::Buy, 15000), 150u);
    EXPECT_EQ(book.BestBid(), 15000);

    // Step 4: cancel B4 empties the 150.00 level
    EXPECT_TRUE(book.Cancel(b4));
    EXPECT_EQ(book.VolumeAt(Side::Buy, 15000), 0u);
    EXPECT_EQ(book.BestBid(), 14999);
    EXPECT_EQ(book.BestAsk(), 15002);
}
