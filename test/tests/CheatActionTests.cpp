/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "TestData.h"

#include <gtest/gtest.h>
#include <memory>
#include <openrct2/Cheats.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/ParkImporter.h>
#include <openrct2/actions/GameActionParameterVisitor.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/cheats/CheatSetAction.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/entity/Duck.h>
#include <openrct2/entity/EntityRegistry.h>
#include <openrct2/entity/EntityTweener.h>
#include <openrct2/entity/Guest.h>
#include <openrct2/entity/Litter.h>
#include <openrct2/entity/Peep.h>
#include <openrct2/entity/Staff.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/scenario/ScenarioObjective.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/MapAnimation.h>
#include <openrct2/world/Park.h>
#include <openrct2/world/Weather.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <string>

using namespace OpenRCT2;

namespace
{
    // Parses the park file and loads its required objects, then imports once into the live GameState.
    void loadParkOnce(IContext& context, const std::string& parkPath)
    {
        const auto importer = ParkImporter::CreateS6(context.GetObjectRepository());
        const auto loadResult = importer->LoadSavedGame(parkPath.c_str(), false);
        context.GetObjectManager().LoadObjects(loadResult.RequiredObjects);

        MapAnimations::ClearAll();
        auto& gameState = getGameState();
        importer->Import(gameState);
        gameState.entities.ResetEntitySpatialIndices();
        ResetAllSpriteQuadrantPlacements();
        LoadPalette();
        EntityTweener::Get().Reset();
        MapAnimations::MarkAllTiles();
        FixInvalidVehicleSpriteSizes();
        gGameSpeed = 1;
    }

    // Field-by-field clone of GameState_t. The default copy constructor is deleted
    // because Ride::measurement is a std::unique_ptr; we work around it with a
    // bytewise copy of the rides array (safe because nothing in the freshly-imported
    // fixture populates `measurement`, so the bytes being copied represent a null
    // pointer).
    void cloneGameState(const GameState_t& src, GameState_t& dst)
    {
        dst.park = src.park;
        dst.scenarioOptions = src.scenarioOptions;
        dst.pluginStorage = src.pluginStorage;
        dst.currentTicks = src.currentTicks;
        dst.date = src.date;
        dst.weatherCurrent = src.weatherCurrent;
        dst.weatherNext = src.weatherNext;
        dst.weatherUpdateTimer = src.weatherUpdateTimer;
        dst.nextGuestNumber = src.nextGuestNumber;
        dst.scenarioParkRatingWarningDays = src.scenarioParkRatingWarningDays;
        dst.scenarioCompletedCompanyValue = src.scenarioCompletedCompanyValue;
        dst.scenarioCompanyValueRecord = src.scenarioCompanyValueRecord;
        // scenarioRand (RotateEngine) has a user-provided copy constructor but its
        // implicit copy assignment is deprecated; bytewise copy avoids the warning.
        std::memcpy(&dst.scenarioRand, &src.scenarioRand, sizeof(src.scenarioRand));
        dst.mapSize = src.mapSize;
        dst.editorStep = src.editorStep;
        dst.scenarioCompletedBy = src.scenarioCompletedBy;
        dst.scenarioFileName = src.scenarioFileName;
        dst.banners = src.banners;
        dst.entities = src.entities;
        dst.ridesEndOfUsedRange = src.ridesEndOfUsedRange;
        dst.rideRatingUpdateStates = src.rideRatingUpdateStates;
        dst.tileElements = src.tileElements;
        dst.restrictedScenery = src.restrictedScenery;
        dst.peepSpawns = src.peepSpawns;
        dst.newsItems = src.newsItems;
        dst.grassSceneryTileLoopPosition = src.grassSceneryTileLoopPosition;
        dst.widePathTileLoopPosition = src.widePathTileLoopPosition;
        dst.researchFundingLevel = src.researchFundingLevel;
        dst.researchPriorities = src.researchPriorities;
        dst.researchProgress = src.researchProgress;
        dst.researchProgressStage = src.researchProgressStage;
        dst.researchExpectedMonth = src.researchExpectedMonth;
        dst.researchExpectedDay = src.researchExpectedDay;
        dst.researchLastItem = src.researchLastItem;
        dst.researchNextItem = src.researchNextItem;
        dst.researchItemsUninvented = src.researchItemsUninvented;
        dst.researchItemsInvented = src.researchItemsInvented;
        dst.researchUncompletedCategories = src.researchUncompletedCategories;
        dst.savedView = src.savedView;
        dst.savedViewRotation = src.savedViewRotation;
        dst.savedViewZoom = src.savedViewZoom;
        dst.lastEntranceStyle = src.lastEntranceStyle;
        dst.cheats = src.cheats;

        for (size_t i = 0; i < src.rides.size(); i++)
        {
            // The default copy of Ride is deleted because it owns a unique_ptr
            // (measurement). We bytewise-copy the bulk of the ride and require the
            // snapshot to have null measurement pointers (enforced in SetUpTestSuite)
            // so the bits we're overwriting don't represent a live resource.
            dst.rides[i].measurement.reset();
            std::memcpy(&dst.rides[i], &src.rides[i], sizeof(Ride));
        }
    }

