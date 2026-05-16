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
#include <gtest/gtest.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

using namespace OpenRCT2;

namespace
{
    // One row of the Phase 1 render matrix: a park, a (rotation, zoom) pair, the minimum
    // non-transparent-pixel count we expect to see in the result, and the buffer hash recorded
    // when the test was first authored.
    //
    // Default mode: only the pixel-count threshold is asserted; the actual hash is printed for
    // every test so contributors can spot when their changes drift.
    // Strict mode (OPENRCT2_TEST_STRICT_HASH=1): the hash must match exactly.
    struct RenderCase
    {
        std::string_view parkName;
        uint8_t rotation;
        int8_t zoom;
        size_t minNonTransparentPixels;
        uint64_t expectedHash;
    };

    std::string FormatCaseName(const ::testing::TestParamInfo<RenderCase>& info)
    {
        const auto& c = info.param;
        std::string parkSlug{ c.parkName };
        for (auto& ch : parkSlug)
        {
            if (!std::isalnum(static_cast<unsigned char>(ch)))
                ch = '_';
        }
        std::ostringstream os;
        os << parkSlug << "_rot" << static_cast<int>(c.rotation) << "_zoom" << static_cast<int>(c.zoom);
        return os.str();
    }

    std::string FormatHexHash(uint64_t value)
    {
        std::ostringstream os;
        os << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
        return os.str();
    }
} // namespace

// Named with the `Paint` prefix and instantiated under the `Paint` prefix so the full gtest
// identifier (`Paint/PaintHeadlessRenderTests.RendersWithStableHash/<param>`) matches the
// `--gtest_filter=Paint*` selector the CMakeLists uses to gather all paint tests into a single
// ctest entry.
class PaintHeadlessRenderTests : public PaintTestFixture,
                                 public ::testing::WithParamInterface<RenderCase>
{
};

TEST_P(PaintHeadlessRenderTests, RendersWithStableHash)
{
    const auto& tc = GetParam();
    LoadParkFixture(tc.parkName);

    // 1024x768 keeps the per-test cost reasonable while still framing the whole map of every
    // fixture park at zoom 0. Higher zooms scale the *world* view, not the pixel buffer, so the
    // buffer dimensions stay constant.
    constexpr int32_t kWidth = 1024;
    constexpr int32_t kHeight = 768;

    auto owned = MakeRenderTarget(kWidth, kHeight);
    const auto viewport = MakeCentredViewport(kWidth, kHeight, tc.rotation, ZoomLevel{ tc.zoom });
    RenderToBuffer(owned, viewport);

    const auto pixels = CountNonTransparentPixels(owned);
    const auto hash = HashBuffer(owned);

    // Always print the hash so contributors who change rendering can see the new value to bake in.
    std::cerr << "[ paint    ] " << tc.parkName << " rot=" << static_cast<int>(tc.rotation)
              << " zoom=" << static_cast<int>(tc.zoom) << " pixels=" << pixels << " hash=" << FormatHexHash(hash)
              << "\n";

    EXPECT_GT(pixels, tc.minNonTransparentPixels)
        << "render produced fewer non-transparent pixels than expected for " << tc.parkName
        << " (rot=" << static_cast<int>(tc.rotation) << ", zoom=" << static_cast<int>(tc.zoom) << ")";

    if (IsStrictHashMode())
    {
        EXPECT_EQ(hash, tc.expectedHash) << "buffer hash drifted for " << tc.parkName
                                         << " (rot=" << static_cast<int>(tc.rotation)
                                         << ", zoom=" << static_cast<int>(tc.zoom) << ")\n"
                                         << "expected " << FormatHexHash(tc.expectedHash) << ", got " << FormatHexHash(hash)
                                         << "\n"
                                         << HistogramDump(owned);
    }
}

