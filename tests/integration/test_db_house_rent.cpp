#include "otpch.h"

#include "database.h"
#include "game.h"
#include "house.h"
#include "town.h"
#include "vocation.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

extern Game g_game;
extern Vocations g_vocations;

namespace {

constexpr std::string_view accountName = "tfs_test_house_rent";
constexpr uint32_t townId = 1000000;
constexpr uint32_t houseId = 4000000000;
constexpr uint32_t rent = 1000;

void requireLoadedGroupsAndVocations()
{
	static const bool loaded = [] {
		const auto previous = std::filesystem::current_path();
		std::filesystem::current_path(TFS_SOURCE_DIR);
		const bool ok = g_game.groups.load() && g_vocations.loadFromXml();
		std::filesystem::current_path(previous);
		return ok;
	}();
	BOOST_REQUIRE(loaded);
}

struct HouseWhoseOwnerCanPayTheRent
{
	HouseWhoseOwnerCanPayTheRent()
	{
		requireLoadedGroupsAndVocations();
		deleteAccount();
		BOOST_REQUIRE(db.executeQuery(
		    fmt::format("INSERT INTO `accounts` (`name`, `password`, `premium_ends_at`) VALUES ({:s}, '', {:d})",
		                db.escapeString(accountName), time(nullptr) + 30 * 24 * 60 * 60)));
		const auto accountId = db.getLastInsertId();
		BOOST_REQUIRE(db.executeQuery(fmt::format(
		    "INSERT INTO `players` (`name`, `account_id`, `town_id`, `balance`, `conditions`) VALUES ('Tfs Test Tenant', "
		    "{:d}, {:d}, {:d}, '')",
		    accountId, townId, 10 * rent)));
		const auto ownerId = static_cast<uint32_t>(db.getLastInsertId());

		if (!g_game.map.towns.getTown(townId)) {
			g_game.map.towns.addTown(townId, new Town(townId));
		}
		house = g_game.map.houses.getHouse(houseId);
		if (!house) {
			house = g_game.map.houses.addHouse(houseId);
		}
		house->setTownId(townId);
		house->setRent(rent);
		house->setOwner(ownerId, false);
		house->setPaidUntil(0);
	}

	~HouseWhoseOwnerCanPayTheRent()
	{
		house->setOwner(0, false);
		deleteAccount();
	}

	void deleteAccount()
	{
		db.executeQuery(fmt::format("DELETE FROM `accounts` WHERE `name` = {:s}", db.escapeString(accountName)));
	}

	Database& db = Database::getInstance();
	House* house = nullptr;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_db_house_rent, HouseWhoseOwnerCanPayTheRent)

BOOST_AUTO_TEST_CASE(paying_the_rent_clears_the_warnings_of_earlier_periods)
{
	house->setPayRentWarnings(6);

	g_game.map.houses.payHouses(RENTPERIOD_DAILY);
	BOOST_TEST(house->getPaidUntil() > time(nullptr));
	BOOST_TEST(house->getPayRentWarnings() == 0u);
}

BOOST_AUTO_TEST_SUITE_END()
