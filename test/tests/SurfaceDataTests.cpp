/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/world/SurfaceData.h>
#include <openrct2/world/tile_element/Slope.h>

using namespace OpenRCT2;

// The 9 rows of the Raise/Lower tables correspond, in order, to the surface
// selection types: corner0, corner1, corner2, corner3, full, edge0..edge3.
namespace
{
    constexpr size_t kSelCorner0 = 0;
    constexpr size_t kSelCorner1 = 1;
    constexpr size_t kSelCorner2 = 2;
    constexpr size_t kSelCorner3 = 3;
    constexpr size_t kSelFull = 4;
    constexpr size_t kSelEdge0 = 5;
    constexpr size_t kSelEdge1 = 6;
    constexpr size_t kSelEdge2 = 7;
    constexpr size_t kSelEdge3 = 8;

    constexpr size_t kRowCount = 9;
    constexpr size_t kSlopeCount = 32;
} // namespace

TEST(SurfaceDataTests, RaiseFromFlat_Corner0RaisesN)
{
    EXPECT_EQ(RaiseSurfaceCornerFlags(kSelCorner0, kTileSlopeFlat), kTileSlopeNCornerUp);
}

TEST(SurfaceDataTests, RaiseFromFlat_Corner1RaisesE)
{
    EXPECT_EQ(RaiseSurfaceCornerFlags(kSelCorner1, kTileSlopeFlat), kTileSlopeECornerUp);
}

TEST(SurfaceDataTests, RaiseFromFlat_Corner2RaisesS)
{
    EXPECT_EQ(RaiseSurfaceCornerFlags(kSelCorner2, kTileSlopeFlat), kTileSlopeSCornerUp);
}

TEST(SurfaceDataTests, RaiseFromFlat_Corner3RaisesW)
{
    EXPECT_EQ(RaiseSurfaceCornerFlags(kSelCorner3, kTileSlopeFlat), kTileSlopeWCornerUp);
}

TEST(SurfaceDataTests, RaiseFromFlat_FullSetsBaseHeightFlagOnly)
{
    EXPECT_EQ(RaiseSurfaceCornerFlags(kSelFull, kTileSlopeFlat), kTileSlopeRaiseOrLowerBaseHeight);
}

TEST(SurfaceDataTests, RaiseFromFlat_EdgesRaiseTheTwoMatchingCorners)
{
    EXPECT_EQ(RaiseSurfaceCornerFlags(kSelEdge0, kTileSlopeFlat), kTileSlopeSCornerUp | kTileSlopeWCornerUp);
    EXPECT_EQ(RaiseSurfaceCornerFlags(kSelEdge1, kTileSlopeFlat), kTileSlopeNCornerUp | kTileSlopeWCornerUp);
    EXPECT_EQ(RaiseSurfaceCornerFlags(kSelEdge2, kTileSlopeFlat), kTileSlopeNCornerUp | kTileSlopeECornerUp);
    EXPECT_EQ(RaiseSurfaceCornerFlags(kSelEdge3, kTileSlopeFlat), kTileSlopeECornerUp | kTileSlopeSCornerUp);
}

TEST(SurfaceDataTests, RaiseCorner0OnSingleNUp_GoesToDiagonalForm)
{
    EXPECT_EQ(
        RaiseSurfaceCornerFlags(kSelCorner0, kTileSlopeNCornerUp),
        static_cast<uint8_t>(kTileSlopeDiagonalFlag | kTileSlopeNCornerUp | kTileSlopeECornerUp | kTileSlopeWCornerUp));
}

TEST(SurfaceDataTests, RaiseCorner0FromValley_BumpsBaseHeight)
{
    const auto next = RaiseSurfaceCornerFlags(kSelCorner0, kTileSlopeNSValley);
    EXPECT_TRUE(next & kTileSlopeRaiseOrLowerBaseHeight)
        << "raising into a non-representable slope should signal base-height change";
}

TEST(SurfaceDataTests, LowerFromFlat_CornerSelectionsBumpBaseHeight)
{
    for (size_t sel = kSelCorner0; sel <= kSelCorner3; ++sel)
    {
        const auto next = LowerSurfaceCornerFlags(sel, kTileSlopeFlat);
        EXPECT_TRUE(next & kTileSlopeRaiseOrLowerBaseHeight) << "sel=" << sel;
    }
}

TEST(SurfaceDataTests, LowerFromFlat_FullSetsBaseHeightFlagOnly)
{
    EXPECT_EQ(LowerSurfaceCornerFlags(kSelFull, kTileSlopeFlat), kTileSlopeRaiseOrLowerBaseHeight);
}

TEST(SurfaceDataTests, LowerMatchingCornerFromSingleUp_ReturnsFlat)
{
    EXPECT_EQ(LowerSurfaceCornerFlags(kSelCorner0, kTileSlopeNCornerUp), kTileSlopeFlat);
    EXPECT_EQ(LowerSurfaceCornerFlags(kSelCorner1, kTileSlopeECornerUp), kTileSlopeFlat);
    EXPECT_EQ(LowerSurfaceCornerFlags(kSelCorner2, kTileSlopeSCornerUp), kTileSlopeFlat);
    EXPECT_EQ(LowerSurfaceCornerFlags(kSelCorner3, kTileSlopeWCornerUp), kTileSlopeFlat);
}

TEST(SurfaceDataTests, LowerEdgeFromMatchingTwoCornersUp_ReturnsFlat)
{
    EXPECT_EQ(LowerSurfaceCornerFlags(kSelEdge0, kTileSlopeSCornerUp | kTileSlopeWCornerUp), kTileSlopeFlat);
    EXPECT_EQ(LowerSurfaceCornerFlags(kSelEdge1, kTileSlopeNCornerUp | kTileSlopeWCornerUp), kTileSlopeFlat);
    EXPECT_EQ(LowerSurfaceCornerFlags(kSelEdge2, kTileSlopeNCornerUp | kTileSlopeECornerUp), kTileSlopeFlat);
    EXPECT_EQ(LowerSurfaceCornerFlags(kSelEdge3, kTileSlopeECornerUp | kTileSlopeSCornerUp), kTileSlopeFlat);
}

TEST(SurfaceDataTests, FullRaiseThenLower_ReturnsToFlatForFlatInput)
{
    EXPECT_EQ(
        RaiseSurfaceCornerFlags(kSelFull, kTileSlopeFlat) & ~kTileSlopeRaiseOrLowerBaseHeight, uint8_t{ 0 });
    EXPECT_EQ(
        LowerSurfaceCornerFlags(kSelFull, kTileSlopeFlat) & ~kTileSlopeRaiseOrLowerBaseHeight, uint8_t{ 0 });
}

TEST(SurfaceDataTests, SlopeBitsAlwaysWithinDefinedMask)
{
    constexpr uint8_t kAllowed = kTileSlopeMask | kTileSlopeRaiseOrLowerBaseHeight;
    for (size_t sel = 0; sel < kRowCount; ++sel)
    {
        for (size_t slope = 0; slope < kSlopeCount; ++slope)
        {
            const auto raised = RaiseSurfaceCornerFlags(sel, slope);
            const auto lowered = LowerSurfaceCornerFlags(sel, slope);
            EXPECT_EQ(raised & ~kAllowed, uint8_t{ 0 }) << "raise sel=" << sel << " slope=" << slope;
            EXPECT_EQ(lowered & ~kAllowed, uint8_t{ 0 }) << "lower sel=" << sel << " slope=" << slope;
        }
    }
}
