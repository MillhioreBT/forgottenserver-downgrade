#include "otpch.h"

#include "database.h"
#include "guild.h"
#include "house.h"
#include "player.h"

#include <boost/test/unit_test.hpp>

namespace {

constexpr std::string_view accountName = "tfs_test_house_access";
constexpr std::string_view guildName = "Tfs Test Guild";

struct GuildMemberWithARank
{
	GuildMemberWithARank()
	{
		deleteAccount();
		BOOST_REQUIRE(db.executeQuery(fmt::format("INSERT INTO `accounts` (`name`, `password`) VALUES ({:s}, '')",
		                                          db.escapeString(accountName))));
		const auto accountId = db.getLastInsertId();
		BOOST_REQUIRE(db.executeQuery(fmt::format(
		    "INSERT INTO `players` (`name`, `account_id`, `conditions`) VALUES ('Tfs Test Leader', {:d}, '')",
		    accountId)));
		const auto ownerId = db.getLastInsertId();
		BOOST_REQUIRE(db.executeQuery(
		    fmt::format("INSERT INTO `guilds` (`name`, `ownerid`, `creationdata`) VALUES ({:s}, {:d}, 0)",
		                db.escapeString(guildName), ownerId)));

		guild.reset(IOGuild::loadGuild(static_cast<uint32_t>(db.getLastInsertId())));
		BOOST_REQUIRE(guild);
		for (const GuildRank_ptr& rank : guild->getRanks()) {
			if (rank->name == "a Vice-Leader") {
				member.setGuildRank(rank);
			}
		}
		BOOST_REQUIRE(member.getGuildRank());
	}

	~GuildMemberWithARank() { deleteAccount(); }

	void deleteAccount()
	{
		db.executeQuery(fmt::format("DELETE FROM `guilds` WHERE `name` = {:s}", db.escapeString(guildName)));
		db.executeQuery(fmt::format("DELETE FROM `accounts` WHERE `name` = {:s}", db.escapeString(accountName)));
	}

	bool isInList(std::string_view list)
	{
		AccessList accessList;
		accessList.parseList(list);
		return accessList.isInList(&member);
	}

	Database& db = Database::getInstance();
	std::unique_ptr<Guild> guild;
	Player member{nullptr};
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_db_house_access_list, GuildMemberWithARank)

BOOST_AUTO_TEST_CASE(a_guild_rank_line_lets_in_the_members_of_that_rank)
{
	BOOST_TEST(isInList("Tfs Test Guild@a Vice-Leader"));
	BOOST_TEST(isInList("tfs test guild @ a vice-leader"));
}

BOOST_AUTO_TEST_CASE(a_guild_rank_line_keeps_out_the_members_of_other_ranks)
{
	BOOST_TEST(!isInList("Tfs Test Guild@a Member"));
}

BOOST_AUTO_TEST_CASE(a_guild_line_lets_in_every_member) { BOOST_TEST(isInList("@Tfs Test Guild")); }

BOOST_AUTO_TEST_SUITE_END()
