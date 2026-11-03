#include "otpch.h"

#include "database.h"
#include "iologindata.h"
#include "tools.h"

#include <boost/test/unit_test.hpp>

namespace {

constexpr std::string_view accountName = "tfs_test_account";
constexpr std::string_view password = "correct horse";
constexpr time_t premiumEndsAt = 1900000000;
constexpr std::string_view base32Secret = "JBSWY3DPEHPK3PXP";
const std::string decodedSecret = "Hello!\xDE\xAD\xBE\xEF";

struct TestAccount
{
	TestAccount()
	{
		deleteLeftoversOfACrashedRun();

		BOOST_REQUIRE(db.executeQuery(fmt::format(
		    "INSERT INTO `accounts` (`name`, `password`, `secret`, `type`, `premium_ends_at`, `email`, `tibia_coins`) "
		    "VALUES ({:s}, SHA1({:s}), {:s}, {:d}, {:d}, 'tfs_test@example.com', 50)",
		    db.escapeString(accountName), db.escapeString(password), db.escapeString(base32Secret),
		    static_cast<int>(ACCOUNT_TYPE_GAMEMASTER), premiumEndsAt)));
		id = static_cast<uint32_t>(db.getLastInsertId());

		bravo = addPlayer("Tfs Test Bravo");
		alpha = addPlayer("Tfs Test Alpha");
		deleted = addPlayer("Tfs Test Deleted", true);
	}
	~TestAccount() { deleteAccountWithItsPlayers(); }

	void deleteLeftoversOfACrashedRun() { deleteAccountWithItsPlayers(); }

	uint32_t addPlayer(std::string_view name, bool isDeleted = false)
	{
		BOOST_REQUIRE(db.executeQuery(
		    fmt::format("INSERT INTO `players` (`name`, `account_id`, `deletion`, `conditions`) VALUES ({:s}, {:d}, "
		                "{:d}, '')",
		                db.escapeString(name), id, isDeleted ? time(nullptr) : 0)));
		return static_cast<uint32_t>(db.getLastInsertId());
	}

	void deleteAccountWithItsPlayers()
	{
		db.executeQuery(fmt::format("DELETE FROM `accounts` WHERE `name` = {:s}", db.escapeString(accountName)));
	}

	Database& db = Database::getInstance();
	uint32_t id = 0, alpha = 0, bravo = 0, deleted = 0;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_db_accounts, TestAccount)

BOOST_AUTO_TEST_CASE(login_with_the_right_password_loads_the_account)
{
	Account account;
	BOOST_REQUIRE(IOLoginData::loginserverAuthentication(accountName, password, account));

	BOOST_TEST(account.id == id);
	BOOST_TEST(account.name == accountName);
	BOOST_TEST(static_cast<int>(account.accountType) == static_cast<int>(ACCOUNT_TYPE_GAMEMASTER));
	BOOST_TEST(account.premiumEndsAt == premiumEndsAt);
	BOOST_TEST(account.tibiaCoins == 50u);
	BOOST_TEST(account.key == decodedSecret);
}

BOOST_AUTO_TEST_CASE(login_lists_the_characters_by_name_without_deleted_ones)
{
	Account account;
	BOOST_REQUIRE(IOLoginData::loginserverAuthentication(accountName, password, account));
	BOOST_TEST(account.characters == (std::vector<std::string>{"Tfs Test Alpha", "Tfs Test Bravo"}),
	           boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(login_fails_with_a_wrong_password_or_account)
{
	Account account;
	BOOST_TEST(!IOLoginData::loginserverAuthentication(accountName, "wrong", account));
	BOOST_TEST(!IOLoginData::loginserverAuthentication(accountName, "", account));
	BOOST_TEST(!IOLoginData::loginserverAuthentication("tfs_test_nobody", password, account));
}

BOOST_AUTO_TEST_CASE(game_login_finds_the_character_by_account_name_or_email)
{
	BOOST_TEST((IOLoginData::gameworldAuthentication(accountName, password, "Tfs Test Alpha") ==
	            std::pair<uint32_t, uint32_t>{id, alpha}));
	BOOST_TEST((IOLoginData::gameworldAuthentication("tfs_test@example.com", password, "Tfs Test Bravo") ==
	            std::pair<uint32_t, uint32_t>{id, bravo}));
}

BOOST_AUTO_TEST_CASE(game_login_fails_for_deleted_or_foreign_characters_and_wrong_passwords)
{
	const std::pair<uint32_t, uint32_t> none{0, 0};
	BOOST_TEST((IOLoginData::gameworldAuthentication(accountName, password, "Tfs Test Deleted") == none));
	BOOST_TEST((IOLoginData::gameworldAuthentication(accountName, "wrong", "Tfs Test Alpha") == none));
	BOOST_TEST((IOLoginData::gameworldAuthentication(accountName, password, "Tfs Test Nobody") == none));
}

BOOST_AUTO_TEST_CASE(names_and_ids_resolve_both_ways)
{
	BOOST_TEST(IOLoginData::getNameByGuid(alpha) == "Tfs Test Alpha");
	BOOST_TEST(IOLoginData::getNameByGuid(0) == "");
	BOOST_TEST(IOLoginData::getGuidByName("Tfs Test Bravo") == bravo);
	BOOST_TEST(IOLoginData::getGuidByName("Tfs Test Nobody") == 0u);
	BOOST_TEST(IOLoginData::getAccountIdByPlayerName("Tfs Test Alpha") == id);
	BOOST_TEST(IOLoginData::getAccountIdByPlayerId(bravo) == id);
}

BOOST_AUTO_TEST_CASE(formatPlayerName_fixes_the_case)
{
	std::string name = "tfs test alpha";
	BOOST_TEST(IOLoginData::formatPlayerName(name));
	BOOST_TEST(name == "Tfs Test Alpha");

	std::string unknown = "tfs test nobody";
	BOOST_TEST(!IOLoginData::formatPlayerName(unknown));
}

BOOST_AUTO_TEST_CASE(account_type_premium_and_coins_are_saved)
{
	IOLoginData::setAccountType(id, ACCOUNT_TYPE_TUTOR);
	BOOST_TEST(static_cast<int>(IOLoginData::getAccountType(id)) == static_cast<int>(ACCOUNT_TYPE_TUTOR));

	IOLoginData::updatePremiumTime(id, premiumEndsAt + 86400);
	BOOST_TEST(IOLoginData::loadAccount(id).premiumEndsAt == premiumEndsAt + 86400);

	IOLoginData::updateTibiaCoins(id, 1234);
	BOOST_TEST(IOLoginData::getTibiaCoins(id) == 1234u);
}

BOOST_AUTO_TEST_CASE(vip_entries_are_added_and_removed)
{
	IOLoginData::addVIPEntry(id, alpha);
	IOLoginData::addVIPEntry(id, bravo);

	std::map<uint32_t, std::string> entries;
	for (const auto& entry : IOLoginData::getVIPEntries(id)) {
		entries.emplace(entry.guid, entry.name);
	}
	BOOST_TEST(entries.size() == 2u);
	BOOST_TEST(entries[alpha] == "Tfs Test Alpha");
	BOOST_TEST(entries[bravo] == "Tfs Test Bravo");

	IOLoginData::removeVIPEntry(id, alpha);
	auto remaining = IOLoginData::getVIPEntries(id);
	BOOST_TEST(std::distance(remaining.begin(), remaining.end()) == 1);
	BOOST_TEST(remaining.front().guid == bravo);
}

BOOST_AUTO_TEST_SUITE_END()
