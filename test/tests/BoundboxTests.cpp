/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <array>
#include <gtest/gtest.h>
#include <openrct2/paint/Boundbox.h>
#include <openrct2/world/Location.hpp>
#include <tuple>

namespace
{
    // Field-by-field comparison via tuple so EXPECT_EQ gives a useful diff on failure
    using BoxTuple = std::tuple<int32_t, int32_t, int32_t, int32_t, int32_t, int32_t>;

    constexpr BoxTuple Flatten(const BoundBoxXYZ& b)
    {
        return { b.offset.x, b.offset.y, b.offset.z, b.length.x, b.length.y, b.length.z };
    }
} // namespace

TEST(BoundboxTests, BoundBoxXY_DefaultConstructorZeroInitializesOffsetAndLength)
{
    constexpr BoundBoxXY kBox{};
    EXPECT_EQ(kBox.offset.x, 0);
    EXPECT_EQ(kBox.offset.y, 0);
    EXPECT_EQ(kBox.length.x, 0);
    EXPECT_EQ(kBox.length.y, 0);
}

TEST(BoundboxTests, BoundBoxXY_TwoArgConstructorStoresArgsVerbatim)
{
    constexpr BoundBoxXY kBox{ { 1, 2 }, { 3, 4 } };
    EXPECT_EQ(kBox.offset.x, 1);
    EXPECT_EQ(kBox.offset.y, 2);
    EXPECT_EQ(kBox.length.x, 3);
    EXPECT_EQ(kBox.length.y, 4);
}

TEST(BoundboxTests, BoundBoxXYZ_DefaultConstructorZeroInitializesAllFields)
{
    constexpr BoundBoxXYZ kBox{};
    EXPECT_EQ(Flatten(kBox), BoxTuple(0, 0, 0, 0, 0, 0));
}

TEST(BoundboxTests, BoundBoxXYZ_TwoArgConstructorStoresArgsVerbatim)
{
    constexpr BoundBoxXYZ kBox{ { 1, 2, 3 }, { 4, 5, 6 } };
    EXPECT_EQ(Flatten(kBox), BoxTuple(1, 2, 3, 4, 5, 6));
}

TEST(BoundboxTests, KBoundingBoxUnimplemented_IsAllZero)
{
    EXPECT_EQ(Flatten(kBoundingBoxUnimplemented), BoxTuple(0, 0, 0, 0, 0, 0));
}

TEST(BoundboxTests, Flip_ReversesDirectionViewsAndSwapsXYOfEveryBox)
{
    using Array = std::array<std::array<std::array<BoundBoxXYZ, 1>, 1>, kNumOrthogonalDirections>;
    Array input{};
    input[0][0][0] = BoundBoxXYZ({ 10, 20, 30 }, { 40, 50, 60 });
    input[1][0][0] = BoundBoxXYZ({ 11, 21, 31 }, { 41, 51, 61 });
    input[2][0][0] = BoundBoxXYZ({ 12, 22, 32 }, { 42, 52, 62 });
    input[3][0][0] = BoundBoxXYZ({ 13, 23, 33 }, { 43, 53, 63 });

    const auto flipped = flipTrackSequenceBoundBoxesXAxis(input);

    for (size_t v = 0; v < kNumOrthogonalDirections; ++v)
    {
        const auto& src = input[kNumOrthogonalDirections - 1 - v][0][0];
        const auto& dst = flipped[v][0][0];
        EXPECT_EQ(dst.offset.x, src.offset.y) << "view=" << v;
        EXPECT_EQ(dst.offset.y, src.offset.x) << "view=" << v;
        EXPECT_EQ(dst.length.x, src.length.y) << "view=" << v;
        EXPECT_EQ(dst.length.y, src.length.x) << "view=" << v;
    }
}

