/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/drawing/Colour.h>
#include <openrct2/drawing/FilterPaletteIds.h>
#include <openrct2/drawing/ImageId.hpp>
#include <openrct2/drawing/ImageIndexType.h>

using OpenRCT2::Drawing::Colour;
using OpenRCT2::Drawing::FilterPaletteID;

// The constexpr-comparable surface of the type is asserted at compile time
static_assert(ImageId{ 1u } == ImageId{ 1u });
static_assert(ImageId{ 1u } != ImageId{ 2u });
static_assert(ImageId{ 1u }.WithIndex(99u) == ImageId{ 99u });
static_assert(ImageId{ 100u }.WithIndexOffset(5u) == ImageId{ 105u });
static_assert(ImageId{ 1u }.WithBlended(true).WithBlended(false) == ImageId{ 1u });

TEST(ImageIdTests, DefaultConstructed_HasNoValueAndIndexEqualsUndefined)
{
    constexpr ImageId kImage;
    EXPECT_FALSE(kImage.HasValue());
    EXPECT_EQ(kImage.GetIndex(), kImageIndexUndefined);
    EXPECT_FALSE(kImage.HasPrimary());
    EXPECT_FALSE(kImage.HasSecondary());
    EXPECT_FALSE(kImage.HasTertiary());
    EXPECT_FALSE(kImage.IsRemap());
    EXPECT_FALSE(kImage.IsBlended());
}

TEST(ImageIdTests, IndexConstructor_StoresIndexAndHasValueReturnsTrue)
{
    constexpr ImageId kImage{ 42u };
    EXPECT_TRUE(kImage.HasValue());
    EXPECT_EQ(kImage.GetIndex(), 42u);
}

TEST(ImageIdTests, ExplicitUndefinedIndex_HasNoValue)
{
    constexpr ImageId kImage{ kImageIndexUndefined };
    EXPECT_FALSE(kImage.HasValue());
}

TEST(ImageIdTests, WithIndex_ReplacesIndexAndPreservesEverythingElse)
{
    constexpr ImageId kSource = ImageId{ 1u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue);
    constexpr ImageId kMoved = kSource.WithIndex(99u);
    EXPECT_EQ(kMoved.GetIndex(), 99u);
    EXPECT_EQ(kMoved.GetPrimary(), Colour::brightRed);
    EXPECT_EQ(kMoved.GetSecondary(), Colour::darkBlue);
    EXPECT_TRUE(kMoved.HasPrimary());
    EXPECT_TRUE(kMoved.HasSecondary());
}

TEST(ImageIdTests, WithIndexOffset_AddsToIndex)
{
    constexpr ImageId kImage = ImageId{ 100u }.WithIndexOffset(5u);
    EXPECT_EQ(kImage.GetIndex(), 105u);
}

TEST(ImageIdTests, WithRemapUint8_SetsPrimaryAndIsRemap)
{
    constexpr ImageId kImage = ImageId{ 1u }.WithRemap(uint8_t{ 7 });
    EXPECT_EQ(kImage.GetRemap(), 7u);
    EXPECT_TRUE(kImage.IsRemap());
    EXPECT_TRUE(kImage.HasPrimary());
    EXPECT_FALSE(kImage.HasSecondary());
    EXPECT_FALSE(kImage.HasTertiary());
}

TEST(ImageIdTests, WithRemapFilterPaletteID_CastsEnumToUint8)
{
    constexpr ImageId kImage = ImageId{ 1u }.WithRemap(FilterPaletteID::paletteWater);
    EXPECT_EQ(kImage.GetRemap(), static_cast<uint8_t>(FilterPaletteID::paletteWater));
    EXPECT_TRUE(kImage.IsRemap());
}

TEST(ImageIdTests, WithRemap_ClearsAnyPreviouslySetSecondary)
{
    constexpr ImageId
        kImage = ImageId{ 1u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue).WithRemap(uint8_t{ 3 });
    EXPECT_TRUE(kImage.IsRemap());
    EXPECT_FALSE(kImage.HasSecondary());
    EXPECT_EQ(kImage.GetRemap(), 3u);
    EXPECT_EQ(kImage.GetSecondary(), static_cast<Colour>(0));
}

TEST(ImageIdTests, WithPrimary_SetsPrimaryFlagAndColourAndIsRemapWhenNoSecondary)
{
    constexpr ImageId kImage = ImageId{ 1u }.WithPrimary(Colour::brightYellow);
    EXPECT_TRUE(kImage.HasPrimary());
    EXPECT_FALSE(kImage.HasSecondary());
    EXPECT_TRUE(kImage.IsRemap());
    EXPECT_EQ(kImage.GetPrimary(), Colour::brightYellow);
}

