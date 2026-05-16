/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/drawing/InvalidationGrid.h>
#include <vector>

using OpenRCT2::Drawing::InvalidationGrid;

namespace
{
    struct DirtyRect
    {
        uint32_t left;
        uint32_t top;
        uint32_t right;
        uint32_t bottom;

        bool operator==(const DirtyRect& other) const noexcept
        {
            return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
        }
    };

    std::vector<DirtyRect> Collect(InvalidationGrid& grid)
    {
        std::vector<DirtyRect> rects;
        grid.traverseDirtyCells([&](const uint32_t l, const uint32_t t, const uint32_t r, const uint32_t b) {
            rects.push_back({ l, t, r, b });
        });
        return rects;
    }
} // namespace

TEST(InvalidationGridTests, Reset_PopulatesAccessorsAndAddsOneOverflowRowAndColumn)
{
    InvalidationGrid grid;
    grid.reset(100, 80, 10, 20);
    EXPECT_EQ(grid.getBlockWidth(), 10u);
    EXPECT_EQ(grid.getBlockHeight(), 20u);
    EXPECT_EQ(grid.getColumnCount(), 100u / 10u + 1u);
    EXPECT_EQ(grid.getRowCount(), 80u / 20u + 1u);
}

TEST(InvalidationGridTests, Reset_CanBeCalledAgainWithDifferentDimensions)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);
    grid.reset(50, 30, 5, 5);
    EXPECT_EQ(grid.getBlockWidth(), 5u);
    EXPECT_EQ(grid.getBlockHeight(), 5u);
    EXPECT_EQ(grid.getColumnCount(), 50u / 5u + 1u);
    EXPECT_EQ(grid.getRowCount(), 30u / 5u + 1u);
}

TEST(InvalidationGridTests, FreshGridHasNoDirtyCells)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);
    EXPECT_TRUE(Collect(grid).empty());
}

TEST(InvalidationGridTests, Invalidate_SingleRectEmitsOneBlockAlignedRect)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);

    grid.invalidate(5, 5, 25, 25);

    const auto rects = Collect(grid);
    ASSERT_EQ(rects.size(), 1u);
    EXPECT_EQ(rects[0], (DirtyRect{ 0, 0, 30, 30 }));
}

TEST(InvalidationGridTests, Invalidate_DegenerateRectEmitsNothing)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);

    grid.invalidate(20, 20, 20, 50);
    grid.invalidate(20, 50, 50, 50);
    grid.invalidate(40, 40, 20, 60);
    grid.invalidate(40, 60, 60, 40);

    EXPECT_TRUE(Collect(grid).empty());
}

TEST(InvalidationGridTests, Invalidate_FullyOffScreenRectIsCulled)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);

    grid.invalidate(-100, -100, -50, -50);
    grid.invalidate(150, 150, 200, 200);

    EXPECT_TRUE(Collect(grid).empty());
}

TEST(InvalidationGridTests, Invalidate_PartiallyOffScreenRectIsClampedToScreen)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);

    grid.invalidate(-20, -20, 5, 5);
    grid.invalidate(95, 95, 200, 200);

    const auto rects = Collect(grid);
    ASSERT_EQ(rects.size(), 2u);
    EXPECT_EQ(rects[0], (DirtyRect{ 0, 0, 10, 10 }));
    EXPECT_EQ(rects[1], (DirtyRect{ 90, 90, 100, 100 }));
}

TEST(InvalidationGridTests, Traverse_MergesTwoHorizontallyAdjacentBlocksIntoOneRect)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);

    grid.invalidate(0, 0, 10, 10);
    grid.invalidate(10, 0, 20, 10);

    const auto rects = Collect(grid);
    ASSERT_EQ(rects.size(), 1u);
    EXPECT_EQ(rects[0], (DirtyRect{ 0, 0, 20, 10 }));
}

TEST(InvalidationGridTests, Traverse_MergesTwoVerticallyAdjacentBlocksIntoOneRect)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);

    grid.invalidate(0, 0, 10, 10);
    grid.invalidate(0, 10, 10, 20);

    const auto rects = Collect(grid);
    ASSERT_EQ(rects.size(), 1u);
    EXPECT_EQ(rects[0], (DirtyRect{ 0, 0, 10, 20 }));
}

TEST(InvalidationGridTests, Traverse_LShapeEmitsTwoRectsBecauseVerticalMergeRequiresMatchingWidth)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);

    grid.invalidate(0, 0, 20, 10);
    grid.invalidate(0, 10, 10, 20);

    const auto rects = Collect(grid);
    ASSERT_EQ(rects.size(), 2u);
    EXPECT_EQ(rects[0], (DirtyRect{ 0, 0, 10, 20 }));
    EXPECT_EQ(rects[1], (DirtyRect{ 10, 0, 20, 10 }));
}

TEST(InvalidationGridTests, Traverse_ClearsDirtyStateSoSecondCallEmitsNothing)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);

    grid.invalidate(5, 5, 25, 25);
    EXPECT_EQ(Collect(grid).size(), 1u);
    EXPECT_TRUE(Collect(grid).empty());
}

TEST(InvalidationGridTests, Reset_ClearsLeftoverDirtyBlocks)
{
    InvalidationGrid grid;
    grid.reset(100, 100, 10, 10);
    grid.invalidate(5, 5, 25, 25);

    grid.reset(100, 100, 10, 10);

    EXPECT_TRUE(Collect(grid).empty());
}

TEST(InvalidationGridTests, Traverse_ClampsRightAndBottomCoordinatesToScreen)
{
    InvalidationGrid grid;
    grid.reset(95, 95, 10, 10);

    grid.invalidate(90, 90, 95, 95);

    const auto rects = Collect(grid);
    ASSERT_EQ(rects.size(), 1u);
    EXPECT_EQ(rects[0], (DirtyRect{ 90, 90, 95, 95 }));
}
