#include "otpch.h"

#include "ban.h"
#include "configmanager.h"

#include <boost/test/unit_test.hpp>

namespace {

constexpr uint32_t ip = 0x0100007F;
constexpr uint32_t otherIp = 0x0200007F;

struct LoginFailureLimitOfThree
{
	LoginFailureLimitOfThree() { ConfigManager::setInteger(ConfigManager::MAX_LOGIN_FAILURES, 3); }
	~LoginFailureLimitOfThree() { ConfigManager::setInteger(ConfigManager::MAX_LOGIN_FAILURES, 0); }

	void failLogins(uint32_t from, int times)
	{
		for (int i = 0; i < times; ++i) {
			bans.onLoginFailed(from);
		}
	}

	Ban bans;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_ban_login_failures, LoginFailureLimitOfThree)

BOOST_AUTO_TEST_CASE(an_ip_below_the_limit_of_failed_logins_can_log_in)
{
	failLogins(ip, 2);
	BOOST_TEST(!bans.isLoginBlocked(ip));
}

BOOST_AUTO_TEST_CASE(an_ip_that_reaches_the_limit_of_failed_logins_is_blocked)
{
	failLogins(ip, 3);
	BOOST_TEST(bans.isLoginBlocked(ip));
}

BOOST_AUTO_TEST_CASE(failed_logins_block_only_the_ip_they_came_from)
{
	failLogins(ip, 3);
	failLogins(otherIp, 2);
	BOOST_TEST(!bans.isLoginBlocked(otherIp));
}

BOOST_AUTO_TEST_CASE(without_a_limit_failed_logins_block_nobody)
{
	ConfigManager::setInteger(ConfigManager::MAX_LOGIN_FAILURES, 0);
	failLogins(ip, 50);
	BOOST_TEST(!bans.isLoginBlocked(ip));
}

BOOST_AUTO_TEST_SUITE_END()
