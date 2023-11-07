// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "game.h"
#include "guild.h"
#include "luascript.h"

extern Game g_game;

namespace {
using namespace Lua;

// Guild
int luaGuildCreate(lua_State* L)
{
	// Guild(id)
	uint32_t id = getInteger<uint32_t>(L, 2);

	Guild* guild = g_game.getGuild(id);
	if (guild) {
		pushUserdata<Guild>(L, guild);
		setMetatable(L, -1, "Guild");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGuildGetId(lua_State* L)
{
	// guild:getId()
	const Guild* guild = getUserdata<const Guild>(L, 1);
	if (guild) {
		lua_pushinteger(L, guild->getId());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGuildGetName(lua_State* L)
{
	// guild:getName()
	const Guild* guild = getUserdata<const Guild>(L, 1);
	if (guild) {
		pushString(L, guild->getName());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGuildGetMembersOnline(lua_State* L)
{
	// guild:getMembersOnline()
	const Guild* guild = getUserdata<const Guild>(L, 1);
	if (!guild) {
		lua_pushnil(L);
		return 1;
	}

	const auto& members = guild->getMembersOnline();
	lua_createtable(L, members.size(), 0);

	int index = 0;
	for (Player* player : members) {
		pushUserdata<Player>(L, player);
		setMetatable(L, -1, "Player");
		lua_rawseti(L, -2, ++index);
	}
	return 1;
}

int luaGuildAddRank(lua_State* L)
{
	// guild:addRank(id, name, level)
	Guild* guild = getUserdata<Guild>(L, 1);
	if (guild) {
		uint32_t id = getInteger<uint32_t>(L, 2);
		const std::string& name = getString(L, 3);
		uint8_t level = getInteger<uint8_t>(L, 4);
		guild->addRank(id, name, level);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGuildGetRankById(lua_State* L)
{
	// guild:getRankById(id)
	const Guild* guild = getUserdata<const Guild>(L, 1);
	if (!guild) {
		lua_pushnil(L);
		return 1;
	}

	uint32_t id = getInteger<uint32_t>(L, 2);
	GuildRank_ptr rank = guild->getRankById(id);
	if (rank) {
		lua_createtable(L, 0, 3);
		setField(L, "id", rank->id);
		setField(L, "name", rank->name);
		setField(L, "level", rank->level);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGuildGetRankByLevel(lua_State* L)
{
	// guild:getRankByLevel(level)
	const Guild* guild = getUserdata<const Guild>(L, 1);
	if (!guild) {
		lua_pushnil(L);
		return 1;
	}

	uint8_t level = getInteger<uint8_t>(L, 2);
	GuildRank_ptr rank = guild->getRankByLevel(level);
	if (rank) {
		lua_createtable(L, 0, 3);
		setField(L, "id", rank->id);
		setField(L, "name", rank->name);
		setField(L, "level", rank->level);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGuildGetMotd(lua_State* L)
{
	// guild:getMotd()
	const Guild* guild = getUserdata<const Guild>(L, 1);
	if (guild) {
		pushString(L, guild->getMotd());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaGuildSetMotd(lua_State* L)
{
	// guild:setMotd(motd)
	Guild* guild = getUserdata<Guild>(L, 1);
	if (guild) {
		guild->setMotd(getString(L, 2));
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}
} // namespace

void LuaScriptInterface::registerGuild()
{
	// Guild
	registerClass("Guild", "", luaGuildCreate);
	registerMetaMethod("Guild", "__eq", LuaScriptInterface::luaUserdataCompare);

	registerMethod("Guild", "getId", luaGuildGetId);
	registerMethod("Guild", "getName", luaGuildGetName);
	registerMethod("Guild", "getMembersOnline", luaGuildGetMembersOnline);

	registerMethod("Guild", "addRank", luaGuildAddRank);
	registerMethod("Guild", "getRankById", luaGuildGetRankById);
	registerMethod("Guild", "getRankByLevel", luaGuildGetRankByLevel);

	registerMethod("Guild", "getMotd", luaGuildGetMotd);
	registerMethod("Guild", "setMotd", luaGuildSetMotd);
}
