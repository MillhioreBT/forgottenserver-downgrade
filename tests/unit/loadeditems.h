#ifndef FS_TESTS_UNIT_LOADEDITEMS_H
#define FS_TESTS_UNIT_LOADEDITEMS_H

#include "item.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

inline void requireLoadedItems()
{
	static const bool loaded = [] {
		const auto previous = std::filesystem::current_path();
		std::filesystem::current_path(TFS_SOURCE_DIR);
		const bool ok = Item::items.loadFromOtb("data/items/items.otb") && Item::items.loadFromXml();
		std::filesystem::current_path(previous);
		return ok;
	}();
	BOOST_REQUIRE(loaded);
}

#endif
