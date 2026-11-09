#include "otpch.h"

#include "creature.h"
#include "events.h"
#include "game.h"
#include "tile.h"
#include "unit/scriptsinterface.h"

#include <boost/test/unit_test.hpp>

extern Events* g_events;
extern Game g_game;

namespace {

const Position targetPosition{120, 100, 7};

class ArmoredCreature final : public Creature
{
public:
	const std::string& getName() const override { return name; }
	const std::string& getNameDescription() const override { return name; }
	std::string getDescription(int32_t) const override { return name; }
	CreatureType_t getType() const override { return CREATURETYPE_MONSTER; }
	int32_t getArmor() const override { return 1000; }

	void setID() override {}
	void addList() override {}
	void removeList() override {}

private:
	const std::string name = "armored creature";
};

struct ArmoredCreatureOnTheMap : LoadingScripts
{
	ArmoredCreatureOnTheMap()
	{
		g_events = new Events();
		tile = g_game.map.getTile(targetPosition);
		if (!tile) {
			tile = new DynamicTile(targetPosition.x, targetPosition.y, targetPosition.z);
			g_game.map.setTile(targetPosition, tile);
		}
		tile->internalAddThing(&creature);
		creature.setParent(tile);
	}

	~ArmoredCreatureOnTheMap()
	{
		tile->removeThing(&creature, 1);
		delete g_events;
		g_events = nullptr;
	}

	int32_t healthLostTo(const std::string& extraArguments)
	{
		const int32_t health = creature.getHealth();
		evaluate(
		    fmt::format("doAreaCombat(0, COMBAT_PHYSICALDAMAGE, Position({:d}, {:d}, {:d}), 0, -100, -100, "
		                "CONST_ME_NONE{:s})",
		                targetPosition.x, targetPosition.y, targetPosition.z, extraArguments));
		return health - creature.getHealth();
	}

	Tile* tile = nullptr;
	ArmoredCreature creature;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_lua_do_area_combat, ArmoredCreatureOnTheMap)

BOOST_AUTO_TEST_CASE(an_origin_does_not_make_the_damage_blocked_by_armor)
{
	BOOST_TEST(healthLostTo(", ORIGIN_SPELL") == 100);
}

BOOST_AUTO_TEST_CASE(block_armor_after_the_origin_makes_the_armor_block_the_damage)
{
	BOOST_TEST(healthLostTo(", ORIGIN_SPELL, true") == 0);
}

BOOST_AUTO_TEST_SUITE_END()
