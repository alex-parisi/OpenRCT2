/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/core/EnumUtils.hpp>
#include <openrct2/ride/ShopItem.h>

using namespace OpenRCT2;

TEST(ShopItemTests, GetShopItemDescriptor_KnownEntriesHaveExpectedFlags)
{
    EXPECT_TRUE(GetShopItemDescriptor(ShopItem::burger).IsFood());
    EXPECT_TRUE(GetShopItemDescriptor(ShopItem::drink).IsDrink());
    EXPECT_TRUE(GetShopItemDescriptor(ShopItem::balloon).IsSouvenir());
    EXPECT_TRUE(GetShopItemDescriptor(ShopItem::photo).IsPhoto());
    EXPECT_TRUE(GetShopItemDescriptor(ShopItem::tShirt).IsRecolourable());

    EXPECT_FALSE(GetShopItemDescriptor(ShopItem::balloon).IsFood());
    EXPECT_FALSE(GetShopItemDescriptor(ShopItem::burger).IsDrink());
    EXPECT_FALSE(GetShopItemDescriptor(ShopItem::burger).IsSouvenir());
    EXPECT_FALSE(GetShopItemDescriptor(ShopItem::drink).IsPhoto());
    EXPECT_FALSE(GetShopItemDescriptor(ShopItem::burger).IsRecolourable());
}

TEST(ShopItemTests, IsFoodOrDrink_TrueForFoodAndDrink)
{
    EXPECT_TRUE(GetShopItemDescriptor(ShopItem::burger).IsFoodOrDrink());
    EXPECT_TRUE(GetShopItemDescriptor(ShopItem::drink).IsFoodOrDrink());
    EXPECT_FALSE(GetShopItemDescriptor(ShopItem::balloon).IsFoodOrDrink());
    EXPECT_FALSE(GetShopItemDescriptor(ShopItem::emptyCan).IsFoodOrDrink());
}

TEST(ShopItemTests, IsFoodOrDrink_AgreesWithIsFoodOrIsDrink)
{
    for (uint8_t i = 0; i < EnumValue(ShopItem::count); ++i)
    {
        const auto item = static_cast<ShopItem>(i);
        const auto& d = GetShopItemDescriptor(item);
        EXPECT_EQ(d.IsFoodOrDrink(), d.IsFood() || d.IsDrink()) << "at index " << static_cast<int>(i);
    }
}

TEST(ShopItemTests, IsFoodAndIsDrink_MutuallyExclusive)
{
    for (uint8_t i = 0; i < EnumValue(ShopItem::count); ++i)
    {
        const auto& d = GetShopItemDescriptor(static_cast<ShopItem>(i));
        EXPECT_FALSE(d.IsFood() && d.IsDrink()) << "at index " << static_cast<int>(i);
    }
}

namespace
{
    uint64_t MaskForFlag(const uint16_t flag)
    {
        uint64_t mask = 0;
        for (uint8_t i = 0; i < EnumValue(ShopItem::count); ++i)
        {
            if (GetShopItemDescriptor(static_cast<ShopItem>(i)).HasFlag(flag))
                mask |= (1uLL << i);
        }
        return mask;
    }
} // namespace

TEST(ShopItemTests, ShopItemsGetAllFoods_MatchesTableSweep)
{
    EXPECT_EQ(ShopItemsGetAllFoods(), MaskForFlag(SHOP_ITEM_FLAG_IS_FOOD));
}

TEST(ShopItemTests, ShopItemsGetAllDrinks_MatchesTableSweep)
{
    EXPECT_EQ(ShopItemsGetAllDrinks(), MaskForFlag(SHOP_ITEM_FLAG_IS_DRINK));
}

TEST(ShopItemTests, ShopItemsGetAllContainers_MatchesTableSweep)
{
    EXPECT_EQ(ShopItemsGetAllContainers(), MaskForFlag(SHOP_ITEM_FLAG_IS_CONTAINER));
}

TEST(ShopItemTests, ShopItemsGetAllFoods_IncludesKnownFoodItems)
{
    const auto foods = ShopItemsGetAllFoods();
    EXPECT_NE(foods & (1uLL << EnumValue(ShopItem::burger)), 0u);
    EXPECT_NE(foods & (1uLL << EnumValue(ShopItem::pizza)), 0u);
    EXPECT_NE(foods & (1uLL << EnumValue(ShopItem::iceCream)), 0u);

    // Non-foods should be absent.
    EXPECT_EQ(foods & (1uLL << EnumValue(ShopItem::balloon)), 0u);
    EXPECT_EQ(foods & (1uLL << EnumValue(ShopItem::drink)), 0u);
}

TEST(ShopItemTests, ShopItemsGetAllDrinks_IncludesKnownDrinks)
{
    const auto drinks = ShopItemsGetAllDrinks();
    EXPECT_NE(drinks & (1uLL << EnumValue(ShopItem::drink)), 0u);
    EXPECT_NE(drinks & (1uLL << EnumValue(ShopItem::coffee)), 0u);
    EXPECT_NE(drinks & (1uLL << EnumValue(ShopItem::lemonade)), 0u);

    EXPECT_EQ(drinks & (1uLL << EnumValue(ShopItem::burger)), 0u);
}

TEST(ShopItemTests, ShopItemsGetAllContainers_IncludesKnownContainers)
{
    const auto containers = ShopItemsGetAllContainers();
    EXPECT_NE(containers & (1uLL << EnumValue(ShopItem::emptyCan)), 0u);
    EXPECT_NE(containers & (1uLL << EnumValue(ShopItem::emptyBurgerBox)), 0u);
    EXPECT_NE(containers & (1uLL << EnumValue(ShopItem::emptyCup)), 0u);

    EXPECT_EQ(containers & (1uLL << EnumValue(ShopItem::burger)), 0u);
}

TEST(ShopItemTests, PostfixIncrement_AdvancesByOne)
{
    auto item = ShopItem::balloon;
    item++;
    EXPECT_EQ(item, ShopItem::toy);
    item++;
    EXPECT_EQ(item, ShopItem::map);
}

TEST(ShopItemTests, PostfixIncrement_WrapsFromCountBackToBalloon)
{
    auto item = ShopItem::count;
    item++;
    EXPECT_EQ(item, ShopItem::balloon);
}

TEST(ShopItemTests, HasFlag_CombinedFlagMaskMatchesIndividual)
{
    for (uint8_t i = 0; i < EnumValue(ShopItem::count); ++i)
    {
        const auto& d = GetShopItemDescriptor(static_cast<ShopItem>(i));
        const bool combined = d.HasFlag(SHOP_ITEM_FLAG_IS_FOOD | SHOP_ITEM_FLAG_IS_DRINK);
        EXPECT_EQ(combined, d.IsFood() || d.IsDrink()) << "at index " << static_cast<int>(i);
    }
}