    // Rewinds GameState to the post-import snapshot and rebuilds the static indices that hold raw pointers into the
    // game's state.
    void restoreFromSnapshot(const GameState_t& snapshot)
    {
        auto& gameState = getGameState();
        cloneGameState(snapshot, gameState);

        std::vector<TileElement> tileElementsCopy = snapshot.tileElements;
        SetTileElements(gameState, std::move(tileElementsCopy));

        gameState.entities.ResetEntitySpatialIndices();
        ResetAllSpriteQuadrantPlacements();
        MapAnimations::ClearAll();
        MapAnimations::MarkAllTiles();
        EntityTweener::Get().Reset();
        FixInvalidVehicleSpriteSizes();

        gGameSpeed = 1;
    }

    GameActions::Result executeCheat(const CheatType type, const int64_t param1 = 0, const int64_t param2 = 0)
    {
        // For unit tests we want the action body to run synchronously, so masquerade as if we're inside the update loop.
        const bool wasInUpdateCode = gInUpdateCode;
        gInUpdateCode = true;
        const GameActions::CheatSetAction action(type, param1, param2);
        auto result = GameActions::Execute(&action, getGameState());
        gInUpdateCode = wasInUpdateCode;
        return result;
    }

    GameActions::Result queryCheat(const CheatType type, const int64_t param1 = 0, const int64_t param2 = 0)
    {
        const GameActions::CheatSetAction action(type, param1, param2);
        return GameActions::Query(&action, getGameState());
    }
} // namespace

class CheatActionTests : public testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;
        _context = CreateContext();
        ASSERT_TRUE(_context->Initialise());
        loadParkOnce(*_context, TestData::GetParkPath("small_park_with_ferris_wheel.sv6"));

        // The cloneGameState memcpy assumes Ride::measurement is null on both sides.
        for (auto& ride : getGameState().rides)
        {
            ride.measurement.reset();
        }

        // Snapshot the post-load state
        _initialGameState = std::make_unique<GameState_t>();
        cloneGameState(getGameState(), *_initialGameState);
    }

    static void TearDownTestSuite()
    {
        _initialGameState.reset();
        _context.reset();
    }

    void SetUp() override
    {
        ASSERT_NE(_context.get(), nullptr);
        restoreFromSnapshot(*_initialGameState);
    }

    static inline std::unique_ptr<IContext> _context;
    static inline std::unique_ptr<GameState_t> _initialGameState;
};

TEST_F(CheatActionTests, CheatsAreAllowedWhilePaused)
{
    const GameActions::CheatSetAction action(CheatType::sandboxMode, 1);
    EXPECT_TRUE((action.GetActionFlags() & GameActions::Flags::AllowWhilePaused) != 0);
}

TEST_F(CheatActionTests, QueryRejectsInvalidCheatType)
{
    constexpr auto kInvalid = static_cast<CheatType>(static_cast<int32_t>(CheatType::count));
    const auto result = queryCheat(kInvalid);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, QueryRejectsOutOfRangeParam1)
{
    const auto result = queryCheat(CheatType::sandboxMode, 2);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, QueryRejectsOutOfRangeParam2)
{
    const auto result = queryCheat(CheatType::sandboxMode, 1, 1);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, QueryRejectsNegativeParam2)
{
    const auto result = queryCheat(CheatType::sandboxMode, 0, -1);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, QueryAcceptsValidBooleanCheat)
{
    const auto result = queryCheat(CheatType::sandboxMode, 1);
    EXPECT_EQ(result.error, GameActions::Status::ok);
}

