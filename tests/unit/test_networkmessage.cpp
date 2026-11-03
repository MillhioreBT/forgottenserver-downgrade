#include "otpch.h"

#include "networkmessage.h"
#include "position.h"

#include <boost/test/unit_test.hpp>

namespace {

std::vector<uint8_t> body(const NetworkMessage& msg)
{
	const uint8_t* start = msg.getBuffer() + NetworkMessage::INITIAL_BUFFER_POSITION;
	return {start, start + msg.getLength()};
}

void rewind(NetworkMessage& msg) { BOOST_REQUIRE(msg.setBufferPosition(0)); }

} // namespace

BOOST_AUTO_TEST_SUITE(test_networkmessage)

BOOST_AUTO_TEST_CASE(numbers_are_written_little_endian)
{
	NetworkMessage msg;
	msg.addByte(0xAB);
	msg.add<uint16_t>(0x1234);
	msg.add<uint32_t>(0xDEADBEEF);

	const std::vector<uint8_t> expected = {0xAB, 0x34, 0x12, 0xEF, 0xBE, 0xAD, 0xDE};
	BOOST_TEST(body(msg) == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(strings_are_prefixed_with_their_length)
{
	NetworkMessage msg;
	msg.addString("Tibia");

	const std::vector<uint8_t> expected = {0x05, 0x00, 'T', 'i', 'b', 'i', 'a'};
	BOOST_TEST(body(msg) == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(positions_are_x_y_then_z)
{
	NetworkMessage msg;
	msg.addPosition({0x0102, 0x0304, 7});

	const std::vector<uint8_t> expected = {0x02, 0x01, 0x04, 0x03, 0x07};
	BOOST_TEST(body(msg) == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(doubles_are_precision_then_offset_fixed_point)
{
	NetworkMessage msg;
	msg.addDouble(1.5, 2);

	rewind(msg);
	BOOST_TEST(msg.getByte() == 2);
	BOOST_TEST(msg.get<uint32_t>() == 150u + std::numeric_limits<int32_t>::max());
}

BOOST_AUTO_TEST_CASE(what_is_written_reads_back)
{
	NetworkMessage msg;
	msg.addByte(0x7A);
	msg.add<uint16_t>(65535);
	msg.add<uint32_t>(4000000000u);
	msg.add<uint64_t>(0x0123456789ABCDEFull);
	msg.addString("Hello, world");
	msg.addString("");
	msg.addPosition({32369, 32241, 7});

	rewind(msg);
	BOOST_TEST(msg.getByte() == 0x7A);
	BOOST_TEST(msg.get<uint16_t>() == 65535);
	BOOST_TEST(msg.get<uint32_t>() == 4000000000u);
	BOOST_TEST(msg.get<uint64_t>() == 0x0123456789ABCDEFull);
	BOOST_TEST(msg.getString() == "Hello, world");
	BOOST_TEST(msg.getString() == "");
	BOOST_TEST((msg.getPosition() == Position{32369, 32241, 7}));
	BOOST_TEST(!msg.isOverrun());
	BOOST_TEST(msg.getBufferPosition() == NetworkMessage::INITIAL_BUFFER_POSITION + msg.getLength());
}

BOOST_AUTO_TEST_CASE(getString_with_a_length_reads_that_many_bytes)
{
	NetworkMessage msg;
	msg.addBytes("abcdef", 6);

	rewind(msg);
	BOOST_TEST(msg.getString(4) == "abcd");
	BOOST_TEST(msg.getString(2) == "ef");
	BOOST_TEST(!msg.isOverrun());
}

BOOST_AUTO_TEST_CASE(reading_past_the_end_returns_zero_and_flags_overrun)
{
	NetworkMessage msg;
	msg.addByte(1);

	rewind(msg);
	BOOST_TEST(msg.getByte() == 1);
	BOOST_TEST(!msg.isOverrun());

	BOOST_TEST(msg.get<uint32_t>() == 0u);
	BOOST_TEST(msg.isOverrun());
}

BOOST_AUTO_TEST_CASE(a_string_longer_than_it_claims_is_not_read)
{
	NetworkMessage msg;
	msg.add<uint16_t>(100);
	msg.addBytes("abc", 3);

	rewind(msg);
	BOOST_TEST(msg.getString() == "");
	BOOST_TEST(msg.isOverrun());
}

BOOST_AUTO_TEST_CASE(strings_over_8192_bytes_are_not_written)
{
	NetworkMessage msg;
	msg.addString(std::string(8193, 'x'));
	BOOST_TEST(msg.getLength() == 0);

	msg.addString(std::string(8192, 'x'));
	BOOST_TEST(msg.getLength() == 8192 + 2);
}

BOOST_AUTO_TEST_CASE(writes_stop_before_the_body_limit)
{
	NetworkMessage msg;
	for (int i = 0; i < NETWORKMESSAGE_MAXSIZE; ++i) {
		msg.addByte(0xFF);
	}
	const auto length = msg.getLength();

	msg.add<uint32_t>(1);
	msg.addString("does not fit");

	BOOST_TEST(msg.getLength() == length);
	BOOST_TEST(msg.getBufferPosition() < NetworkMessage::MAX_BODY_LENGTH);
}

BOOST_AUTO_TEST_SUITE_END()
