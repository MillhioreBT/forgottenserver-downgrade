#include "otpch.h"

#include "house.h"
#include "housetile.h"
#include "player.h"
#include "unit/immunecreature.h"

#include <boost/test/unit_test.hpp>

namespace {

struct HouseTileAndAPlayer
{
	HouseTileAndAPlayer()
	{
		player.setGUID(5);
		player.setGroup(&guestGroup);
	}

	ReturnValue queryAdd(const Creature& creature) const { return tile.queryAdd(0, creature, 1, FLAG_NOLIMIT); }

	House house{1};
	HouseTile tile{100, 100, 7, &house};
	Group guestGroup{"player", 0, 0, 0, 1, false};
	Group houseEditorGroup{"god", PlayerFlag_CanEditHouses, 0, 0, 3, true};
	Player player{nullptr};
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_house_tile_query_add, HouseTileAndAPlayer)

BOOST_AUTO_TEST_CASE(a_teleport_does_not_put_an_uninvited_player_into_a_house)
{
	BOOST_TEST(static_cast<int>(queryAdd(player)) == static_cast<int>(RETURNVALUE_PLAYERISNOTINVITED));
}

BOOST_AUTO_TEST_CASE(a_teleport_does_not_put_a_monster_into_a_house)
{
	ImmuneCreature monster;
	BOOST_TEST(static_cast<int>(queryAdd(monster)) == static_cast<int>(RETURNVALUE_NOTPOSSIBLE));
}

BOOST_AUTO_TEST_CASE(a_teleport_puts_an_invited_player_into_a_house)
{
	player.setGroup(&houseEditorGroup);
	BOOST_TEST(static_cast<int>(queryAdd(player)) == static_cast<int>(RETURNVALUE_NOERROR));
}

BOOST_AUTO_TEST_SUITE_END()
