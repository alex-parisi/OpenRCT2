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
#include <openrct2/world/MapSelection.h>

using namespace OpenRCT2;

// The test fixture wipes globals and tile vectors before each case.
namespace
{
    class MapSelectionTests : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            gMapSelectFlags = {};
            gMapSelectType = MapSelectType::corner0;
            gMapSelectPositionA = {};
            gMapSelectPositionB = {};
            gMapSelectArrowPosition = {};
            gMapSelectArrowDirection = 0;
            MapSelection::clearSelectedTiles();
        }
    };
} // namespace

TEST(MapSelectionHelpers, GetCornerForEachDirection)
{
    EXPECT_EQ(getMapSelectCorner(0), MapSelectType::corner0);
    EXPECT_EQ(getMapSelectCorner(1), MapSelectType::corner1);
    EXPECT_EQ(getMapSelectCorner(2), MapSelectType::corner2);
    EXPECT_EQ(getMapSelectCorner(3), MapSelectType::corner3);
}

TEST(MapSelectionHelpers, GetQuarterForEachDirection)
{
    EXPECT_EQ(getMapSelectQuarter(0), MapSelectType::quarter0);
    EXPECT_EQ(getMapSelectQuarter(1), MapSelectType::quarter1);
    EXPECT_EQ(getMapSelectQuarter(2), MapSelectType::quarter2);
    EXPECT_EQ(getMapSelectQuarter(3), MapSelectType::quarter3);
}

TEST(MapSelectionHelpers, GetEdgeForEachDirection)
{
    EXPECT_EQ(getMapSelectEdge(0), MapSelectType::edge0);
    EXPECT_EQ(getMapSelectEdge(1), MapSelectType::edge1);
    EXPECT_EQ(getMapSelectEdge(2), MapSelectType::edge2);
    EXPECT_EQ(getMapSelectEdge(3), MapSelectType::edge3);
}

TEST_F(MapSelectionTests, GetMapSelectRange_ReturnsCurrentGlobals)
{
    gMapSelectPositionA = { 32, 64 };
    gMapSelectPositionB = { 128, 256 };

    const auto range = getMapSelectRange();
    EXPECT_EQ(range.Point1, CoordsXY(32, 64));
    EXPECT_EQ(range.Point2, CoordsXY(128, 256));
}

TEST_F(MapSelectionTests, SetMapSelectRange_NormalisesSwappedCorners)
{
    setMapSelectRange(MapRange{ 100, 200, 50, 80 });
    EXPECT_EQ(gMapSelectPositionA, CoordsXY(50, 80));
    EXPECT_EQ(gMapSelectPositionB, CoordsXY(100, 200));
}

TEST_F(MapSelectionTests, SetMapSelectRange_AlreadyOrderedIsUnchanged)
{
    setMapSelectRange(MapRange{ 10, 20, 30, 40 });
    EXPECT_EQ(gMapSelectPositionA, CoordsXY(10, 20));
    EXPECT_EQ(gMapSelectPositionB, CoordsXY(30, 40));
}

TEST_F(MapSelectionTests, SetMapSelectRange_MixedAxisIsNormalisedPerAxis)
{
    setMapSelectRange(MapRange{ 10, 200, 30, 50 });
    EXPECT_EQ(gMapSelectPositionA, CoordsXY(10, 50));
    EXPECT_EQ(gMapSelectPositionB, CoordsXY(30, 200));
}

TEST_F(MapSelectionTests, SetMapSelectRange_SingleCoordCollapsesBothCorners)
{
    setMapSelectRange(CoordsXY{ 64, 96 });
    EXPECT_EQ(gMapSelectPositionA, CoordsXY(64, 96));
    EXPECT_EQ(gMapSelectPositionB, CoordsXY(64, 96));
}

TEST_F(MapSelectionTests, AddSelectedTile_PreservesInsertionOrder)
{
    MapSelection::addSelectedTile({ 0, 0 });
    MapSelection::addSelectedTile({ 32, 0 });
    MapSelection::addSelectedTile({ 32, 32 });

    const auto& tiles = MapSelection::getSelectedTiles();
    ASSERT_EQ(tiles.size(), 3u);
    EXPECT_EQ(tiles[0], CoordsXY(0, 0));
    EXPECT_EQ(tiles[1], CoordsXY(32, 0));
    EXPECT_EQ(tiles[2], CoordsXY(32, 32));
}

TEST_F(MapSelectionTests, ClearSelectedTiles_EmptiesTheVector)
{
    MapSelection::addSelectedTile({ 0, 0 });
    MapSelection::addSelectedTile({ 32, 0 });
    ASSERT_FALSE(MapSelection::getSelectedTiles().empty());

    MapSelection::clearSelectedTiles();
    EXPECT_TRUE(MapSelection::getSelectedTiles().empty());
}

TEST_F(MapSelectionTests, GetSelectedTiles_EmptyByDefault)
{
    EXPECT_TRUE(MapSelection::getSelectedTiles().empty());
}

TEST_F(MapSelectionTests, Invalidate_NoOpWhenNothingChangedAndNoFlagsSet)
{
    MapSelection::invalidate();
    SUCCEED();
}

TEST_F(MapSelectionTests, Invalidate_HandlesTransitionFromDisabledToEnabled)
{
    gMapSelectFlags.set(MapSelectFlag::enable);
    gMapSelectPositionA = { 0, 0 };
    gMapSelectPositionB = { 64, 64 };
    MapSelection::invalidate();
    SUCCEED();
}

TEST_F(MapSelectionTests, Invalidate_HandlesArrowEnableAndPositionChange)
{
    gMapSelectFlags.set(MapSelectFlag::enableArrow);
    gMapSelectArrowPosition = { 32, 32, 16 };
    MapSelection::invalidate();

    gMapSelectArrowPosition = { 64, 64, 16 };
    MapSelection::invalidate();
    SUCCEED();
}
