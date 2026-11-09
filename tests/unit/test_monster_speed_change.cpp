#include "otpch.h"

#include "creature.h"
#include "monsters.h"
#include "spells.h"
#include "tile.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>

extern Spells* g_spells;

namespace {

constexpr int32_t speedChange = 500;

class SpeedCreature final : public Creature
{
public:
	SpeedCreature() { setParent(&tile); }

	const std::string& getName() const override { return name; }
	const std::string& getNameDescription() const override { return name; }
	std::string getDescription(int32_t) const override { return name; }
	CreatureType_t getType() const override { return CREATURETYPE_MONSTER; }

	void setID() override {}
	void addList() override {}
	void removeList() override {}

private:
	const std::string name = "speed creature";
	StaticTile tile{100, 100, 7};
};

struct MonsterWithAHasteFromXml
{
	MonsterWithAHasteFromXml()
	{
		if (!g_spells) {
			g_spells = new Spells();
			ownsSpells = true;
		}

		const auto dataDir = directory / "data" / "monster";
		std::filesystem::create_directories(dataDir);
		std::ofstream(dataDir / "monsters.xml") << R"(<monsters><monster name="Hasty" file="hasty.xml"/></monsters>)";
		std::ofstream(dataDir / "hasty.xml") << fmt::format(
		    R"(<monster name="Hasty" speed="200"><health now="100" max="100"/><look type="21"/>)"
		    R"(<defenses><defense name="speed" interval="2000" chance="100" speedchange="{:d}" duration="5000"/>)"
		    R"(</defenses></monster>)",
		    speedChange);

		const auto previous = std::filesystem::current_path();
		std::filesystem::current_path(directory);
		BOOST_REQUIRE(monsters.loadFromXml());
		monsterType = monsters.getMonsterType("Hasty");
		std::filesystem::current_path(previous);

		BOOST_REQUIRE(monsterType);
		BOOST_REQUIRE(monsterType->info.defenseSpells.size() == 1);
	}

	~MonsterWithAHasteFromXml()
	{
		std::filesystem::remove_all(directory);
		if (ownsSpells) {
			delete g_spells;
			g_spells = nullptr;
		}
	}

	const std::filesystem::path directory = std::filesystem::temp_directory_path() / "tfs_test_monster_speed_change";
	Monsters monsters;
	MonsterType* monsterType = nullptr;
	bool ownsSpells = false;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_monster_speed_change, MonsterWithAHasteFromXml)

BOOST_AUTO_TEST_CASE(a_speedchange_attribute_changes_the_speed_by_exactly_that_much)
{
	BaseSpell* haste = monsterType->info.defenseSpells.front().spell;
	for (int attempt = 0; attempt < 20; ++attempt) {
		SpeedCreature creature;
		const int32_t baseSpeed = creature.getBaseSpeed();

		haste->castSpell(&creature, &creature);
		BOOST_TEST(creature.getSpeed() - baseSpeed == baseSpeed * speedChange / 1000, "attempt " << attempt);
	}
}

BOOST_AUTO_TEST_SUITE_END()
