/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/drawing/Colour.h>
#include <openrct2/interface/ColourWithFlags.h>

using namespace OpenRCT2;

// The legacy byte layout:
//   bits 0..4 (mask 0x1F) — base color index
//   bit  5    (0x20)      — outline flag
//   bit  6    (0x40)      — inset flag
//   bit  7    (0x80)      — translucent flag
namespace
{
    constexpr uint8_t kLegacyOutlineBit = 1u << 5;
    constexpr uint8_t kLegacyInsetBit = 1u << 6;
    constexpr uint8_t kLegacyTranslucentBit = 1u << 7;
} // namespace

TEST(ColourWithFlagsTests, FromLegacy_BaseColourIsLowFiveBits)
{
    const auto [colour, flags] = ColourWithFlags::fromLegacy(0x05);
    EXPECT_EQ(colour, static_cast<Drawing::Colour>(5));
    EXPECT_TRUE(flags.isEmpty());
}

TEST(ColourWithFlagsTests, FromLegacy_MasksColourToFiveBits)
{
    const auto [colour, flags] = ColourWithFlags::fromLegacy(0xFF);
    EXPECT_EQ(colour, static_cast<Drawing::Colour>(0x1F));
}

TEST(ColourWithFlagsTests, FromLegacy_TranslucentFlag)
{
    const auto [colour, flags] = ColourWithFlags::fromLegacy(kLegacyTranslucentBit);
    EXPECT_EQ(colour, static_cast<Drawing::Colour>(0));
    EXPECT_TRUE(flags.has(ColourFlag::translucent));
    EXPECT_FALSE(flags.has(ColourFlag::inset));
    EXPECT_FALSE(flags.has(ColourFlag::withOutline));
}

TEST(ColourWithFlagsTests, FromLegacy_InsetFlag)
{
    const auto [colour, flags] = ColourWithFlags::fromLegacy(kLegacyInsetBit);
    EXPECT_TRUE(flags.has(ColourFlag::inset));
    EXPECT_FALSE(flags.has(ColourFlag::translucent));
    EXPECT_FALSE(flags.has(ColourFlag::withOutline));
}

TEST(ColourWithFlagsTests, FromLegacy_OutlineFlag)
{
    const auto [colour, flags] = ColourWithFlags::fromLegacy(kLegacyOutlineBit);
    EXPECT_TRUE(flags.has(ColourFlag::withOutline));
    EXPECT_FALSE(flags.has(ColourFlag::translucent));
    EXPECT_FALSE(flags.has(ColourFlag::inset));
}

TEST(ColourWithFlagsTests, FromLegacy_AllFlagsCombinedWithColour)
{
    constexpr uint8_t kLegacy
        = static_cast<uint8_t>(7) | kLegacyOutlineBit | kLegacyInsetBit | kLegacyTranslucentBit;
    const auto [colour, flags] = ColourWithFlags::fromLegacy(kLegacy);
    EXPECT_EQ(colour, static_cast<Drawing::Colour>(7));
    EXPECT_TRUE(flags.has(ColourFlag::translucent));
    EXPECT_TRUE(flags.has(ColourFlag::inset));
    EXPECT_TRUE(flags.has(ColourFlag::withOutline));
}

TEST(ColourWithFlagsTests, WithFlag_SetReturnsNewWithFlagOn)
{
    constexpr ColourWithFlags kOriginal{ Drawing::Colour::brightRed, {} };
    const auto [colour, flags] = kOriginal.withFlag(ColourFlag::inset, true);

    EXPECT_TRUE(flags.has(ColourFlag::inset));
    EXPECT_EQ(colour, Drawing::Colour::brightRed);

    EXPECT_FALSE(kOriginal.flags.has(ColourFlag::inset));
}

TEST(ColourWithFlagsTests, WithFlag_FalseClearsExistingFlag)
{
    const auto value = ColourWithFlags::fromLegacy(kLegacyInsetBit | kLegacyOutlineBit);
    ASSERT_TRUE(value.flags.has(ColourFlag::inset));

    const auto [colour, flags] = value.withFlag(ColourFlag::inset, false);
    EXPECT_FALSE(flags.has(ColourFlag::inset));

    EXPECT_TRUE(flags.has(ColourFlag::withOutline));
}

TEST(ColourWithFlagsTests, WithFlag_DoesNotMutateOriginal)
{
    constexpr ColourWithFlags kOriginal{ Drawing::Colour::darkGreen, {} };
    (void)kOriginal.withFlag(ColourFlag::translucent, true);
    EXPECT_FALSE(kOriginal.flags.has(ColourFlag::translucent));
}

TEST(ColourWithFlagsTests, AssignColour_ResetsFlags)
{
    auto value = ColourWithFlags::fromLegacy(kLegacyTranslucentBit | kLegacyInsetBit | 3);
    ASSERT_TRUE(value.flags.has(ColourFlag::translucent));
    ASSERT_TRUE(value.flags.has(ColourFlag::inset));

    value = Drawing::Colour::yellow;

    EXPECT_EQ(value.colour, Drawing::Colour::yellow);
    EXPECT_TRUE(value.flags.isEmpty());
}

TEST(ColourWithFlagsTests, AssignColour_ReturnsReferenceToSelf)
{
    ColourWithFlags value{};
    auto& ref = (value = Drawing::Colour::darkBlue);
    EXPECT_EQ(&ref, &value);
    EXPECT_EQ(value.colour, Drawing::Colour::darkBlue);
}
