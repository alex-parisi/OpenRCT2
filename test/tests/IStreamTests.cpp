/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/core/IStream.hpp>
#include <openrct2/core/MemoryStream.h>
#include <string>
#include <vector>

using namespace OpenRCT2;

namespace
{
    std::vector<uint8_t> Drain(const MemoryStream& ms)
    {
        const auto* data = static_cast<const uint8_t*>(ms.GetData());
        return { data, data + ms.GetLength() };
    }

    // Minimal IStream that does NOT override CopyFromStream — so writes against this sink
    // exercise the base-class chunked-copy implementation in IStream.cpp directly.
    class VectorSink final : public IStream
    {
    public:
        std::vector<uint8_t> data;

        bool CanRead() const override { return false; }
        bool CanWrite() const override { return true; }
        uint64_t GetLength() const override { return data.size(); }
        uint64_t GetPosition() const override { return data.size(); }
        void SetPosition(uint64_t) override {}
        void Seek(int64_t, int32_t) override {}
        void Read(void*, uint64_t) override {}
        uint64_t TryRead(void*, uint64_t) override { return 0; }
        void Write(const void* buffer, uint64_t length) override
        {
            const auto* src = static_cast<const uint8_t*>(buffer);
            data.insert(data.end(), src, src + length);
        }
    };
} // namespace

TEST(IStreamTests, ReadString_StopsAtNullTerminator)
{
    constexpr uint8_t kBytes[] = { 'H', 'e', 'l', 'l', 'o', 0, 'X', 'Y', 'Z' };
    MemoryStream ms(kBytes, sizeof(kBytes));
    EXPECT_EQ(ms.ReadString(), std::string("Hello"));
    EXPECT_EQ(ms.GetPosition(), uint64_t{ 6 });
}

TEST(IStreamTests, ReadString_EmptyStringIsJustTheTerminator)
{
    constexpr uint8_t kBytes[] = { 0, 'A', 'B' };
    MemoryStream ms(kBytes, sizeof(kBytes));
    EXPECT_EQ(ms.ReadString(), std::string{});
    EXPECT_EQ(ms.GetPosition(), uint64_t{ 1 });
}

TEST(IStreamTests, ReadString_HandlesNonAsciiBytes)
{
    const uint8_t kBytes[] = { 0xC3, 0xA9, 0xE2, 0x9C, 0x93, 0 };
    MemoryStream ms(kBytes, sizeof(kBytes));
    const auto result = ms.ReadString();
    EXPECT_EQ(result.size(), size_t{ 5 });
    EXPECT_EQ(static_cast<uint8_t>(result[0]), uint8_t{ 0xC3 });
    EXPECT_EQ(static_cast<uint8_t>(result[4]), uint8_t{ 0x93 });
}

TEST(IStreamTests, WriteString_EmitsBytesPlusNullTerminator)
{
    MemoryStream ms;
    ms.WriteString("hi");
    const auto bytes = Drain(ms);
    ASSERT_EQ(bytes.size(), size_t{ 3 });
    EXPECT_EQ(bytes[0], 'h');
    EXPECT_EQ(bytes[1], 'i');
    EXPECT_EQ(bytes[2], 0);
}

TEST(IStreamTests, WriteString_EmptyStringEmitsJustTheNull)
{
    MemoryStream ms;
    ms.WriteString("");
    const auto bytes = Drain(ms);
    ASSERT_EQ(bytes.size(), size_t{ 1 });
    EXPECT_EQ(bytes[0], 0);
}

TEST(IStreamTests, WriteString_TruncatesAtFirstEmbeddedNull)
{
    constexpr std::string_view kInput{ "abc\0def", 7 };
    MemoryStream ms;
    ms.WriteString(kInput);
    const auto bytes = Drain(ms);
    ASSERT_EQ(bytes.size(), size_t{ 4 }); // "abc" + terminator
    EXPECT_EQ(bytes[0], 'a');
    EXPECT_EQ(bytes[1], 'b');
    EXPECT_EQ(bytes[2], 'c');
    EXPECT_EQ(bytes[3], 0);
}

TEST(IStreamTests, WriteString_RoundTripsWithReadString)
{
    MemoryStream ms;
    ms.WriteString("OpenRCT2");
    ms.SetPosition(0);
    EXPECT_EQ(ms.ReadString(), std::string("OpenRCT2"));
}

TEST(IStreamTests, CopyFromStream_CopiesExactlyRequestedBytes)
{
    MemoryStream src;
    constexpr std::string kData = "abcdefghij";
    src.Write(kData.data(), kData.size());
    src.SetPosition(0);

    VectorSink dst;
    dst.CopyFromStream(src, 5);

    ASSERT_EQ(dst.data.size(), size_t{ 5 });
    EXPECT_EQ(std::string(dst.data.begin(), dst.data.end()), "abcde");
}

TEST(IStreamTests, CopyFromStream_ZeroLengthIsNoOp)
{
    MemoryStream src;
    src.WriteString("ignored");
    src.SetPosition(0);

    VectorSink dst;
    dst.CopyFromStream(src, 0);

    EXPECT_TRUE(dst.data.empty());
    EXPECT_EQ(src.GetPosition(), uint64_t{ 0 });
}

TEST(IStreamTests, CopyFromStream_MultiChunkTransferIsExact)
{
    constexpr size_t kSize = 40 * 1024;
    std::vector<uint8_t> payload(kSize);
    for (size_t i = 0; i < kSize; ++i)
        payload[i] = static_cast<uint8_t>(i & 0xFF);

    MemoryStream src;
    src.Write(payload.data(), payload.size());
    src.SetPosition(0);

    VectorSink dst;
    dst.CopyFromStream(src, kSize);

    ASSERT_EQ(dst.data.size(), kSize);
    EXPECT_EQ(std::memcmp(dst.data.data(), payload.data(), kSize), 0);
}
