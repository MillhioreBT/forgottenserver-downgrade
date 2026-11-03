#define BOOST_TEST_MODULE tfs_integration
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API

#include "otpch.h"

#include "configmanager.h"
#include "database.h"

#include <boost/test/included/unit_test.hpp>

namespace {

constexpr int skippedExitCode = 77;

const char* env(const char* name, const char* defaultValue)
{
	const char* value = std::getenv(name);
	return value && *value ? value : defaultValue;
}

} // namespace

int main(int argc, char* argv[])
{
	const char* database = env("TFS_TEST_MYSQL_DATABASE", nullptr);
	if (!database) {
		std::cout << "Integration tests skipped: TFS_TEST_MYSQL_DATABASE is not set (see tests/README.md)" << std::endl;
		return skippedExitCode;
	}

	ConfigManager::setString(ConfigManager::MYSQL_DB, database);
	ConfigManager::setString(ConfigManager::MYSQL_HOST, env("TFS_TEST_MYSQL_HOST", "127.0.0.1"));
	ConfigManager::setString(ConfigManager::MYSQL_USER, env("TFS_TEST_MYSQL_USER", "forgottenserver"));
	ConfigManager::setString(ConfigManager::MYSQL_PASS, env("TFS_TEST_MYSQL_PASSWORD", ""));
	ConfigManager::setString(ConfigManager::MYSQL_SOCK, env("TFS_TEST_MYSQL_SOCK", ""));
	ConfigManager::setInteger(ConfigManager::SQL_PORT, std::stoi(env("TFS_TEST_MYSQL_PORT", "3306")));

	if (!Database::getInstance().connect()) {
		std::cout << "Integration tests failed: cannot connect to the database " << database << std::endl;
		return 1;
	}

	return boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