TEST(ImageIdTests, WithSecondary_SetsSecondaryAndDisablesIsRemap)
{
    constexpr ImageId kImage = ImageId{ 1u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue);
    EXPECT_TRUE(kImage.HasPrimary());
    EXPECT_TRUE(kImage.HasSecondary());
    EXPECT_FALSE(kImage.HasTertiary());
    EXPECT_FALSE(kImage.IsRemap());
    EXPECT_EQ(kImage.GetSecondary(), Colour::darkBlue);
}

TEST(ImageIdTests, HasPrimary_IsTrueWhenOnlySecondaryFlagSet)
{
    constexpr ImageId kImage = ImageId{ 1u }.WithTertiary(Colour::brightGreen);
    EXPECT_TRUE(kImage.HasPrimary());
}

TEST(ImageIdTests, WithoutSecondary_ClearsFlagAndZeroesSecondaryByte)
{
    constexpr ImageId kImage = ImageId{ 1u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue).WithoutSecondary();
    EXPECT_TRUE(kImage.HasPrimary());
    EXPECT_FALSE(kImage.HasSecondary());
    EXPECT_EQ(kImage.GetSecondary(), static_cast<Colour>(0));
    EXPECT_EQ(kImage.GetPrimary(), Colour::brightRed);
}

TEST(ImageIdTests, WithTertiary_AfterSecondary_PreservesPrimaryAndSecondaryBytes)
{
    constexpr ImageId
        kImage = ImageId{ 1u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue).WithTertiary(Colour::brightGreen);
    EXPECT_TRUE(kImage.HasPrimary());
    EXPECT_TRUE(kImage.HasSecondary());
    EXPECT_TRUE(kImage.HasTertiary());
    EXPECT_FALSE(kImage.IsRemap());
    EXPECT_EQ(kImage.GetPrimary(), Colour::brightRed);
    EXPECT_EQ(kImage.GetSecondary(), Colour::darkBlue);
    EXPECT_EQ(kImage.GetTertiary(), Colour::brightGreen);
}

TEST(ImageIdTests, WithTertiary_AfterRemapOnly_ZeroesSecondaryByteBecauseRemapUsesItAsExtraPrimaryBits)
{
    constexpr ImageId kImage = ImageId{ 1u }.WithRemap(uint8_t{ 0xAB }).WithTertiary(Colour::brightGreen);
    EXPECT_TRUE(kImage.HasTertiary());
    EXPECT_EQ(kImage.GetSecondary(), static_cast<Colour>(0));
    EXPECT_EQ(kImage.GetTertiary(), Colour::brightGreen);
}

TEST(ImageIdTests, WithBlendedTrue_SetsBlendFlagAndIsBlended)
{
    constexpr ImageId kImage = ImageId{ 1u }.WithBlended(true);
    EXPECT_TRUE(kImage.IsBlended());
}

TEST(ImageIdTests, WithBlendedFalse_ClearsBlendFlag)
{
    constexpr ImageId kImage = ImageId{ 1u }.WithBlended(true).WithBlended(false);
    EXPECT_FALSE(kImage.IsBlended());
}

TEST(ImageIdTests, WithTransparencyFilterPaletteID_ReplacesFlagsWithBlendOnlyAndClearsColours)
{
    const ImageId image = ImageId{ 1u }
                              .WithPrimary(Colour::brightRed)
                              .WithSecondary(Colour::darkBlue)
                              .WithTransparency(FilterPaletteID::paletteWater);
    EXPECT_TRUE(image.IsBlended());
    EXPECT_FALSE(image.IsRemap());
    EXPECT_FALSE(image.HasSecondary());
    EXPECT_FALSE(image.HasPrimary());
    EXPECT_EQ(image.GetRemap(), static_cast<uint8_t>(FilterPaletteID::paletteWater));
    EXPECT_EQ(image.GetSecondary(), static_cast<Colour>(0));
    EXPECT_EQ(image.GetTertiary(), static_cast<Colour>(0));
}

TEST(ImageIdTests, WithTransparencyColour_ResolvesGlassPaletteForThatColour)
{
    const ImageId byColour = ImageId{ 1u }.WithTransparency(Colour::brightRed);
    const ImageId byPalette = ImageId{ 1u }.WithTransparency(GetGlassPaletteId(Colour::brightRed));
    EXPECT_EQ(byColour, byPalette);
    EXPECT_TRUE(byColour.IsBlended());
}

TEST(ImageIdTests, Equality_RequiresAllFieldsToMatch)
{
    constexpr ImageId kA = ImageId{ 1u }.WithPrimary(Colour::brightRed);
    constexpr ImageId kB = ImageId{ 1u }.WithPrimary(Colour::brightRed);
    constexpr ImageId kDifferentIndex = ImageId{ 2u }.WithPrimary(Colour::brightRed);
    constexpr ImageId kDifferentPrimary = ImageId{ 1u }.WithPrimary(Colour::darkBlue);
    EXPECT_EQ(kA, kB);
    EXPECT_FALSE(kA != kB);
    EXPECT_NE(kA, kDifferentIndex);
    EXPECT_NE(kA, kDifferentPrimary);
}

