/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/paint/track/TrackPaintGeneric.h>

using OpenRCT2::createSpriteMap;

TEST(SpriteMapTests, AllFalseProducesZero_BothFlipModes)
{
    constexpr bool kArr[kNumOrthogonalDirections][1][1] = {};
    EXPECT_EQ(createSpriteMap<false>(kArr), 0u);
    EXPECT_EQ(createSpriteMap<true>(kArr), 0u);
}

TEST(SpriteMapTests, AllTrueOnOneByOneArrayLightsTheFourDirectionBits)
{
    constexpr bool kArr[kNumOrthogonalDirections][1][1] = { { { true } }, { { true } }, { { true } }, { { true } } };
    EXPECT_EQ(createSpriteMap<false>(kArr), 0b1111u);
    EXPECT_EQ(createSpriteMap<true>(kArr), 0b1111u);
}

TEST(SpriteMapTests, SingleBitOnDirectionAxisLandsAtBitEqualToDirection_NoFlip)
{
    for (size_t d = 0; d < kNumOrthogonalDirections; ++d)
    {
        bool arr[kNumOrthogonalDirections][1][1] = {};
        arr[d][0][0] = true;
        EXPECT_EQ(createSpriteMap<false>(arr), uint64_t{ 1 } << d) << "direction=" << d;
    }
}

TEST(SpriteMapTests, SingleBitOnDirectionAxisLandsAtBitEqualToReversedDirection_WithFlip)
{
    for (size_t d = 0; d < kNumOrthogonalDirections; ++d)
    {
        bool arr[kNumOrthogonalDirections][1][1] = {};
        arr[d][0][0] = true;
        EXPECT_EQ(createSpriteMap<true>(arr), uint64_t{ 1 } << (kNumOrthogonalDirections - 1 - d)) << "direction=" << d;
    }
}

TEST(SpriteMapTests, SingleBitOnSequenceAxisLandsAtBitEqualToSequenceIndex_NoFlip)
{
    for (size_t s = 0; s < 3; ++s)
    {
        bool arr[kNumOrthogonalDirections][3][1] = {};
        arr[0][s][0] = true;
        EXPECT_EQ(createSpriteMap<false>(arr), uint64_t{ 1 } << s) << "sequence=" << s;
    }
}

TEST(SpriteMapTests, SingleBitOnSpriteAxisLandsAtBitEqualToSpriteIndex_NoFlip)
{
    for (size_t p = 0; p < 3; ++p)
    {
        bool arr[kNumOrthogonalDirections][1][3] = {};
        arr[0][0][p] = true;
        EXPECT_EQ(createSpriteMap<false>(arr), uint64_t{ 1 } << p) << "sprite=" << p;
    }
}

TEST(SpriteMapTests, BitLayoutFormulaInTwoByTwoArrayCombinesAllThreeStrides)
{
    for (size_t d = 0; d < kNumOrthogonalDirections; ++d)
    {
        for (size_t s = 0; s < 2; ++s)
        {
            for (size_t p = 0; p < 2; ++p)
            {
                bool arr[kNumOrthogonalDirections][2][2] = {};
                arr[d][s][p] = true;
                const uint64_t expected = uint64_t{ 1 } << (p + s * 2 + d * 4);
                EXPECT_EQ(createSpriteMap<false>(arr), expected) << "d=" << d << " s=" << s << " p=" << p;
            }
        }
    }
}

TEST(SpriteMapTests, MultipleSetCellsCombineAsTheOrOfTheirIndividualMaps)
{
    bool both[kNumOrthogonalDirections][2][2] = {};
    both[0][0][0] = true;
    both[2][1][1] = true;

    bool first[kNumOrthogonalDirections][2][2] = {};
    first[0][0][0] = true;

    bool second[kNumOrthogonalDirections][2][2] = {};
    second[2][1][1] = true;

    EXPECT_EQ(createSpriteMap<false>(both), createSpriteMap<false>(first) | createSpriteMap<false>(second));
}

TEST(SpriteMapTests, FlipIsEquivalentToReversingDirectionAxisOfTheInput)
{
    bool original[kNumOrthogonalDirections][2][2] = {};
    original[0][0][0] = true;
    original[2][1][0] = true;
    original[3][0][1] = true;

    bool reversed[kNumOrthogonalDirections][2][2] = {};
    for (size_t d = 0; d < kNumOrthogonalDirections; ++d)
        for (size_t s = 0; s < 2; ++s)
            for (size_t p = 0; p < 2; ++p)
                reversed[kNumOrthogonalDirections - 1 - d][s][p] = original[d][s][p];

    EXPECT_EQ(createSpriteMap<true>(original), createSpriteMap<false>(reversed));
}

TEST(SpriteMapTests, ConcreteTwoByTwoExampleMatchesHandComputedBitmaps)
{
    bool arr[kNumOrthogonalDirections][2][2] = {};
    arr[0][0][0] = true;
    arr[2][1][1] = true;
    arr[3][1][0] = true;

    EXPECT_EQ(createSpriteMap<false>(arr), (uint64_t{ 1 } << 0) | (uint64_t{ 1 } << 11) | (uint64_t{ 1 } << 14));
    EXPECT_EQ(createSpriteMap<true>(arr), (uint64_t{ 1 } << 2) | (uint64_t{ 1 } << 7) | (uint64_t{ 1 } << 12));
}