TEST_F(CheatActionTests, ExecuteWithInvalidCheatTypeReturnsError)
{
    // To exercise the Execute switch's default case, call Execute on the action directly, bypassing the runner.
    constexpr auto kInvalid = static_cast<CheatType>(static_cast<int32_t>(CheatType::count) + 5);
    const GameActions::CheatSetAction action(kInvalid);
    auto& gameState = getGameState();
    const auto result = action.Execute(gameState, gameState.park);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, AcceptParametersVisitsAllThreeFields)
{
    struct CaptureVisitor : GameActions::GameActionParameterVisitor
    {
        std::vector<std::string> names;
        void Visit(std::string_view name, int32_t&) override
        {
            names.emplace_back(name);
        }
    };

    GameActions::CheatSetAction action(CheatType::sandboxMode, 1, 0);
    CaptureVisitor visitor;
    action.AcceptParameters(visitor);
    EXPECT_EQ(visitor.names, (std::vector<std::string>{ "type", "param1", "param2" }));
}

TEST_F(CheatActionTests, SandboxModeTogglesFlag)
{
    auto& cheats = getGameState().cheats;
    cheats.sandboxMode = false;
    executeCheat(CheatType::sandboxMode, 1);
    EXPECT_TRUE(cheats.sandboxMode);
    executeCheat(CheatType::sandboxMode, 0);
    EXPECT_FALSE(cheats.sandboxMode);
}

TEST_F(CheatActionTests, DisableClearanceChecksTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::disableClearanceChecks, 1);
    EXPECT_TRUE(cheats.disableClearanceChecks);
    executeCheat(CheatType::disableClearanceChecks, 0);
    EXPECT_FALSE(cheats.disableClearanceChecks);
}

TEST_F(CheatActionTests, DisableSupportLimitsTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::disableSupportLimits, 1);
    EXPECT_TRUE(cheats.disableSupportLimits);
}

TEST_F(CheatActionTests, ShowAllOperatingModesTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::showAllOperatingModes, 1);
    EXPECT_TRUE(cheats.showAllOperatingModes);
}

TEST_F(CheatActionTests, ShowVehiclesFromOtherTrackTypesTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::showVehiclesFromOtherTrackTypes, 1);
    EXPECT_TRUE(cheats.showVehiclesFromOtherTrackTypes);
}

TEST_F(CheatActionTests, FastLiftHillSetsUnlockOperatingLimits)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::fastLiftHill, 1);
    EXPECT_TRUE(cheats.unlockOperatingLimits);
}

TEST_F(CheatActionTests, DisableBrakesFailureTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::disableBrakesFailure, 1);
    EXPECT_TRUE(cheats.disableBrakesFailure);
}

TEST_F(CheatActionTests, DisableAllBreakdownsTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::disableAllBreakdowns, 1);
    EXPECT_TRUE(cheats.disableAllBreakdowns);
}

TEST_F(CheatActionTests, DisableTrainLengthLimitTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::disableTrainLengthLimit, 1);
    EXPECT_TRUE(cheats.disableTrainLengthLimit);
}

TEST_F(CheatActionTests, EnableChainLiftOnAllTrackTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::enableChainLiftOnAllTrack, 1);
    EXPECT_TRUE(cheats.enableChainLiftOnAllTrack);
}

TEST_F(CheatActionTests, BuildInPauseModeTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::buildInPauseMode, 1);
    EXPECT_TRUE(cheats.buildInPauseMode);
}

TEST_F(CheatActionTests, IgnoreRideIntensityTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::ignoreRideIntensity, 1);
    EXPECT_TRUE(cheats.ignoreRideIntensity);
}

TEST_F(CheatActionTests, IgnorePriceTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::ignorePrice, 1);
    EXPECT_TRUE(cheats.ignorePrice);
}

TEST_F(CheatActionTests, DisableVandalismTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::disableVandalism, 1);
    EXPECT_TRUE(cheats.disableVandalism);
}

TEST_F(CheatActionTests, DisableLitteringTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::disableLittering, 1);
    EXPECT_TRUE(cheats.disableLittering);
}

TEST_F(CheatActionTests, DisablePlantAgingTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::disablePlantAging, 1);
    EXPECT_TRUE(cheats.disablePlantAging);
}

TEST_F(CheatActionTests, MakeDestructibleTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::makeDestructible, 1);
    EXPECT_TRUE(cheats.makeAllDestructible);
}

