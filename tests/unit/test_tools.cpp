#include "otpch.h"

#include "tools.h"

#include <boost/test/unit_test.hpp>

namespace {

std::string toHex(std::string_view bytes)
{
	std::string hex;
	for (unsigned char byte : bytes) {
		hex += fmt::format("{:02x}", byte);
	}
	return hex;
}

using Views = std::vector<std::string_view>;

} // namespace

BOOST_AUTO_TEST_SUITE(test_tools)

BOOST_AUTO_TEST_CASE(transformToSHA1_returns_the_raw_digest)
{
	// FIPS 180-2 test vectors
	BOOST_TEST(toHex(transformToSHA1("abc")) == "a9993e364706816aba3e25717850c26c9cd0d89d");
	BOOST_TEST(toHex(transformToSHA1("")) == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
	BOOST_TEST(transformToSHA1("abc").size() == 20u);
}

BOOST_AUTO_TEST_CASE(adlerChecksum_matches_reference)
{
	const std::string_view text = "Wikipedia";
	BOOST_TEST(adlerChecksum(reinterpret_cast<const uint8_t*>(text.data()), text.size()) == 0x11E60398u);
}

BOOST_AUTO_TEST_CASE(adlerChecksum_refuses_more_than_a_message)
{
	const std::vector<uint8_t> data(NETWORKMESSAGE_MAXSIZE + 1, 0x42);
	BOOST_TEST(adlerChecksum(data.data(), data.size()) == 0u);
}

BOOST_AUTO_TEST_CASE(caseInsensitiveEqual_ignores_case_only)
{
	BOOST_TEST(caseInsensitiveEqual("Demon Helmet", "demon helmet"));
	BOOST_TEST(caseInsensitiveEqual("", ""));
	BOOST_TEST(!caseInsensitiveEqual("demon", "demon helmet"));
	BOOST_TEST(!caseInsensitiveEqual("demon helmet", "demon helmes"));
}

BOOST_AUTO_TEST_CASE(caseInsensitiveStartsWith_checks_the_prefix)
{
	BOOST_TEST(caseInsensitiveStartsWith("Demon Helmet", "DEMON"));
	BOOST_TEST(caseInsensitiveStartsWith("demon", ""));
	BOOST_TEST(!caseInsensitiveStartsWith("demon", "demon helmet"));
	BOOST_TEST(!caseInsensitiveStartsWith("demon helmet", "helmet"));
}

BOOST_AUTO_TEST_CASE(explodeString_splits_on_every_separator)
{
	BOOST_TEST(explodeString("a,b,,c", ",") == (Views{"a", "b", "", "c"}), boost::test_tools::per_element());
	BOOST_TEST(explodeString("one::two::three", "::") == (Views{"one", "two", "three"}),
	           boost::test_tools::per_element());
	BOOST_TEST(explodeString("no separator", ",") == (Views{"no separator"}), boost::test_tools::per_element());
	BOOST_TEST(explodeString("", ",") == (Views{""}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(explodeString_limit_caps_the_number_of_splits)
{
	// the limit counts splits, not pieces (unlike PHP's explode): the rest of the string stays in the last piece
	BOOST_TEST(explodeString("a b c d", " ", 1) == (Views{"a", "b c d"}), boost::test_tools::per_element());
	BOOST_TEST(explodeString("a b c d", " ", 2) == (Views{"a", "b", "c d"}), boost::test_tools::per_element());
	BOOST_TEST(explodeString("a b", " ", 5) == (Views{"a", "b"}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(vectorAtoi_parses_each_piece)
{
	BOOST_TEST(vectorAtoi(explodeString("10;-20;30", ";")) == (IntegerVector{10, -20, 30}),
	           boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(getFirstLine_and_getStringLine)
{
	const std::string text = "first\nsecond\nthird";

	BOOST_TEST(getFirstLine(text) == "first");
	BOOST_TEST(getFirstLine("single") == "single");
	BOOST_TEST(getStringLine(text, 1) == "first");
	BOOST_TEST(getStringLine(text, 3) == "third");
	BOOST_TEST(getStringLine(text, 4) == "");
}

BOOST_AUTO_TEST_CASE(convertIPToString_reads_network_byte_order)
{
	BOOST_TEST(convertIPToString(0x0100007F) == "127.0.0.1");
	BOOST_TEST(convertIPToString(0xFFFFFFFF) == "255.255.255.255");
	BOOST_TEST(convertIPToString(0) == "0.0.0.0");
}

BOOST_AUTO_TEST_CASE(ucfirst_and_ucwords)
{
	BOOST_TEST(ucfirst("demon helmet") == "Demon helmet");
	BOOST_TEST(ucfirst("  leading spaces") == "  Leading spaces");
	BOOST_TEST(ucfirst("") == "");
	BOOST_TEST(ucwords("demon helmet of doom") == "Demon Helmet Of Doom");
	BOOST_TEST(ucwords("") == "");
}

BOOST_AUTO_TEST_CASE(booleanString_is_false_only_for_no_words)
{
	for (std::string_view value : {"yes", "true", "1", "on", "Y"}) {
		BOOST_TEST(booleanString(value), value);
	}
	for (std::string_view value : {"no", "false", "0", "NO", "False", ""}) {
		BOOST_TEST(!booleanString(value), value);
	}
}

BOOST_AUTO_TEST_CASE(hasBitSet_checks_one_flag)
{
	BOOST_TEST(hasBitSet(1 << 3, 0b1010));
	BOOST_TEST(!hasBitSet(1 << 2, 0b1010));
	BOOST_TEST(!hasBitSet(1, 0));
}

BOOST_AUTO_TEST_CASE(combat_types_and_indexes_round_trip)
{
	for (size_t index = 0; index < COMBAT_COUNT; ++index) {
		BOOST_TEST(combatTypeToIndex(indexToCombatType(index)) == index, "index " << index);
	}
	BOOST_TEST(indexToCombatType(0) == COMBAT_PHYSICALDAMAGE);
	BOOST_TEST(indexToCombatType(COMBAT_COUNT - 1) == COMBAT_DEATHDAMAGE);
}

BOOST_AUTO_TEST_CASE(fluids_round_trip_between_client_and_server)
{
	for (uint8_t clientFluid = 0; clientFluid < std::size(clientToServerFluidMap); ++clientFluid) {
		BOOST_TEST(serverFluidToClient(clientFluidToServer(clientFluid)) == clientFluid,
		           "client fluid " << static_cast<int>(clientFluid));
	}
	BOOST_TEST(clientFluidToServer(std::size(clientToServerFluidMap)) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
