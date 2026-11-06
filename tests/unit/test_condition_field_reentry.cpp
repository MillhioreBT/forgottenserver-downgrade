#include "otpch.h"

#include "condition.h"
#include "unit/immunecreature.h"

#include <boost/test/unit_test.hpp>

namespace {

ConditionDamage fieldCondition(int32_t initialDamage)
{
	ConditionDamage condition{CONDITIONID_COMBAT, CONDITION_FIRE};
	condition.setParam(CONDITION_PARAM_FIELD, 1);
	condition.setInitDamage(initialDamage);
	condition.addDamage(7, 1000, -10);
	return condition;
}

ConditionDamage burningWithDamagesLeft(int32_t damages)
{
	ConditionDamage condition{CONDITIONID_COMBAT, CONDITION_FIRE};
	condition.setParam(CONDITION_PARAM_FIELD, 1);
	condition.addDamage(damages, 1000, -10);
	return condition;
}

} // namespace

BOOST_AUTO_TEST_SUITE(test_condition_field_reentry)

BOOST_AUTO_TEST_CASE(stepping_into_a_field_again_hits_with_its_initial_damage_and_keeps_every_tick)
{
	ImmuneCreature creature;
	ConditionDamage burning = burningWithDamagesLeft(3);
	const ConditionDamage field = fieldCondition(-20);

	burning.addCondition(&creature, &field);
	BOOST_TEST(burning.getTotalDamage() == 70);
}

BOOST_AUTO_TEST_CASE(stepping_into_a_field_without_initial_damage_again_takes_its_next_damage_at_once)
{
	ImmuneCreature creature;
	ConditionDamage burning = burningWithDamagesLeft(3);
	const ConditionDamage field = fieldCondition(0);

	burning.addCondition(&creature, &field);
	BOOST_TEST(burning.getTotalDamage() == 60);
}

BOOST_AUTO_TEST_SUITE_END()
