#include "otpch.h"

#include "const.h"
#include "game.h"
#include "player.h"
#include "spells.h"
#include "tile.h"
#include "unit/scriptsinterface.h"

#include <boost/test/unit_test.hpp>

extern Game g_game;
extern Spells* g_spells;

namespace {

struct SpellWithAPlayerNameAndAPlayerInTheGame : LoadingScripts
{
	SpellWithAPlayerNameAndAPlayerInTheGame()
	{
		g_spells = new Spells();
		evaluate(
		    "local spell = Spell(SPELL_INSTANT) "
		    "spell:name('Tfs Test Find') "
		    "spell:words('tfs test find') "
		    "spell:hasParams(true) "
		    "spell:hasPlayerNameParam(true) "
		    "spell:onCastSpell(function(creature, variant) castParam = variant:getString() return true end) "
		    "spell:register()");
		spell = g_spells->getInstantSpellByName("Tfs Test Find");
		BOOST_REQUIRE(spell);

		caster.setGroup(&casterGroup);
		caster.setParent(&tile);

		target->setName("Tfs Test Target");
		target->setGroup(&targetGroup);
		target->setParent(&tile);
		target->setID();
		g_game.addPlayer(target);
	}

	~SpellWithAPlayerNameAndAPlayerInTheGame()
	{
		g_game.removePlayer(target);
		delete target;
		delete g_spells;
		g_spells = nullptr;
	}

	std::string castWith(std::string param)
	{
		BOOST_REQUIRE(spell->playerCastInstant(&caster, param));
		return evaluate("return castParam");
	}

	InstantSpell* spell = nullptr;
	Group casterGroup{"caster", PlayerFlag_IgnoreSpellCheck | PlayerFlag_HasNoExhaustion, 0, 0, 1, false};
	Group targetGroup{"player", 0, 0, 0, 1, false};
	StaticTile tile{100, 100, 7};
	Player caster{nullptr};
	Player* target = new Player(nullptr);
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_spell_player_name_param, SpellWithAPlayerNameAndAPlayerInTheGame)

BOOST_AUTO_TEST_CASE(a_name_ending_in_a_tilde_is_completed_to_the_online_player)
{
	BOOST_TEST(castWith("tfs test ta~") == "Tfs Test Target");
}

BOOST_AUTO_TEST_CASE(a_name_in_the_wrong_case_is_corrected_to_the_online_player)
{
	BOOST_TEST(castWith("tfs test target") == "Tfs Test Target");
}

BOOST_AUTO_TEST_CASE(a_name_of_nobody_online_is_passed_on_as_written)
{
	BOOST_TEST(castWith("Nobody Online") == "Nobody Online");
}

BOOST_AUTO_TEST_SUITE_END()
