#include "otpch.h"

#include "spells.h"
#include "unit/scriptsinterface.h"

#include <boost/test/unit_test.hpp>

extern Spells* g_spells;

namespace {

struct LoadingSpells : LoadingScripts
{
	LoadingSpells() { g_spells = new Spells(); }
	~LoadingSpells()
	{
		delete g_spells;
		g_spells = nullptr;
	}
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_lua_spell_group, LoadingSpells)

BOOST_AUTO_TEST_CASE(group_sets_the_primary_and_the_secondary_group)
{
	BOOST_TEST(evaluate("local spell = Spell(SPELL_INSTANT) "
	                    "spell:group(SPELLGROUP_ATTACK, SPELLGROUP_HEALING) "
	                    "local primary, secondary = spell:group() "
	                    "return primary == SPELLGROUP_ATTACK and secondary == SPELLGROUP_HEALING") == "true");
}

BOOST_AUTO_TEST_CASE(group_by_name_sets_the_primary_and_the_secondary_group)
{
	BOOST_TEST(evaluate("local spell = Spell(SPELL_INSTANT) "
	                    "spell:group('attack', 'healing') "
	                    "local primary, secondary = spell:group() "
	                    "return primary == SPELLGROUP_ATTACK and secondary == SPELLGROUP_HEALING") == "true");
}

BOOST_AUTO_TEST_SUITE_END()
