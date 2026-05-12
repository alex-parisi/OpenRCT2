/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifndef DISABLE_NETWORK

    #include "TestData.h"

    #include <chrono>
    #include <cstring>
    #include <gtest/gtest.h>
    #include <openrct2/Context.h>
    #include <openrct2/Game.h>
    #include <openrct2/OpenRCT2.h>
    #include <openrct2/core/Crypt.h>
    #include <openrct2/core/Endianness.h>
    #include <openrct2/core/File.h>
    #include <openrct2/core/Path.hpp>
    #include <openrct2/network/Network.h>
    #include <openrct2/network/NetworkPacket.h>
    #include <openrct2/network/NetworkTypes.h>
    #include <openrct2/network/Socket.h>
    #include <thread>
    #include <vector>

using namespace OpenRCT2;
using namespace OpenRCT2::Network;

// Send a fully-formed wire packet (header + payload) over the socket.
static void SendRawPacket(ITcpSocket& socket, Command id, const std::vector<uint8_t>& payload)
{
    PacketHeader header{};
    header.magic = Convert::HostToNetwork(PacketHeader::kMagic);
    header.version = Convert::HostToNetwork(PacketHeader::kVersion);
    header.size = Convert::HostToNetwork(static_cast<uint32_t>(payload.size()));
    header.id = Convert::HostToNetwork(id);

    std::vector<uint8_t> buffer;
    buffer.insert(
        buffer.end(), reinterpret_cast<const uint8_t*>(&header), reinterpret_cast<const uint8_t*>(&header) + sizeof(header));
    buffer.insert(buffer.end(), payload.begin(), payload.end());

    socket.SendData(buffer.data(), buffer.size());
}