TEST_F(CheatActionTests, FreezeWeatherTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::freezeWeather, 1);
    EXPECT_TRUE(cheats.freezeWeather);
}

TEST_F(CheatActionTests, NeverendingMarketingTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::neverendingMarketing, 1);
    EXPECT_TRUE(cheats.neverendingMarketing);
}

TEST_F(CheatActionTests, AllowArbitraryRideTypeChangesTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::allowArbitraryRideTypeChanges, 1);
    EXPECT_TRUE(cheats.allowArbitraryRideTypeChanges);
}

TEST_F(CheatActionTests, DisableRideValueAgingTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::disableRideValueAging, 1);
    EXPECT_TRUE(cheats.disableRideValueAging);
}

TEST_F(CheatActionTests, IgnoreResearchStatusTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::ignoreResearchStatus, 1);
    EXPECT_TRUE(cheats.ignoreResearchStatus);
}

TEST_F(CheatActionTests, EnableAllDrawableTrackPiecesTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::enableAllDrawableTrackPieces, 1);
    EXPECT_TRUE(cheats.enableAllDrawableTrackPieces);
}

TEST_F(CheatActionTests, AllowTrackPlaceInvalidHeightsTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::allowTrackPlaceInvalidHeights, 1);
    EXPECT_TRUE(cheats.allowTrackPlaceInvalidHeights);
}

TEST_F(CheatActionTests, AllowRegularPathAsQueueTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::allowRegularPathAsQueue, 1);
    EXPECT_TRUE(cheats.allowRegularPathAsQueue);
}

TEST_F(CheatActionTests, AllowSpecialColourSchemesTogglesFlag)
{
    const auto& cheats = getGameState().cheats;
    executeCheat(CheatType::allowSpecialColourSchemes, 1);
    EXPECT_TRUE(cheats.allowSpecialColourSchemes);
}

TEST_F(CheatActionTests, NoMoneyTogglesParkFlag)
{
    auto& park = getGameState().park;
    park.flags &= ~PARK_FLAGS_NO_MONEY;

    executeCheat(CheatType::noMoney, 1);
    EXPECT_TRUE((park.flags & PARK_FLAGS_NO_MONEY) != 0);

    executeCheat(CheatType::noMoney, 0);
    EXPECT_FALSE((park.flags & PARK_FLAGS_NO_MONEY) != 0);
}

TEST_F(CheatActionTests, SetMoneyAssignsCashExactly)
{
    const auto& park = getGameState().park;
    executeCheat(CheatType::setMoney, 12345);
    EXPECT_EQ(park.cash, 12345);

    executeCheat(CheatType::setMoney, -500);
    EXPECT_EQ(park.cash, -500);
}

TEST_F(CheatActionTests, AddMoneyAccumulatesCash)
{
    const auto& park = getGameState().park;
    executeCheat(CheatType::setMoney, 1000);
    executeCheat(CheatType::addMoney, 250);
    EXPECT_EQ(park.cash, 1250);

    executeCheat(CheatType::addMoney, -750);
    EXPECT_EQ(park.cash, 500);
}

TEST_F(CheatActionTests, ClearLoanZeroesBankLoanAndPreservesCash)
{
    auto& park = getGameState().park;
    executeCheat(CheatType::setMoney, 1000);
    park.bankLoan = 5000;

    executeCheat(CheatType::clearLoan);

    EXPECT_EQ(park.bankLoan, 0);
    EXPECT_EQ(park.cash, 1000);
}

TEST_F(CheatActionTests, OpenClosePark_TogglesOpenFlag)
{
    const auto& park = getGameState().park;
    const bool wasOpen = (park.flags & PARK_FLAGS_PARK_OPEN) != 0;

    executeCheat(CheatType::openClosePark);
    EXPECT_NE((park.flags & PARK_FLAGS_PARK_OPEN) != 0, wasOpen);

    executeCheat(CheatType::openClosePark);
    EXPECT_EQ((park.flags & PARK_FLAGS_PARK_OPEN) != 0, wasOpen);
}

TEST_F(CheatActionTests, HaveFunSetsObjectiveType)
{
    executeCheat(CheatType::haveFun);
    EXPECT_EQ(getGameState().scenarioOptions.objective.Type, Scenario::ObjectiveType::haveFun);
}

