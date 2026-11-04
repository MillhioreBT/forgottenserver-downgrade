#include "otpch.h"

#include "configmanager.h"
#include "connection.h"
#include "server.h"

#include <boost/test/unit_test.hpp>

namespace {

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

uint16_t freeLoopbackPort()
{
	boost::asio::io_context io_context;
	const tcp::acceptor probe{io_context, tcp::endpoint{address_v4::loopback(), 0}};
	return probe.local_endpoint().port();
}

bool acceptsConnections(uint16_t port)
{
	boost::asio::io_context io_context;
	tcp::socket client{io_context};
	boost::system::error_code error;
	client.connect(tcp::endpoint{address_v4::loopback(), port}, error);
	return !error;
}

struct LoopbackOnlyBinding
{
	LoopbackOnlyBinding()
	{
		ConfigManager::setBoolean(ConfigManager::BIND_ONLY_GLOBAL_ADDRESS, true);
		ConfigManager::setString(ConfigManager::IP, "127.0.0.1");
	}

	~LoopbackOnlyBinding()
	{
		ConfigManager::setString(ConfigManager::IP, previousIp);
		ConfigManager::setBoolean(ConfigManager::BIND_ONLY_GLOBAL_ADDRESS, previousBindOnlyGlobalAddress);
	}

	const bool previousBindOnlyGlobalAddress = ConfigManager::getBoolean(ConfigManager::BIND_ONLY_GLOBAL_ADDRESS);
	const std::string previousIp{ConfigManager::getString(ConfigManager::IP)};
};

struct ServicePortListeningOnLoopback
{
	ServicePortListeningOnLoopback() { servicePort->open(port); }

	~ServicePortListeningOnLoopback()
	{
		servicePort->close();
		io_context.run_for(std::chrono::seconds(2));
		ConnectionManager::getInstance().closeAll();
	}

	LoopbackOnlyBinding loopbackOnlyBinding;
	boost::asio::io_context io_context;
	const uint16_t port = freeLoopbackPort();
	const std::shared_ptr<ServicePort> servicePort = std::make_shared<ServicePort>(io_context);
};

void failAccept(ServicePort& servicePort, boost::asio::io_context& io_context, boost::system::errc::errc_t error)
{
	servicePort.onAccept(ConnectionManager::getInstance().createConnection(io_context, servicePort.shared_from_this()),
	                     boost::system::errc::make_error_code(error));
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_accept_errors, ServicePortListeningOnLoopback)

BOOST_AUTO_TEST_CASE(the_port_keeps_accepting_after_a_temporary_accept_error)
{
	BOOST_REQUIRE(acceptsConnections(port));

	for (const auto error :
	     {boost::system::errc::too_many_files_open, boost::system::errc::too_many_files_open_in_system,
	      boost::system::errc::no_buffer_space, boost::system::errc::not_enough_memory}) {
		failAccept(*servicePort, io_context, error);
		BOOST_TEST(acceptsConnections(port), "after " << boost::system::errc::make_error_code(error).message());
	}
}

BOOST_AUTO_TEST_CASE(the_connection_of_a_failed_accept_is_not_kept)
{
	const Connection_ptr connection =
	    ConnectionManager::getInstance().createConnection(io_context, servicePort->shared_from_this());
	servicePort->onAccept(connection, boost::system::errc::make_error_code(boost::system::errc::too_many_files_open));

	BOOST_TEST(connection.use_count() == 1);
}

BOOST_AUTO_TEST_SUITE_END()
