#include "otpch.h"

#include "events.h"
#include "unit/immunecreature.h"
#include "unit/scriptsinterface.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>

namespace {

struct TargetCombatEventReturning : LoadingScripts
{
	explicit TargetCombatEventReturning(std::string_view result)
	{
		const auto eventsDir = directory / "data" / "events";
		std::filesystem::create_directories(eventsDir / "scripts");
		std::ofstream(eventsDir / "events.xml")
		    << R"(<events><event class="Creature" method="onTargetCombat" enabled="1"/></events>)";
		std::ofstream(eventsDir / "scripts" / "creature.lua")
		    << "function Creature:onTargetCombat(target) return " << result << " end";

		const auto previous = std::filesystem::current_path();
		std::filesystem::current_path(directory);
		const bool loaded = events.load();
		std::filesystem::current_path(previous);
		BOOST_REQUIRE(loaded);
	}

	~TargetCombatEventReturning() { std::filesystem::remove_all(directory); }

	int targetCombat() { return static_cast<int>(events.eventCreatureOnTargetCombat(&attacker, &target)); }

	const std::filesystem::path directory = std::filesystem::temp_directory_path() / "tfs_test_events_return_value";
	Events events;
	ImmuneCreature attacker;
	ImmuneCreature target;
};

struct ReturningFalse : TargetCombatEventReturning
{
	ReturningFalse() : TargetCombatEventReturning("false") {}
};

struct ReturningTrue : TargetCombatEventReturning
{
	ReturningTrue() : TargetCombatEventReturning("true") {}
};

struct ReturningAReturnValue : TargetCombatEventReturning
{
	ReturningAReturnValue() : TargetCombatEventReturning("RETURNVALUE_YOUMAYNOTATTACKTHISPLAYER") {}
};

} // namespace

BOOST_AUTO_TEST_SUITE(test_events_return_value)

BOOST_FIXTURE_TEST_CASE(an_event_returning_false_refuses, ReturningFalse)
{
	BOOST_TEST(targetCombat() == static_cast<int>(RETURNVALUE_NOTPOSSIBLE));
}

BOOST_FIXTURE_TEST_CASE(an_event_returning_true_allows, ReturningTrue)
{
	BOOST_TEST(targetCombat() == static_cast<int>(RETURNVALUE_NOERROR));
}

BOOST_FIXTURE_TEST_CASE(an_event_returning_a_return_value_returns_it, ReturningAReturnValue)
{
	BOOST_TEST(targetCombat() == static_cast<int>(RETURNVALUE_YOUMAYNOTATTACKTHISPLAYER));
}

BOOST_AUTO_TEST_SUITE_END()