// Three parks × four rotations × three zooms = 36 cases. Hashes were captured locally on macOS
// arm64 (clang) with the X8 software drawing engine and confirmed bit-identical across two runs.
// In default mode only the pixel-count threshold is checked; the hash is printed every run.
// In strict mode (OPENRCT2_TEST_STRICT_HASH=1) the hash must match — flip it on once the value
// is known to be stable on the platform in question. Until per-platform stability is established,
// keep strict mode off in CI.
//
// On a working render the X8 engine fills the entire 1024×768 viewport with terrain at these
// zoom levels (pixel counts always 786431-786432). The 1000-pixel floor exists purely to catch
// the "literally nothing rendered" failure mode (e.g. g1 unloaded, drawing engine misconfigured).
// Subtle regressions show up as hash drift, not pixel-count drift.
INSTANTIATE_TEST_SUITE_P(
    Paint, PaintHeadlessRenderTests,
    ::testing::Values(
        // small_park_with_ferris_wheel.sv6
        RenderCase{ "small_park_with_ferris_wheel.sv6", 0, 0, 1000, 0x86c85d76fd8f3c92ULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 1, 0, 1000, 0x1a1cdf1f71713eb4ULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 2, 0, 1000, 0xb87f1956e06d2468ULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 3, 0, 1000, 0x018ad5375b7ddf41ULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 0, 1, 1000, 0xb6dbc56c5f5667a4ULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 1, 1, 1000, 0xd5d9d80d6fc5e229ULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 2, 1, 1000, 0x9ae64b922a3eb7d0ULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 3, 1, 1000, 0x6f90adf1b674ebfaULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 0, 2, 1000, 0x12cab31eec441b23ULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 1, 2, 1000, 0x66ec4ede7dae69acULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 2, 2, 1000, 0xdc612385a35a3e1bULL },
        RenderCase{ "small_park_with_ferris_wheel.sv6", 3, 2, 1000, 0xcc3671dcc5601ab7ULL },
        // small_park_car_ride_one_car.sv6
        RenderCase{ "small_park_car_ride_one_car.sv6", 0, 0, 1000, 0x6a90f89c5bd8402eULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 1, 0, 1000, 0xb0ad9aee8f0706f2ULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 2, 0, 1000, 0x220344778db5bb93ULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 3, 0, 1000, 0x5cf828056ef0e9a6ULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 0, 1, 1000, 0xc616001a56edf1e9ULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 1, 1, 1000, 0x3c55dc4de5fc62b8ULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 2, 1, 1000, 0x98f899039fc4a26cULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 3, 1, 1000, 0x36babb536a1254a5ULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 0, 2, 1000, 0x5e35aaf65ab25178ULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 1, 2, 1000, 0xdb3a30faa19bcb77ULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 2, 2, 1000, 0xed09ab5ba6066dfbULL },
        RenderCase{ "small_park_car_ride_one_car.sv6", 3, 2, 1000, 0xc73ce3a4ed98fa38ULL },
        // bpb.sv6 (the big production-style fixture, ~134 rides)
        RenderCase{ "bpb.sv6", 0, 0, 1000, 0x4435252d013fee43ULL },
        RenderCase{ "bpb.sv6", 1, 0, 1000, 0x16c16520deaec095ULL },
        RenderCase{ "bpb.sv6", 2, 0, 1000, 0xf6b6718c175cccd4ULL },
        RenderCase{ "bpb.sv6", 3, 0, 1000, 0xbfdce9b14ba2da3bULL },
        RenderCase{ "bpb.sv6", 0, 1, 1000, 0xae5b7f2adeaa57abULL },
        RenderCase{ "bpb.sv6", 1, 1, 1000, 0x4f6a18f8915c91a0ULL },
        RenderCase{ "bpb.sv6", 2, 1, 1000, 0xcbc32760965c6a0fULL },
        RenderCase{ "bpb.sv6", 3, 1, 1000, 0x2cff26774e6651adULL },
        RenderCase{ "bpb.sv6", 0, 2, 1000, 0xf8ece2b99581238bULL },
        RenderCase{ "bpb.sv6", 1, 2, 1000, 0xe7c5abd387f1ab6cULL },
        RenderCase{ "bpb.sv6", 2, 2, 1000, 0x107fb5e6123aa83cULL },
        RenderCase{ "bpb.sv6", 3, 2, 1000, 0xe3d8fbd52afaa2deULL }),
    FormatCaseName);
