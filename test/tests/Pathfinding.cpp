#include "TestData.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <memory>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/String.hpp>
#include <openrct2/entity/Guest.h>
#include <openrct2/entity/Staff.h>
#include <openrct2/peep/GuestPathfinding.h>
#include <openrct2/platform/Platform.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <string>

using namespace OpenRCT2;

static std::ostream& operator<<(std::ostream& os, const TileCoordsXYZ& coords)
{
    return os << "(" << coords.x << ", " << coords.y << ", " << coords.z << ")";
}

class PathfindingTestBase : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;
        _context = CreateContext();
        const bool initialised = _context->Initialise();
        ASSERT_TRUE(initialised);

        std::string parkPath = TestData::GetParkPath("pathfinding-tests.sv6");
        GetContext()->LoadParkFromFile(parkPath);
        GameLoadInit();
    }

    void SetUp() override
    {
        // Use a consistent random seed in every test
        ScenarioRandSeed(0x12345678, 0x87654321);
    }

    static void TearDownTestCase()
    {
        _context = nullptr;
    }

protected:
    static Ride* FindRideByName(const char* name)
    {
        auto& gameState = getGameState();
        for (auto& ride : RideManager(gameState))
        {
            auto thisName = ride.getName();
            if (String::startsWith(thisName, u8string{ name }, true))
            {
                return &ride;
            }
        }
        return nullptr;
    }

    static bool FindPath(TileCoordsXYZ* pos, const TileCoordsXYZ& goal, int expectedSteps, RideId targetRideID)
    {
        // Our start position is in tile coordinates, but we need to give the peep spawn
        // position in actual world coords (32 units per tile X/Y, 8 per Z level).
        // Add 16 so the peep spawns in the centre of the tile.
        auto* peep = Guest::Generate(pos->ToCoordsXYZ().ToTileCentre());

        // Peeps that are outside of the park use specialized pathfinding which we don't want to
        // use here
        peep->OutsideOfPark = false;

        // An earlier iteration of this code just gave peeps a target position to walk to, but it turns out
        // that with no actual ride to head towards, when a peep reaches a junction they use the 'aimless'
        // pathfinder instead of pursuing their original pathfinding target. So, we always need to give them
        // an actual ride to walk to the entrance of.
        peep->GuestHeadingToRideId = targetRideID;

        // Pick the direction the peep should initially move in, given the goal position.
        // This will also store the goal position and initialize pathfinding data for the peep.
        const Direction moveDir = PathFinding::ChooseDirection(*pos, goal, *peep, false, RideId::GetNull());
        if (moveDir == kInvalidDirection)
        {
            // Couldn't determine a direction to move off in
            return false;
        }

        // We have already set up the peep's overall pathfinding goal, but we also have to set their initial
        // 'destination' which is a close position that they will walk towards in a straight line - in this case, one
        // tile away. Stepping the peep will move them towards their destination, and once they reach it, a new
        // destination will be picked, to try and get the peep towards the overall pathfinding goal.
        peep->PeepDirection = moveDir;
        auto destination = CoordsDirectionDelta[moveDir] + peep->getLocation();
        peep->SetDestination(destination, 2);

        // Repeatedly step the peep, until they reach the target position or until the expected number of steps have
        // elapsed. Each step, check that the tile they are standing on is not marked as forbidden in the test data
        // (red neon ground type).
        int step = 0;
        while (*pos != goal && step < expectedSteps)
        {
            peep->PerformNextAction();
            ++step;

            *pos = TileCoordsXYZ(peep->getLocation());

            EXPECT_PRED_FORMAT1(AssertIsNotForbiddenPosition, *pos);

            // Check that the peep is still on a footpath. Use next_z instead of pos->z here because pos->z will change
            // when the peep is halfway up a slope, but next_z will not change until they move to the next tile.
            EXPECT_NE(MapGetFootpathElement({ pos->ToCoordsXY(), peep->NextLoc.z }), nullptr);
        }

        // Clean up the peep, because we're reusing this loaded context for all tests.
        PeepEntityRemove(peep);

        // Require that the number of steps taken is exactly what we expected. The pathfinder is supposed to be
        // deterministic, and we reset the RNG seed for each test, everything should be entirely repeatable; as
        // such a change in the number of steps taken on one of these paths needs to be reviewed. For the negative
        // tests, we will not have reached the goal but we still expect the loop to have run for the total number
        // of steps requested before giving up.
        EXPECT_EQ(step, expectedSteps);

        return *pos == goal;
    }

    static testing::AssertionResult AssertIsStartPosition(const char*, const TileCoordsXYZ& location)
    {
        const uint32_t expectedSurfaceStyle = 11u;
        const uint32_t style = MapGetSurfaceElementAt(location.ToCoordsXYZ())->GetSurfaceObjectIndex();

        if (style != expectedSurfaceStyle)
            return testing::AssertionFailure()
                << "Start location " << location << " should have surface style " << expectedSurfaceStyle
                << " but actually has style " << style
                << ". Either the test map is not set up correctly, or you got the coordinates wrong.";

        return testing::AssertionSuccess();
    }

    static testing::AssertionResult AssertIsNotForbiddenPosition(const char*, const TileCoordsXYZ& location)
    {
        const uint32_t forbiddenSurfaceStyle = 8u;

        const uint32_t style = MapGetSurfaceElementAt(location.ToCoordsXYZ())->GetSurfaceObjectIndex();

        if (style == forbiddenSurfaceStyle)
            return testing::AssertionFailure()
                << "Path traversed location " << location << ", but it is marked as a forbidden location (surface style "
                << forbiddenSurfaceStyle << "). Either the map is set up incorrectly, or the pathfinder went the wrong way.";

        return testing::AssertionSuccess();
    }

