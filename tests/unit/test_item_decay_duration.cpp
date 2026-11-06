#include "otpch.h"

#include "item.h"
#include "unit/loadeditems.h"

#include <boost/test/unit_test.hpp>

namespace {

constexpr uint16_t closedPitfall = 293;
constexpr uint16_t openPitfall = 294;

} // namespace

BOOST_AUTO_TEST_SUITE(test_item_decay_duration)

BOOST_AUTO_TEST_CASE(an_item_turned_into_one_with_a_fixed_duration_gets_that_duration)
{
	requireLoadedItems();
	BOOST_REQUIRE(Item::items[openPitfall].decayTimeMin == 300u);
	BOOST_REQUIRE(Item::items[openPitfall].decayTimeMax == 0u);

	for (int attempt = 0; attempt < 20; ++attempt) {
		const std::unique_ptr<Item> pitfall{Item::CreateItem(closedPitfall)};
		pitfall->setID(openPitfall);
		BOOST_TEST(pitfall->getDuration() == 300000, "attempt " << attempt);
	}
}

BOOST_AUTO_TEST_SUITE_END()
