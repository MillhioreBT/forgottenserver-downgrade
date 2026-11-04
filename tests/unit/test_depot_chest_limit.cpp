#include "otpch.h"

#include "depotchest.h"
#include "depotlocker.h"
#include "tile.h"
#include "unit/loadeditems.h"

#include <boost/test/unit_test.hpp>

namespace {

struct LoadedItems
{
	LoadedItems() { requireLoadedItems(); }
};

struct FullDepotChestInALockerOnATile : LoadedItems
{
	FullDepotChestInALockerOnATile()
	{
		tile.internalAddThing(locker);
		chest->setMaxDepotItems(2);
		locker->internalAddThing(chest);
		chest->internalAddThing(bag);
		chest->internalAddThing(coins);
	}

	StaticTile tile{100, 100, 7};
	DepotLocker* locker = new DepotLocker(ITEM_LOCKER);
	DepotChest* chest = new DepotChest(ITEM_DEPOT);
	Container* bag = Item::CreateItem(ITEM_BAG)->getContainer();
	Item* coins = Item::CreateItem(ITEM_GOLD_COIN, 10);
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_depot_chest_limit, FullDepotChestInALockerOnATile)

BOOST_AUTO_TEST_CASE(a_full_depot_chest_takes_no_more_items)
{
	const std::unique_ptr<Item> more{Item::CreateItem(ITEM_PLATINUM_COIN)};
	BOOST_TEST(static_cast<int>(chest->queryAdd(INDEX_WHEREEVER, *more, 1, 0)) ==
	           static_cast<int>(RETURNVALUE_DEPOTISFULL));
}

BOOST_AUTO_TEST_CASE(a_container_in_a_full_depot_chest_takes_no_more_items)
{
	const std::unique_ptr<Item> more{Item::CreateItem(ITEM_PLATINUM_COIN)};
	BOOST_TEST(static_cast<int>(bag->queryAdd(INDEX_WHEREEVER, *more, 1, 0)) ==
	           static_cast<int>(RETURNVALUE_DEPOTISFULL));
}

BOOST_AUTO_TEST_CASE(items_can_move_into_a_container_in_the_same_full_depot_chest)
{
	BOOST_TEST(static_cast<int>(bag->queryAdd(INDEX_WHEREEVER, *coins, 10, 0)) ==
	           static_cast<int>(RETURNVALUE_NOERROR));
}

BOOST_AUTO_TEST_CASE(splitting_a_stack_into_a_container_in_a_full_depot_chest_is_refused)
{
	BOOST_TEST(static_cast<int>(bag->queryAdd(INDEX_WHEREEVER, *coins, 4, 0)) ==
	           static_cast<int>(RETURNVALUE_DEPOTISFULL));
}

BOOST_AUTO_TEST_SUITE_END()
