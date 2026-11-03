#include "otpch.h"

#include "condition.h"
#include "player.h"

#include <boost/test/unit_test.hpp>

namespace {

constexpr std::array experienceRateTypes = {ExperienceRateType::BASE, ExperienceRateType::LOW_LEVEL,
                                            ExperienceRateType::BONUS, ExperienceRateType::STAMINA};

Condition_ptr attributeCondition(ConditionParam_t param, int32_t value)
{
	Condition_ptr condition{Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_ATTRIBUTES, 60000)};
	condition->setParam(param, value);
	return condition;
}

void requireRates(const Player& player, int32_t base)
{
	BOOST_TEST(player.getExperienceRate(ExperienceRateType::BASE) == base);
	BOOST_TEST(player.getExperienceRate(ExperienceRateType::LOW_LEVEL) == 100);
	BOOST_TEST(player.getExperienceRate(ExperienceRateType::BONUS) == 100);
	BOOST_TEST(player.getExperienceRate(ExperienceRateType::STAMINA) == 100);
}

} // namespace

BOOST_AUTO_TEST_SUITE(test_condition_experience_rate)

BOOST_AUTO_TEST_CASE(a_condition_without_experience_rates_leaves_the_rates_alone)
{
	Player player{nullptr};
	const Condition_ptr potion = attributeCondition(CONDITION_PARAM_STAT_MAGICPOINTS, 3);

	BOOST_REQUIRE(potion->startCondition(&player));
	for (const auto type : experienceRateTypes) {
		BOOST_TEST(player.getExperienceRate(type) == 100, "rate " << static_cast<int>(type));
	}

	potion->endCondition(&player);
	for (const auto type : experienceRateTypes) {
		BOOST_TEST(player.getExperienceRate(type) == 100, "rate " << static_cast<int>(type));
	}
}

BOOST_AUTO_TEST_CASE(a_condition_adds_its_experience_rates_and_takes_them_off_when_it_ends)
{
	Player player{nullptr};
	const Condition_ptr condition = attributeCondition(CONDITION_PARAM_EXPERIENCERATE_BASE, 50);

	BOOST_REQUIRE(condition->startCondition(&player));
	requireRates(player, 150);

	condition->endCondition(&player);
	requireRates(player, 100);
}

BOOST_AUTO_TEST_CASE(a_refreshed_condition_keeps_its_experience_rates)
{
	Player player{nullptr};
	const Condition_ptr condition = attributeCondition(CONDITION_PARAM_EXPERIENCERATE_BASE, 50);
	BOOST_REQUIRE(condition->startCondition(&player));

	const Condition_ptr refreshed = attributeCondition(CONDITION_PARAM_EXPERIENCERATE_BASE, 50);
	condition->addCondition(&player, refreshed.get());
	requireRates(player, 150);
}

BOOST_AUTO_TEST_SUITE_END()