TEST_F(CheatActionTests, SetForcedParkRatingAssignsValue)
{
    executeCheat(CheatType::setForcedParkRating, 750);
    EXPECT_EQ(getGameState().cheats.forcedParkRating, 750);

    executeCheat(CheatType::setForcedParkRating, kForcedParkRatingDisabled);
    EXPECT_EQ(getGameState().cheats.forcedParkRating, kForcedParkRatingDisabled);
}

TEST_F(CheatActionTests, SetForcedParkRatingRejectsOutOfRange)
{
    auto result = queryCheat(CheatType::setForcedParkRating, 1000);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);

    result = queryCheat(CheatType::setForcedParkRating, -2);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, WinScenarioRecordsCompanyValue)
{
    auto& gameState = getGameState();
    gameState.scenarioCompletedCompanyValue = kMoney64Undefined;
    gameState.park.companyValue = 42;

    executeCheat(CheatType::winScenario);

    EXPECT_EQ(gameState.scenarioCompletedCompanyValue, 42);
}

TEST_F(CheatActionTests, ForceWeatherSetsCurrentWeatherType)
{
    executeCheat(CheatType::forceWeather, EnumValue(Weather::Type::HeavyRain));
    EXPECT_EQ(getGameState().weatherCurrent.weatherType, Weather::Type::HeavyRain);

    executeCheat(CheatType::forceWeather, EnumValue(Weather::Type::Sunny));
    EXPECT_EQ(getGameState().weatherCurrent.weatherType, Weather::Type::Sunny);
}

TEST_F(CheatActionTests, ForceWeatherRejectsOutOfRange)
{
    const auto result = queryCheat(CheatType::forceWeather, EnumValue(Weather::Type::Count));
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, GenerateGuestsIncreasesGuestCount)
{
    const auto& gameState = getGameState();
    const auto before = gameState.park.numGuestsInPark + gameState.park.numGuestsHeadingForPark;

    executeCheat(CheatType::generateGuests, 5);

    const auto after = gameState.park.numGuestsInPark + gameState.park.numGuestsHeadingForPark;
    EXPECT_EQ(after - before, 5u);
}

TEST_F(CheatActionTests, GenerateGuestsRejectsZeroCount)
{
    const auto result = queryCheat(CheatType::generateGuests, 0);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, GenerateGuestsRejectsAboveMax)
{
    const auto result = queryCheat(CheatType::generateGuests, 10001);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, RemoveAllGuestsClearsGuestList)
{
    Park::GenerateGuest();
    auto& entities = getGameState().entities;
    ASSERT_GT(entities.GetEntityListCount(EntityType::guest), 0u);

    executeCheat(CheatType::removeAllGuests);
    EXPECT_EQ(entities.GetEntityListCount(EntityType::guest), 0u);
}

TEST_F(CheatActionTests, RemoveAllGuestsPreservesFrozenGuests)
{
    auto* normal = Park::GenerateGuest();
    auto* frozen = Park::GenerateGuest();
    ASSERT_NE(normal, nullptr);
    ASSERT_NE(frozen, nullptr);
    frozen->PeepFlags |= PEEP_FLAGS_POSITION_FROZEN;
    const auto frozenId = frozen->Id;

    executeCheat(CheatType::removeAllGuests);

    auto& entities = getGameState().entities;
    EXPECT_NE(entities.TryGetEntity<Guest>(frozenId), nullptr);
}

TEST_F(CheatActionTests, SetGuestParameterHappinessAssignsValue)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    guest->Happiness = 0;
    guest->HappinessTarget = 0;
    guest->PeepFlags |= PEEP_FLAGS_ANGRY;
    guest->Angriness = 50;

    executeCheat(CheatType::setGuestParameter, GUEST_PARAMETER_HAPPINESS, 200);

    EXPECT_EQ(guest->Happiness, 200);
    EXPECT_EQ(guest->HappinessTarget, 200);

    EXPECT_FALSE(guest->PeepFlags & PEEP_FLAGS_ANGRY);
    EXPECT_EQ(guest->Angriness, 0);
}

TEST_F(CheatActionTests, SetGuestParameterHappinessZeroLeavesAngerAlone)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    guest->PeepFlags |= PEEP_FLAGS_ANGRY;
    guest->Angriness = 50;

    executeCheat(CheatType::setGuestParameter, GUEST_PARAMETER_HAPPINESS, 0);

    EXPECT_EQ(guest->Happiness, 0);
    EXPECT_TRUE(guest->PeepFlags & PEEP_FLAGS_ANGRY);
    EXPECT_EQ(guest->Angriness, 50);
}