private:
    static std::shared_ptr<IContext> _context;
};

std::shared_ptr<IContext> PathfindingTestBase::_context;

struct SimplePathfindingScenario
{
    const char* name;
    TileCoordsXYZ start;
    uint32_t steps;

    SimplePathfindingScenario(const char* _name, const TileCoordsXYZ& _start, int _steps)
        : name(_name)
        , start(_start)
        , steps(_steps)
    {
    }

    static std::string ToName(const testing::TestParamInfo<SimplePathfindingScenario>& param_info)
    {
        return param_info.param.name;
    }
};

class SimplePathfindingTest : public PathfindingTestBase, public testing::WithParamInterface<SimplePathfindingScenario>
{
};

TEST_P(SimplePathfindingTest, CanFindPathFromStartToGoal)
{
    const SimplePathfindingScenario& scenario = GetParam();

    ASSERT_PRED_FORMAT1(AssertIsStartPosition, scenario.start);
    TileCoordsXYZ pos = scenario.start;

    auto ride = FindRideByName(scenario.name);
    ASSERT_NE(ride, nullptr);

    auto entrancePos = ride->getStation().Entrance;
    TileCoordsXYZ goal = TileCoordsXYZ(
        entrancePos.x - TileDirectionDelta[entrancePos.direction].x,
        entrancePos.y - TileDirectionDelta[entrancePos.direction].y, entrancePos.z);

    const auto succeeded = FindPath(&pos, goal, scenario.steps, ride->id) ? testing::AssertionSuccess()
                                                                          : testing::AssertionFailure()
            << "Failed to find path from " << scenario.start << " to " << goal << " in " << scenario.steps << " steps; reached "
            << pos << " before giving up.";

    EXPECT_TRUE(succeeded);
}

INSTANTIATE_TEST_SUITE_P(
    ForScenario, SimplePathfindingTest,
    ::testing::Values(
        SimplePathfindingScenario("StraightFlat", { 19, 15, 14 }, 24), SimplePathfindingScenario("SBend", { 15, 12, 14 }, 87),
        SimplePathfindingScenario("UBend", { 17, 9, 14 }, 87), SimplePathfindingScenario("CBend", { 14, 5, 14 }, 164),
        SimplePathfindingScenario("TwoEqualRoutes", { 9, 13, 14 }, 89),
        SimplePathfindingScenario("TwoUnequalRoutes", { 3, 13, 14 }, 89),
        SimplePathfindingScenario("StraightUpBridge", { 12, 15, 14 }, 24),
        SimplePathfindingScenario("StraightUpSlope", { 14, 15, 14 }, 24),
        SimplePathfindingScenario("SelfCrossingPath", { 6, 5, 14 }, 211)),
    SimplePathfindingScenario::ToName);

class ImpossiblePathfindingTest : public PathfindingTestBase, public testing::WithParamInterface<SimplePathfindingScenario>
{
};

TEST_P(ImpossiblePathfindingTest, CannotFindPathFromStartToGoal)
{
    const SimplePathfindingScenario& scenario = GetParam();
    TileCoordsXYZ pos = scenario.start;
    ASSERT_PRED_FORMAT1(AssertIsStartPosition, scenario.start);

    auto ride = FindRideByName(scenario.name);
    ASSERT_NE(ride, nullptr);

    auto entrancePos = ride->getStation().Entrance;
    TileCoordsXYZ goal = TileCoordsXYZ(
        entrancePos.x + TileDirectionDelta[entrancePos.direction].x,
        entrancePos.y + TileDirectionDelta[entrancePos.direction].y, entrancePos.z);

    EXPECT_FALSE(FindPath(&pos, goal, 10000, ride->id));
}

