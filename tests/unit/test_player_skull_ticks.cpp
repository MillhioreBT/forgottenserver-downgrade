#include "otpch.h"

#include "creature.h"
#include "game.h"
#include "player.h"
#include "tile.h"
#include "vocation.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

extern Game g_game;
extern Vocations g_vocations;

namespace {

void requireLoadedVocations()
{
	static const bool loaded = [] {
		const auto previous = std::filesystem::current_path();
		std::filesystem::current_path(TFS_SOURCE_DIR);
		const bool ok = g_vocations.loadFromXml();
		std::filesystem::current_path(previous);
		return ok;
	}();
	BOOST_REQUIRE(loaded);
}

struct PlayerWithARedSkull
{
	PlayerWithARedSkull()
	{
		requireLoadedVocations();
		BOOST_REQUIRE(player->setVocation(VOCATION_NONE));
		player->setGroup(&group);
		player->setParent(&tile);
		player->setSkull(SKULL_RED);
		player->setSkullTicks(1);
	}

	~PlayerWithARedSkull() { delete player; }

	void thinkFor(uint32_t milliseconds)
	{
		for (uint32_t elapsed = 0; elapsed < milliseconds; elapsed += EVENT_CREATURE_THINK_INTERVAL) {
			player->onThink(EVENT_CREATURE_THINK_INTERVAL);
		}
	}

	Group group{"player", 0, 0, 0, 1, false};
	StaticTile tile{100, 100, 7};
	Player* player = new Player(nullptr);
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_player_skull_ticks, PlayerWithARedSkull)

BOOST_AUTO_TEST_CASE(a_red_skull_wears_off_while_the_player_is_online)
{
	BOOST_REQUIRE(g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED);

	thinkFor(500);
	BOOST_TEST(static_cast<int>(player->getSkull()) == static_cast<int>(SKULL_RED));

	thinkFor(1000);
	BOOST_TEST(player->getSkullTicks() == 0);
	BOOST_TEST(static_cast<int>(player->getSkull()) == static_cast<int>(SKULL_NONE));
}

BOOST_AUTO_TEST_SUITE_END()
