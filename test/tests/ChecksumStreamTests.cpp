/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifndef DISABLE_NETWORK

    #include <array>
    #include <gtest/gtest.h>
    #include <openrct2/core/ChecksumStream.h>

using namespace OpenRCT2;

// Mirror of the FNV-1a constants in ChecksumStream.h
namespace
{
    constexpr uint64_t kFnvSeed = 0xcbf29ce484222325uLL;
    constexpr uint64_t kFnvPrime = 0x00000100000001B3uLL;

    uint64_t HashAsUint64(const std::array<std::byte, 20>& buf)
    {
        uint64_t out{};
        std::memcpy(&out, buf.data(), sizeof(out));
        return out;
    }

    constexpr uint64_t FnvStep(const uint64_t state, const uint64_t value)
    {
        return (state ^ value) * kFnvPrime;
    }
} // namespace

TEST(ChecksumStreamTests, ConstructionSeedsBufferWithFnvOffsetBasis)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);
    EXPECT_EQ(HashAsUint64(buf), kFnvSeed);
}

TEST(ChecksumStreamTests, StreamMetadata)
{
    std::array<std::byte, 20> buf{};
    const ChecksumStream stream(buf);
    EXPECT_FALSE(stream.CanRead());
    EXPECT_TRUE(stream.CanWrite());
    EXPECT_EQ(stream.GetLength(), 20u);
    EXPECT_EQ(stream.GetPosition(), 0u);
    EXPECT_EQ(stream.GetData(), buf.data());
}

TEST(ChecksumStreamTests, SetPositionAndSeekAreNoOps)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);
    stream.SetPosition(123);
    stream.Seek(-5, 0);
    EXPECT_EQ(stream.GetPosition(), 0u);
    EXPECT_EQ(HashAsUint64(buf), kFnvSeed);
}

TEST(ChecksumStreamTests, ReadAndTryReadAreNoOps)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    uint64_t sentinel = 0xDEADBEEFCAFEBABEuLL;
    stream.Read(&sentinel, sizeof(sentinel));
    EXPECT_EQ(sentinel, 0xDEADBEEFCAFEBABEuLL);
    EXPECT_EQ(stream.TryRead(&sentinel, sizeof(sentinel)), 0u);
    EXPECT_EQ(sentinel, 0xDEADBEEFCAFEBABEuLL);
}

TEST(ChecksumStreamTests, WriteOneUint64UpdatesHashWithSingleStep)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    constexpr uint64_t kValue = 0x0102030405060708uLL;
    stream.Write(&kValue, sizeof(kValue));
    EXPECT_EQ(HashAsUint64(buf), FnvStep(kFnvSeed, kValue));
}

TEST(ChecksumStreamTests, WriteMultipleUint64Chunks_ChainsSteps)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    constexpr uint64_t kValues[2] = { 0x1111111111111111uLL, 0x2222222222222222uLL };
    stream.Write(kValues, sizeof(kValues));

    constexpr uint64_t kExpected = FnvStep(FnvStep(kFnvSeed, kValues[0]), kValues[1]);
    EXPECT_EQ(HashAsUint64(buf), kExpected);
}

TEST(ChecksumStreamTests, WritePartialChunk_PacksLowBytesOnly)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    // Three bytes — the upper five must be treated as zero in the step.
    constexpr uint8_t kBytes[3] = { 0xAA, 0xBB, 0xCC };
    stream.Write(kBytes, sizeof(kBytes));

    constexpr uint64_t kPacked = 0x0000000000CCBBAAuLL;
    EXPECT_EQ(HashAsUint64(buf), FnvStep(kFnvSeed, kPacked));
}

TEST(ChecksumStreamTests, WriteZeroLength_LeavesHashUnchanged)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    const uint64_t dummy = 0;
    stream.Write(&dummy, 0);
    EXPECT_EQ(HashAsUint64(buf), kFnvSeed);
}

TEST(ChecksumStreamTests, WriteValueDispatchesByWidth_OneByte)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    constexpr uint8_t kValue = 0x5A;
    stream.WriteValue(kValue);

    EXPECT_EQ(HashAsUint64(buf), FnvStep(kFnvSeed, kValue));
}

TEST(ChecksumStreamTests, WriteValueDispatchesByWidth_TwoBytes)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    constexpr uint16_t kValue = 0xABCD;
    stream.WriteValue(kValue);
    EXPECT_EQ(HashAsUint64(buf), FnvStep(kFnvSeed, kValue));
}

TEST(ChecksumStreamTests, WriteValueDispatchesByWidth_FourBytes)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    constexpr uint32_t kValue = 0xDEADBEEFu;
    stream.WriteValue(kValue);
    EXPECT_EQ(HashAsUint64(buf), FnvStep(kFnvSeed, kValue));
}

TEST(ChecksumStreamTests, WriteValueDispatchesByWidth_EightBytes)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    constexpr uint64_t kValue = 0x0123456789ABCDEFuLL;
    stream.WriteValue(kValue);
    EXPECT_EQ(HashAsUint64(buf), FnvStep(kFnvSeed, kValue));
}

TEST(ChecksumStreamTests, WriteValueDispatchesByWidth_SixteenBytesSplitsIntoTwoSteps)
{
    std::array<std::byte, 20> buf{};
    ChecksumStream stream(buf);

    const struct alignas(8) Sixteen
    {
        uint64_t lo;
        uint64_t hi;
    } value{ 0xAAAAAAAAAAAAAAAAuLL, 0xBBBBBBBBBBBBBBBBuLL };
    static_assert(sizeof(value) == 16);

    stream.WriteValue(value);
    const uint64_t expected = FnvStep(FnvStep(kFnvSeed, value.lo), value.hi);
    EXPECT_EQ(HashAsUint64(buf), expected);
}

TEST(ChecksumStreamTests, TemplatedWriteN_MatchesRuntimeWrite)
{
    std::array<std::byte, 20> bufTemplated{};
    std::array<std::byte, 20> bufRuntime{};
    ChecksumStream a(bufTemplated);
    ChecksumStream b(bufRuntime);

    constexpr uint8_t kInput[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    a.Write<sizeof(kInput)>(kInput);
    b.Write(kInput, sizeof(kInput));
    EXPECT_EQ(HashAsUint64(bufTemplated), HashAsUint64(bufRuntime));
}

#endif // DISABLE_NETWORK
