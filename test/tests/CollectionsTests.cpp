/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <cstdint>
#include <gtest/gtest.h>
#include <openrct2/core/Collections.hpp>
#include <string>
#include <vector>

using namespace OpenRCT2;

TEST(CollectionsTests, AddRange_AppendsInOrderToEmpty)
{
    std::vector<int> dst;
    Collections::AddRange<std::vector<int>, int>(dst, { 1, 2, 3 });
    EXPECT_EQ(dst, (std::vector<int>{ 1, 2, 3 }));
}

TEST(CollectionsTests, AddRange_AppendsToNonEmpty)
{
    std::vector<int> dst{ 7, 8 };
    Collections::AddRange<std::vector<int>, int>(dst, { 9, 10 });
    EXPECT_EQ(dst, (std::vector<int>{ 7, 8, 9, 10 }));
}

TEST(CollectionsTests, AddRange_EmptyInitializerLeavesCollectionUnchanged)
{
    std::vector<int> dst{ 1, 2 };
    Collections::AddRange<std::vector<int>, int>(dst, {});
    EXPECT_EQ(dst, (std::vector<int>{ 1, 2 }));
}

namespace
{
    constexpr auto kIntEq = [](const int a, const int b) { return a == b; };
} // namespace

TEST(CollectionsTests, ContainsWithComparer_FoundAndNotFound)
{
    const std::vector<int> v{ 4, 8, 15, 16, 23, 42 };
    EXPECT_TRUE(Collections::Contains(v, 15, kIntEq));
    EXPECT_FALSE(Collections::Contains(v, 5, kIntEq));
}

TEST(CollectionsTests, ContainsWithComparer_EmptyCollectionIsFalse)
{
    constexpr std::vector<int> kV;
    EXPECT_FALSE(Collections::Contains(kV, 1, kIntEq));
}

TEST(CollectionsTests, IndexOfWithComparer_ReturnsFirstMatchIndex)
{
    const std::vector<int> v{ 10, 20, 30, 20, 40 };
    EXPECT_EQ(Collections::IndexOf(v, 20, kIntEq), 1u);
    EXPECT_EQ(Collections::IndexOf(v, 10, kIntEq), 0u);
    EXPECT_EQ(Collections::IndexOf(v, 40, kIntEq), 4u);
}

TEST(CollectionsTests, IndexOfWithComparer_NotFoundReturnsSizeMax)
{
    const std::vector<int> v{ 1, 2, 3 };
    EXPECT_EQ(Collections::IndexOf(v, 99, kIntEq), SIZE_MAX);
}

TEST(CollectionsTests, IndexOfWithPredicate_FindsFirstMatch)
{
    const std::vector<int> v{ 1, 3, 5, 4, 6 };
    const auto idx = Collections::IndexOf(v, [](const int x) { return x % 2 == 0; });
    EXPECT_EQ(idx, 3u);
}

TEST(CollectionsTests, IndexOfWithPredicate_NotFoundReturnsSizeMax)
{
    const std::vector<int> v{ 1, 3, 5 };
    const auto idx = Collections::IndexOf(v, [](const int x) { return x > 100; });
    EXPECT_EQ(idx, SIZE_MAX);
}

TEST(CollectionsTests, IndexOfWithPredicate_EmptyCollectionReturnsSizeMax)
{
    constexpr std::vector<int> kV;
    EXPECT_EQ(Collections::IndexOf(kV, [](int) { return true; }), SIZE_MAX);
}

TEST(CollectionsTests, ContainsCString_CaseSensitiveDefault)
{
    const std::vector<const char*> v{ "apple", "banana", "Cherry" };
    EXPECT_TRUE(Collections::Contains(v, "banana"));
    EXPECT_FALSE(Collections::Contains(v, "Banana"));
    EXPECT_FALSE(Collections::Contains(v, "cherry"));
}

TEST(CollectionsTests, ContainsCString_CaseInsensitive)
{
    const std::vector<const char*> v{ "apple", "banana", "Cherry" };
    EXPECT_TRUE(Collections::Contains(v, "Banana", true));
    EXPECT_TRUE(Collections::Contains(v, "CHERRY", true));
    EXPECT_FALSE(Collections::Contains(v, "grape", true));
}

TEST(CollectionsTests, IndexOfCString_CaseSensitive)
{
    std::vector<const char*> v{ "alpha", "Beta", "gamma" };
    EXPECT_EQ(Collections::IndexOf(v, "alpha"), 0u);
    EXPECT_EQ(Collections::IndexOf(v, "Beta"), 1u);
    EXPECT_EQ(Collections::IndexOf(v, "beta"), SIZE_MAX);
}

TEST(CollectionsTests, IndexOfCString_CaseInsensitiveFindsMixedCase)
{
    std::vector<const char*> v{ "alpha", "Beta", "gamma" };
    EXPECT_EQ(Collections::IndexOf(v, "BETA", true), 1u);
    EXPECT_EQ(Collections::IndexOf(v, "GAMMA", true), 2u);
    EXPECT_EQ(Collections::IndexOf(v, "delta", true), SIZE_MAX);
}