TEST_F(CheatActionTests, SetGuestParameterEnergyAssignsValue)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);

    executeCheat(CheatType::setGuestParameter, GUEST_PARAMETER_ENERGY, 100);

    EXPECT_EQ(guest->Energy, 100);
    EXPECT_EQ(guest->EnergyTarget, 100);
}

TEST_F(CheatActionTests, SetGuestParameterHungerAssignsValue)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);

    executeCheat(CheatType::setGuestParameter, GUEST_PARAMETER_HUNGER, 64);
    EXPECT_EQ(guest->Hunger, 64);
}

TEST_F(CheatActionTests, SetGuestParameterThirstAssignsValue)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    executeCheat(CheatType::setGuestParameter, GUEST_PARAMETER_THIRST, 200);
    EXPECT_EQ(guest->Thirst, 200);
}

TEST_F(CheatActionTests, SetGuestParameterNauseaAssignsValue)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    executeCheat(CheatType::setGuestParameter, GUEST_PARAMETER_NAUSEA, 150);
    EXPECT_EQ(guest->Nausea, 150);
    EXPECT_EQ(guest->NauseaTarget, 150);
}

TEST_F(CheatActionTests, SetGuestParameterNauseaToleranceAssignsValue)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    executeCheat(CheatType::setGuestParameter, GUEST_PARAMETER_NAUSEA_TOLERANCE, EnumValue(PeepNauseaTolerance::High));
    EXPECT_EQ(guest->NauseaTolerance, PeepNauseaTolerance::High);
}

TEST_F(CheatActionTests, SetGuestParameterToiletAssignsValue)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    executeCheat(CheatType::setGuestParameter, GUEST_PARAMETER_TOILET, 100);
    EXPECT_EQ(guest->Toilet, 100);
}

TEST_F(CheatActionTests, SetGuestParameterPreferredRideIntensityAssignsValue)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    executeCheat(CheatType::setGuestParameter, GUEST_PARAMETER_PREFERRED_RIDE_INTENSITY, 5);
    EXPECT_EQ(guest->Intensity.GetMinimum(), 5);
}

TEST_F(CheatActionTests, SetGuestParameterRejectsOutOfRangeHappiness)
{
    const auto result = queryCheat(CheatType::setGuestParameter, GUEST_PARAMETER_HAPPINESS, 999);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, SetGuestParameterRejectsUnknownParameter)
{
    const auto result = queryCheat(CheatType::setGuestParameter, 999, 1);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, GiveAllGuestsMoneyAssignsCash)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    guest->CashInPocket = 0;

    executeCheat(CheatType::giveAllGuests, OBJECT_MONEY);
    EXPECT_EQ(guest->CashInPocket, kCheatsGiveGuestsMoney);
}

TEST_F(CheatActionTests, GiveAllGuestsParkMapGivesItem)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    EXPECT_FALSE(guest->HasItem(ShopItem::map));
    executeCheat(CheatType::giveAllGuests, OBJECT_PARK_MAP);
    EXPECT_TRUE(guest->HasItem(ShopItem::map));
}

TEST_F(CheatActionTests, GiveAllGuestsBalloonGivesItem)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    executeCheat(CheatType::giveAllGuests, OBJECT_BALLOON);
    EXPECT_TRUE(guest->HasItem(ShopItem::balloon));
}

TEST_F(CheatActionTests, GiveAllGuestsUmbrellaGivesItem)
{
    auto* guest = Park::GenerateGuest();
    ASSERT_NE(guest, nullptr);
    executeCheat(CheatType::giveAllGuests, OBJECT_UMBRELLA);
    EXPECT_TRUE(guest->HasItem(ShopItem::umbrella));
}

TEST_F(CheatActionTests, GiveAllGuestsRejectsOutOfRangeObject)
{
    const auto result = queryCheat(CheatType::giveAllGuests, OBJECT_UMBRELLA + 1);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, SetStaffSpeedSetsEnergyOnAllStaff)
{
    auto& entities = getGameState().entities;
    auto* staff = entities.CreateEntity<Staff>();
    ASSERT_NE(staff, nullptr);
    staff->Energy = 0;
    staff->EnergyTarget = 0;

    executeCheat(CheatType::setStaffSpeed, kCheatsStaffFastSpeed);

    EXPECT_EQ(staff->Energy, kCheatsStaffFastSpeed);
    EXPECT_EQ(staff->EnergyTarget, kCheatsStaffFastSpeed);
}

