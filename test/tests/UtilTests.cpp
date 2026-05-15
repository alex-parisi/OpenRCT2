/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <cmath>
#include <gtest/gtest.h>
#include <openrct2/util/Util.h>
#include <set>

TEST(UtilTests, Lerp_TZeroReturnsA)
{
    EXPECT_EQ(Lerp(10, 200, 0.0f), uint8_t{ 10 });
}

TEST(UtilTests, Lerp_TOneReturnsB)
{
    EXPECT_EQ(Lerp(10, 200, 1.0f), uint8_t{ 200 });
}

TEST(UtilTests, Lerp_TBelowZeroClampsToA)
{
    EXPECT_EQ(Lerp(10, 200, -0.5f), uint8_t{ 10 });
    EXPECT_EQ(Lerp(10, 200, -1000.0f), uint8_t{ 10 });
}

TEST(UtilTests, Lerp_TAboveOneClampsToB)
{
    EXPECT_EQ(Lerp(10, 200, 1.5f), uint8_t{ 200 });
    EXPECT_EQ(Lerp(10, 200, 1000.0f), uint8_t{ 200 });
}

TEST(UtilTests, Lerp_Midpoint)
{
    EXPECT_EQ(Lerp(0, 100, 0.5f), uint8_t{ 50 });
    EXPECT_EQ(Lerp(0, 200, 0.5f), uint8_t{ 100 });
}

TEST(UtilTests, Lerp_DescendingRange)
{
    EXPECT_EQ(Lerp(200, 0, 0.0f), uint8_t{ 200 });
    EXPECT_EQ(Lerp(200, 0, 1.0f), uint8_t{ 0 });
    EXPECT_EQ(Lerp(200, 0, 0.5f), uint8_t{ 100 });
}

TEST(UtilTests, Lerp_EqualEndpointsReturnsThatValue)
{
    EXPECT_EQ(Lerp(42, 42, 0.0f), uint8_t{ 42 });
    EXPECT_EQ(Lerp(42, 42, 0.5f), uint8_t{ 42 });
    EXPECT_EQ(Lerp(42, 42, 1.0f), uint8_t{ 42 });
}

TEST(UtilTests, Lerp_FullByteRange)
{
    EXPECT_EQ(Lerp(0, 255, 0.0f), uint8_t{ 0 });
    EXPECT_EQ(Lerp(0, 255, 1.0f), uint8_t{ 255 });
    EXPECT_EQ(Lerp(0, 255, 0.5f), uint8_t{ 127 });
}

