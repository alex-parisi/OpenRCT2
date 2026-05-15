/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/core/UnitConversion.h>

using namespace OpenRCT2;

TEST(UnitConversionTests, SquaredMetresToSquaredFeet_Zero)
{
    EXPECT_EQ(SquaredMetresToSquaredFeet(0), 0);
}

TEST(UnitConversionTests, SquaredMetresToSquaredFeet_ScalesBy11)
{
    EXPECT_EQ(SquaredMetresToSquaredFeet(1), 11);
    EXPECT_EQ(SquaredMetresToSquaredFeet(10), 110);
    EXPECT_EQ(SquaredMetresToSquaredFeet(100), 1100);
}

TEST(UnitConversionTests, SquaredMetresToSquaredFeet_Negative)
{
    EXPECT_EQ(SquaredMetresToSquaredFeet(-1), -11);
    EXPECT_EQ(SquaredMetresToSquaredFeet(-50), -550);
}

TEST(UnitConversionTests, MetresToFeet_KnownValues)
{
    EXPECT_EQ(MetresToFeet(0), 0);
    EXPECT_EQ(MetresToFeet(1), 3);
    EXPECT_EQ(MetresToFeet(256), 840);
    EXPECT_EQ(MetresToFeet(100), 328);
}

TEST(UnitConversionTests, FeetToMetres_KnownValues)
{
    EXPECT_EQ(FeetToMetres(0), 0);
    EXPECT_EQ(FeetToMetres(840), 256);
    EXPECT_EQ(FeetToMetres(3), 0);
    EXPECT_EQ(FeetToMetres(4), 1);
}

TEST(UnitConversionTests, FeetToMetres_IsApproximateInverseOfMetresToFeet)
{
    for (int32_t m = 256; m <= 256 * 10; m += 256)
    {
        EXPECT_EQ(FeetToMetres(MetresToFeet(m)), m) << "m=" << m;
    }
}

TEST(UnitConversionTests, MphToKmph_Zero)
{
    EXPECT_EQ(MphToKmph(0), 0);
}

TEST(UnitConversionTests, MphToKmph_OneMphIsOneKmphAfterTruncation)
{
    EXPECT_EQ(MphToKmph(1), 1);
}

TEST(UnitConversionTests, MphToKmph_ScalesNearExpectedRatio)
{
    EXPECT_EQ(MphToKmph(100), (100 * 1648) >> 10);
    EXPECT_EQ(MphToKmph(60), (60 * 1648) >> 10);
}

TEST(UnitConversionTests, MphToKmph_CalibrationPointIsExact)
{
    EXPECT_EQ(MphToKmph(1024), 1648);
}

TEST(UnitConversionTests, MphToDmps_Zero)
{
    EXPECT_EQ(MphToDmps(0), 0);
}

TEST(UnitConversionTests, MphToDmps_OneMphIsAboutFourDmps)
{
    EXPECT_EQ(MphToDmps(1), 4);
}

TEST(UnitConversionTests, MphToDmps_LinearOverPositiveRange)
{
    for (const int32_t mph : { 5, 50, 500 })
    {
        EXPECT_EQ(MphToDmps(mph), (mph * 73243) >> 14);
    }
}

TEST(UnitConversionTests, BaseZToMetres_BaseZ14IsGroundLevel)
{
    EXPECT_EQ(BaseZToMetres(14), 0);
}

TEST(UnitConversionTests, BaseZToMetres_KnownValues)
{
    EXPECT_EQ(BaseZToMetres(16), 1);
    EXPECT_EQ(BaseZToMetres(20), 4);
    EXPECT_EQ(BaseZToMetres(24), 7);
    EXPECT_EQ(BaseZToMetres(0), -10);
}

TEST(UnitConversionTests, MetresToBaseZ_ZeroMetresIs14)
{
    EXPECT_EQ(MetresToBaseZ(0), uint8_t{ 14 });
}

TEST(UnitConversionTests, MetresToBaseZ_KnownValues)
{
    EXPECT_EQ(MetresToBaseZ(3), uint8_t{ 18 });
    EXPECT_EQ(MetresToBaseZ(6), uint8_t{ 22 });
    EXPECT_EQ(MetresToBaseZ(12), uint8_t{ 30 });
}

TEST(UnitConversionTests, MetresToBaseZ_ApproximateInverseOfBaseZToMetres)
{
    for (int16_t m : { 0, 3, 6, 9, 12, 15 })
    {
        const auto baseZ = MetresToBaseZ(m);
        EXPECT_EQ(BaseZToMetres(baseZ), m) << "m=" << m;
    }
}

TEST(UnitConversionTests, HeightUnitsToMetres_Zero)
{
    EXPECT_EQ(HeightUnitsToMetres(0), 0);
}

TEST(UnitConversionTests, HeightUnitsToMetres_FourUnitsIsThreeMetres)
{
    EXPECT_EQ(HeightUnitsToMetres(4), 3);
    EXPECT_EQ(HeightUnitsToMetres(8), 6);
    EXPECT_EQ(HeightUnitsToMetres(40), 30);
}

TEST(UnitConversionTests, HeightUnitsToMetres_SingleUnitTruncatesToZero)
{
    EXPECT_EQ(HeightUnitsToMetres(1), 0);
    EXPECT_EQ(HeightUnitsToMetres(2), 1);
    EXPECT_EQ(HeightUnitsToMetres(3), 2);
}

TEST(UnitConversionTests, ToHumanReadableSpeed_Zero)
{
    EXPECT_EQ(ToHumanReadableSpeed(0), 0);
}

TEST(UnitConversionTests, ToHumanReadableSpeed_DividesByRoughlyTwoToTheEighteenOverNine)
{
    EXPECT_EQ(ToHumanReadableSpeed(29128), 1);
    EXPECT_EQ(ToHumanReadableSpeed(60000), 2);
    EXPECT_EQ(ToHumanReadableSpeed(1000000), 34);
}

TEST(UnitConversionTests, ToHumanReadableSpeed_BelowOneUnitFloorsToZero)
{
    EXPECT_EQ(ToHumanReadableSpeed(29127), 0);
    EXPECT_EQ(ToHumanReadableSpeed(1), 0);
}

TEST(UnitConversionTests, ToHumanReadableAirTime_ScalesByThree)
{
    EXPECT_EQ(ToHumanReadableAirTime(0), 0);
    EXPECT_EQ(ToHumanReadableAirTime(1), 3);
    EXPECT_EQ(ToHumanReadableAirTime(100), 300);
}

TEST(UnitConversionTests, ToHumanReadableAirTime_MaxUint16DoesNotOverflowInt32)
{
    EXPECT_EQ(ToHumanReadableAirTime(0xFFFFu), 65535 * 3);
}

TEST(UnitConversionTests, ToHumanReadableRideLength_FractionalLengthsTruncate)
{
    EXPECT_EQ(ToHumanReadableRideLength(0), 0);
    EXPECT_EQ(ToHumanReadableRideLength(65535), 0);
    EXPECT_EQ(ToHumanReadableRideLength(1 << 16), 1);
    EXPECT_EQ(ToHumanReadableRideLength(2 << 16), 2);
    EXPECT_EQ(ToHumanReadableRideLength((10 << 16) + 1234), 10);
}
