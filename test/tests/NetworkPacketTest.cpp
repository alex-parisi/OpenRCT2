/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/network/NetworkPacket.h>
#include <openrct2/network/NetworkTypes.h>
#include <openrct2/object/Object.h>

using namespace OpenRCT2::Network;

TEST(NetworkPacketTest, MapRequest_RequiresAuth)
{
    Packet packet(Command::mapRequest);
    ASSERT_TRUE(packet.CommandRequiresAuth());
}

// A malformed mapRequest declares N DAT objects but provides no object data.
// Before the fix, ServerHandleMapRequest would dereference the nullptr returned
// by packet.Read(), crashing the server.
TEST(NetworkPacketTest, MalformedMapRequest_DATObjectReadReturnsNull)
{
    Packet packet(Command::mapRequest);
    packet << static_cast<uint32_t>(1);           // claims 1 object
    packet << static_cast<uint8_t>(OpenRCT2::ObjectGeneration::DAT); // DAT generation
    // no RCTObjectEntry data follows
    packet.Header.size = static_cast<uint32_t>(packet.Data.size());

    uint32_t size;
    packet >> size;
    ASSERT_EQ(size, 1u);

    uint8_t generation;
    packet >> generation;
    ASSERT_EQ(generation, static_cast<uint8_t>(OpenRCT2::ObjectGeneration::DAT));

    const auto* entry = reinterpret_cast<const OpenRCT2::RCTObjectEntry*>(packet.Read(sizeof(OpenRCT2::RCTObjectEntry)));
    ASSERT_EQ(entry, nullptr);
}

// A malformed mapRequest declares N JSON objects but provides no string data.
// Before the fix, ServerHandleMapRequest would loop indefinitely or access
// invalid memory on packet exhaustion.
TEST(NetworkPacketTest, MalformedMapRequest_JSONObjectReadReturnsEmpty)
{
    Packet packet(Command::mapRequest);
    packet << static_cast<uint32_t>(1);            // claims 1 object
    packet << static_cast<uint8_t>(OpenRCT2::ObjectGeneration::JSON); // JSON generation
    // no string data follows
    packet.Header.size = static_cast<uint32_t>(packet.Data.size());

    uint32_t size;
    packet >> size;
    ASSERT_EQ(size, 1u);

    uint8_t generation;
    packet >> generation;
    ASSERT_EQ(generation, static_cast<uint8_t>(OpenRCT2::ObjectGeneration::JSON));

    ASSERT_TRUE(packet.ReadString().empty());
}

