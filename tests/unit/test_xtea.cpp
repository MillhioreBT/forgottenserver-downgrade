#include "otpch.h"

#include "xtea.h"

#include <boost/test/unit_test.hpp>

namespace {

const xtea::key testKey = {0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef};

} // namespace

BOOST_AUTO_TEST_SUITE(test_xtea)

BOOST_AUTO_TEST_CASE(expand_key_matches_reference)
{
	const xtea::round_keys expected = {
	    0xdeadbeef, 0x7ce538a8, 0x7ce538a8, 0x1b1cb261, 0x1b1cb261, 0xb9542c1a, 0xb9542c1a, 0x578ba5d3,
	    0x578ba5d3, 0xf5c31f8c, 0xf5c31f8c, 0x93fa9945, 0x93fa9945, 0x323212fe, 0x323212fe, 0xd0698cb7,
	    0xd0698cb7, 0x6ea10670, 0x6ea10670, 0x0cd88029, 0x0cd88029, 0xab0ff9e2, 0xab0ff9e2, 0x4947739b,
	    0x4947739b, 0xe77eed54, 0xe77eed54, 0x85b6670d, 0x85b6670d, 0x23ede0c6, 0x23ede0c6, 0xc2255a7f,
	    0xc2255a7f, 0x605cd438, 0x605cd438, 0xfe944df1, 0xfe944df1, 0x9ccbc7aa, 0x9ccbc7aa, 0x3b034163,
	    0x3b034163, 0xd93abb1c, 0xd93abb1c, 0x777234d5, 0x777234d5, 0x15a9ae8e, 0x15a9ae8e, 0xb3e12847,
	    0xb3e12847, 0x5218a200, 0x5218a200, 0xf0501bb9, 0xf0501bb9, 0x8e879572, 0x8e879572, 0x2cbf0f2b,
	    0x2cbf0f2b, 0xcaf688e4, 0xcaf688e4, 0x692e029d, 0x692e029d, 0x07657c56, 0x07657c56, 0xa59cf60f};

	BOOST_TEST(xtea::expand_key(testKey) == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(encrypt_matches_reference)
{
	std::vector<uint8_t> data = {0xef, 0xbe, 0xad, 0xde, 0xef, 0xbe, 0xad, 0xde};
	const std::vector<uint8_t> expected = {0xb5, 0x8c, 0xf2, 0xfa, 0xe0, 0xc0, 0x40, 0x09};

	xtea::encrypt(data.data(), data.size(), xtea::expand_key(testKey));

	BOOST_TEST(data == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(decrypt_matches_reference)
{
	std::vector<uint8_t> data = {0xb5, 0x8c, 0xf2, 0xfa, 0xe0, 0xc0, 0x40, 0x09};
	const std::vector<uint8_t> expected = {0xef, 0xbe, 0xad, 0xde, 0xef, 0xbe, 0xad, 0xde};

	xtea::decrypt(data.data(), data.size(), xtea::expand_key(testKey));

	BOOST_TEST(data == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(decrypt_reverses_encrypt_over_several_blocks)
{
	// messages are encrypted in 8-byte blocks; use several so every block is covered
	std::vector<uint8_t> data(64);
	std::iota(data.begin(), data.end(), uint8_t{0});
	const auto original = data;
	const auto roundKeys = xtea::expand_key({0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210});

	xtea::encrypt(data.data(), data.size(), roundKeys);
	BOOST_TEST(data != original);

	xtea::decrypt(data.data(), data.size(), roundKeys);
	BOOST_TEST(data == original, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
