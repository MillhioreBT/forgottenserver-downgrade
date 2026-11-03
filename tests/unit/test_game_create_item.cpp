#include "otpch.h"

#include "game.h"
#include "movement.h"
#include "unit/loadeditems.h"
#include "unit/scriptsinterface.h"

#include <boost/test/unit_test.hpp>

extern Game g_game;
extern MoveEvents* g_moveEvents;

namespace {

constexpr uint16_t grass = 4526;

struct ItemsOnTheMap : LoadingScripts
{
	ItemsOnTheMap()
	{
		requireLoadedItems();
		g_moveEvents = new MoveEvents();
	}

	~ItemsOnTheMap()
	{
		delete g_moveEvents;
		g_moveEvents = nullptr;
	}

	Tile* grassTileAt(const Position& position)
	{
		Tile* tile = new DynamicTile(position.x, position.y, position.z);
		g_game.map.setTile(position, tile);
		tile->internalAddThing(Item::CreateItem(grass));
		return tile;
	}
};

std::vector<uint16_t> sortedCountsOf(const Tile* tile, uint16_t itemId)
{
	std::vector<uint16_t> counts;
	if (const TileItemVector* items = tile->getItemList()) {
		for (const Item* item : *items) {
			if (item->getID() == itemId) {
				counts.push_back(item->getItemCount());
			}
		}
	}
	std::sort(counts.begin(), counts.end());
	return counts;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_game_create_item, ItemsOnTheMap)

BOOST_AUTO_TEST_CASE(createItem_on_a_stack_of_the_same_item_returns_the_item_it_placed)
{
	Tile* tile = grassTileAt({100, 100, 7});
	tile->internalAddThing(Item::CreateItem(ITEM_GOLD_COIN, 5));

	BOOST_TEST(evaluate("return Game.createItem(2148, 10, Position(100, 100, 7)):getCount()") == "10");
	BOOST_TEST(sortedCountsOf(tile, ITEM_GOLD_COIN) == (std::vector<uint16_t>{5, 10}),
	           boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(createItem_on_a_tile_returns_an_item_on_that_tile)
{
	grassTileAt({101, 100, 7});

	BOOST_TEST(evaluate("return Game.createItem(2148, 10, Position(101, 100, 7)):getPosition() == "
	                    "Position(101, 100, 7)") == "true");
}

BOOST_AUTO_TEST_SUITE_END()
