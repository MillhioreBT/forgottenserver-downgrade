#include "otpch.h"

#include "configmanager.h"
#include "game.h"
#include "outfit.h"
#include "player.h"
#include "tile.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

extern Game g_game;

namespace {

constexpr uint16_t citizen = 128;
constexpr uint8_t bothAddons = 3;

void requireLoadedOutfitsAndMounts()
{
	static const bool loaded = [] {
		const auto previous = std::filesystem::current_path();
		std::filesystem::current_path(TFS_SOURCE_DIR);
		const bool ok = Outfits::getInstance().loadFromXml() && g_game.mounts.loadFromXml();
		std::filesystem::current_path(previous);
		return ok;
	}();
	BOOST_REQUIRE(loaded);
}

struct PlayerWithAMountInTheGame
{
	PlayerWithAMountInTheGame()
	{
		requireLoadedOutfitsAndMounts();
		ConfigManager::setBoolean(ConfigManager::ALLOW_CHANGEOUTFIT, true);
		ConfigManager::setBoolean(ConfigManager::FREE_PREMIUM, true);

		mount = &g_game.mounts.getMounts().front();
		player->setName("Test Rider");
		player->setGroup(&group);
		player->setParent(&tile);
		player->setID();
		BOOST_REQUIRE(player->tameMount(mount->id));
		g_game.addPlayer(player);
	}

	~PlayerWithAMountInTheGame()
	{
		g_game.removePlayer(player);
		delete player;
		ConfigManager::setBoolean(ConfigManager::ALLOW_CHANGEOUTFIT, false);
		ConfigManager::setBoolean(ConfigManager::FREE_PREMIUM, false);
	}

	Group group{"player", 0, 0, 0, 1, false};
	StaticTile tile{100, 100, 7};
	Player* player = new Player(nullptr);
	const Mount* mount = nullptr;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_game_change_outfit, PlayerWithAMountInTheGame)

BOOST_AUTO_TEST_CASE(an_outfit_the_player_cannot_wear_leaves_the_speed_alone)
{
	const int32_t speed = player->getSpeed();

	Outfit_t outfit;
	outfit.lookType = citizen;
	outfit.lookAddons = bothAddons;
	outfit.lookMount = mount->clientId;
	for (int attempt = 0; attempt < 3; ++attempt) {
		g_game.playerChangeOutfit(player->getID(), outfit);
	}

	BOOST_TEST(player->getSpeed() == speed);
	BOOST_TEST(!player->isMounted());
}

BOOST_AUTO_TEST_SUITE_END()