TEST_F(CheatActionTests, SetStaffSpeedRejectsOutOfRange)
{
    const auto result = queryCheat(CheatType::setStaffSpeed, 256);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, RenewRidesResetsBuildDate)
{
    const auto& gameState = getGameState();
    auto rides = RideManager(gameState);
    ASSERT_NE(rides.begin(), rides.end());

    for (auto& ride : rides)
    {
        ride.buildDate = 100;
    }

    executeCheat(CheatType::renewRides);

    for (auto& ride : rides)
    {
        EXPECT_EQ(ride.buildDate, gameState.date.GetMonthsElapsed());
    }
}

TEST_F(CheatActionTests, TenMinuteInspectionsSetsIntervalOnAllRides)
{
    const auto& gameState = getGameState();
    auto rides = RideManager(gameState);
    ASSERT_NE(rides.begin(), rides.end());

    for (auto& ride : rides)
    {
        ride.inspectionInterval = RideInspection::never;
    }

    executeCheat(CheatType::tenMinuteInspections);

    for (auto& ride : rides)
    {
        EXPECT_EQ(ride.inspectionInterval, RideInspection::every10Minutes);
    }
}

TEST_F(CheatActionTests, ResetCrashStatusClearsCrashedFlag)
{
    const auto& gameState = getGameState();
    auto rides = RideManager(gameState);
    ASSERT_NE(rides.begin(), rides.end());

    for (auto& ride : rides)
    {
        ride.flags.set(RideFlag::crashed);
        ride.lastCrashType = RIDE_CRASH_TYPE_NO_FATALITIES;
    }

    executeCheat(CheatType::resetCrashStatus);

    for (auto& ride : rides)
    {
        EXPECT_FALSE(ride.flags.has(RideFlag::crashed));
        EXPECT_EQ(ride.lastCrashType, RIDE_CRASH_TYPE_NONE);
    }
}

TEST_F(CheatActionTests, FixRidesClearsBreakdownFlags)
{
    const auto& gameState = getGameState();
    auto rides = RideManager(gameState);
    ASSERT_NE(rides.begin(), rides.end());

    for (auto& ride : rides)
    {
        ride.flags.set(RideFlag::brokenDown);
    }

    executeCheat(CheatType::fixRides);

    for (auto& ride : rides)
    {
        EXPECT_FALSE(ride.flags.has(RideFlag::brokenDown));
        EXPECT_FALSE(ride.flags.has(RideFlag::breakdownPending));
    }
}

TEST_F(CheatActionTests, FixRidesRedirectsMechanicThatIsFixing)
{
    auto& gameState = getGameState();
    auto rides = RideManager(gameState);
    ASSERT_NE(rides.begin(), rides.end());
    auto& ride = *rides.begin();

    auto* mechanic = gameState.entities.CreateEntity<Staff>();
    ASSERT_NE(mechanic, nullptr);
    mechanic->AssignedStaffType = StaffType::mechanic;
    mechanic->RideSubState = PeepRideSubState::atEntrance;

    ride.flags.set(RideFlag::brokenDown);
    ride.mechanicStatus = MechanicStatus::fixing;
    ride.mechanic = mechanic->Id;

    executeCheat(CheatType::fixRides);

    EXPECT_EQ(mechanic->RideSubState, PeepRideSubState::approachExit);
    EXPECT_FALSE(ride.flags.has(RideFlag::brokenDown));
}

TEST_F(CheatActionTests, FixRidesRemovesHeadingMechanic)
{
    auto& gameState = getGameState();
    auto rides = RideManager(gameState);
    ASSERT_NE(rides.begin(), rides.end());
    auto& ride = *rides.begin();

    auto* mechanic = gameState.entities.CreateEntity<Staff>();
    ASSERT_NE(mechanic, nullptr);
    mechanic->AssignedStaffType = StaffType::mechanic;
    mechanic->CurrentRide = ride.id;
    mechanic->State = PeepState::answering;

    ride.flags.set(RideFlag::brokenDown);
    ride.mechanicStatus = MechanicStatus::heading;
    ride.mechanic = mechanic->Id;

    executeCheat(CheatType::fixRides);

    EXPECT_EQ(mechanic->State, PeepState::one);
    EXPECT_FALSE(ride.flags.has(RideFlag::brokenDown));
}

