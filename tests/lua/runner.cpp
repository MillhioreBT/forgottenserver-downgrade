#include "otpch.h"

#include "luascript.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

extern LuaEnvironment g_luaEnvironment;

namespace {

namespace fs = std::filesystem;

const fs::path sourceDirectory = TFS_SOURCE_DIR;

std::vector<fs::path> luaTestFilesFromArgumentsOrDirectory()
{
	const auto& master = boost::unit_test::framework::master_test_suite();
	std::vector<fs::path> files(master.argv + 1, master.argv + master.argc);
	if (files.empty()) {
		for (const auto& entry : fs::directory_iterator(sourceDirectory / "tests" / "lua")) {
			const auto name = entry.path().filename().string();
			if (name.starts_with("test_") && entry.path().extension() == ".lua") {
				files.push_back(entry.path());
			}
		}
		std::sort(files.begin(), files.end());
	}
	return files;
}

struct InSourceDirectory
{
	InSourceDirectory() { fs::current_path(sourceDirectory); }
	~InSourceDirectory() { fs::current_path(previous); }

	const fs::path previous = fs::current_path();
};

struct LuaTestResults
{
	lua_Integer count = 0;
	std::vector<std::string> failures;
};

void requireSuccess(lua_State* L, int status)
{
	if (status != LUA_OK) {
		const std::string error = lua_tostring(L, -1) ? lua_tostring(L, -1) : "unknown error";
		lua_pop(L, 1);
		BOOST_FAIL(error);
	}
}

LuaTestResults runLuaTestFile(const fs::path& file)
{
	BOOST_REQUIRE(g_luaEnvironment.reInitState());
	BOOST_REQUIRE_MESSAGE(g_luaEnvironment.loadFile("data/global.lua") == 0, g_luaEnvironment.getLastLuaError());

	lua_State* L = g_luaEnvironment.getLuaState();
	requireSuccess(L, luaL_dofile(L, "tests/lua/harness.lua"));
	requireSuccess(L, luaL_dofile(L, fs::relative(file, sourceDirectory).string().c_str()));

	lua_getglobal(L, "runTests");
	requireSuccess(L, lua_pcall(L, 0, 2, 0));

	LuaTestResults results;
	results.count = lua_tointeger(L, -2);
	for (lua_Integer i = 1, failures = luaL_len(L, -1); i <= failures; ++i) {
		lua_rawgeti(L, -1, i);
		results.failures.emplace_back(lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 2);
	return results;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(lua, InSourceDirectory)

BOOST_AUTO_TEST_CASE(lua_test_files_pass)
{
	const auto files = luaTestFilesFromArgumentsOrDirectory();
	BOOST_REQUIRE(!files.empty());

	for (const auto& file : files) {
		BOOST_TEST_CONTEXT(file.filename().string())
		{
			const auto results = runLuaTestFile(file);
			BOOST_TEST(results.count > 0, "the file defines no test");
			for (const auto& failure : results.failures) {
				BOOST_ERROR(failure);
			}
			BOOST_TEST_MESSAGE(file.filename().string() << ": " << results.count - std::ssize(results.failures) << "/"
			                                            << results.count << " Lua tests passed");
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()
