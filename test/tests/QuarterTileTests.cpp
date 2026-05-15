/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/world/Location.hpp>
#include <openrct2/world/MapLimits.h>
#include <openrct2/world/QuarterTile.h>

// QuarterTile packs two 4-bit nibbles into one byte:
//   low nibble  — which of the 4 tile quarters (N, E, S, W) are occupied
//   high nibble — which of those quarters have their Z raised by one land step
// Bit layout per nibble: bit 0 = N, bit 1 = E, bit 2 = S, bit 3 = W.

TEST(QuarterTileTests, TwoArgConstructorPacksTileThenZ)
{
    constexpr QuarterTile kQt{ 0b0101, 0b1010 };
    EXPECT_EQ(kQt.GetBaseQuarterOccupied(), uint8_t{ 0b0101 });
    EXPECT_EQ(kQt.GetZQuarterOccupied(), uint8_t{ 0b1010 });
}

TEST(QuarterTileTests, SingleArgConstructorReadsRawByte)
{
    constexpr QuarterTile kQt{ 0b1010'0101 };
    EXPECT_EQ(kQt.GetBaseQuarterOccupied(), uint8_t{ 0b0101 });
    EXPECT_EQ(kQt.GetZQuarterOccupied(), uint8_t{ 0b1010 });
}

TEST(QuarterTileTests, AccessorsIgnoreTheOppositeNibble)
{
    constexpr QuarterTile kQtBaseOnly{ 0b1111, 0 };
    EXPECT_EQ(kQtBaseOnly.GetBaseQuarterOccupied(), uint8_t{ 0b1111 });
    EXPECT_EQ(kQtBaseOnly.GetZQuarterOccupied(), uint8_t{ 0 });

    constexpr QuarterTile kQtZOnly{ 0, 0b1111 };
    EXPECT_EQ(kQtZOnly.GetBaseQuarterOccupied(), uint8_t{ 0 });
    EXPECT_EQ(kQtZOnly.GetZQuarterOccupied(), uint8_t{ 0b1111 });
}

TEST(QuarterTileTests, Rotate_ZeroIsIdentity)
{
    constexpr QuarterTile kQt{ 0b0011, 0b1001 };
    const auto rotated = kQt.Rotate(0);
    EXPECT_EQ(rotated.GetBaseQuarterOccupied(), kQt.GetBaseQuarterOccupied());
    EXPECT_EQ(rotated.GetZQuarterOccupied(), kQt.GetZQuarterOccupied());
}

TEST(QuarterTileTests, Rotate_OneShiftsEachBitByOnePositionCyclically)
{
    constexpr QuarterTile kQt{ 0b0001, 0b1000 };
    const auto rotated = kQt.Rotate(1);
    EXPECT_EQ(rotated.GetBaseQuarterOccupied(), uint8_t{ 0b0010 });
    EXPECT_EQ(rotated.GetZQuarterOccupied(), uint8_t{ 0b0001 });
}

TEST(QuarterTileTests, Rotate_TwoSwapsOppositeQuarters)
{
    constexpr QuarterTile kQt{ 0b1001, 0b0110 };
    const auto rotated = kQt.Rotate(2);
    EXPECT_EQ(rotated.GetBaseQuarterOccupied(), uint8_t{ 0b0110 });
    EXPECT_EQ(rotated.GetZQuarterOccupied(), uint8_t{ 0b1001 });
}

TEST(QuarterTileTests, Rotate_ThreeIsInverseOfOne)
{
    constexpr QuarterTile kQt{ 0b0001, 0b1000 };
    const auto rotated = kQt.Rotate(3);
    EXPECT_EQ(rotated.GetBaseQuarterOccupied(), uint8_t{ 0b1000 });
    EXPECT_EQ(rotated.GetZQuarterOccupied(), uint8_t{ 0b0100 });
}

TEST(QuarterTileTests, Rotate_FourSequentialOnesReturnToOriginal)
{
    constexpr QuarterTile kQt{ 0b1011, 0b0110 };
    const auto cur = kQt.Rotate(1).Rotate(1).Rotate(1).Rotate(1);
    EXPECT_EQ(cur.GetBaseQuarterOccupied(), kQt.GetBaseQuarterOccupied());
    EXPECT_EQ(cur.GetZQuarterOccupied(), kQt.GetZQuarterOccupied());
}

TEST(QuarterTileTests, Rotate_TwoEqualsRotateOneTwice)
{
    constexpr QuarterTile kQt{ 0b1100, 0b0011 };
    const auto direct = kQt.Rotate(2);
    const auto indirect = kQt.Rotate(1).Rotate(1);
    EXPECT_EQ(direct.GetBaseQuarterOccupied(), indirect.GetBaseQuarterOccupied());
    EXPECT_EQ(direct.GetZQuarterOccupied(), indirect.GetZQuarterOccupied());
}

TEST(QuarterTileTests, Rotate_ThreeEqualsRotateOneThrice)
{
    constexpr QuarterTile kQt{ 0b1010, 0b0101 };
    const auto direct = kQt.Rotate(3);
    const auto indirect = kQt.Rotate(1).Rotate(1).Rotate(1);
    EXPECT_EQ(direct.GetBaseQuarterOccupied(), indirect.GetBaseQuarterOccupied());
    EXPECT_EQ(direct.GetZQuarterOccupied(), indirect.GetZQuarterOccupied());
}

TEST(QuarterTileTests, Rotate_AllOnesIsRotationInvariant)
{
    constexpr QuarterTile kQt{ 0b1111, 0b1111 };
    for (uint8_t amount = 0; amount <= 3; ++amount)
    {
        const auto rotated = kQt.Rotate(amount);
        EXPECT_EQ(rotated.GetBaseQuarterOccupied(), uint8_t{ 0b1111 }) << "amount=" << +amount;
        EXPECT_EQ(rotated.GetZQuarterOccupied(), uint8_t{ 0b1111 }) << "amount=" << +amount;
    }
}

TEST(QuarterTileTests, Rotate_InvalidAmountReturnsZero)
{
    constexpr QuarterTile kQt{ 0b1111, 0b1111 };
    const auto rotated = kQt.Rotate(4);
    EXPECT_EQ(rotated.GetBaseQuarterOccupied(), uint8_t{ 0 });
    EXPECT_EQ(rotated.GetZQuarterOccupied(), uint8_t{ 0 });
}

TEST(QuarterTileTests, GetQuarterHeights_NoQuartersRaisedReturnsBaseHeight)
{
    constexpr QuarterTile kQt{ 0b1111, 0b0000 };
    const auto [north, east, south, west] = kQt.GetQuarterHeights(100);
    EXPECT_EQ(north, 100);
    EXPECT_EQ(east, 100);
    EXPECT_EQ(south, 100);
    EXPECT_EQ(west, 100);
}

TEST(QuarterTileTests, GetQuarterHeights_AllQuartersRaisedAddsOneStepEverywhere)
{
    constexpr QuarterTile kQt{ 0, 0b1111 };
    const auto [north, east, south, west] = kQt.GetQuarterHeights(100);
    EXPECT_EQ(north, 100 + kLandHeightStep);
    EXPECT_EQ(east, 100 + kLandHeightStep);
    EXPECT_EQ(south, 100 + kLandHeightStep);
    EXPECT_EQ(west, 100 + kLandHeightStep);
}

TEST(QuarterTileTests, GetQuarterHeights_PerQuarterBitMapping)
{
    constexpr int32_t kBase = 32;
    constexpr auto kStep = kLandHeightStep;

    const auto [north, east, south, west] = QuarterTile{ 0, 0b0001 }.GetQuarterHeights(kBase);
    EXPECT_EQ(north, kBase + kStep);
    EXPECT_EQ(east, kBase);
    EXPECT_EQ(south, kBase);
    EXPECT_EQ(west, kBase);

    const auto onlyE = QuarterTile{ 0, 0b0010 }.GetQuarterHeights(kBase);
    EXPECT_EQ(onlyE.east, kBase + kStep);
    EXPECT_EQ(onlyE.north, kBase);

    const auto onlyS = QuarterTile{ 0, 0b0100 }.GetQuarterHeights(kBase);
    EXPECT_EQ(onlyS.south, kBase + kStep);
    EXPECT_EQ(onlyS.west, kBase);

    const auto onlyW = QuarterTile{ 0, 0b1000 }.GetQuarterHeights(kBase);
    EXPECT_EQ(onlyW.west, kBase + kStep);
    EXPECT_EQ(onlyW.north, kBase);
}

TEST(QuarterTileTests, GetQuarterHeights_IgnoresBaseQuarterNibble)
{
    constexpr QuarterTile kQt{ 0b1111, 0b0001 };
    const auto [north, east, south, west] = kQt.GetQuarterHeights(0);
    EXPECT_EQ(north, kLandHeightStep);
    EXPECT_EQ(east, 0);
    EXPECT_EQ(south, 0);
    EXPECT_EQ(west, 0);
}
