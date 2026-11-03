#include "otpch.h"

#include "database.h"

#include <boost/test/unit_test.hpp>

namespace {

struct TemporaryTable
{
	TemporaryTable()
	{
		BOOST_REQUIRE(
		    db.executeQuery("CREATE TEMPORARY TABLE `tfs_test_values` (`id` INT NOT NULL, "
		                    "`flag` TINYINT NOT NULL DEFAULT 0, `small` SMALLINT UNSIGNED NULL, "
		                    "`big` BIGINT UNSIGNED NULL, `negative` INT NULL, `data` BLOB NULL) "
		                    "ENGINE=InnoDB"));
	}
	~TemporaryTable() { db.executeQuery("DROP TEMPORARY TABLE IF EXISTS `tfs_test_values`"); }

	uint64_t count() const
	{
		auto result = db.storeQuery("SELECT COUNT(*) AS `count` FROM `tfs_test_values`");
		return result ? result->getNumber<uint64_t>("count") : 0;
	}

	Database& db = Database::getInstance();
};

} // namespace

BOOST_AUTO_TEST_SUITE(test_db_queries)

BOOST_AUTO_TEST_CASE(connect_reads_max_allowed_packet)
{
	BOOST_TEST(Database::getInstance().getMaxQueryLength() >= 1020u);
}

BOOST_AUTO_TEST_CASE(storeQuery_returns_the_rows_in_order)
{
	auto result = Database::getInstance().storeQuery("SELECT 1 AS `n` UNION ALL SELECT 2 UNION ALL SELECT 3");
	BOOST_REQUIRE(result);

	std::vector<int32_t> values;
	do {
		values.push_back(result->getNumber<int32_t>("n"));
	} while (result->next());
	BOOST_TEST(values == (std::vector<int32_t>{1, 2, 3}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(storeQuery_returns_null_without_rows)
{
	BOOST_TEST(!Database::getInstance().storeQuery("SELECT 1 FROM DUAL WHERE 0"));
}

BOOST_AUTO_TEST_CASE(escapeString_round_trips_any_text)
{
	Database& db = Database::getInstance();
	const std::vector<std::string> texts = {"",
	                                        "plain",
	                                        "it's",
	                                        "a \"quote\"",
	                                        "back\\slash",
	                                        "new\nline",
	                                        std::string("nul\0byte", 8),
	                                        "ünïcödé ✓",
	                                        "'; DROP TABLE `accounts`; --"};

	for (const auto& text : texts) {
		auto result = db.storeQuery("SELECT " + db.escapeString(text) + " AS `text`");
		BOOST_REQUIRE(result);
		BOOST_TEST(result->getString("text") == text);
	}
}

BOOST_FIXTURE_TEST_CASE(getNumber_reads_each_column_type_and_zero_for_null_or_missing, TemporaryTable)
{
	BOOST_REQUIRE(
	    db.executeQuery("INSERT INTO `tfs_test_values` (`id`, `flag`, `small`, `big`, `negative`) VALUES "
	                    "(1, 0, 65535, 18446744073709551615, -5), (2, 1, NULL, NULL, NULL)"));

	auto result = db.storeQuery("SELECT * FROM `tfs_test_values` ORDER BY `id`");
	BOOST_REQUIRE(result);
	BOOST_TEST(result->getNumber<uint8_t>("flag") == 0);
	BOOST_TEST(result->getNumber<uint16_t>("flag") == 0);
	BOOST_TEST(result->getNumber<uint16_t>("small") == 65535);
	BOOST_TEST(result->getNumber<uint64_t>("big") == std::numeric_limits<uint64_t>::max());
	BOOST_TEST(result->getNumber<int32_t>("negative") == -5);

	BOOST_REQUIRE(result->next());
	BOOST_TEST(result->getNumber<uint8_t>("flag") == 1);
	BOOST_TEST(result->getNumber<uint16_t>("small") == 0);
	BOOST_TEST(result->getNumber<uint64_t>("big") == 0u);
	BOOST_TEST(result->getNumber<uint32_t>("missing") == 0u);
	BOOST_TEST(!result->next());
}

BOOST_FIXTURE_TEST_CASE(blobs_round_trip_with_any_byte, TemporaryTable)
{
	std::string blob;
	for (int byte = 0; byte < 256; ++byte) {
		blob.push_back(static_cast<char>(byte));
	}
	BOOST_REQUIRE(db.executeQuery("INSERT INTO `tfs_test_values` (`id`, `data`) VALUES (1, " +
	                              db.escapeBlob(blob.data(), static_cast<uint32_t>(blob.size())) + ")"));

	auto result = db.storeQuery("SELECT `data` FROM `tfs_test_values`");
	BOOST_REQUIRE(result);
	unsigned long size = 0;
	auto data = result->getStream("data", size);
	BOOST_TEST(size == blob.size());
	BOOST_TEST(std::string(data.data(), size) == blob);
	BOOST_TEST(result->getString("data") == blob);
}

BOOST_FIXTURE_TEST_CASE(a_transaction_without_commit_rolls_back, TemporaryTable)
{
	{
		DBTransaction transaction;
		BOOST_REQUIRE(transaction.begin());
		BOOST_REQUIRE(db.executeQuery("INSERT INTO `tfs_test_values` (`id`) VALUES (1), (2)"));
		BOOST_TEST(count() == 2u);
	}
	BOOST_TEST(count() == 0u);
}

BOOST_FIXTURE_TEST_CASE(a_committed_transaction_keeps_its_rows, TemporaryTable)
{
	{
		DBTransaction transaction;
		BOOST_REQUIRE(transaction.begin());
		BOOST_REQUIRE(db.executeQuery("INSERT INTO `tfs_test_values` (`id`) VALUES (1), (2)"));
		BOOST_REQUIRE(transaction.commit());
	}
	BOOST_TEST(count() == 2u);
}

BOOST_AUTO_TEST_SUITE_END()
