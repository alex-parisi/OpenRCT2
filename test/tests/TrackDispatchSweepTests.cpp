/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "fixture/PaintTestFixture.hpp"

#include <cstdint>
#include <exception>
#include <gtest/gtest.h>
#include <map>
#include <openrct2/GameState.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/paint/track/Support.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/TrackData.h>
#include <openrct2/ride/TrackPaint.h>
#include <openrct2/ride/TrackStyle.h>
#include <openrct2/ride/ted/TrackElemType.h>
#include <openrct2/ride/ted/TrackElementDescriptor.h>
#include <openrct2/world/Location.hpp>
#include <openrct2/world/tile_element/TrackElement.h>
#include <string>
#include <utility>

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;
using namespace OpenRCT2::TrackMetadata;

// Tier B dispatch sweep. For every TrackStyle present in the loaded fixture park, invoke its paint
// function for every (TrackElemType × direction × trackSequence) combination, using a real Ride
// and TrackElement pulled from the park. The point is to make every switch-arm in
// `paint/track/<category>/*.cpp` execute so the LLVM coverage instrumentation records it.
//
// Assertions are deliberately soft: no image-ID or bbox pinning, just "does not crash, does not
// throw". The paint functions are pure (they emit PaintStruct entries; they don't mutate world
// state), so calling each one many times with a synthesized session state is safe.

namespace
{
    // bpb.sv6 has ~134 rides covering most ride styles. Phase 1 already proved the fixture loads
    // it cleanly; the sweep reuses the same park.
    constexpr std::string_view kSweepParkName = "bpb.sv6";

    // TrackStyle is a uint8_t. Real styles are 0..78 (see TrackStyle.h); 255 is `null`. The
    // dispatch table has 256 entries (DummyGetter fills the gap), so iterating 0..78 covers
    // every non-dummy style.
    constexpr uint8_t kFirstStyleValue = 0;
    constexpr uint8_t kLastStyleValue = 78;

    // TrackElemType::count == 350.
    constexpr uint16_t kTrackElemTypeCount = 350;

    // Picked at the centre of bpb.sv6's map so the synthesized SpritePosition lies inside the
    // park and any in-pipeline tile lookups (rare in paint generation, but they exist) return
    // real data. The exact value isn't load-bearing — the RT culling check uses the world
    // coordinates we set on it below.
    CoordsXY ParkCentre()
    {
        const auto& mapSize = getGameState().mapSize;
        return { mapSize.x * kCoordsXYStep / 2, mapSize.y * kCoordsXYStep / 2 };
    }

    // TrackPaintFunction is `void (&)(...)`; binding a function reference into `auto` decays it
    // into a function pointer, so the dispatch check works through pointer-typed parameters.
    using PaintFunctionPtr = void (*)(
        PaintSession&, const Ride&, uint8_t, uint8_t, int32_t, const OpenRCT2::TrackElement&, SupportType);

    bool IsDummy(PaintFunctionPtr fn)
    {
        return fn == &TrackPaintFunctionDummy;
    }
} // namespace

class PaintTrackDispatchSweepTests : public PaintTestFixture,
                                     public ::testing::WithParamInterface<uint8_t>
{
public:
    static void SetUpTestSuite()
    {
        PaintTestFixture::SetUpTestSuite();
        LoadParkFixture(kSweepParkName);
        BuildStyleToRideMap();
    }

    // Map from TrackStyle (as uint8_t) to a sample (rideIndex, trackElement) pair for every style
    // that has at least one ride in the loaded park. The trackElement is one belonging to that
    // ride; its actual track type may not match the type we sweep with, but paint functions
    // generally read trackElement for color scheme + ride index + lift-chain bits, which are
    // valid for any TrackElement.
    static void BuildStyleToRideMap()
    {
        _styleToSample.clear();
        const auto& gameState = getGameState();
        for (const auto& ride : RideManager(gameState))
        {
            // TrackStyle lives one level deeper than the descriptor: a ride has Regular and
            // Inverted drawer entries (TrackDrawerDescriptor), each with its own trackStyle.
            // The Regular entry is the right one for non-inverted rides; inverted-only styles
            // are reachable via a future enhancement that also iterates Inverted entries.
            const auto style = ride.getRideTypeDescriptor().TrackPaintFunctions.Regular.trackStyle;
            if (style == TrackStyle::null)
                continue;
            if (_styleToSample.contains(style))
                continue;
            auto* trackElement = FindFirstTrackElementForRide(static_cast<uint16_t>(ride.id.ToUnderlying()));
            if (trackElement == nullptr)
                continue;
            _styleToSample.emplace(style, std::make_pair(static_cast<uint16_t>(ride.id.ToUnderlying()), trackElement));
        }
    }

    static inline std::map<TrackStyle, std::pair<uint16_t, OpenRCT2::TrackElement*>> _styleToSample;
};

