#include "otpch.h"

#include "item.h"
#include "unit/loadeditems.h"
#include "unit/scriptsinterface.h"

#include <boost/test/unit_test.hpp>

namespace {

constexpr uint16_t itemTypeNotInItemsXml = 3246;

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_lua_weapon_extra_element, LoadingScripts)

BOOST_AUTO_TEST_CASE(extra_element_gives_an_item_type_without_abilities_its_element_damage)
{
	requireLoadedItems();
	BOOST_REQUIRE(!Item::items[itemTypeNotInItemsXml].abilities);

	BOOST_TEST(evaluate("local weapon = Weapon(WEAPON_SWORD) weapon:id(" + std::to_string(itemTypeNotInItemsXml) +
	                    ") return weapon:extraElement(10, 'fire')") == "true");

	const auto& abilities = Item::items[itemTypeNotInItemsXml].abilities;
	BOOST_REQUIRE(abilities);
	BOOST_TEST(abilities->elementDamage == 10);
	BOOST_TEST(static_cast<int>(abilities->elementType) == static_cast<int>(COMBAT_FIREDAMAGE));
}

BOOST_AUTO_TEST_SUITE_END()
