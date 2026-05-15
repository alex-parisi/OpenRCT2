/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <memory>
#include <openrct2/core/Guard.hpp>
#include <vector>

using namespace OpenRCT2;

// All tests stay on the passing-assertion side of Guard's API

namespace
{
    class GuardTests : public ::testing::Test
    {
    protected:
        AssertBehaviour _saved{};

        void SetUp() override
        {
            _saved = Guard::GetAssertBehaviour();
        }
        void TearDown() override
        {
            Guard::SetAssertBehaviour(_saved);
        }
    };
} // namespace

TEST_F(GuardTests, GetAndSetAssertBehaviour_RoundTrips)
{
    Guard::SetAssertBehaviour(AssertBehaviour::abort);
    EXPECT_EQ(Guard::GetAssertBehaviour(), AssertBehaviour::abort);

    Guard::SetAssertBehaviour(AssertBehaviour::cAssert);
    EXPECT_EQ(Guard::GetAssertBehaviour(), AssertBehaviour::cAssert);

    Guard::SetAssertBehaviour(AssertBehaviour::messageBox);
    EXPECT_EQ(Guard::GetAssertBehaviour(), AssertBehaviour::messageBox);
}

TEST(GuardTests_Assert, SourceLocationOverloadReturnsOnTruthyExpression)
{
    Guard::Assert(true);
    SUCCEED();
}

TEST(GuardTests_Assert, MessageOverloadReturnsOnTruthyExpression)
{
    Guard::Assert(true, "this message must never be formatted");
    Guard::Assert(true, "formatted: %d %s", 42, "ok");
    SUCCEED();
}

TEST(GuardTests_Assert, NullMessagePointerOnTruthyExpressionIsSafe)
{
    Guard::Assert(true, static_cast<const char*>(nullptr));
    SUCCEED();
}

TEST(GuardTests_ArgumentNotNull, AcceptsNonNullRawPointer)
{
    int dummy = 0;
    Guard::ArgumentNotNull(&dummy);
    Guard::ArgumentNotNull(&dummy, "with-message: %d", 7);
    SUCCEED();
}

TEST(GuardTests_ArgumentNotNull, AcceptsNonNullSharedPointer)
{
    const auto ptr = std::make_shared<int>(123);
    Guard::ArgumentNotNull(ptr);
    Guard::ArgumentNotNull(ptr, "also fine");
    SUCCEED();
}

TEST(GuardTests_ArgumentInRange, AcceptsValueAtAndInsideBounds)
{
    Guard::ArgumentInRange(0, 0, 10);
    Guard::ArgumentInRange(10, 0, 10);
    Guard::ArgumentInRange(5, 0, 10);
    Guard::ArgumentInRange(5, 0, 10, "value=%d", 5);
    SUCCEED();
}

TEST(GuardTests_IndexInRange, AcceptsValidIndices)
{
    std::vector<int> v(8, 0);
    Guard::IndexInRange(0u, v);
    Guard::IndexInRange(7u, v);
    SUCCEED();
}

TEST(GuardTests_GetLastAssertMessage, IsReadableAndDoesNotThrow)
{
    const auto msg = Guard::GetLastAssertMessage();
    (void)msg;
    SUCCEED();
}