TEST_P(PaintTrackDispatchSweepTests, AllTypesDirsAndSequencesNoCrash)
{
    const auto style = static_cast<TrackStyle>(GetParam());
    const auto it = _styleToSample.find(style);
    if (it == _styleToSample.end())
    {
        GTEST_SKIP() << "TrackStyle " << static_cast<int>(GetParam()) << " is not represented in " << kSweepParkName
                     << " — coverage for this style requires a fixture park that contains a ride of this type.";
    }

    auto* ride = GetRide(RideId::FromUnderlying(it->second.first));
    ASSERT_NE(ride, nullptr);
    auto* trackElement = it->second.second;
    ASSERT_NE(trackElement, nullptr);

    // The synthesized SpritePosition is at the centre of the map; we don't actually care where
    // emitted structs land in screen space, only that imageWithinRT (paint/Paint.cpp:110) doesn't
    // discard them on the way out. The viewport pipeline normally sets the culling rect per
    // column inside ViewportPaint (Viewport.cpp:958); a direct paint-function call doesn't go
    // through that, so we widen culling to a permissive +/- 1M box that no projected sprite will
    // fall outside of.
    constexpr int32_t kRTSize = 1024;
    auto owned = MakeRenderTarget(kRTSize, kRTSize);
    constexpr int32_t kPermissiveCullExtent = 1'000'000;
    owned.rt.x = -kPermissiveCullExtent / 2;
    owned.rt.y = -kPermissiveCullExtent / 2;
    owned.rt.cullingX = -kPermissiveCullExtent / 2;
    owned.rt.cullingY = -kPermissiveCullExtent / 2;
    owned.rt.cullingWidth = kPermissiveCullExtent;
    owned.rt.cullingHeight = kPermissiveCullExtent;
    const auto centre = ParkCentre();

    size_t totalInvocations = 0;
    size_t totalDummies = 0;
    size_t totalStructs = 0;
    size_t totalExceptions = 0;
    std::string firstFailure;

    for (uint8_t direction = 0; direction < 4; ++direction)
    {
        auto* session = PaintSessionAlloc(owned.rt, 0u, direction);
        ASSERT_NE(session, nullptr);

        for (uint16_t t = 0; t < kTrackElemTypeCount; ++t)
        {
            const auto trackType = static_cast<TrackElemType>(t);
            const auto fn = GetTrackPaintFunction(style, trackType);
            if (IsDummy(fn))
            {
                ++totalDummies;
                continue;
            }

            const auto& ted = GetTrackElementDescriptor(trackType);
            const uint8_t numSequences = ted.sequenceData.numSequences;
            if (numSequences == 0)
                continue;

            // TileElement and TrackElement share TileElementBase's layout; the tile-grid stores
            // them as a discriminated union with the same byte size, so a reinterpret_cast is
            // safe here (and is what other call sites do when round-tripping through type-erased
            // TileElement* pointers).
            auto* asTileElement = reinterpret_cast<OpenRCT2::TileElement*>(trackElement);
            for (uint8_t seq = 0; seq < numSequences; ++seq)
            {
                ResetSessionForTrackPiece(*session, centre, asTileElement);
                // Constant base height for the synthesized piece — most paint functions only
                // care that it's above zero; the actual value doesn't drive coverage.
                try
                {
                    constexpr int32_t kSyntheticHeight = 64;
                    fn(*session, *ride, seq, direction, kSyntheticHeight, *trackElement, SupportType{});
                    ++totalInvocations;
                    totalStructs += CountPaintStructs(*session);
                }
                catch (const std::exception& e)
                {
                    ++totalExceptions;
                    if (firstFailure.empty())
                    {
                        firstFailure = "style=" + std::to_string(static_cast<int>(GetParam())) + " type=" + std::to_string(t)
                            + " dir=" + std::to_string(direction) + " seq=" + std::to_string(seq) + " what=" + e.what();
                    }
                }
            }
        }

        PaintSessionFree(session);
    }

    std::cerr << "[ sweep    ] style=" << static_cast<int>(GetParam()) << " invocations=" << totalInvocations
              << " structs=" << totalStructs << " dummies=" << totalDummies << " exceptions=" << totalExceptions << "\n";

    EXPECT_EQ(totalExceptions, 0u) << "paint function threw during sweep. First failure: " << firstFailure;
    EXPECT_GT(totalInvocations, 0u) << "no non-dummy paint functions invoked for style " << static_cast<int>(GetParam())
                                    << " — the dispatch table entry may have changed, or all TED numSequences are zero.";
}

INSTANTIATE_TEST_SUITE_P(
    Paint, PaintTrackDispatchSweepTests, ::testing::Range<uint8_t>(kFirstStyleValue, kLastStyleValue + 1),
    [](const ::testing::TestParamInfo<uint8_t>& info) { return "style" + std::to_string(static_cast<int>(info.param)); });
