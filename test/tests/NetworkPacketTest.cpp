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

using namespace OpenRCT2::Network;

TEST(NetworkPacketTest, MapRequest_RequiresAuth)
{
    Packet packet(Command::mapRequest);
    ASSERT_TRUE(packet.CommandRequiresAuth());
}

TEST(NetworkPacketTest, ExemptCommands_DoNotRequireAuth)
{
    for (auto cmd : { Command::ping, Command::auth, Command::token, Command::gameInfo,
                      Command::objectsList, Command::scriptsHeader, Command::scriptsData,
                      Command::heartbeat })
    {
        Packet packet(cmd);
        ASSERT_FALSE(packet.CommandRequiresAuth());
    }
}

TEST(NetworkPacketTest, Read_ReturnsNullOnEmptyPacket)
{
    Packet packet(Command::mapRequest);
    ASSERT_EQ(packet.Read(1), nullptr);
}

TEST(NetworkPacketTest, Read_ReturnsNullWhenExhausted)
{
    Packet packet(Command::mapRequest);
    packet.Write("abc", 3);

    ASSERT_NE(packet.Read(3), nullptr);
    ASSERT_EQ(packet.Read(1), nullptr);
}

TEST(NetworkPacketTest, Read_ReturnsNullOnOversizedRead)
{
    Packet packet(Command::mapRequest);
    packet.Write("ab", 2);

    ASSERT_EQ(packet.Read(100), nullptr);
}

TEST(NetworkPacketTest, ReadString_ReturnsEmptyOnEmptyPacket)
{
    Packet packet(Command::mapRequest);
    ASSERT_TRUE(packet.ReadString().empty());
}

TEST(NetworkPacketTest, ReadString_ReturnsStringWhenValid)
{
    Packet packet(Command::mapRequest);
    packet.WriteString("RollerCoaster");

    ASSERT_EQ(packet.ReadString(), "RollerCoaster");
}

TEST(NetworkPacketTest, ReadString_ReturnsEmptyAfterExhausted)
{
    Packet packet(Command::mapRequest);
    packet.WriteString("first");

    ASSERT_EQ(packet.ReadString(), "first");
    ASSERT_TRUE(packet.ReadString().empty());
}