TEST(UtilTests, FLerp_EndpointsExact)
{
    EXPECT_FLOAT_EQ(FLerp(0.0f, 10.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(FLerp(0.0f, 10.0f, 1.0f), 10.0f);
}

TEST(UtilTests, FLerp_Midpoint)
{
    EXPECT_FLOAT_EQ(FLerp(0.0f, 10.0f, 0.5f), 5.0f);
    EXPECT_FLOAT_EQ(FLerp(-4.0f, 4.0f, 0.5f), 0.0f);
}

TEST(UtilTests, FLerp_DoesNotClampOutsideUnitInterval)
{
    EXPECT_FLOAT_EQ(FLerp(0.0f, 10.0f, -1.0f), -10.0f);
    EXPECT_FLOAT_EQ(FLerp(0.0f, 10.0f, 2.0f), 20.0f);
}

TEST(UtilTests, FLerp_NegativeRange)
{
    EXPECT_FLOAT_EQ(FLerp(-10.0f, -2.0f, 0.0f), -10.0f);
    EXPECT_FLOAT_EQ(FLerp(-10.0f, -2.0f, 1.0f), -2.0f);
    EXPECT_FLOAT_EQ(FLerp(-10.0f, -2.0f, 0.25f), -8.0f);
}

TEST(UtilTests, SoftLight_BlackBaseStaysBlack)
{
    for (int b = 0; b <= 255; b += 17)
    {
        EXPECT_EQ(SoftLight(0, static_cast<uint8_t>(b)), uint8_t{ 0 }) << "b=" << b;
    }
}

TEST(UtilTests, SoftLight_WhiteBaseStaysWhite)
{
    for (int b = 0; b <= 255; b += 17)
    {
        EXPECT_EQ(SoftLight(255, static_cast<uint8_t>(b)), uint8_t{ 255 }) << "b=" << b;
    }
}

TEST(UtilTests, SoftLight_BlackBlendDarkensBase)
{
    EXPECT_EQ(SoftLight(128, 0), uint8_t{ static_cast<uint8_t>((128.0f / 255.0f) * (128.0f / 255.0f) * 255.0f) });
    EXPECT_EQ(SoftLight(64, 0), uint8_t{ static_cast<uint8_t>((64.0f / 255.0f) * (64.0f / 255.0f) * 255.0f) });
}

TEST(UtilTests, SoftLight_WhiteBlendBrightensBase)
{
    EXPECT_EQ(SoftLight(64, 255), uint8_t{ static_cast<uint8_t>(std::sqrt(64.0f / 255.0f) * 255.0f) });
    EXPECT_EQ(SoftLight(144, 255), uint8_t{ static_cast<uint8_t>(std::sqrt(144.0f / 255.0f) * 255.0f) });
}

TEST(UtilTests, SoftLight_OutputAlwaysWithinByteRange)
{
    for (int a = 0; a <= 255; a += 31)
    {
        for (int b = 0; b <= 255; b += 31)
        {
            const auto result = SoftLight(static_cast<uint8_t>(a), static_cast<uint8_t>(b));
            EXPECT_GE(result, 0);
            EXPECT_LE(result, 255);
        }
    }
}

TEST(UtilTests, UtilRand_NotDegenerate)
{
    std::set<uint32_t> seen;
    for (int i = 0; i < 64; ++i)
        seen.insert(UtilRand());
    EXPECT_GT(seen.size(), 1u);
}

TEST(UtilTests, UtilRandNormalDistributed_MeanAndStdDevAreReasonable)
{
    constexpr int kN = 4096;
    double sum = 0.0;
    double sumSq = 0.0;
    for (int i = 0; i < kN; ++i)
    {
        const float x = UtilRandNormalDistributed();
        sum += x;
        sumSq += static_cast<double>(x) * x;
    }
    const double mean = sum / kN;
    const double variance = (sumSq / kN) - (mean * mean);
    const double stddev = std::sqrt(variance);

    EXPECT_NEAR(mean, 0.0, 0.15);
    EXPECT_NEAR(stddev, 1.0, 0.15);
}

TEST(UtilTests, AddClamp_NormalAdditionInRange)
{
    EXPECT_EQ(AddClamp<int32_t>(5, 7), 12);
    EXPECT_EQ(AddClamp<int32_t>(-3, 8), 5);
}

TEST(UtilTests, AddClamp_SaturatesAtMaxOnPositiveOverflow)
{
    constexpr auto kMax = std::numeric_limits<int32_t>::max();
    EXPECT_EQ(AddClamp<int32_t>(kMax - 1, 5), kMax);
    EXPECT_EQ(AddClamp<int32_t>(kMax, 1), kMax);
}

TEST(UtilTests, AddClamp_SaturatesAtMinOnNegativeOverflow)
{
    constexpr auto kMin = std::numeric_limits<int32_t>::lowest();
    EXPECT_EQ(AddClamp<int32_t>(kMin + 1, -5), kMin);
    EXPECT_EQ(AddClamp<int32_t>(kMin, -1), kMin);
}

TEST(UtilTests, AddClamp_AddingZeroIsIdentity)
{
    EXPECT_EQ(AddClamp<int32_t>(42, 0), 42);
    EXPECT_EQ(AddClamp<int32_t>(std::numeric_limits<int32_t>::max(), 0), std::numeric_limits<int32_t>::max());
    EXPECT_EQ(AddClamp<int32_t>(std::numeric_limits<int32_t>::lowest(), 0), std::numeric_limits<int32_t>::lowest());
}

TEST(UtilTests, HiByteExtractsUpperOctet)
{
    EXPECT_EQ(HiByte(0xABCDu), uint8_t{ 0xAB });
    EXPECT_EQ(HiByte(0x0100u), uint8_t{ 0x01 });
    EXPECT_EQ(HiByte(0x00FFu), uint8_t{ 0x00 });
}

TEST(UtilTests, LoByteExtractsLowerOctet)
{
    EXPECT_EQ(LoByte(0xABCDu), uint8_t{ 0xCD });
    EXPECT_EQ(LoByte(0x0100u), uint8_t{ 0x00 });
    EXPECT_EQ(LoByte(0x00FFu), uint8_t{ 0xFF });
}

TEST(UtilTests, HiByteAndLoByteCompose)
{
    constexpr uint16_t kValue = 0x1234;
    EXPECT_EQ((static_cast<uint16_t>(HiByte(kValue)) << 8) | LoByte(kValue), kValue);
}
