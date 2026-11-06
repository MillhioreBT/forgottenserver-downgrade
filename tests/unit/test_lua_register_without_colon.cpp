#include "otpch.h"

#include "unit/scriptsinterface.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(test_lua_register_without_colon, LoadingScripts)

BOOST_AUTO_TEST_CASE(register_called_with_a_dot_returns_false)
{
	for (const std::string type :
	     {"Action", "CreatureEvent", "GlobalEvent", "MoveEvent", "Spell", "TalkAction", "Weapon"}) {
		BOOST_TEST(evaluate("return " + type + ".register()") == "false", type);
	}
}

BOOST_AUTO_TEST_SUITE_END()
