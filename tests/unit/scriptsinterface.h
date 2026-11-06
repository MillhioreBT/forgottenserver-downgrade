#ifndef FS_TESTS_UNIT_SCRIPTSINTERFACE_H
#define FS_TESTS_UNIT_SCRIPTSINTERFACE_H

#include "luascript.h"
#include "script.h"

#include <boost/test/unit_test.hpp>

extern LuaEnvironment g_luaEnvironment;
extern Scripts* g_scripts;

struct LoadingScripts
{
	LoadingScripts()
	{
		BOOST_REQUIRE(g_luaEnvironment.reInitState());
		g_scripts = new Scripts();
		BOOST_REQUIRE(LuaScriptInterface::reserveScriptEnv());
		LuaScriptInterface::getScriptEnv()->setScriptId(0, &g_scripts->getScriptInterface());
	}

	~LoadingScripts()
	{
		LuaScriptInterface::resetScriptEnv();
		delete g_scripts;
		g_scripts = nullptr;
	}

	LoadingScripts(const LoadingScripts&) = delete;
	LoadingScripts& operator=(const LoadingScripts&) = delete;

	std::string evaluate(const std::string& chunk)
	{
		lua_State* L = g_luaEnvironment.getLuaState();
		const int top = lua_gettop(L);
		if (luaL_dostring(L, chunk.c_str()) != LUA_OK) {
			const std::string error = lua_tostring(L, -1);
			lua_settop(L, top);
			BOOST_FAIL(error);
		}

		std::string result = "no value";
		if (lua_gettop(L) > top) {
			result = luaL_tolstring(L, top + 1, nullptr);
		}
		lua_settop(L, top);
		return result;
	}
};

#endif