TEST(ImageIdTests, Inequality_DetectsTertiaryDifferences)
{
    constexpr ImageId kBase = ImageId{ 1u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue);
    constexpr ImageId kWithTertiary = kBase.WithTertiary(Colour::brightGreen);
    EXPECT_NE(kBase, kWithTertiary);
}

TEST(ImageIdTests, Equality_ExercisesShortCircuitAtEachField)
{
    constexpr ImageId kBase = ImageId{ 1u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue);
    constexpr ImageId kDifferentSecondary = ImageId{ 1u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::brightGreen);
    constexpr ImageId kDifferentTertiary = kBase.WithTertiary(Colour::brightGreen);
    constexpr ImageId
        kDifferentFlagsOnly = ImageId{ 1u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue).WithBlended(true);
    EXPECT_NE(kBase, kDifferentSecondary);
    EXPECT_NE(kBase, kDifferentTertiary);
    EXPECT_NE(kBase, kDifferentFlagsOnly);
}

TEST(ImageIdTests, IndexAndPaletteConstructor_DelegatesToWithRemap)
{
    constexpr ImageId kDirect{ 5u, FilterPaletteID::paletteWater };
    constexpr ImageId kBuilt = ImageId{ 5u }.WithRemap(FilterPaletteID::paletteWater);
    EXPECT_EQ(kDirect, kBuilt);
}

TEST(ImageIdTests, IndexAndPrimaryConstructor_DelegatesToWithPrimary)
{
    constexpr ImageId kDirect{ 5u, Colour::brightRed };
    constexpr ImageId kBuilt = ImageId{ 5u }.WithPrimary(Colour::brightRed);
    EXPECT_EQ(kDirect, kBuilt);
}

TEST(ImageIdTests, IndexPrimarySecondaryConstructor_DelegatesToBuilderChain)
{
    constexpr ImageId kDirect{ 5u, Colour::brightRed, Colour::darkBlue };
    constexpr ImageId kBuilt = ImageId{ 5u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue);
    EXPECT_EQ(kDirect, kBuilt);
}

TEST(ImageIdTests, IndexPrimarySecondaryTertiaryConstructor_DelegatesToBuilderChain)
{
    constexpr ImageId kDirect{ 5u, Colour::brightRed, Colour::darkBlue, Colour::brightGreen };
    constexpr ImageId
        kBuilt = ImageId{ 5u }.WithPrimary(Colour::brightRed).WithSecondary(Colour::darkBlue).WithTertiary(Colour::brightGreen);
    EXPECT_EQ(kDirect, kBuilt);
    EXPECT_TRUE(kDirect.HasTertiary());
    EXPECT_EQ(kDirect.GetTertiary(), Colour::brightGreen);
}

TEST(ImageIdTests, GetCatalogue_IndexZeroIsG1)
{
    EXPECT_EQ(ImageId{ 0u }.GetCatalogue(), ImageCatalogue::G1);
}

TEST(ImageIdTests, GetCatalogue_JustBelowG1EndIsG1)
{
    EXPECT_EQ(ImageId{ SPR_RCTC_G1_END - 1 }.GetCatalogue(), ImageCatalogue::G1);
}

TEST(ImageIdTests, GetCatalogue_AtG2BeginIsG2)
{
    EXPECT_EQ(ImageId{ SPR_G2_BEGIN }.GetCatalogue(), ImageCatalogue::G2);
}

TEST(ImageIdTests, GetCatalogue_AtCsgBeginIsCsg)
{
    EXPECT_EQ(ImageId{ SPR_CSG_BEGIN }.GetCatalogue(), ImageCatalogue::CSG);
}

TEST(ImageIdTests, GetCatalogue_AtImageListBeginIsObject)
{
    EXPECT_EQ(ImageId{ SPR_IMAGE_LIST_BEGIN }.GetCatalogue(), ImageCatalogue::OBJECT);
}

TEST(ImageIdTests, GetCatalogue_TempRangeIsTemporary)
{
    EXPECT_EQ(ImageId{ SPR_TEMP_BEGIN }.GetCatalogue(), ImageCatalogue::TEMPORARY);
    EXPECT_EQ(ImageId{ SPR_TEMP_END - 1 }.GetCatalogue(), ImageCatalogue::TEMPORARY);
}

TEST(ImageIdTests, GetCatalogue_UndefinedIndexIsUnknown)
{
    EXPECT_EQ(ImageId{ kImageIndexUndefined }.GetCatalogue(), ImageCatalogue::UNKNOWN);
}