// Drives the server's accept/read/process loop for a short period.
// Update() reads bytes off the socket; Tick() processes parsed packets.
static void PumpServer(int iterations = 20)
{
    for (int i = 0; i < iterations; ++i)
    {
        Network::Update();
        Network::Tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Block until a complete packet of the given command id arrives, pumping the
// server in between reads so it can flush queued outbound packets.
static bool ReceivePacketOfType(
    ITcpSocket& socket, Command expectedId, std::vector<uint8_t>& payload, int maxIterations = 50)
{
    std::vector<uint8_t> buffer;
    for (int i = 0; i < maxIterations; ++i)
    {
        PumpServer(2);

        uint8_t chunk[4096];
        size_t received = 0;
        socket.ReceiveData(chunk, sizeof(chunk), &received);
        if (received > 0)
            buffer.insert(buffer.end(), chunk, chunk + received);

        while (buffer.size() >= sizeof(PacketHeader))
        {
            PacketHeader hdr{};
            std::memcpy(&hdr, buffer.data(), sizeof(hdr));
            hdr.magic = Convert::NetworkToHost(hdr.magic);
            hdr.version = Convert::NetworkToHost(hdr.version);
            hdr.size = Convert::NetworkToHost(hdr.size);
            hdr.id = Convert::NetworkToHost(hdr.id);

            if (hdr.magic != PacketHeader::kMagic)
                return false;

            const size_t total = sizeof(PacketHeader) + hdr.size;
            if (buffer.size() < total)
                break;

            if (hdr.id == expectedId)
            {
                payload.assign(buffer.begin() + sizeof(PacketHeader), buffer.begin() + total);
                return true;
            }
            buffer.erase(buffer.begin(), buffer.begin() + total);
        }
    }
    return false;
}

// Append a null-terminated string to a payload buffer (matches Packet::WriteString).
static void AppendString(std::vector<uint8_t>& buf, std::string_view s)
{
    buf.insert(buf.end(), s.begin(), s.end());
    buf.push_back(0);
}

// Append a value in big-endian network order (matches Packet::operator<<).
template<typename T>
static void AppendBE(std::vector<uint8_t>& buf, T value)
{
    T swapped = Convert::HostToNetwork(value);
    auto* p = reinterpret_cast<const uint8_t*>(&swapped);
    buf.insert(buf.end(), p, p + sizeof(T));
}

// Drive the full token + auth handshake. Returns true on Auth::ok.
static bool AuthenticateClient(ITcpSocket& client, std::string_view playerName)
{
    // 1. Send token request, receive token (challenge) from server.
    SendRawPacket(client, Command::token, {});
    std::vector<uint8_t> tokenPayload;
    if (!ReceivePacketOfType(client, Command::token, tokenPayload))
        return false;
    if (tokenPayload.size() < sizeof(uint32_t))
        return false;

    uint32_t challengeSize = 0;
    std::memcpy(&challengeSize, tokenPayload.data(), sizeof(challengeSize));
    challengeSize = Convert::NetworkToHost(challengeSize);
    if (tokenPayload.size() < sizeof(uint32_t) + challengeSize)
        return false;
    std::vector<uint8_t> challenge(
        tokenPayload.begin() + sizeof(uint32_t), tokenPayload.begin() + sizeof(uint32_t) + challengeSize);

    // 2. Generate a fresh keypair and sign the challenge.
    auto key = Crypt::CreateRSAKey();
    key->Generate();
    auto rsa = Crypt::CreateRSA();
    auto signature = rsa->SignData(*key, challenge.data(), challenge.size());
    const std::string pubkey = key->GetPublic();

    // 3. Build and send the auth packet.
    std::vector<uint8_t> authPayload;
    AppendString(authPayload, Network::GetVersion());
    AppendString(authPayload, playerName);
    AppendString(authPayload, ""); // empty password
    AppendString(authPayload, pubkey);
    AppendBE<uint32_t>(authPayload, static_cast<uint32_t>(signature.size()));
    authPayload.insert(authPayload.end(), signature.begin(), signature.end());

    SendRawPacket(client, Command::auth, authPayload);

    // 4. Read auth response and confirm Auth::ok.
    std::vector<uint8_t> authResp;
    if (!ReceivePacketOfType(client, Command::auth, authResp))
        return false;
    if (authResp.size() < sizeof(uint32_t))
        return false;
    uint32_t authStatus = 0;
    std::memcpy(&authStatus, authResp.data(), sizeof(authStatus));
    authStatus = Convert::NetworkToHost(authStatus);
    return static_cast<Auth>(authStatus) == Auth::ok;
}

class NetworkIntegrationTest : public ::testing::Test
{
protected:
    std::shared_ptr<IContext> context;
    static constexpr uint16_t kTestPort = 11760;

    void SetUp() override
    {
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;

        context = CreateContext();
        ASSERT_TRUE(context->Initialise());

        const auto parkPath = TestData::GetParkPath("bpb.sv6");
        GetContext()->LoadParkFromFile(parkPath);
        GameLoadInit();

        ASSERT_NE(Network::BeginServer(kTestPort, "127.0.0.1"), 0);
        PumpServer(5);
    }

    void TearDown() override
    {
        context.reset();
    }
};

// A misbehaving client sends a `mapRequest` packet without authenticating.
// Before the fix, this dispatched to `ServerHandleMapRequest` which dereferenced
// the null `connection.player` and crashed the server.
// After the fix, `mapRequest` requires auth and the packet is silently dropped.
TEST_F(NetworkIntegrationTest, UnauthenticatedMapRequest_DoesNotCrashServer)
{
    auto client = CreateTcpSocket();
    client->Connect("127.0.0.1", kTestPort);
    ASSERT_EQ(client->GetStatus(), SocketStatus::connected);

    PumpServer(5);

    // Empty mapRequest payload (claims 0 objects). Without our fix, this still
    // reaches ServerHandleMapRequest and crashes on `connection.player->Name`.
    SendRawPacket(*client, Command::mapRequest, {});

    PumpServer(20);

    // If we made it here, the server didn't crash. That's the assertion.
    SUCCEED();
}

// An authenticated client sends a malformed `mapRequest`: the declared object
// count is larger than the actual data. Before the fix, the parsing loop in
// `ServerHandleMapRequest` blindly dereferenced the nullptr returned by
// `packet.Read()` when reading past the end of the packet, crashing the server.
// After the fix, the loop breaks cleanly on null/empty reads.
TEST_F(NetworkIntegrationTest, AuthenticatedMalformedMapRequest_DoesNotCrashServer)
{
    auto client = CreateTcpSocket();
    client->Connect("127.0.0.1", kTestPort);
    ASSERT_EQ(client->GetStatus(), SocketStatus::connected);

    ASSERT_TRUE(AuthenticateClient(*client, "tester"));

    // Malformed: claim 1000 DAT objects, supply only the generation byte for one.
    // The server will read the count, loop, read one generation byte, then call
    // packet.Read(sizeof(RCTObjectEntry)) which returns null (packet exhausted).
    std::vector<uint8_t> payload;
    AppendBE<uint32_t>(payload, 1000);
    payload.push_back(0); // generation = DAT
    // ... no RCTObjectEntry data follows

    SendRawPacket(*client, Command::mapRequest, payload);
    PumpServer(20);

    // If the server is still alive we made it past the crash site.
    SUCCEED();
}

#endif // !DISABLE_NETWORK
