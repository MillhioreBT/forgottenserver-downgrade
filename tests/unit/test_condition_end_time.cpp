#include "otpch.h"

#include "condition.h"
#include "tools.h"
#include "unit/immunecreature.h"

#include <boost/test/unit_test.hpp>
#include <thread>

BOOST_AUTO_TEST_SUITE(test_condition_end_time)

BOOST_AUTO_TEST_CASE(a_copy_of_a_condition_built_earlier_lasts_its_ticks_from_its_start)
{
	ConditionDamage field{CONDITIONID_COMBAT, CONDITION_FIRE};
	field.setParam(CONDITION_PARAM_DELAYED, 1);
	field.addDamage(7, 1000, -10);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	const Condition_ptr condition{field.clone()};
	ImmuneCreature creature;
	const int64_t startedAt = OTSYS_TIME();
	BOOST_REQUIRE(condition->startCondition(&creature));
	BOOST_TEST(condition->getEndTime() >= startedAt + field.getTicks());
}

BOOST_AUTO_TEST_CASE(a_constant_condition_keeps_the_end_time_it_was_loaded_with)
{
	const Condition_ptr condition{Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_MUTED, 60000)};
	condition->setConstant(true);
	const int64_t savedEndTime = OTSYS_TIME() + 30000;
	condition->setEndTime(savedEndTime);

	ImmuneCreature creature;
	BOOST_REQUIRE(condition->startCondition(&creature));
	BOOST_TEST(condition->getEndTime() == savedEndTime);
}

BOOST_AUTO_TEST_SUITE_END()
