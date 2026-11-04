#include "otpch.h"

#include "depotchest.h"
#include "depotlocker.h"
#include "player.h"
#include "tile.h"
#include "unit/loadeditems.h"

#include <boost/test/unit_test.hpp>

namespace {

struct LoadedItems
{
	LoadedItems() { requireLoadedItems(); }
};

struct OpenedDepotAndAPlayerOnTiles : LoadedItems
{
	OpenedDepotAndAPlayerOnTiles()
	{
		depotTile.internalAddThing(locker);
		locker->internalAddThing(chest);
		chest->internalAddThing(bag);
		chest->internalAddThing(coinsInChest);
		bag->internalAddThing(coinsInBag);

		groundTile.internalAddThing(coinsOnTheGround);

		player.setParent(&playerTile);
		static_cast<Cylinder&>(player).internalAddThing(CONST_SLOT_BACKPACK, backpack);
		backpack->internalAddThing(coinsInBackpack);
	}

	StaticTile depotTile{100, 100, 7};
	DepotLocker* locker = new DepotLocker(ITEM_LOCKER);
	DepotChest* chest = new DepotChest(ITEM_DEPOT);
	Container* bag = Item::CreateItem(ITEM_BAG)->getContainer();
	Item* coinsInChest = Item::CreateItem(ITEM_PLATINUM_COIN, 10);
	Item* coinsInBag = Item::CreateItem(ITEM_PLATINUM_COIN, 10);

	StaticTile groundTile{101, 100, 7};
	Item* coinsOnTheGround = Item::CreateItem(ITEM_PLATINUM_COIN, 10);

	StaticTile playerTile{102, 100, 7};
	Player player{nullptr};
	Container* backpack = Item::CreateItem(ITEM_BACKPACK)->getContainer();
	Item* coinsInBackpack = Item::CreateItem(ITEM_PLATINUM_COIN, 10);

	const std::unique_ptr<Item> slotItem{Item::CreateItem(ITEM_BAG)};
};

int queryMoveOntoSlotItem(const Item* slotItem, const Item& item)
{
	return static_cast<int>(Player::queryMoveOntoSlotItem(slotItem, item));
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_player_slot_exchange, OpenedDepotAndAPlayerOnTiles)

BOOST_AUTO_TEST_CASE(an_item_in_the_opened_depot_chest_swaps_with_the_slot_item)
{
	BOOST_TEST(queryMoveOntoSlotItem(slotItem.get(), *coinsInChest) == static_cast<int>(RETURNVALUE_NEEDEXCHANGE));
}

BOOST_AUTO_TEST_CASE(an_item_in_a_container_in_the_opened_depot_chest_swaps_with_the_slot_item)
{
	BOOST_TEST(queryMoveOntoSlotItem(slotItem.get(), *coinsInBag) == static_cast<int>(RETURNVALUE_NEEDEXCHANGE));
}

BOOST_AUTO_TEST_CASE(an_item_carried_by_the_player_swaps_with_the_slot_item)
{
	BOOST_TEST(queryMoveOntoSlotItem(slotItem.get(), *coinsInBackpack) == static_cast<int>(RETURNVALUE_NEEDEXCHANGE));
}

BOOST_AUTO_TEST_CASE(an_item_on_the_ground_does_not_swap_with_the_slot_item)
{
	BOOST_TEST(queryMoveOntoSlotItem(slotItem.get(), *coinsOnTheGround) == static_cast<int>(RETURNVALUE_NOTENOUGHROOM));
}

BOOST_AUTO_TEST_CASE(an_item_onto_a_stack_of_the_same_item_is_added_to_it)
{
	const std::unique_ptr<Item> coinsInSlot{Item::CreateItem(ITEM_PLATINUM_COIN, 5)};

	BOOST_TEST(queryMoveOntoSlotItem(coinsInSlot.get(), *coinsInChest) == static_cast<int>(RETURNVALUE_NOERROR));
}

BOOST_AUTO_TEST_CASE(an_item_into_an_empty_slot_is_added_to_it)
{
	BOOST_TEST(queryMoveOntoSlotItem(nullptr, *coinsOnTheGround) == static_cast<int>(RETURNVALUE_NOERROR));
}

BOOST_AUTO_TEST_SUITE_END()
