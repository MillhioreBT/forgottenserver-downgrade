#include "otpch.h"

#include "database.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_database)

// Regression: getNumber<uint8_t>("randomizemount") read the column as a character, so "0" was 48 and randomizeMount
// was always true (#62). One-byte integers must be read as numbers.
BOOST_AUTO_TEST_CASE(parseNumber_reads_one_byte_integers_as_numbers)
{
	BOOST_TEST(DBResult::parseNumber<uint8_t>("0") == 0);
	BOOST_TEST(DBResult::parseNumber<uint8_t>("1") == 1);
	BOOST_TEST(DBResult::parseNumber<uint8_t>("10") == 10);
	BOOST_TEST(DBResult::parseNumber<uint8_t>("255") == 255);

	BOOST_TEST(DBResult::parseNumber<int8_t>("-128") == -128);
	BOOST_TEST(DBResult::parseNumber<int8_t>("127") == 127);
}

BOOST_AUTO_TEST_CASE(parseNumber_gives_zero_when_the_value_does_not_fit)
{
	BOOST_TEST(DBResult::parseNumber<uint8_t>("256") == 0);
	BOOST_TEST(DBResult::parseNumber<uint8_t>("-1") == 0);
	BOOST_TEST(DBResult::parseNumber<int8_t>("128") == 0);
	BOOST_TEST(DBResult::parseNumber<int8_t>("-129") == 0);
	BOOST_TEST(DBResult::parseNumber<uint16_t>("65536") == 0);
}

BOOST_AUTO_TEST_CASE(parseNumber_reads_wider_types)
{
	BOOST_TEST(DBResult::parseNumber<uint16_t>("65535") == 65535);
	BOOST_TEST(DBResult::parseNumber<int32_t>("-2147483648") == std::numeric_limits<int32_t>::min());
	BOOST_TEST(DBResult::parseNumber<uint32_t>("4000000000") == 4000000000u);
	BOOST_TEST(DBResult::parseNumber<int64_t>("-9000000000") == -9000000000ll);
	BOOST_TEST(DBResult::parseNumber<uint64_t>("18446744073709551615") == std::numeric_limits<uint64_t>::max());
	BOOST_TEST(DBResult::parseNumber<time_t>("1700000000") == time_t{1700000000});
	BOOST_TEST(DBResult::parseNumber<double>("1.5") == 1.5);
}

BOOST_AUTO_TEST_CASE(parseNumber_gives_zero_for_values_that_are_not_numbers)
{
	BOOST_TEST(DBResult::parseNumber<uint32_t>("") == 0u);
	BOOST_TEST(DBResult::parseNumber<uint32_t>("abc") == 0u);
	BOOST_TEST(DBResult::parseNumber<uint32_t>("12abc") == 0u);
	BOOST_TEST(DBResult::parseNumber<uint8_t>("x") == 0);
	BOOST_TEST(DBResult::parseNumber<double>("1.5.2") == 0.0);
}

BOOST_AUTO_TEST_SUITE_END()
