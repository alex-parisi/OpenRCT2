/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/entity/Yaw.hpp>

namespace Yaw = OpenRCT2::Entity::Yaw;
using SP = Yaw::SpritePrecision;

TEST(YawTests, Add_ZeroPlusZeroIsZero)
{
    EXPECT_EQ(Yaw::Add(0, 0), 0);
}

TEST(YawTests, Add_ZeroOnTheRightIsIdentityForBaseRotationValues)
{
    for (int32_t yaw = 0; yaw < Yaw::kBaseRotation; ++yaw)
    {
        EXPECT_EQ(Yaw::Add(yaw, 0), yaw) << "yaw=" << yaw;
    }
}

TEST(YawTests, Add_WrapsAtThirtyTwo)
{
    EXPECT_EQ(Yaw::Add(31, 1), 0);
    EXPECT_EQ(Yaw::Add(16, 16), 0);
    EXPECT_EQ(Yaw::Add(20, 20), 8);
    EXPECT_EQ(Yaw::Add(31, 31), 30);
}

TEST(YawTests, Add_AgreesWithSumMaskedByThirtyOneOverAGrid)
{
    for (int32_t a = 0; a < Yaw::kBaseRotation; ++a)
    {
        for (int32_t b = 0; b < Yaw::kBaseRotation; ++b)
        {
            EXPECT_EQ(Yaw::Add(a, b), (a + b) & 0x1F) << "a=" << a << " b=" << b;
        }
    }
}

TEST(YawTests, YawFrom4_MapsEachOfTheFourCardinalsToEightYawSteps)
{
    EXPECT_EQ(Yaw::YawFrom4(0), 0);
    EXPECT_EQ(Yaw::YawFrom4(1), 8);
    EXPECT_EQ(Yaw::YawFrom4(2), 16);
    EXPECT_EQ(Yaw::YawFrom4(3), 24);
}

TEST(YawTests, YawTo4_AfterYawFrom4_RoundTripsForEveryRotation)
{
    for (int32_t rotation = 0; rotation < 4; ++rotation)
    {
        EXPECT_EQ(Yaw::YawTo4(Yaw::YawFrom4(rotation)), rotation) << "rotation=" << rotation;
    }
}

TEST(YawTests, YawTo32_IsTheIdentityOnBaseYawValues)
{
    for (int32_t yaw = 0; yaw < Yaw::kBaseRotation; ++yaw)
    {
        EXPECT_EQ(Yaw::YawTo32(yaw), yaw) << "yaw=" << yaw;
    }
}

TEST(YawTests, YawToN_MatchTheirCorrespondingRightShifts)
{
    for (int32_t yaw = 0; yaw < Yaw::kBaseRotation; ++yaw)
    {
        EXPECT_EQ(Yaw::YawTo16(yaw), yaw >> 1) << "yaw=" << yaw;
        EXPECT_EQ(Yaw::YawTo8(yaw), yaw >> 2) << "yaw=" << yaw;
        EXPECT_EQ(Yaw::YawTo4(yaw), yaw >> 3) << "yaw=" << yaw;
        EXPECT_EQ(Yaw::YawTo64(yaw), yaw << 1) << "yaw=" << yaw;
    }
}

TEST(YawTests, YawToPrecision_AgreesWithExplicitYawToFunctionsForEveryShiftedPrecision)
{
    for (int32_t yaw = 0; yaw < Yaw::kBaseRotation; ++yaw)
    {
        EXPECT_EQ(Yaw::YawToPrecision(yaw, SP::Sprites4), Yaw::YawTo4(yaw)) << "yaw=" << yaw;
        EXPECT_EQ(Yaw::YawToPrecision(yaw, SP::Sprites8), Yaw::YawTo8(yaw)) << "yaw=" << yaw;
        EXPECT_EQ(Yaw::YawToPrecision(yaw, SP::Sprites16), Yaw::YawTo16(yaw)) << "yaw=" << yaw;
        EXPECT_EQ(Yaw::YawToPrecision(yaw, SP::Sprites32), Yaw::YawTo32(yaw)) << "yaw=" << yaw;
    }
}

TEST(YawTests, YawToPrecision_NoneAndSprites1CollapseAnyBaseYawToZero)
{
    for (int32_t yaw = 0; yaw < Yaw::kBaseRotation; ++yaw)
    {
        EXPECT_EQ(Yaw::YawToPrecision(yaw, SP::None), 0) << "yaw=" << yaw;
        EXPECT_EQ(Yaw::YawToPrecision(yaw, SP::Sprites1), 0) << "yaw=" << yaw;
    }
}

TEST(YawTests, YawToPrecision_Sprites2HalvesBaseYawToTwoDirections)
{
    for (int32_t yaw = 0; yaw < 16; ++yaw)
    {
        EXPECT_EQ(Yaw::YawToPrecision(yaw, SP::Sprites2), 0) << "yaw=" << yaw;
    }
    for (int32_t yaw = 16; yaw < Yaw::kBaseRotation; ++yaw)
    {
        EXPECT_EQ(Yaw::YawToPrecision(yaw, SP::Sprites2), 1) << "yaw=" << yaw;
    }
}

TEST(YawTests, NumSpritesPrecision_MatchesTheNumericSuffixOfEachPrecisionTag)
{
    EXPECT_EQ(Yaw::NumSpritesPrecision(SP::None), 0);
    EXPECT_EQ(Yaw::NumSpritesPrecision(SP::Sprites1), 1);
    EXPECT_EQ(Yaw::NumSpritesPrecision(SP::Sprites2), 2);
    EXPECT_EQ(Yaw::NumSpritesPrecision(SP::Sprites4), 4);
    EXPECT_EQ(Yaw::NumSpritesPrecision(SP::Sprites8), 8);
    EXPECT_EQ(Yaw::NumSpritesPrecision(SP::Sprites16), 16);
    EXPECT_EQ(Yaw::NumSpritesPrecision(SP::Sprites32), 32);
    EXPECT_EQ(Yaw::NumSpritesPrecision(SP::Sprites64), 64);
}

TEST(YawTests, NumSpritesPrecision_DoublesAtEachStepFromSprites1ToSprites32)
{
    constexpr SP kSteps[] = { SP::Sprites1, SP::Sprites2, SP::Sprites4, SP::Sprites8, SP::Sprites16, SP::Sprites32 };
    for (size_t i = 0; i + 1 < std::size(kSteps); ++i)
    {
        const auto cur = Yaw::NumSpritesPrecision(kSteps[i]);
        const auto next = Yaw::NumSpritesPrecision(kSteps[i + 1]);
        EXPECT_EQ(next, cur * 2) << "step " << i;
    }
}
