/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <limits>
#include <openrct2/core/Money.hpp>

TEST(MoneyTests, GBP_LiteralMultipliesPoundsByTen)
{
    EXPECT_EQ(1.0_GBP, money64{ 10 });
    EXPECT_EQ(0.0_GBP, money64{ 0 });
    EXPECT_EQ(0.5_GBP, money64{ 5 });
}

TEST(MoneyTests, GBP_LiteralCarriesNegativeSign)
{
    EXPECT_EQ(-2.5_GBP, money64{ -25 });
}

TEST(MoneyTests, ToMoney64FromGBP_Int32_MultipliesByTen)
{
    EXPECT_EQ(ToMoney64FromGBP(int32_t{ 0 }), money64{ 0 });
    EXPECT_EQ(ToMoney64FromGBP(int32_t{ 1 }), money64{ 10 });
    EXPECT_EQ(ToMoney64FromGBP(int32_t{ -100 }), money64{ -1000 });
}

TEST(MoneyTests, ToMoney64FromGBP_Int64_WidensBeforeMultiplyingSoLargeValuesDoNotTruncate)
{
    // 0x100000000 doesn't fit in int32_t, but the int64_t overload should handle it.
    constexpr int64_t kAboveInt32Max = int64_t{ 1 } << 32;
    EXPECT_EQ(ToMoney64FromGBP(kAboveInt32Max), money64{ kAboveInt32Max * 10 });
}

TEST(MoneyTests, ToMoney64FromGBP_Double_TruncatesFractionalTenthsTowardZero)
{
    EXPECT_EQ(ToMoney64FromGBP(1.5), money64{ 15 });
    EXPECT_EQ(ToMoney64FromGBP(-3.0), money64{ -30 });
    EXPECT_EQ(ToMoney64FromGBP(0.05), money64{ 0 });
}

TEST(MoneyTests, KMoney16Undefined_HasAllOnesBitPattern)
{
    EXPECT_EQ(static_cast<uint16_t>(kMoney16Undefined), uint16_t{ 0xFFFF });
}

TEST(MoneyTests, KMoney32Undefined_EqualsInt32Min)
{
    EXPECT_EQ(kMoney32Undefined, std::numeric_limits<int32_t>::min());
}

TEST(MoneyTests, KMoney64Undefined_EqualsInt64Min)
{
    EXPECT_EQ(kMoney64Undefined, std::numeric_limits<int64_t>::min());
}

TEST(MoneyTests, ToMoney64_FromMoney32_PreservesValidValuesIncludingNegatives)
{
    EXPECT_EQ(ToMoney64(money32{ 100 }), money64{ 100 });
    EXPECT_EQ(ToMoney64(money32{ 0 }), money64{ 0 });
    EXPECT_EQ(ToMoney64(money32{ -100 }), money64{ -100 });
    EXPECT_EQ(ToMoney64(std::numeric_limits<money32>::max()), money64{ std::numeric_limits<int32_t>::max() });
}

TEST(MoneyTests, ToMoney64_FromMoney16_PreservesValidValuesIncludingNegatives)
{
    EXPECT_EQ(ToMoney64(money16{ 100 }), money64{ 100 });
    EXPECT_EQ(ToMoney64(money16{ 0 }), money64{ 0 });
    EXPECT_EQ(ToMoney64(money16{ -2 }), money64{ -2 });
}

TEST(MoneyTests, ToMoney16_FromMoney64_PreservesValidValuesFittingInSixteenBits)
{
    EXPECT_EQ(ToMoney16(money64{ 100 }), money16{ 100 });
    EXPECT_EQ(ToMoney16(money64{ 0 }), money16{ 0 });
    EXPECT_EQ(ToMoney16(money64{ -100 }), money16{ -100 });
}

TEST(MoneyTests, WidthConversions_RemapSentinelsAcrossWidthsRatherThanNaivelyCastingTheBits)
{
    EXPECT_EQ(ToMoney64(kMoney16Undefined), kMoney64Undefined);
    EXPECT_EQ(ToMoney64(kMoney32Undefined), kMoney64Undefined);
    EXPECT_EQ(ToMoney16(kMoney64Undefined), kMoney16Undefined);
}
