/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/core/EnumUtils.hpp>
#include <openrct2/paint/tile_element/Segment.h>

using OpenRCT2::PaintSegment;

namespace
{
    // Cast the uint64_t-returning EnumToFlag down to the uint16_t the segment API uses
    constexpr uint16_t kTop = static_cast<uint16_t>(EnumToFlag(PaintSegment::top));
    constexpr uint16_t kTopRight = static_cast<uint16_t>(EnumToFlag(PaintSegment::topRight));
    constexpr uint16_t kRight = static_cast<uint16_t>(EnumToFlag(PaintSegment::right));
    constexpr uint16_t kBottomRight = static_cast<uint16_t>(EnumToFlag(PaintSegment::bottomRight));
    constexpr uint16_t kBottom = static_cast<uint16_t>(EnumToFlag(PaintSegment::bottom));
    constexpr uint16_t kBottomLeft = static_cast<uint16_t>(EnumToFlag(PaintSegment::bottomLeft));
    constexpr uint16_t kLeft = static_cast<uint16_t>(EnumToFlag(PaintSegment::left));
    constexpr uint16_t kTopLeft = static_cast<uint16_t>(EnumToFlag(PaintSegment::topLeft));
    constexpr uint16_t kCentre = static_cast<uint16_t>(EnumToFlag(PaintSegment::centre));
    constexpr uint16_t kAllOuter = kTop | kTopRight | kRight | kBottomRight | kBottom | kBottomLeft | kLeft | kTopLeft;
} // namespace

TEST(SegmentTests, PaintSegmentsRotate_ZeroIsIdentity)
{
    constexpr uint16_t kInput = kTop | kRight | kCentre;
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kInput, 0), kInput);
}

TEST(SegmentTests, PaintSegmentsRotate_FourReturnsToOriginal)
{
    constexpr uint16_t kInput = kTop | kBottomRight | kLeft | kCentre;
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kInput, 4), kInput);
}

TEST(SegmentTests, PaintSegmentsRotate_OneStepShiftsEachOuterSegmentClockwiseByTwoBits)
{
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kTop, 1), kRight);
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kTopRight, 1), kBottomRight);
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kRight, 1), kBottom);
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kBottomRight, 1), kBottomLeft);
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kBottom, 1), kLeft);
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kBottomLeft, 1), kTopLeft);
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kLeft, 1), kTop);
    EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kTopLeft, 1), kTopRight);
}

TEST(SegmentTests, PaintSegmentsRotate_TwoEqualsRotateOneTwice)
{
    constexpr uint16_t kInput = kTop | kBottomRight | kLeft;
    EXPECT_EQ(
        OpenRCT2::paintSegmentsRotate(kInput, 2), OpenRCT2::paintSegmentsRotate(OpenRCT2::paintSegmentsRotate(kInput, 1), 1));
}

TEST(SegmentTests, PaintSegmentsRotate_ThreeEqualsRotateOneThrice)
{
    constexpr uint16_t kInput = kTopRight | kBottom | kTopLeft;
    constexpr auto kDirect = OpenRCT2::paintSegmentsRotate(kInput, 3);
    constexpr auto kIndirect = OpenRCT2::paintSegmentsRotate(
        OpenRCT2::paintSegmentsRotate(OpenRCT2::paintSegmentsRotate(kInput, 1), 1), 1);
    EXPECT_EQ(kDirect, kIndirect);
}

TEST(SegmentTests, PaintSegmentsRotate_CentreBitIsPreservedAtEveryRotation)
{
    constexpr uint16_t kInput = kCentre;
    for (uint8_t r = 0; r <= 4; ++r)
    {
        EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kInput, r), kCentre) << "rotation=" << +r;
    }
}

TEST(SegmentTests, PaintSegmentsRotate_AllOuterSegmentsAreRotationInvariant)
{
    for (uint8_t r = 0; r <= 4; ++r)
    {
        EXPECT_EQ(OpenRCT2::paintSegmentsRotate(kAllOuter, r), kAllOuter) << "rotation=" << +r;
    }
}

TEST(SegmentTests, PaintSegmentsRotate_NoneIsRotationInvariant)
{
    for (uint8_t r = 0; r <= 4; ++r)
    {
        EXPECT_EQ(OpenRCT2::paintSegmentsRotate(OpenRCT2::kSegmentsNone, r), OpenRCT2::kSegmentsNone) << "rotation=" << +r;
    }
}

TEST(SegmentTests, PaintSegmentsFlipXAxis_TopRightAndBottomLeftAreFixedPoints)
{
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kTopRight), kTopRight);
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kBottomLeft), kBottomLeft);
}

