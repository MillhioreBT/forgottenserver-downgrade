#include "otpch.h"

#include "database.h"
#include "game.h"

#include <boost/test/unit_test.hpp>

extern Game g_game;

namespace {

constexpr std::string_view accountName = "tfs_test_account_storage";
constexpr uint32_t key = 12345;
constexpr int32_t value = 42;
constexpr uint32_t firstAccountWithoutValues = 4000000000;
constexpr uint32_t accountsWithoutValues = 50;

struct AccountWithAStorageValue
{
	AccountWithAStorageValue()
	{
		deleteAccount();
		BOOST_REQUIRE(db.executeQuery(fmt::format("INSERT INTO `accounts` (`name`, `password`) VALUES ({:s}, '')",
		                                          db.escapeString(accountName))));
		id = static_cast<uint32_t>(db.getLastInsertId());
	}

	~AccountWithAStorageValue()
	{
		g_game.setAccountStorageValue(id, key, -1);
		deleteAccount();
	}

	void deleteAccount()
	{
		db.executeQuery(fmt::format("DELETE FROM `accounts` WHERE `name` = {:s}", db.escapeString(accountName)));
	}

	bool valueIsSaved() const
	{
		return db.storeQuery(
		           fmt::format("SELECT `value` FROM `account_storage` WHERE `account_id` = {:d} AND `key` = {:d} "
		                       "AND `value` = {:d}",
		                       id, key, value)) != nullptr;
	}

	Database& db = Database::getInstance();
	uint32_t id = 0;
};

void clearValuesOfAccountsWithoutAny(uint32_t first, uint32_t count)
{
	for (uint32_t accountId = first; accountId < first + count; ++accountId) {
		g_game.setAccountStorageValue(accountId, key, -1);
	}
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_db_account_storage, AccountWithAStorageValue)

BOOST_AUTO_TEST_CASE(accounts_whose_values_were_all_cleared_do_not_stop_the_others_from_being_saved)
{
	clearValuesOfAccountsWithoutAny(firstAccountWithoutValues, accountsWithoutValues);
	g_game.setAccountStorageValue(id, key, value);
	clearValuesOfAccountsWithoutAny(firstAccountWithoutValues + accountsWithoutValues, accountsWithoutValues);

	BOOST_REQUIRE(g_game.saveAccountStorageValues());
	BOOST_TEST(valueIsSaved());
}

BOOST_AUTO_TEST_SUITE_END()
