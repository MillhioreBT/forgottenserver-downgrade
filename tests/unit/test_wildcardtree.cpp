#include "otpch.h"

#include "wildcardtree.h"

#include <boost/test/unit_test.hpp>

namespace {

// The game keeps the names of online players in a WildcardTreeNode, lowercase, for "name~" searches
struct OnlinePlayers
{
	OnlinePlayers()
	{
		for (auto name : {"bob", "bobby", "alice", "alfred", "charlie"}) {
			tree.insert(name);
		}
	}

	std::pair<ReturnValue, std::string> find(std::string_view query) const
	{
		std::string result;
		auto ret = tree.findOne(query, result);
		return {ret, result};
	}

	WildcardTreeNode tree{false};
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(test_wildcardtree, OnlinePlayers)

BOOST_AUTO_TEST_CASE(a_unique_prefix_completes_the_name)
{
	auto [ret, name] = find("char");
	BOOST_TEST(ret == RETURNVALUE_NOERROR);
	BOOST_TEST(name == "charlie");

	std::tie(ret, name) = find("alf");
	BOOST_TEST(ret == RETURNVALUE_NOERROR);
	BOOST_TEST(name == "alfred");
}

BOOST_AUTO_TEST_CASE(the_full_name_is_found)
{
	auto [ret, name] = find("charlie");
	BOOST_TEST(ret == RETURNVALUE_NOERROR);
	BOOST_TEST(name == "charlie");
}

BOOST_AUTO_TEST_CASE(a_shared_prefix_is_ambiguous)
{
	BOOST_TEST(find("al").first == RETURNVALUE_NAMEISTOOAMBIGUOUS);
	// "bob" is a whole name but also the start of "bobby"
	BOOST_TEST(find("bob").first == RETURNVALUE_NAMEISTOOAMBIGUOUS);
}

BOOST_AUTO_TEST_CASE(an_unknown_prefix_is_not_online)
{
	BOOST_TEST(find("dave").first == RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE);
	BOOST_TEST(find("bobx").first == RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE);
}

BOOST_AUTO_TEST_CASE(removing_a_name_updates_the_matches)
{
	tree.remove("bobby");
	auto [ret, name] = find("bo");
	BOOST_TEST(ret == RETURNVALUE_NOERROR);
	BOOST_TEST(name == "bob");

	tree.remove("bob");
	BOOST_TEST(find("bo").first == RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE);
	BOOST_TEST(find("b").first == RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE);
}

BOOST_AUTO_TEST_CASE(removing_a_prefix_name_keeps_the_longer_one)
{
	tree.remove("bob");
	auto [ret, name] = find("bob");
	BOOST_TEST(ret == RETURNVALUE_NOERROR);
	BOOST_TEST(name == "bobby");
}

BOOST_AUTO_TEST_CASE(removing_an_unknown_name_changes_nothing)
{
	tree.remove("dave");
	tree.remove("bo");
	BOOST_TEST(find("char").second == "charlie");
	BOOST_TEST(find("bob").first == RETURNVALUE_NAMEISTOOAMBIGUOUS);
}

BOOST_AUTO_TEST_SUITE_END()