TEST(SegmentTests, PaintSegmentsFlipXAxis_SwapsTheThreePairsAcrossTheDiagonal)
{
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kTop), kRight);
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kRight), kTop);
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kBottomRight), kTopLeft);
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kTopLeft), kBottomRight);
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kBottom), kLeft);
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kLeft), kBottom);
}

TEST(SegmentTests, PaintSegmentsFlipXAxis_CentreBitIsPreserved)
{
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kCentre), kCentre);
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kTop | kCentre), kRight | kCentre);
}

TEST(SegmentTests, PaintSegmentsFlipXAxis_AppliedTwiceIsIdentity)
{
    constexpr uint16_t kInput = kTop | kBottomRight | kLeft | kCentre;
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(OpenRCT2::paintSegmentsFlipXAxis(kInput)), kInput);
}

TEST(SegmentTests, PaintSegmentsFlipXAxis_AllOuterAndNoneAreFixedPoints)
{
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(kAllOuter), kAllOuter);
    EXPECT_EQ(OpenRCT2::paintSegmentsFlipXAxis(OpenRCT2::kSegmentsNone), OpenRCT2::kSegmentsNone);
}

TEST(SegmentTests, KSegmentsAll_IncludesAllNineSegments)
{
    EXPECT_EQ(static_cast<uint16_t>(OpenRCT2::kSegmentsAll), static_cast<uint16_t>(kAllOuter | kCentre));
}

TEST(SegmentTests, BlockedSegmentsAllTypes_FillsAllThreeSlotsWithTheSameValue)
{
    constexpr uint16_t kInput = kTop | kCentre;
    constexpr auto kResult = OpenRCT2::blockedSegmentsAllTypes(kInput);
    ASSERT_EQ(kResult.size(), OpenRCT2::kBlockedSegmentsTypeCount);
    EXPECT_EQ(kResult[0], kInput);
    EXPECT_EQ(kResult[1], kInput);
    EXPECT_EQ(kResult[2], kInput);
}

TEST(SegmentTests, BlockedSegmentsRotate_AppliesPaintSegmentsRotateToEachSlotIndependently)
{
    constexpr std::array<uint16_t, OpenRCT2::kBlockedSegmentsTypeCount> kInput{ kTop, kRight, kBottom };
    constexpr auto kRotated = OpenRCT2::blockedSegmentsRotate(kInput, 1);
    EXPECT_EQ(kRotated[0], OpenRCT2::paintSegmentsRotate(kInput[0], 1));
    EXPECT_EQ(kRotated[1], OpenRCT2::paintSegmentsRotate(kInput[1], 1));
    EXPECT_EQ(kRotated[2], OpenRCT2::paintSegmentsRotate(kInput[2], 1));
}

TEST(SegmentTests, BlockedSegmentsRotate_ZeroIsIdentityOnEachSlot)
{
    constexpr std::array<uint16_t, OpenRCT2::kBlockedSegmentsTypeCount> kInput{ kTop, kRight | kCentre, kBottom };
    constexpr auto kRotated = OpenRCT2::blockedSegmentsRotate(kInput, 0);
    EXPECT_EQ(kRotated, kInput);
}

TEST(SegmentTests, BlockedSegmentsFlipXAxis_AppliesPaintSegmentsFlipXAxisToEachSlotIndependently)
{
    constexpr std::array<uint16_t, OpenRCT2::kBlockedSegmentsTypeCount> kInput{ kTop, kBottomRight, kBottom | kCentre };
    constexpr auto kFlipped = OpenRCT2::blockedSegmentsFlipXAxis(kInput);
    EXPECT_EQ(kFlipped[0], OpenRCT2::paintSegmentsFlipXAxis(kInput[0]));
    EXPECT_EQ(kFlipped[1], OpenRCT2::paintSegmentsFlipXAxis(kInput[1]));
    EXPECT_EQ(kFlipped[2], OpenRCT2::paintSegmentsFlipXAxis(kInput[2]));
}

TEST(SegmentTests, BlockedSegmentsFlipXAxis_AppliedTwiceIsIdentityOnEachSlot)
{
    constexpr std::array<uint16_t, OpenRCT2::kBlockedSegmentsTypeCount> kInput{ kTop | kBottomRight, kLeft, kCentre };
    EXPECT_EQ(OpenRCT2::blockedSegmentsFlipXAxis(OpenRCT2::blockedSegmentsFlipXAxis(kInput)), kInput);
}
