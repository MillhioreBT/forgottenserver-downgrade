// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "game.h"
#include "luascript.h"
#include "town.h"

extern Game g_game;

namespace {
using namespace Lua;

// Town
int luaTownCreate(lua_State* L)
{
	// Town(id or name)
	Town* town;
	if (isInteger(L, 2)) {
		town = g_game.map.towns.getTown(getInteger<uint32_t>(L, 2));
	} else if (isString(L, 2)) {
		town = g_game.map.towns.getTown(getString(L, 2));
	} else {
		town = nullptr;
	}

	if (town) {
		pushUserdata<Town>(L, town);
		setMetatable(L, -1, "Town");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaTownGetId(lua_State* L)
{
	// town:getId()
	Town* town = getUserdata<Town>(L, 1);
	if (town) {
		lua_pushinteger(L, town->getID());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaTownGetName(lua_State* L)
{
	// town:getName()
	Town* town = getUserdata<Town>(L, 1);
	if (town) {
		pushString(L, town->getName());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaTownGetTemplePosition(lua_State* L)
{
	// town:getTemplePosition()
	Town* town = getUserdata<Town>(L, 1);
	if (town) {
		pushPosition(L, town->getTemplePosition());
	} else {
		lua_pushnil(L);
	}
	return 1;
}
} // namespace

void LuaScriptInterface::registerTown()
{
	// Town
	registerClass("Town", "", luaTownCreate);
	registerMetaMethod("Town", "__eq", LuaScriptInterface::luaUserdataCompare);

	registerMethod("Town", "getId", luaTownGetId);
	registerMethod("Town", "getName", luaTownGetName);
	registerMethod("Town", "getTemplePosition", luaTownGetTemplePosition);
}
