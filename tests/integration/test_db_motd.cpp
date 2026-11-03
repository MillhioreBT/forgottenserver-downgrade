#include "otpch.h"

#include "configmanager.h"
#include "database.h"
#include "game.h"
#include "tools.h"

#include <boost/test/unit_test.hpp>

extern Game g_game;

namespace {

std::string motdWhoseDigestHasAQuoteOrABackslash()
{
	for (int i = 0;; ++i) {
		std::string motd = "Welcome to the server " + std::to_string(i);
		if (transformToSHA1(motd).find_first_of("'\\") != std::string::npos) {
			return motd;
		}
	}
}

std::string toUpperHex(std::string_view bytes)
{
	std::string hex;
	for (const unsigned char byte : bytes) {
		hex += fmt::format("{:02X}", byte);
	}
	return hex;
}

std::string serverConfig(std::string_view config)
{
	Database& db = Database::getInstance();
	DBResult_ptr result = db.storeQuery(
	    fmt::format("SELECT `value` FROM `server_config` WHERE `config` = {:s}", db.escapeString(config)));
	return result ? std::string{result->getString("value")} : std::string{};
}

void setServerConfig(std::string_view config, std::string_view value)
{
	Database& db = Database::getInstance();
	db.executeQuery(fmt::format("UPDATE `server_config` SET `value` = {:s} WHERE `config` = {:s}",
	                            db.escapeString(value), db.escapeString(config)));
}

struct SavedMotdRestoredAfterwards
{
	~SavedMotdRestoredAfterwards()
	{
		setServerConfig("motd_hash", hash);
		setServerConfig("motd_num", number);
	}

	const std::string hash = serverConfig("motd_hash");
	const std::string number = serverConfig("motd_num");
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_db_motd, SavedMotdRestoredAfterwards)

BOOST_AUTO_TEST_CASE(saveMotdNum_saves_the_hash_of_a_motd_whose_digest_has_a_quote_or_a_backslash)
{
	const std::string motd = motdWhoseDigestHasAQuoteOrABackslash();
	ConfigManager::setString(ConfigManager::MOTD, motd);
	setServerConfig("motd_hash", "");

	g_game.saveMotdNum();
	BOOST_TEST(serverConfig("motd_hash") == toUpperHex(transformToSHA1(motd)));
}

BOOST_AUTO_TEST_SUITE_END()
