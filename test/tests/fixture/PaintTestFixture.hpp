/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <openrct2/Context.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/interface/ZoomLevel.h>
#include <openrct2/world/Location.hpp>
#include <string>
#include <string_view>
#include <vector>

struct PaintSession;

namespace OpenRCT2
{
    struct TileElement;
    struct TrackElement;
} // namespace OpenRCT2

// Shared base for tests that exercise the paint subsystem. Unlike the rest of the suite, which sets
// `gOpenRCT2NoGraphics = true` and skips `LoadBaseGraphics()`, this fixture boots the engine with
// graphics loaded so the g1 sprite table is populated. Without it, every paint helper short-circuits
// inside `CreateNormalPaintStruct` (`paint/Paint.cpp:181`) and emits zero PaintStructs.
class PaintTestFixture : public ::testing::Test
{
protected:
    // Heap-owning RenderTarget wrapper. The engine's `RenderTarget` itself does not own its `bits`,
    // so tests use this to keep the buffer alive for the duration of a render and free it
    // automatically.
    struct OwnedRenderTarget
    {
        OpenRCT2::Drawing::RenderTarget rt{};
        std::vector<OpenRCT2::Drawing::PaletteIndex> buffer;
    };

    static void SetUpTestSuite();
    static void TearDownTestSuite();

    // Loads a park from `test/tests/testdata/parks/<name>` into the live GameState, mirroring the
    // `loadParkOnce` recipe used by CheatActionTests. Must be called after `SetUpTestSuite`.
    static void LoadParkFixture(std::string_view name);

    // Allocates an indexed-colour buffer sized for the given viewport dimensions. Initially filled
    // with `PaletteIndex::transparent` so that a render that emits no sprites is detectable as
    // unchanged.
    static OwnedRenderTarget MakeRenderTarget(int32_t width, int32_t height);

    // Constructs a Viewport centred on the middle of the loaded park's map. Mirrors the minimal
    // field set used by `Screenshot.cpp::GetGiantViewport` so the paint pipeline accepts it.
    static OpenRCT2::Viewport MakeCentredViewport(
        int32_t width, int32_t height, uint8_t rotation = 0, ZoomLevel zoom = ZoomLevel{ 0 });

    // Runs the full headless paint pipeline (PaintSessionAlloc -> Generate -> Arrange -> DrawStructs)
    // by delegating to `ViewportRender` exactly the way `Screenshot.cpp::RenderViewport` does.
    // Creates a transient X8DrawingEngine bound to the Context's DummyUiContext (no SDL).
    static void RenderToBuffer(OwnedRenderTarget& owned, const OpenRCT2::Viewport& viewport);

    // Counts entries in the buffer that are not `PaletteIndex::transparent`. Used as a coarse
    // smoke-check that the pipeline emitted *something*.
    static size_t CountNonTransparentPixels(const OwnedRenderTarget& owned);

    // Deterministic 64-bit hash of the buffer (FNV-1a). Cross-compiler stable as long as the
    // rendered pixels are bit-identical. Used by the integration tests as the regression signal
    // when OPENRCT2_TEST_STRICT_HASH=1 is set.
    static uint64_t HashBuffer(const OwnedRenderTarget& owned);

    // Bands the buffer into 16-pixel-tall strips and reports the non-transparent pixel count per
    // strip as a multi-line string. Printed on a strict-mode hash mismatch so the failure mode
    // (entire render missing, one ride disappeared, edge artifacts only, etc.) is identifiable
    // without dumping the full buffer.
    static std::string HistogramDump(const OwnedRenderTarget& owned);

    // Returns true when the OPENRCT2_TEST_STRICT_HASH env var is set to "1". In strict mode the
    // hash mismatch is a hard EXPECT failure; in lax mode the actual hash is just printed.
    static bool IsStrictHashMode();

    // Initializes the per-tile session fields a paint function may read when invoked outside the
    // normal viewport pipeline. PaintSessionAlloc leaves most of these at their pool-recycled
    // values; production code re-populates them in PaintTileElementBase before each tile. This
    // helper mirrors that init for direct-call sweeps.
    //
    // `mapPosition` should be inside the loaded park's map so any tile lookups the paint function
    // performs (e.g. via session.MapPosition) return real data; `kCoordsXYStep * mapSize/2` is a
    // safe default and what the Phase 2 sweep uses.
    static void ResetSessionForTrackPiece(
        PaintSession& session, const CoordsXY& mapPosition, OpenRCT2::TileElement* currentlyDrawnTileElement);

    // Counts every PaintStruct on every quadrant of the session. Tier B uses this as a soft
    // signal — "this (style, type, dir, seq) combo emitted N structs" — without pinning specific
    // image IDs or bbox dimensions.
    static size_t CountPaintStructs(const PaintSession& session);

    // Walks every tile of the loaded park and returns a pointer to the first TrackElement owned by
    // the given ride index, or nullptr if none was found. Used by the sweep to pull real
    // (well-populated) TrackElement instances out of the fixture instead of synthesizing stubs.
    static OpenRCT2::TrackElement* FindFirstTrackElementForRide(uint16_t rideIndex);

    static inline std::unique_ptr<OpenRCT2::IContext> _context;
};
