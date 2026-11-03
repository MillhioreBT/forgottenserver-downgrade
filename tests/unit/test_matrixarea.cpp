#include "otpch.h"

#include "matrixarea.h"

#include <boost/test/unit_test.hpp>

namespace {

using Rows = std::vector<std::string>;

// Renders an area as one string per row in createArea's notation, so expectations read like the input: '3' is the
// center, '1' a cell in the area and '0' a cell outside it ('c' would be a center outside the area).
Rows rows(const MatrixArea& area)
{
	auto&& [centerX, centerY] = area.getCenter();

	Rows result;
	for (uint32_t row = 0; row < area.getRows(); ++row) {
		std::string line;
		for (uint32_t col = 0; col < area.getCols(); ++col) {
			if (row == centerY && col == centerX) {
				line += area(row, col) ? '3' : 'c';
			} else {
				line += area(row, col) ? '1' : '0';
			}
		}
		result.push_back(std::move(line));
	}
	return result;
}

// clang-format off
const MatrixArea asymmetric = createArea({
    3, 1, 1, 1,
    0, 1, 1, 1,
    0, 0, 1, 0,
}, 3);
// clang-format on

} // namespace

BOOST_AUTO_TEST_SUITE(test_matrixarea)

BOOST_AUTO_TEST_CASE(createArea_reads_rows_and_center)
{
	// clang-format off
	const auto area = createArea({
	    0, 0, 1, 1,
	    3, 1, 1, 1,
	    0, 0, 1, 1,
	}, 3);
	// clang-format on

	BOOST_TEST(area.getRows() == 3u);
	BOOST_TEST(area.getCols() == 4u);
	BOOST_TEST(area.getCenter().first == 0u);
	BOOST_TEST(area.getCenter().second == 1u);
	BOOST_TEST(rows(area) == (Rows{"0011", "3111", "0011"}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(flip_turns_upside_down)
{
	BOOST_TEST(rows(asymmetric.flip()) == (Rows{"0010", "0111", "3111"}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(mirror_turns_left_to_right)
{
	BOOST_TEST(rows(asymmetric.mirror()) == (Rows{"1113", "1110", "0100"}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(rotate90)
{
	BOOST_TEST(rows(asymmetric.rotate90()) == (Rows{"003", "011", "111", "011"}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(rotate180)
{
	BOOST_TEST(rows(asymmetric.rotate180()) == (Rows{"0100", "1110", "1113"}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(rotate270)
{
	BOOST_TEST(rows(asymmetric.rotate270()) == (Rows{"110", "111", "110", "300"}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(transformations_compose)
{
	const auto original = rows(asymmetric);

	BOOST_TEST(rows(asymmetric.flip().flip()) == original, boost::test_tools::per_element());
	BOOST_TEST(rows(asymmetric.mirror().mirror()) == original, boost::test_tools::per_element());
	BOOST_TEST(rows(asymmetric.rotate90().rotate90().rotate90().rotate90()) == original,
	           boost::test_tools::per_element());
	BOOST_TEST(rows(asymmetric.rotate90().rotate90()) == rows(asymmetric.rotate180()),
	           boost::test_tools::per_element());
	BOOST_TEST(rows(asymmetric.rotate180().rotate90()) == rows(asymmetric.rotate270()),
	           boost::test_tools::per_element());
	BOOST_TEST(rows(asymmetric.flip().mirror()) == rows(asymmetric.rotate180()), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
