#include "otpch.h"

#include "monster.h"
#include "monsters.h"
#include "tile.h"

#include <boost/test/unit_test.hpp>

namespace {

const Position monsterPosition{100, 100, 7};
const Position adjacentPosition{101, 100, 7};
const Position farPosition{106, 100, 7};

struct MonsterOnATile
{
	explicit MonsterOnATile(uint32_t targetDistance)
	{
		monsterType.info.targetDistance = targetDistance;
		monsterType.info.runAwayHealth = 50;
		monster = new Monster(&monsterType);
		monster->incrementReferenceCounter();
		monster->setParent(&tile);
		normalStepDuration = monster->getStepDuration();
	}

	~MonsterOnATile() { monster->decrementReferenceCounter(); }

	void stepFrom(const Position& targetPosition, bool flee, int times)
	{
		Direction direction;
		for (int step = 0; step < times; ++step) {
			monster->getDistanceStep(targetPosition, direction, flee);
		}
	}

	MonsterType monsterType;
	StaticTile tile{monsterPosition.x, monsterPosition.y, monsterPosition.z};
	Monster* monster = nullptr;
	int64_t normalStepDuration = 0;
};

struct MeleeMonster : MonsterOnATile
{
	MeleeMonster() : MonsterOnATile(1) {}
};

struct DistanceMonster : MonsterOnATile
{
	DistanceMonster() : MonsterOnATile(4) {}
};

} // namespace

BOOST_AUTO_TEST_SUITE(test_monster_step_duration)

BOOST_FIXTURE_TEST_CASE(a_melee_monster_walks_at_its_speed_again_after_fleeing_from_an_adjacent_target, MeleeMonster)
{
	monster->changeHealth(-60, false);
	BOOST_REQUIRE(monster->isFleeing());
	stepFrom(adjacentPosition, true, 2);

	monster->changeHealth(60, false);
	BOOST_REQUIRE(!monster->isFleeing());
	BOOST_TEST(monster->getStepDuration() == normalStepDuration);
}

BOOST_FIXTURE_TEST_CASE(a_distance_monster_is_slow_next_to_its_target, DistanceMonster)
{
	stepFrom(adjacentPosition, false, 2);
	BOOST_TEST(monster->getStepDuration() == 3 * normalStepDuration);
}

BOOST_FIXTURE_TEST_CASE(a_distance_monster_walks_at_its_speed_again_when_its_target_is_far, DistanceMonster)
{
	stepFrom(adjacentPosition, false, 2);
	stepFrom(farPosition, false, 2);
	BOOST_TEST(monster->getStepDuration() == normalStepDuration);
}

BOOST_AUTO_TEST_SUITE_END()
