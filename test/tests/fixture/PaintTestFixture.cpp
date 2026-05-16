/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "PaintTestFixture.hpp"

#include "../TestData.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/ParkImporter.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/entity/EntityRegistry.h>
#include <openrct2/entity/EntityTweener.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/world/Location.hpp>
#include <openrct2/world/Map.h>
#include <openrct2/world/MapAnimation.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <sstream>
#include <string>
#include <string_view>

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;

void PaintTestFixture::SetUpTestSuite()
{
    // The load-bearing flag flip: every other test sets gOpenRCT2NoGraphics = true, which causes
    // Context::Initialise to skip LoadBaseGraphics() (Context.cpp:530). Without g1 loaded,
    // CreateNormalPaintStruct (paint/Paint.cpp:181) returns nullptr on every call and paint
    // functions silently emit nothing. Headless is fine; nographics is not.
    gOpenRCT2Headless = true;
    gOpenRCT2NoGraphics = false;

    _context = CreateContext();
    ASSERT_TRUE(_context->Initialise());
}

void PaintTestFixture::TearDownTestSuite()
{
    _context.reset();
}

void PaintTestFixture::LoadParkFixture(std::string_view name)
{
    ASSERT_NE(_context.get(), nullptr) << "SetUpTestSuite must run before LoadParkFixture";

    const auto parkPath = TestData::GetParkPath(std::string(name));
    const auto importer = ParkImporter::CreateS6(_context->GetObjectRepository());
    const auto loadResult = importer->LoadSavedGame(parkPath.c_str(), false);
    _context->GetObjectManager().LoadObjects(loadResult.RequiredObjects);

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

PaintTestFixture::OwnedRenderTarget PaintTestFixture::MakeRenderTarget(int32_t width, int32_t height)
{
    OwnedRenderTarget owned;
    owned.buffer.assign(static_cast<size_t>(width) * static_cast<size_t>(height), PaletteIndex::transparent);
    owned.rt.bits = owned.buffer.data();
    owned.rt.width = width;
    owned.rt.height = height;
    owned.rt.pitch = 0;
    owned.rt.zoom_level = ZoomLevel{ 0 };
    // x, y, culling{X,Y,Width,Height} default to 0; ViewportPaint sets per-column culling itself.
    return owned;
}

Viewport PaintTestFixture::MakeCentredViewport(int32_t width, int32_t height, uint8_t rotation, ZoomLevel zoom)
{
    Viewport viewport{};
    viewport.pos = { 0, 0 };
    viewport.width = width;
    viewport.height = height;
    viewport.zoom = zoom;
    viewport.rotation = rotation;

    // Pick the world-coordinate centre of the loaded park and project it to screen-space so the
    // viewport actually frames something. Mirrors the trick used by GetGiantViewport.
    const auto& gameState = getGameState();
    const auto mapCentre = CoordsXY{ gameState.mapSize.x * kCoordsXYStep / 2, gameState.mapSize.y * kCoordsXYStep / 2 };
    const auto centreScreen = Translate3DTo2DWithZ(rotation, { mapCentre, 0 });
    viewport.viewPos = { centreScreen.x - viewport.ViewWidth() / 2, centreScreen.y - viewport.ViewHeight() / 2 };

    return viewport;
}

void PaintTestFixture::RenderToBuffer(OwnedRenderTarget& owned, const Viewport& viewport)
{
    // Match the recipe in Screenshot.cpp:312 RenderViewport, minus the optional-engine branch:
    // tests always own their engine. X8DrawingEngine::X8DrawingEngine ignores its IUiContext arg
    // (X8DrawingEngine.cpp:123), so DummyUiContext from CreateContext() is fine and no SDL is
    // touched.
    ResetAllSpriteQuadrantPlacements();

    auto engine = std::make_unique<X8DrawingEngine>(_context->GetUiContext());
    engine->BeginDraw();
    owned.rt.DrawingEngine = engine.get();
    ViewportRender(owned.rt, &viewport);
    engine->EndDraw();
    owned.rt.DrawingEngine = nullptr;
}

size_t PaintTestFixture::CountNonTransparentPixels(const OwnedRenderTarget& owned)
{
    size_t count = 0;
    for (auto pixel : owned.buffer)
    {
        if (pixel != PaletteIndex::transparent)
            ++count;
    }
    return count;
}

uint64_t PaintTestFixture::HashBuffer(const OwnedRenderTarget& owned)
{
    // FNV-1a (64-bit). Chosen for portability — std::hash is implementation-defined and would
    // produce different values across libstdc++/libc++/MSVC builds, defeating the point of a
    // regression hash.
    constexpr uint64_t kOffsetBasis = 0xcbf29ce484222325ULL;
    constexpr uint64_t kPrime = 0x100000001b3ULL;
    uint64_t h = kOffsetBasis;
    for (auto pixel : owned.buffer)
    {
        h ^= static_cast<uint64_t>(static_cast<uint8_t>(pixel));
        h *= kPrime;
    }
    return h;
}

std::string PaintTestFixture::HistogramDump(const OwnedRenderTarget& owned)
{
    constexpr int32_t kBandHeight = 16;
    const int32_t width = owned.rt.width;
    const int32_t height = owned.rt.height;

    std::ostringstream os;
    os << "non-transparent pixel count per " << kBandHeight << "-row band (width=" << width
       << ", height=" << height << "):\n";
    for (int32_t bandStart = 0; bandStart < height; bandStart += kBandHeight)
    {
        const int32_t bandEnd = std::min(bandStart + kBandHeight, height);
        size_t bandCount = 0;
        for (int32_t y = bandStart; y < bandEnd; ++y)
        {
            for (int32_t x = 0; x < width; ++x)
            {
                if (owned.buffer[static_cast<size_t>(y) * static_cast<size_t>(width) + x] != PaletteIndex::transparent)
                    ++bandCount;
            }
        }
        os << "  y=" << bandStart << ".." << (bandEnd - 1) << ": " << bandCount << "\n";
    }
    return os.str();
}

bool PaintTestFixture::IsStrictHashMode()
{
    const char* env = std::getenv("OPENRCT2_TEST_STRICT_HASH");
    return env != nullptr && std::string_view{ env } == "1";
}

void PaintTestFixture::ResetSessionForTrackPiece(
    PaintSession& session, const CoordsXY& mapPosition, TileElement* currentlyDrawnTileElement)
{
    // Mirrors what PaintTileElementBase (Paint.TileElement.cpp) sets per-tile, plus a clean
    // SupportSegments slate. Painter::CreateSession leaves these at their pool-recycled state, so
    // a sweep that calls paint functions directly (skipping the per-tile dispatcher) must reset
    // them itself or it inherits stale heights from a previous test's last invocation.
    session.MapPosition = mapPosition;
    session.SpritePosition = mapPosition;
    session.LeftTunnels.clear();
    session.RightTunnels.clear();
    session.VerticalTunnelHeight = 0xFF;
    session.Flags = 0;
    session.WaterHeight = 0;
    session.CurrentlyDrawnTileElement = currentlyDrawnTileElement;
    session.CurrentlyDrawnEntity = nullptr;
    session.Surface = nullptr;
    session.PathElementOnSameHeight = nullptr;
    session.TrackElementOnSameHeight = nullptr;
    session.WoodenSupportsPrependTo = nullptr;
    for (auto& segment : session.SupportSegments)
    {
        segment.height = 0xFFFF;
        segment.slope = 0xFF;
        segment.pad = 0;
    }
    session.Support.height = 0;
    session.Support.slope = 0;
    session.Support.pad = 0;
    session.TrackColours = ImageId(0, OpenRCT2::Drawing::Colour::brightRed);
    session.SupportColours = ImageId(0, OpenRCT2::Drawing::Colour::lightBlue);
}

size_t PaintTestFixture::CountPaintStructs(const PaintSession& session)
{
    size_t count = 0;
    for (auto* head : session.Quadrants)
    {
        for (auto* ps = head; ps != nullptr; ps = ps->NextQuadrantEntry)
        {
            ++count;
            for (auto* child = ps->Children; child != nullptr; child = child->Children)
                ++count;
        }
    }
    return count;
}

OpenRCT2::TrackElement* PaintTestFixture::FindFirstTrackElementForRide(uint16_t rideIndex)
{
    const auto& mapSize = getGameState().mapSize;
    for (int32_t y = 0; y < mapSize.y; ++y)
    {
        for (int32_t x = 0; x < mapSize.x; ++x)
        {
            auto* element = MapGetFirstElementAt(TileCoordsXY{ x, y });
            if (element == nullptr)
                continue;
            do
            {
                if (element->GetType() == TileElementType::Track)
                {
                    auto* track = element->AsTrack();
                    if (track != nullptr && static_cast<uint16_t>(track->GetRideIndex().ToUnderlying()) == rideIndex)
                        return track;
                }
            } while (!(element++)->IsLastForTile());
        }
    }
    return nullptr;
}
