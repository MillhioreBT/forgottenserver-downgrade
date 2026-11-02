#ifndef FS_TESTS_UNIT_IMMUNECREATURE_H
#define FS_TESTS_UNIT_IMMUNECREATURE_H

#include "creature.h"
#include "tile.h"

class ImmuneCreature final : public Creature
{
public:
	ImmuneCreature() { setParent(&tile); }

	const std::string& getName() const override { return name; }
	const std::string& getNameDescription() const override { return name; }
	std::string getDescription(int32_t) const override { return name; }
	CreatureType_t getType() const override { return CREATURETYPE_MONSTER; }

	void setID() override {}
	void addList() override {}
	void removeList() override {}

	using Creature::isImmune;
	bool isImmune(ConditionType_t) const override { return true; }

private:
	const std::string name = "immune creature";
	StaticTile tile{100, 100, 7};
};

#endif