TEST(BoundboxTests, Flip_PreservesZCoordinatesOfBothOffsetAndLength)
{
    using Array = std::array<std::array<std::array<BoundBoxXYZ, 1>, 1>, kNumOrthogonalDirections>;
    Array input{};
    input[0][0][0] = BoundBoxXYZ({ 0, 0, 100 }, { 0, 0, 200 });
    input[1][0][0] = BoundBoxXYZ({ 0, 0, 101 }, { 0, 0, 201 });
    input[2][0][0] = BoundBoxXYZ({ 0, 0, 102 }, { 0, 0, 202 });
    input[3][0][0] = BoundBoxXYZ({ 0, 0, 103 }, { 0, 0, 203 });

    const auto flipped = flipTrackSequenceBoundBoxesXAxis(input);

    EXPECT_EQ(flipped[0][0][0].offset.z, 103);
    EXPECT_EQ(flipped[1][0][0].offset.z, 102);
    EXPECT_EQ(flipped[2][0][0].offset.z, 101);
    EXPECT_EQ(flipped[3][0][0].offset.z, 100);
    EXPECT_EQ(flipped[0][0][0].length.z, 203);
    EXPECT_EQ(flipped[1][0][0].length.z, 202);
    EXPECT_EQ(flipped[2][0][0].length.z, 201);
    EXPECT_EQ(flipped[3][0][0].length.z, 200);
}

TEST(BoundboxTests, Flip_PreservesTrackSequenceOrderWithinEachView)
{
    using Array = std::array<std::array<std::array<BoundBoxXYZ, 1>, 3>, kNumOrthogonalDirections>;
    Array input{};
    for (size_t v = 0; v < kNumOrthogonalDirections; ++v)
    {
        for (size_t s = 0; s < 3; ++s)
        {
            input[v][s][0] = BoundBoxXYZ({ static_cast<int32_t>(s), 0, 0 }, { 0, 0, 0 });
        }
    }

    const auto flipped = flipTrackSequenceBoundBoxesXAxis(input);

    for (size_t v = 0; v < kNumOrthogonalDirections; ++v)
    {
        for (size_t s = 0; s < 3; ++s)
        {
            EXPECT_EQ(flipped[v][s][0].offset.y, static_cast<int32_t>(s)) << "v=" << v << " s=" << s;
        }
    }
}

TEST(BoundboxTests, Flip_PreservesSpriteOrderWithinEachSequence)
{
    using Array = std::array<std::array<std::array<BoundBoxXYZ, 3>, 1>, kNumOrthogonalDirections>;
    Array input{};
    for (size_t v = 0; v < kNumOrthogonalDirections; ++v)
    {
        for (size_t p = 0; p < 3; ++p)
        {
            input[v][0][p] = BoundBoxXYZ({ static_cast<int32_t>(p), 0, 0 }, { 0, 0, 0 });
        }
    }

    const auto flipped = flipTrackSequenceBoundBoxesXAxis(input);

    for (size_t v = 0; v < kNumOrthogonalDirections; ++v)
    {
        for (size_t p = 0; p < 3; ++p)
        {
            EXPECT_EQ(flipped[v][0][p].offset.y, static_cast<int32_t>(p)) << "v=" << v << " p=" << p;
        }
    }
}

TEST(BoundboxTests, Flip_AppliedTwiceYieldsTheOriginalArray)
{
    using Array = std::array<std::array<std::array<BoundBoxXYZ, 2>, 2>, kNumOrthogonalDirections>;
    Array input{};
    for (size_t v = 0; v < kNumOrthogonalDirections; ++v)
    {
        for (size_t s = 0; s < 2; ++s)
        {
            for (size_t p = 0; p < 2; ++p)
            {
                const int32_t base = static_cast<int32_t>(v * 100 + s * 10 + p);
                input[v][s][p] = BoundBoxXYZ({ base + 1, base + 2, base + 3 }, { base + 4, base + 5, base + 6 });
            }
        }
    }

    const auto twice = flipTrackSequenceBoundBoxesXAxis(flipTrackSequenceBoundBoxesXAxis(input));

    for (size_t v = 0; v < kNumOrthogonalDirections; ++v)
    {
        for (size_t s = 0; s < 2; ++s)
        {
            for (size_t p = 0; p < 2; ++p)
            {
                EXPECT_EQ(Flatten(twice[v][s][p]), Flatten(input[v][s][p])) << "v=" << v << " s=" << s << " p=" << p;
            }
        }
    }
}
