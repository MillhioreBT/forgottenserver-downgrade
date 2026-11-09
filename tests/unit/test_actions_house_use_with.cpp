#include "otpch.h"

#include "actions.h"
#include "configmanager.h"
#include "game.h"
#include "house.h"
#include "housetile.h"
#include "player.h"
#include "unit/immunecreature.h"
#include "unit/loadeditems.h"
#include "unit/scriptsinterface.h"

#include <boost/test/unit_test.hpp>

extern Actions* g_actions;
extern Game g_game;

namespace {

constexpr uint16_t rope = 2120;
const Position playerPosition{300, 100, 7};
const Position housePosition{301, 100, 7};
const Position streetPosition{299, 100, 7};

struct HouseOnTheMap
{
	HouseOnTheMap()
	{
		g_game.map.setTile(playerPosition, playerTile);
		g_game.map.setTile(streetPosition, streetTile);
		g_game.map.setTile(housePosition, houseTile);
	}

	House* house = new House(1);
	Tile* playerTile = new DynamicTile(playerPosition.x, playerPosition.y, playerPosition.z);
	Tile* streetTile = new DynamicTile(streetPosition.x, streetPosition.y, streetPosition.z);
	Tile* houseTile = new HouseTile(housePosition.x, housePosition.y, housePosition.z, house);
};

const HouseOnTheMap& houseOnTheMap()
{
	static const HouseOnTheMap map;
	return map;
}

struct UninvitedPlayerNextToAHouse : LoadingScripts
{
	UninvitedPlayerNextToAHouse()
	{
		requireLoadedItems();
		ConfigManager::setBoolean(ConfigManager::ONLY_INVITED_CAN_MOVE_HOUSE_ITEMS, true);
		g_actions = new Actions();
		evaluate(
		    fmt::format("local action = Action() "
		                "action:onUse(function() used = true return true end) "
		                "action:id({:d}) "
		                "action:register()",
		                rope));

		player.setGUID(5);
		player.setGroup(&group);
		playerTile->internalAddThing(&player);
		player.setParent(playerTile);
	}

	~UninvitedPlayerNextToAHouse()
	{
		for (Tile* tile : {playerTile, streetTile, houseTile}) {
			while (tile->getThingCount() > 0) {
				tile->removeThing(tile->getThing(0), 1);
			}
		}
		delete g_actions;
		g_actions = nullptr;
		ConfigManager::setBoolean(ConfigManager::ONLY_INVITED_CAN_MOVE_HOUSE_ITEMS, false);
	}

	Item* ropeOn(Tile* tile)
	{
		Item* item = Item::CreateItem(rope);
		tile->internalAddThing(item);
		return item;
	}

	bool useWith(Item* item, const Position& toPosition)
	{
		evaluate("used = false");
		g_actions->useItemEx(&player, item->getPosition(), toPosition, 0, item, false);
		return evaluate("return used") == "true";
	}

	Tile* playerTile = houseOnTheMap().playerTile;
	Tile* streetTile = houseOnTheMap().streetTile;
	Tile* houseTile = houseOnTheMap().houseTile;
	Group group{"player", 0, 0, 0, 1, false};
	Player player{nullptr};
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_actions_house_use_with, UninvitedPlayerNextToAHouse)

BOOST_AUTO_TEST_CASE(an_uninvited_player_cannot_use_an_item_lying_in_a_house)
{
	BOOST_TEST(!useWith(ropeOn(houseTile), streetPosition));
}

BOOST_AUTO_TEST_CASE(an_uninvited_player_cannot_use_an_item_on_a_house_tile)
{
	BOOST_TEST(!useWith(ropeOn(playerTile), housePosition));
}

BOOST_AUTO_TEST_CASE(an_uninvited_player_can_use_an_item_on_a_creature_in_a_house)
{
	ImmuneCreature creature;
	houseTile->internalAddThing(&creature);
	creature.setParent(houseTile);

	const bool used = useWith(ropeOn(playerTile), housePosition);
	houseTile->removeThing(&creature, 1);
	BOOST_TEST(used);
}

BOOST_AUTO_TEST_CASE(an_uninvited_player_can_use_an_item_outside_houses)
{
	BOOST_TEST(useWith(ropeOn(playerTile), streetPosition));
}

BOOST_AUTO_TEST_SUITE_END()
