// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "game.h"
#include "groups.h"
#include "luascript.h"

extern Game g_game;

namespace {
using namespace Lua;

// Group
int luaGroupCreate(lua_State* L)
{
	// Group(id)
	uint16_t id = getInteger<uint16_t>(L, 2);
	Group* group = g_game.groups.getGroup(id);
	if (group) {
		pushUserdata<Group>(L, group);
		setMetatable(L, -1, "Group");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGroupGetId(lua_State* L)
{
	// group:getId()
	const Group* group = getUserdata<const Group>(L, 1);
	if (group) {
		lua_pushinteger(L, group->id);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGroupGetName(lua_State* L)
{
	// group:getName()
	const Group* group = getUserdata<const Group>(L, 1);
	if (group) {
		pushString(L, group->name);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGroupGetFlags(lua_State* L)
{
	// group:getFlags()
	const Group* group = getUserdata<const Group>(L, 1);
	if (group) {
		lua_pushinteger(L, group->flags);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGroupGetAccess(lua_State* L)
{
	// group:getAccess()
	const Group* group = getUserdata<const Group>(L, 1);
	if (group) {
		pushBoolean(L, group->access);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGroupGetMaxDepotItems(lua_State* L)
{
	// group:getMaxDepotItems()
	const Group* group = getUserdata<const Group>(L, 1);
	if (group) {
		lua_pushinteger(L, group->maxDepotItems);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGroupGetMaxVipEntries(lua_State* L)
{
	// group:getMaxVipEntries()
	const Group* group = getUserdata<const Group>(L, 1);
	if (group) {
		lua_pushinteger(L, group->maxVipEntries);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGroupHasFlag(lua_State* L)
{
	// group:hasFlag(flag)
	const Group* group = getUserdata<const Group>(L, 1);
	if (group) {
		PlayerFlags flag = getInteger<PlayerFlags>(L, 2);
		pushBoolean(L, (group->flags & flag) != 0);
	} else {
		lua_pushnil(L);
	}
	return 1;
}
} // namespace

void LuaScriptInterface::registerGroup()
{
	// Group
	registerClass("Group", "", luaGroupCreate);
	registerMetaMethod("Group", "__eq", LuaScriptInterface::luaUserdataCompare);

	registerMethod("Group", "getId", luaGroupGetId);
	registerMethod("Group", "getName", luaGroupGetName);
	registerMethod("Group", "getFlags", luaGroupGetFlags);
	registerMethod("Group", "getAccess", luaGroupGetAccess);
	registerMethod("Group", "getMaxDepotItems", luaGroupGetMaxDepotItems);
	registerMethod("Group", "getMaxVipEntries", luaGroupGetMaxVipEntries);
	registerMethod("Group", "hasFlag", luaGroupHasFlag);
}
