#include "otpch.h"

#include "condition.h"
#include "unit/savedconditions.h"

#include <boost/test/unit_test.hpp>

namespace {

Condition_ptr attributeCondition()
{
	Condition_ptr condition{Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_ATTRIBUTES, 60000)};
	condition->setParam(CONDITION_PARAM_SKILL_SWORD, 3);
	condition->setParam(CONDITION_PARAM_SPECIALSKILL_CRITICALHITCHANCE, 5);
	condition->setParam(CONDITION_PARAM_EXPERIENCERATE_BASE, 50);
	condition->setParam(CONDITION_PARAM_EXPERIENCERATE_LOW_LEVEL, 10);
	condition->setParam(CONDITION_PARAM_EXPERIENCERATE_BONUS, 20);
	condition->setParam(CONDITION_PARAM_EXPERIENCERATE_STAMINA, 30);
	return condition;
}

} // namespace

BOOST_AUTO_TEST_SUITE(test_condition_attributes_serialization)

BOOST_AUTO_TEST_CASE(the_experience_rates_of_an_attribute_condition_are_loaded_as_they_were_saved)
{
	const Condition_ptr condition = attributeCondition();

	const auto loaded = loadConditions(saveConditions({condition.get()}));
	BOOST_REQUIRE(loaded.size() == 1u);
	BOOST_TEST(loaded[0]->getParam(CONDITION_PARAM_EXPERIENCERATE_BASE) == 50);
	BOOST_TEST(loaded[0]->getParam(CONDITION_PARAM_EXPERIENCERATE_LOW_LEVEL) == 10);
	BOOST_TEST(loaded[0]->getParam(CONDITION_PARAM_EXPERIENCERATE_BONUS) == 20);
	BOOST_TEST(loaded[0]->getParam(CONDITION_PARAM_EXPERIENCERATE_STAMINA) == 30);
}

BOOST_AUTO_TEST_CASE(the_skills_of_an_attribute_condition_are_loaded_as_they_were_saved)
{
	const Condition_ptr condition = attributeCondition();

	const auto loaded = loadConditions(saveConditions({condition.get()}));
	BOOST_REQUIRE(loaded.size() == 1u);
	BOOST_TEST(loaded[0]->getParam(CONDITION_PARAM_SKILL_SWORD) == 3);
	BOOST_TEST(loaded[0]->getParam(CONDITION_PARAM_SPECIALSKILL_CRITICALHITCHANCE) == 5);
}

BOOST_AUTO_TEST_CASE(the_conditions_saved_after_an_attribute_condition_are_loaded_too)
{
	const Condition_ptr condition = attributeCondition();
	const Condition_ptr muted{Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_MUTED, 60000)};

	const auto loaded = loadConditions(saveConditions({condition.get(), muted.get()}));
	BOOST_REQUIRE(loaded.size() == 2u);
	BOOST_TEST(static_cast<int>(loaded[1]->getType()) == static_cast<int>(CONDITION_MUTED));
}

BOOST_AUTO_TEST_SUITE_END()
