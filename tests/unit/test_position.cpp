#include "otpch.h"

#include "position.h"
#include "tools.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_position)

BOOST_AUTO_TEST_CASE(offsets_are_signed_and_distances_absolute)
{
	const Position from{100, 200, 7};
	const Position to{103, 195, 5};

	BOOST_TEST(from.getOffsetX(to) == -3);
	BOOST_TEST(from.getOffsetY(to) == 5);
	BOOST_TEST(from.getOffsetZ(to) == 2);

	BOOST_TEST(from.getDistanceX(to) == 3);
	BOOST_TEST(from.getDistanceY(to) == 5);
	BOOST_TEST(from.getDistanceZ(to) == 2);
	BOOST_TEST(to.getDistanceX(from) == 3);
}

BOOST_AUTO_TEST_CASE(isInRange_includes_the_edge)
{
	const Position center{100, 100, 7};

	BOOST_TEST(center.isInRange({108, 94, 7}, 8, 6));
	BOOST_TEST(!center.isInRange({109, 94, 7}, 8, 6));
	BOOST_TEST(!center.isInRange({108, 93, 7}, 8, 6));

	BOOST_TEST(center.isInRange({108, 94, 6}, 8, 6, 1));
	BOOST_TEST(!center.isInRange({108, 94, 5}, 8, 6, 1));
}

BOOST_AUTO_TEST_CASE(ordering_is_by_floor_then_row_then_column)
{
	BOOST_TEST((Position{500, 500, 6} < Position{100, 100, 7}));
	BOOST_TEST((Position{500, 100, 7} < Position{100, 101, 7}));
	BOOST_TEST((Position{100, 100, 7} < Position{101, 100, 7}));
	BOOST_TEST(!(Position{100, 100, 7} < Position{100, 100, 7}));
}

BOOST_AUTO_TEST_CASE(getNextPosition_moves_one_step)
{
	const Position pos{100, 100, 7};

	BOOST_TEST((getNextPosition(DIRECTION_NORTH, pos) == Position{100, 99, 7}));
	BOOST_TEST((getNextPosition(DIRECTION_SOUTH, pos) == Position{100, 101, 7}));
	BOOST_TEST((getNextPosition(DIRECTION_WEST, pos) == Position{99, 100, 7}));
	BOOST_TEST((getNextPosition(DIRECTION_EAST, pos) == Position{101, 100, 7}));
	BOOST_TEST((getNextPosition(DIRECTION_NORTHWEST, pos) == Position{99, 99, 7}));
	BOOST_TEST((getNextPosition(DIRECTION_NORTHEAST, pos) == Position{101, 99, 7}));
	BOOST_TEST((getNextPosition(DIRECTION_SOUTHWEST, pos) == Position{99, 101, 7}));
	BOOST_TEST((getNextPosition(DIRECTION_SOUTHEAST, pos) == Position{101, 101, 7}));
	BOOST_TEST((getNextPosition(DIRECTION_NONE, pos) == pos));
}

BOOST_AUTO_TEST_CASE(getDirectionTo_points_at_the_target)
{
	const Position from{100, 100, 7};

	BOOST_TEST(getDirectionTo(from, from) == DIRECTION_NONE);
	BOOST_TEST(getDirectionTo(from, {100, 90, 7}) == DIRECTION_NORTH);
	BOOST_TEST(getDirectionTo(from, {100, 110, 7}) == DIRECTION_SOUTH);
	BOOST_TEST(getDirectionTo(from, {90, 100, 7}) == DIRECTION_WEST);
	BOOST_TEST(getDirectionTo(from, {110, 100, 7}) == DIRECTION_EAST);

	BOOST_TEST(getDirectionTo(from, {103, 90, 7}) == DIRECTION_NORTH);
	BOOST_TEST(getDirectionTo(from, {110, 97, 7}) == DIRECTION_EAST);
}

BOOST_AUTO_TEST_CASE(getDirectionTo_diagonals_only_when_extended)
{
	const Position from{100, 100, 7};

	BOOST_TEST(getDirectionTo(from, {105, 95, 7}) == DIRECTION_NORTHEAST);
	BOOST_TEST(getDirectionTo(from, {95, 95, 7}) == DIRECTION_NORTHWEST);
	BOOST_TEST(getDirectionTo(from, {105, 105, 7}) == DIRECTION_SOUTHEAST);
	BOOST_TEST(getDirectionTo(from, {95, 105, 7}) == DIRECTION_SOUTHWEST);

	BOOST_TEST(getDirectionTo(from, {105, 95, 7}, false) == DIRECTION_EAST);
	BOOST_TEST(getDirectionTo(from, {95, 105, 7}, false) == DIRECTION_WEST);
}

BOOST_AUTO_TEST_CASE(getDirectionTo_without_extended_never_returns_a_diagonal)
{
	const Position from{100, 100, 7};
	for (int dx = -10; dx <= 10; ++dx) {
		for (int dy = -10; dy <= 10; ++dy) {
			if (dx == 0 && dy == 0) {
				continue;
			}
			const Position to(static_cast<uint16_t>(100 + dx), static_cast<uint16_t>(100 + dy), 7);
			const Direction dir = getDirectionTo(from, to, false);
			BOOST_TEST((dir & DIRECTION_DIAGONAL_MASK) == 0, "offset " << dx << ", " << dy << ": " << dir);
		}
	}
}

BOOST_AUTO_TEST_CASE(getNextPosition_and_getDirectionTo_agree)
{
	const Position from{100, 100, 7};
	for (auto dir : {DIRECTION_NORTH, DIRECTION_EAST, DIRECTION_SOUTH, DIRECTION_WEST, DIRECTION_SOUTHWEST,
	                 DIRECTION_SOUTHEAST, DIRECTION_NORTHWEST, DIRECTION_NORTHEAST}) {
		BOOST_TEST_CONTEXT("direction " << static_cast<int>(dir))
		{
			BOOST_TEST(getDirectionTo(from, getNextPosition(dir, from)) == dir);
		}
	}
}

BOOST_AUTO_TEST_CASE(prints_padded_coordinates)
{
	std::ostringstream os;
	os << Position{95, 1024, 7};
	BOOST_TEST(os.str() == "( 00095 / 01024 / 007 )");
}

BOOST_AUTO_TEST_SUITE_END()