// Note: the `MechanicStatus::calling` branch in FixBrokenRides is unreachable in
// practice — RideGetAssignedMechanic only returns non-null for `heading`, `fixing`,
// or `hasFixedStationBrakes`, so the inner block never runs with status=calling.

TEST_F(CheatActionTests, SetGrassLengthSetsValueOnOwnedTiles)
{
    executeCheat(CheatType::setGrassLength, 7);

    bool anyChecked = false;
    const auto& gameState = getGameState();
    for (int32_t y = 0; y < gameState.mapSize.y; y++)
    {
        for (int32_t x = 0; x < gameState.mapSize.x; x++)
        {
            const auto* surface = MapGetSurfaceElementAt(TileCoordsXY{ x, y });
            if (surface == nullptr)
                continue;
            if (!(surface->GetOwnership() & OWNERSHIP_OWNED))
                continue;
            if (surface->GetWaterHeight() != 0 || !surface->CanGrassGrow())
                continue;
            anyChecked = true;
            EXPECT_EQ(surface->GetGrassLength(), 7);
        }
    }
    EXPECT_TRUE(anyChecked) << "fixture has no owned grass tiles";
}

TEST_F(CheatActionTests, SetGrassLengthRejectsOutOfRange)
{
    const auto result = queryCheat(CheatType::setGrassLength, 8);
    EXPECT_EQ(result.error, GameActions::Status::invalidParameters);
}

TEST_F(CheatActionTests, SetGrassLengthSkipsTilesWithNoSurface)
{
    auto& gameState = getGameState();
    const auto originalSize = gameState.mapSize;
    gameState.mapSize.x += 4;
    gameState.mapSize.y += 4;

    const auto result = executeCheat(CheatType::setGrassLength, 3);
    EXPECT_EQ(result.error, GameActions::Status::ok);

    gameState.mapSize = originalSize;
}

TEST_F(CheatActionTests, OwnAllLandSkipsTilesWithNoSurface)
{
    auto& gameState = getGameState();
    const auto originalSize = gameState.mapSize;
    gameState.mapSize.x += 4;
    gameState.mapSize.y += 4;

    const auto result = executeCheat(CheatType::ownAllLand);
    EXPECT_EQ(result.error, GameActions::Status::ok);

    gameState.mapSize = originalSize;
}

TEST_F(CheatActionTests, OwnAllLandMarksTilesOwned)
{
    executeCheat(CheatType::ownAllLand);

    bool foundOwned = false;
    const auto& gameState = getGameState();
    for (int32_t y = 1; y < gameState.mapSize.y - 1 && !foundOwned; y++)
    {
        for (int32_t x = 1; x < gameState.mapSize.x - 1 && !foundOwned; x++)
        {
            auto* surface = MapGetSurfaceElementAt(TileCoordsXY{ x, y });
            if (surface != nullptr && (surface->GetOwnership() & OWNERSHIP_OWNED) != 0)
                foundOwned = true;
        }
    }
    EXPECT_TRUE(foundOwned);
}

TEST_F(CheatActionTests, RemoveParkFencesClearsAllFences)
{
    executeCheat(CheatType::removeParkFences);

    const auto& gameState = getGameState();
    for (int32_t y = 0; y < gameState.mapSize.y; y++)
    {
        for (int32_t x = 0; x < gameState.mapSize.x; x++)
        {
            auto* surface = MapGetSurfaceElementAt(TileCoordsXY{ x, y });
            if (surface != nullptr)
            {
                ASSERT_EQ(surface->GetParkFences(), 0);
            }
        }
    }
}

TEST_F(CheatActionTests, RemoveLitterDeletesLitterEntities)
{
    auto& entities = getGameState().entities;
    for (int i = 0; i < 3; i++)
    {
        ASSERT_NE(entities.CreateEntity<Litter>(), nullptr);
    }
    ASSERT_GT(entities.GetEntityListCount(EntityType::litter), 0u);

    executeCheat(CheatType::removeLitter);
    EXPECT_EQ(entities.GetEntityListCount(EntityType::litter), 0u);
}

TEST_F(CheatActionTests, WaterPlantsCompletes)
{
    const auto result = executeCheat(CheatType::waterPlants);
    EXPECT_EQ(result.error, GameActions::Status::ok);
}

TEST_F(CheatActionTests, FixVandalismCompletes)
{
    const auto result = executeCheat(CheatType::fixVandalism);
    EXPECT_EQ(result.error, GameActions::Status::ok);
}
