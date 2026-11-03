#include "otpch.h"

#include "database.h"

#include <boost/test/unit_test.hpp>

namespace {

constexpr uint64_t testMaxPacketSize = 64 * 1024;

struct InsertTable
{
	InsertTable() : serverMaxPacketSize{db.getMaxPacketSize()}
	{
		db.setMaxPacketSize(std::min(serverMaxPacketSize, testMaxPacketSize));
		BOOST_REQUIRE(
		    db.executeQuery("CREATE TEMPORARY TABLE `tfs_test_insert` (`id` INT NOT NULL PRIMARY KEY, "
		                    "`data` VARCHAR(255) NOT NULL) ENGINE=InnoDB"));
	}

	~InsertTable()
	{
		db.executeQuery("DROP TEMPORARY TABLE IF EXISTS `tfs_test_insert`");
		db.setMaxPacketSize(serverMaxPacketSize);
	}

	uint32_t insertRowsUntil(DBInsert& insert, size_t dataSize, uint64_t totalBytes)
	{
		uint32_t rows = 0;
		for (uint64_t written = 0; written < totalBytes;) {
			const auto row = fmt::format("{:d}, '{:s}'", ++rows, std::string(dataSize, 'x'));
			BOOST_REQUIRE_MESSAGE(insert.addRow(row), "row " << rows);
			written += row.size() + 3;
		}
		BOOST_REQUIRE(insert.execute());
		return rows;
	}

	std::pair<uint64_t, uint64_t> countAndSumOfIds() const
	{
		auto result =
		    db.storeQuery("SELECT COUNT(*) AS `count`, COALESCE(SUM(`id`), 0) AS `sum` FROM `tfs_test_insert`");
		BOOST_REQUIRE(result);
		return {result->getNumber<uint64_t>("count"), result->getNumber<uint64_t>("sum")};
	}

	static uint64_t sumOfIds(uint64_t rows) { return rows * (rows + 1) / 2; }

	Database& db = Database::getInstance();
	const uint64_t serverMaxPacketSize;
	const std::string query = "INSERT INTO `tfs_test_insert` (`id`, `data`) VALUES ";
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_db_insert, InsertTable)

BOOST_AUTO_TEST_CASE(rows_over_max_allowed_packet_are_split_into_batches_without_losing_any)
{
	DBInsert insert(query);
	const uint32_t rows = insertRowsUntil(insert, 200, 3 * db.getMaxQueryLength());

	auto [count, sum] = countAndSumOfIds();
	BOOST_TEST(count == rows);
	BOOST_TEST(sum == sumOfIds(rows));
}

BOOST_AUTO_TEST_CASE(batches_end_cleanly_whatever_the_row_size)
{
	for (size_t dataSize : {1, 7, 64, 199, 250}) {
		BOOST_TEST_CONTEXT("rows with " << dataSize << " bytes of data")
		{
			BOOST_REQUIRE(db.executeQuery("DELETE FROM `tfs_test_insert`"));
			DBInsert insert(query);
			const uint32_t rows = insertRowsUntil(insert, dataSize, 2 * db.getMaxQueryLength() + 17);

			auto [count, sum] = countAndSumOfIds();
			BOOST_TEST(count == rows);
			BOOST_TEST(sum == sumOfIds(rows));
		}
	}
}

BOOST_AUTO_TEST_CASE(execute_without_rows_does_nothing)
{
	DBInsert insert(query);
	BOOST_TEST(insert.execute());
	BOOST_TEST(countAndSumOfIds().first == 0u);
}

BOOST_AUTO_TEST_CASE(a_single_row_over_the_limit_is_not_sent)
{
	DBInsert insert(query);
	BOOST_TEST(insert.addRow(fmt::format("1, '{:s}'", std::string(db.getMaxQueryLength(), 'x'))));
	BOOST_TEST(!insert.execute());
	BOOST_TEST(countAndSumOfIds().first == 0u);
}

BOOST_AUTO_TEST_SUITE_END()
