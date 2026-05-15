/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/interface/ZoomLevel.h>

TEST(ZoomLevelTests, DefaultConstructorIsZero)
{
    constexpr ZoomLevel kZ{};
    EXPECT_EQ(static_cast<int8_t>(kZ), int8_t{ 0 });
}

TEST(ZoomLevelTests, ExplicitConstructorStoresLevel)
{
    constexpr ZoomLevel kZ{ 2 };
    EXPECT_EQ(static_cast<int8_t>(kZ), int8_t{ 2 });
}

TEST(ZoomLevelTests, CopyConstructorPreservesLevel)
{
    constexpr ZoomLevel kSrc{ -1 };
    constexpr ZoomLevel kCopy{ kSrc };
    EXPECT_EQ(static_cast<int8_t>(kCopy), int8_t{ -1 });
}

TEST(ZoomLevelTests, MinAndMaxBounds)
{
    EXPECT_EQ(static_cast<int8_t>(ZoomLevel::min()), int8_t{ -2 });
    EXPECT_EQ(static_cast<int8_t>(ZoomLevel::max()), int8_t{ 3 });
}

TEST(ZoomLevelTests, PreIncrementReturnsRefToUpdated)
{
    ZoomLevel z{ 0 };
    auto& ref = ++z;
    EXPECT_EQ(&ref, &z);
    EXPECT_EQ(static_cast<int8_t>(z), int8_t{ 1 });
}

TEST(ZoomLevelTests, PostIncrementReturnsOldValueByCopy)
{
    ZoomLevel z{ 0 };
    const auto old = z++;
    EXPECT_EQ(static_cast<int8_t>(old), int8_t{ 0 });
    EXPECT_EQ(static_cast<int8_t>(z), int8_t{ 1 });
}

TEST(ZoomLevelTests, PreDecrementReturnsRefToUpdated)
{
    ZoomLevel z{ 0 };
    auto& ref = --z;
    EXPECT_EQ(&ref, &z);
    EXPECT_EQ(static_cast<int8_t>(z), int8_t{ -1 });
}

TEST(ZoomLevelTests, PostDecrementReturnsOldValueByCopy)
{
    ZoomLevel z{ 0 };
    const auto old = z--;
    EXPECT_EQ(static_cast<int8_t>(old), int8_t{ 0 });
    EXPECT_EQ(static_cast<int8_t>(z), int8_t{ -1 });
}

TEST(ZoomLevelTests, CopyAssignmentReturnsSelf)
{
    ZoomLevel a{ 1 };
    constexpr ZoomLevel kB{ 3 };
    auto& ref = (a = kB);
    EXPECT_EQ(&ref, &a);
    EXPECT_EQ(static_cast<int8_t>(a), int8_t{ 3 });
}

TEST(ZoomLevelTests, CompoundAddSubMutateAndReturnRef)
{
    ZoomLevel a{ 1 };
    auto& addRef = (a += ZoomLevel{ 2 });
    EXPECT_EQ(&addRef, &a);
    EXPECT_EQ(static_cast<int8_t>(a), int8_t{ 3 });

    auto& subRef = (a -= ZoomLevel{ 5 });
    EXPECT_EQ(&subRef, &a);
    EXPECT_EQ(static_cast<int8_t>(a), int8_t{ -2 });
}

TEST(ZoomLevelTests, FreeAddSubWithZoomLevelOperand)
{
    constexpr ZoomLevel kA{ 1 };
    constexpr ZoomLevel kB{ 2 };
    EXPECT_EQ(static_cast<int8_t>(kA + kB), int8_t{ 3 });
    EXPECT_EQ(static_cast<int8_t>(kA - kB), int8_t{ -1 });
}

TEST(ZoomLevelTests, FreeAddSubWithInt8Operand)
{
    constexpr ZoomLevel kA{ 1 };
    EXPECT_EQ(static_cast<int8_t>(kA + int8_t{ 2 }), int8_t{ 3 });
    EXPECT_EQ(static_cast<int8_t>(kA - int8_t{ 4 }), int8_t{ -3 });
}

TEST(ZoomLevelTests, ComparisonsAcrossLevels)
{
    constexpr ZoomLevel kLow{ -1 };
    constexpr ZoomLevel kSame{ -1 };
    constexpr ZoomLevel kHigh{ 2 };

    EXPECT_TRUE(kLow == kSame);
    EXPECT_FALSE(kLow == kHigh);
    EXPECT_TRUE(kLow != kHigh);
    EXPECT_FALSE(kLow != kSame);

    EXPECT_TRUE(kLow < kHigh);
    EXPECT_FALSE(kHigh < kLow);
    EXPECT_TRUE(kHigh > kLow);
    EXPECT_FALSE(kLow > kHigh);

    EXPECT_TRUE(kLow <= kSame);
    EXPECT_TRUE(kLow <= kHigh);
    EXPECT_FALSE(kHigh <= kLow);

    EXPECT_TRUE(kLow >= kSame);
    EXPECT_TRUE(kHigh >= kLow);
    EXPECT_FALSE(kLow >= kHigh);
}

TEST(ZoomLevelTests, ApplyTo_PositiveLevelLeftShifts)
{
    constexpr ZoomLevel kZ{ 2 };
    EXPECT_EQ(kZ.ApplyTo<int32_t>(5), 5 << 2);
    EXPECT_EQ(kZ.ApplyTo<int32_t>(1), 4);
}

TEST(ZoomLevelTests, ApplyTo_NegativeLevelRightShifts)
{
    constexpr ZoomLevel kZ{ -2 };
    EXPECT_EQ(kZ.ApplyTo<int32_t>(20), 20 >> 2);
    EXPECT_EQ(kZ.ApplyTo<int32_t>(1), 0);
}

TEST(ZoomLevelTests, ApplyTo_ZeroLevelIsIdentity)
{
    constexpr ZoomLevel kZ{ 0 };
    EXPECT_EQ(kZ.ApplyTo<int32_t>(42), 42);
}

TEST(ZoomLevelTests, ApplyInversedTo_PositiveLevelRightShifts)
{
    constexpr ZoomLevel kZ{ 2 };
    EXPECT_EQ(kZ.ApplyInversedTo<int32_t>(20), 20 >> 2);
}

TEST(ZoomLevelTests, ApplyInversedTo_NegativeLevelLeftShifts)
{
    constexpr ZoomLevel kZ{ -2 };
    EXPECT_EQ(kZ.ApplyInversedTo<int32_t>(5), 5 << 2);
}

TEST(ZoomLevelTests, ApplyInversedTo_IsInverseOfApplyToForNonNegativeOperand)
{
    for (int8_t lvl = -2; lvl <= 3; ++lvl)
    {
        constexpr int32_t kValue = 0x1000;
        const ZoomLevel z{ lvl };
        EXPECT_EQ(z.ApplyInversedTo<int32_t>(z.ApplyTo<int32_t>(kValue)), kValue) << "level " << static_cast<int>(lvl);
    }
}
