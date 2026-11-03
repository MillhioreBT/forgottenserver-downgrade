#include "otpch.h"

#include "account.h"

#include <boost/test/unit_test.hpp>

namespace {

constexpr time_t now = 1700000000;
constexpr time_t day = 86400;

Account premiumUntil(time_t premiumEndsAt)
{
	Account account;
	account.premiumEndsAt = premiumEndsAt;
	return account;
}

} // namespace

BOOST_AUTO_TEST_SUITE(test_account)

// Regression: the character list sent premiumEndsAt - now / 86400 (operator precedence), a timestamp, as the days
// left (#59)
BOOST_AUTO_TEST_CASE(premium_days_left_counts_whole_days)
{
	BOOST_TEST(premiumUntil(now + 30 * day).getPremiumDaysLeft(now) == 30);
	BOOST_TEST(premiumUntil(now + 30 * day + day / 2).getPremiumDaysLeft(now) == 30);
	BOOST_TEST(premiumUntil(now + day).getPremiumDaysLeft(now) == 1);
	BOOST_TEST(premiumUntil(now + day - 1).getPremiumDaysLeft(now) == 0);
}

BOOST_AUTO_TEST_CASE(no_premium_days_once_it_ended)
{
	BOOST_TEST(premiumUntil(now).getPremiumDaysLeft(now) == 0);
	BOOST_TEST(premiumUntil(now - 5 * day).getPremiumDaysLeft(now) == 0);
	BOOST_TEST(premiumUntil(0).getPremiumDaysLeft(now) == 0);
}

BOOST_AUTO_TEST_CASE(premium_days_left_stay_below_the_free_premium_marker)
{
	// 0xFFFF tells the client the server has free premium
	BOOST_TEST(premiumUntil(now + 70000 * day).getPremiumDaysLeft(now) == 0xFFFE);
	BOOST_TEST(premiumUntil(now + 65534 * day).getPremiumDaysLeft(now) == 65534);
}

BOOST_AUTO_TEST_SUITE_END()
