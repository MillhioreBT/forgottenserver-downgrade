#include "otpch.h"

#include "condition.h"
#include "tools.h"
#include "unit/immunecreature.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_condition_damage_list)

BOOST_AUTO_TEST_CASE(a_damage_condition_lasts_until_its_last_damage_after_its_end_time)
{
	ConditionDamage condition{CONDITIONID_COMBAT, CONDITION_FIRE};
	condition.setParam(CONDITION_PARAM_DELAYED, 1);
	condition.addDamage(2, 1000, -10);

	ImmuneCreature creature;
	BOOST_REQUIRE(condition.startCondition(&creature));
	condition.setEndTime(OTSYS_TIME() - 1);

	BOOST_TEST(condition.executeCondition(&creature, 1000));
	BOOST_TEST(condition.getTotalDamage() == 10);
	BOOST_TEST(!condition.executeCondition(&creature, 1000));
}

BOOST_AUTO_TEST_SUITE_END()
