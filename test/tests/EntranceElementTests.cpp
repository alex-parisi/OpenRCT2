/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/Identifiers.h>
#include <openrct2/object/ObjectTypes.h>
#include <openrct2/world/tile_element/EntranceElement.h>

using namespace OpenRCT2;

TEST(EntranceElementTests, EntranceTypeRoundTrip)
{
    EntranceElement e{};
    e.SetEntranceType(ENTRANCE_TYPE_RIDE_ENTRANCE);
    EXPECT_EQ(e.GetEntranceType(), ENTRANCE_TYPE_RIDE_ENTRANCE);

    e.SetEntranceType(ENTRANCE_TYPE_PARK_ENTRANCE);
    EXPECT_EQ(e.GetEntranceType(), ENTRANCE_TYPE_PARK_ENTRANCE);
}

TEST(EntranceElementTests, RideIndexRoundTrip)
{
    EntranceElement e{};
    constexpr auto kRide = RideId::FromUnderlying(7);
    e.SetRideIndex(kRide);
    EXPECT_EQ(e.GetRideIndex(), kRide);

    e.SetRideIndex(RideId::GetNull());
    EXPECT_TRUE(e.GetRideIndex().IsNull());
}

TEST(EntranceElementTests, StationIndexRoundTrip)
{
    EntranceElement e{};
    constexpr auto kStation = StationIndex::FromUnderlying(3);
    e.SetStationIndex(kStation);
    EXPECT_EQ(e.GetStationIndex(), kStation);

    e.SetStationIndex(StationIndex::GetNull());
    EXPECT_TRUE(e.GetStationIndex().IsNull());
}

TEST(EntranceElementTests, EntryIndexRoundTrip)
{
    EntranceElement e{};
    e.setEntryIndex(42);
    EXPECT_EQ(e.getEntryIndex(), ObjectEntryIndex{ 42 });

    e.setEntryIndex(kObjectEntryIndexNull);
    EXPECT_EQ(e.getEntryIndex(), kObjectEntryIndexNull);
}

TEST(EntranceElementTests, SequenceIndex_ReadsLowNibbleOnly)
{
    EntranceElement e{};
    e.SetSequenceIndex(0x1F);
    EXPECT_EQ(e.GetSequenceIndex(), uint8_t{ 0xF });

    e.SetSequenceIndex(0xF0);
    EXPECT_EQ(e.GetSequenceIndex(), uint8_t{ 0x0 });

    e.SetSequenceIndex(EntranceSequence::Left);
    EXPECT_EQ(e.GetSequenceIndex(), EntranceSequence::Left);
}

TEST(EntranceElementTests, SequenceIndex_KnownSentinels)
{
    EntranceElement e{};
    e.SetSequenceIndex(EntranceSequence::Centre);
    EXPECT_EQ(e.GetSequenceIndex(), uint8_t{ 0 });
    e.SetSequenceIndex(EntranceSequence::Right);
    EXPECT_EQ(e.GetSequenceIndex(), uint8_t{ 2 });
}

TEST(EntranceElementTests, DefaultElementHasNoLegacyPathEntry)
{
    const EntranceElement e{};
    EXPECT_FALSE(e.HasLegacyPathEntry());
    EXPECT_EQ(e.GetLegacyPathEntryIndex(), kObjectEntryIndexNull);
}

TEST(EntranceElementTests, SetLegacyPathEntry_FlipsFlagAndStoresIndex)
{
    EntranceElement e{};
    e.SetLegacyPathEntryIndex(11);

    EXPECT_TRUE(e.HasLegacyPathEntry());
    EXPECT_EQ(e.GetLegacyPathEntryIndex(), ObjectEntryIndex{ 11 });
    EXPECT_EQ(e.GetSurfaceEntryIndex(), kObjectEntryIndexNull);
}

TEST(EntranceElementTests, SetSurfaceEntry_ClearsLegacyFlagAndStoresIndex)
{
    EntranceElement e{};
    e.SetLegacyPathEntryIndex(11);
    ASSERT_TRUE(e.HasLegacyPathEntry());

    e.SetSurfaceEntryIndex(22);

    EXPECT_FALSE(e.HasLegacyPathEntry());
    EXPECT_EQ(e.GetSurfaceEntryIndex(), ObjectEntryIndex{ 22 });
    EXPECT_EQ(e.GetLegacyPathEntryIndex(), kObjectEntryIndexNull);
}

TEST(EntranceElementTests, LegacyAndSurfaceShareTheSameByteButDifferentSemantics)
{
    EntranceElement e{};
    e.SetSurfaceEntryIndex(50);
    EXPECT_FALSE(e.HasLegacyPathEntry());
    EXPECT_EQ(e.GetSurfaceEntryIndex(), ObjectEntryIndex{ 50 });

    e.SetLegacyPathEntryIndex(50);
    EXPECT_TRUE(e.HasLegacyPathEntry());
    EXPECT_EQ(e.GetLegacyPathEntryIndex(), ObjectEntryIndex{ 50 });
}

TEST(EntranceElementTests, GetDirections_RideEntranceAndExitAtCentreSequence)
{
    EntranceElement e{};
    e.SetSequenceIndex(0);

    e.SetEntranceType(ENTRANCE_TYPE_RIDE_ENTRANCE);
    EXPECT_EQ(e.GetDirections(), 4);

    e.SetEntranceType(ENTRANCE_TYPE_RIDE_EXIT);
    EXPECT_EQ(e.GetDirections(), 4);
}

TEST(EntranceElementTests, GetDirections_ParkEntranceAtCentreHasExtraFlagBit)
{
    EntranceElement e{};
    e.SetEntranceType(ENTRANCE_TYPE_PARK_ENTRANCE);
    e.SetSequenceIndex(0);
    EXPECT_EQ(e.GetDirections(), 4 | 1);
}

TEST(EntranceElementTests, GetDirections_NonZeroSequenceReturnsZero)
{
    EntranceElement e{};
    for (uint8_t seq = 1; seq < 8; ++seq)
    {
        e.SetEntranceType(ENTRANCE_TYPE_RIDE_ENTRANCE);
        e.SetSequenceIndex(seq);
        EXPECT_EQ(e.GetDirections(), 0) << "ride-entrance seq=" << +seq;

        e.SetEntranceType(ENTRANCE_TYPE_PARK_ENTRANCE);
        EXPECT_EQ(e.GetDirections(), 0) << "park-entrance seq=" << +seq;
    }
}
