#include "otpch.h"

#include "configmanager.h"
#include "connection.h"

#include <boost/test/unit_test.hpp>

namespace {

constexpr uint32_t ip = 0x0100007F;
constexpr uint32_t otherIp = 0x0200007F;

struct ConnectionLimitOfTwo
{
	ConnectionLimitOfTwo() { ConfigManager::setInteger(ConfigManager::MAX_CONNECTIONS_PER_IP, 2); }
	~ConnectionLimitOfTwo()
	{
		for (const auto& connection : connections) {
			manager.releaseConnection(connection);
		}
		ConfigManager::setInteger(ConfigManager::MAX_CONNECTIONS_PER_IP, 0);
	}

	Connection_ptr connect()
	{
		connections.push_back(manager.createConnection(io_context, nullptr));
		return connections.back();
	}

	boost::asio::io_context io_context;
	ConnectionManager& manager = ConnectionManager::getInstance();
	std::vector<Connection_ptr> connections;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_connection_limit, ConnectionLimitOfTwo)

BOOST_AUTO_TEST_CASE(an_ip_can_open_connections_up_to_the_limit)
{
	BOOST_TEST(manager.acceptIp(connect(), ip));
	BOOST_TEST(manager.acceptIp(connect(), ip));
	BOOST_TEST(!manager.acceptIp(connect(), ip));
}

BOOST_AUTO_TEST_CASE(a_released_connection_makes_room_for_another)
{
	const Connection_ptr first = connect();
	BOOST_TEST(manager.acceptIp(first, ip));
	BOOST_TEST(manager.acceptIp(connect(), ip));

	manager.releaseConnection(first);
	BOOST_TEST(manager.acceptIp(connect(), ip));
}

BOOST_AUTO_TEST_CASE(the_limit_counts_each_ip_on_its_own)
{
	BOOST_TEST(manager.acceptIp(connect(), ip));
	BOOST_TEST(manager.acceptIp(connect(), ip));
	BOOST_TEST(manager.acceptIp(connect(), otherIp));
}

BOOST_AUTO_TEST_CASE(without_a_limit_an_ip_can_open_any_number_of_connections)
{
	ConfigManager::setInteger(ConfigManager::MAX_CONNECTIONS_PER_IP, 0);
	for (int i = 0; i < 50; ++i) {
		BOOST_TEST(manager.acceptIp(connect(), ip), "connection " << i);
	}
}

BOOST_AUTO_TEST_SUITE_END()
