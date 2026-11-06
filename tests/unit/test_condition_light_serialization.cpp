#include "otpch.h"

#include "condition.h"
#include "unit/savedconditions.h"

#include <boost/test/unit_test.hpp>

namespace {

Condition_ptr lightCondition()
{
	Condition_ptr light{Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_LIGHT, 60000)};
	light->setParam(CONDITION_PARAM_LIGHT_LEVEL, 6);
	light->setParam(CONDITION_PARAM_LIGHT_COLOR, 215);
	return light;
}

} // namespace

BOOST_AUTO_TEST_SUITE(test_condition_light_serialization)

BOOST_AUTO_TEST_CASE(a_light_condition_is_loaded_as_it_was_saved)
{
	const Condition_ptr light = lightCondition();

	const auto loaded = loadConditions(saveConditions({light.get()}));
	BOOST_REQUIRE(loaded.size() == 1u);
	BOOST_TEST(loaded[0]->getParam(CONDITION_PARAM_LIGHT_LEVEL) == 6);
	BOOST_TEST(loaded[0]->getParam(CONDITION_PARAM_LIGHT_COLOR) == 215);
}

BOOST_AUTO_TEST_CASE(the_conditions_saved_after_a_light_condition_are_loaded_too)
{
	const Condition_ptr light = lightCondition();
	const Condition_ptr muted{Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_MUTED, 60000)};

	const auto loaded = loadConditions(saveConditions({light.get(), muted.get()}));
	BOOST_REQUIRE(loaded.size() == 2u);
	BOOST_TEST(static_cast<int>(loaded[0]->getType()) == static_cast<int>(CONDITION_LIGHT));
	BOOST_TEST(static_cast<int>(loaded[1]->getType()) == static_cast<int>(CONDITION_MUTED));
	BOOST_TEST(loaded[1]->getTicks() == 60000);
}

BOOST_AUTO_TEST_SUITE_END()
