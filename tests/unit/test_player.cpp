#include "otpch.h"

#include "player.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_player)

// The progress towards the next level, magic level or skill, in basis points (0-10000); the client shows it as a
// percent, so callers divide by 100 (#55: sending the basis points as a byte overflowed it)
BOOST_AUTO_TEST_CASE(basis_point_level_is_the_progress_in_ten_thousandths)
{
	BOOST_TEST(Player::getBasisPointLevel(0, 100) == 0);
	BOOST_TEST(Player::getBasisPointLevel(50, 100) == 5000);
	BOOST_TEST(Player::getBasisPointLevel(1, 3) == 3333);
	BOOST_TEST(Player::getBasisPointLevel(99, 100) == 9900);
	BOOST_TEST(Player::getBasisPointLevel(100, 100) == 10000);
	BOOST_TEST(Player::getBasisPointLevel(1234567890123ull, 2469135780246ull) == 5000);
}

BOOST_AUTO_TEST_CASE(basis_point_level_fits_a_percent_byte)
{
	for (uint64_t count = 0; count <= 1000; ++count) {
		BOOST_TEST(Player::getBasisPointLevel(count, 1000) / 100 <= 100, "count " << count);
	}
}

BOOST_AUTO_TEST_CASE(basis_point_level_is_zero_without_a_next_level)
{
	BOOST_TEST(Player::getBasisPointLevel(0, 0) == 0);
	BOOST_TEST(Player::getBasisPointLevel(500, 0) == 0);
}

// Regression: past the next level the value was converted to uint16_t before the range check; from 6.5 times the next
// level that conversion was undefined behavior, and on x86 it wrapped (7 times gave 4464, 44%)
BOOST_AUTO_TEST_CASE(basis_point_level_is_zero_past_the_next_level)
{
	BOOST_TEST(Player::getBasisPointLevel(101, 100) == 0);
	BOOST_TEST(Player::getBasisPointLevel(7, 1) == 0);
	BOOST_TEST(Player::getBasisPointLevel(7000, 1000) == 0);
	BOOST_TEST(Player::getBasisPointLevel(std::numeric_limits<uint64_t>::max(), 1) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
