#include "otpch.h"

#include "monster.h"
#include "monsters.h"
#include "spawn.h"
#include "unit/immunecreature.h"

#include <boost/test/unit_test.hpp>

namespace {

const Position spawnCenter{100, 100, 7};
const Position outsideTheSpawn{110, 100, 7};

struct MonsterOfASpawn
{
	MonsterOfASpawn()
	{
		Monster::despawnRadius = 2;
		monster->incrementReferenceCounter();
		monster->setMasterPos(spawnCenter);
		monster->setSpawn(&spawn);
	}

	~MonsterOfASpawn()
	{
		monster->setMaster(nullptr);
		monster->decrementReferenceCounter();
		Monster::despawnRadius = previousDespawnRadius;
	}

	const int32_t previousDespawnRadius = Monster::despawnRadius;
	MonsterType monsterType;
	Spawn spawn{spawnCenter, 2};
	Monster* monster = new Monster(&monsterType);
	ImmuneCreature master;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_monster_convinced_spawn, MonsterOfASpawn)

BOOST_AUTO_TEST_CASE(a_convinced_monster_can_follow_its_master_out_of_its_spawn)
{
	BOOST_REQUIRE(!monster->isInSpawnRange(outsideTheSpawn));

	monster->setMaster(&master);
	BOOST_TEST(monster->isInSpawnRange(outsideTheSpawn));
}

BOOST_AUTO_TEST_SUITE_END()