INSTANTIATE_TEST_SUITE_P(
    ForScenario, ImpossiblePathfindingTest,
    ::testing::Values(
        SimplePathfindingScenario("PathWithGap", { 1, 6, 14 }, 10000),
        SimplePathfindingScenario("PathWithFences", { 11, 6, 14 }, 10000),
        SimplePathfindingScenario("PathWithCliff", { 7, 17, 14 }, 10000)),
    SimplePathfindingScenario::ToName);

// The A* pathfinder restricts mechanics to their patrol area (see
// ForEachWalkableNeighbour in GuestPathfinding.cpp). These tests verify a mechanic
// finds a route when its patrol area covers the path, but not when the goal lies
// outside it. They run with A* pathfinding enabled, since that restriction only
// applies to the A* pathfinder.
class MechanicPatrolPathfindingTest : public PathfindingTestBase
{
protected:
    void SetUp() override
    {
        PathfindingTestBase::SetUp();
        _previousAStarSetting = Config::Get().general.useAStarPathfinding;
        Config::Get().general.useAStarPathfinding = true;
    }

    void TearDown() override
    {
        Config::Get().general.useAStarPathfinding = _previousAStarSetting;
    }

    // Staff::IsLocationInPatrol() requires owned land, so own the whole map to isolate
    // the patrol-area restriction under test from the land-ownership check.
    static void OwnEntireMap()
    {
        const auto mapSize = getGameState().mapSize;
        for (int32_t y = 0; y < mapSize.y; y++)
        {
            for (int32_t x = 0; x < mapSize.x; x++)
            {
                auto* surface = MapGetSurfaceElementAt(TileCoordsXY{ x, y });
                if (surface != nullptr)
                    surface->SetOwnership(OWNERSHIP_OWNED);
            }
        }
    }

    static Staff* CreateMechanic()
    {
        auto* mechanic = getGameState().entities.CreateEntity<Staff>();
        mechanic->AssignedStaffType = StaffType::mechanic;
        mechanic->ResetPathfindGoal();
        return mechanic;
    }

    // Inclusive tile bounding box of a and b, expanded by `margin` tiles, in world coords.
    static MapRange TileBox(const TileCoordsXY& a, const TileCoordsXY& b, int32_t margin)
    {
        const int32_t x1 = (std::min(a.x, b.x) - margin) * kCoordsXYStep;
        const int32_t y1 = (std::min(a.y, b.y) - margin) * kCoordsXYStep;
        const int32_t x2 = (std::max(a.x, b.x) + margin) * kCoordsXYStep;
        const int32_t y2 = (std::max(a.y, b.y) + margin) * kCoordsXYStep;
        return MapRange(CoordsXY{ x1, y1 }, CoordsXY{ x2, y2 });
    }

    bool _previousAStarSetting = false;
};

TEST_F(MechanicPatrolPathfindingTest, MechanicStaysWithinPatrolArea)
{
    OwnEntireMap();

    // Use the straight-path scenario: a clear route between a start tile and the
    // entrance of the "StraightFlat" ride (the same layout the guest tests use).
    const TileCoordsXYZ start{ 19, 15, 14 };
    ASSERT_PRED_FORMAT1(AssertIsStartPosition, start);

    auto* ride = FindRideByName("StraightFlat");
    ASSERT_NE(ride, nullptr);

    const auto entrancePos = ride->getStation().Entrance;
    const TileCoordsXYZ goal(
        entrancePos.x - TileDirectionDelta[entrancePos.direction].x,
        entrancePos.y - TileDirectionDelta[entrancePos.direction].y, entrancePos.z);

    // 1. A patrol area covering the whole route: the mechanic can pathfind.
    {
        auto* mechanic = CreateMechanic();
        mechanic->SetPatrolArea(TileBox(start, goal, 4), true);

        const Direction dir = PathFinding::ChooseDirection(start, goal, *mechanic, false, RideId::GetNull());
        EXPECT_NE(dir, kInvalidDirection) << "Mechanic should find a route when its patrol area covers the path.";

        PeepEntityRemove(mechanic);
    }

    // 2. A patrol area that covers the route but seals off the tiles around the goal:
    //    the mechanic cannot step into the goal's neighbourhood, so no route reaches it.
    {
        auto* mechanic = CreateMechanic();
        mechanic->SetPatrolArea(TileBox(start, goal, 4), true);
        mechanic->SetPatrolArea(TileBox({ goal.x, goal.y }, { goal.x, goal.y }, 1), false);

        const Direction dir = PathFinding::ChooseDirection(start, goal, *mechanic, false, RideId::GetNull());
        EXPECT_EQ(dir, kInvalidDirection) << "Mechanic should not find a route that leaves its patrol area.";

        PeepEntityRemove(mechanic);
    }
}
