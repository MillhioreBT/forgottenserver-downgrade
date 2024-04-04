// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "game.h"
#include "luascript.h"
#include "npc.h"

extern Game g_game;

namespace {
using namespace Lua;

// Npc
int luaNpcCreate(lua_State* L)
{
	// Npc([id or name or userdata])
	Npc* npc;
	if (lua_gettop(L) >= 2) {
		if (isInteger(L, 2)) {
			npc = g_game.getNpcByID(getInteger<uint32_t>(L, 2));
		} else if (isString(L, 2)) {
			npc = g_game.getNpcByName(getString(L, 2));
		} else if (isUserdata(L, 2)) {
			npc = getUserdata<Npc>(L, 2);
		} else {
			npc = nullptr;
		}
	} else {
		npc = LuaScriptInterface::getScriptEnv()->getNpc();
	}

	if (npc) {
		pushUserdata<Npc>(L, npc);
		setMetatable(L, -1, "Npc");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaNpcIsNpc(lua_State* L)
{
	// npc:isNpc()
	pushBoolean(L, getUserdata<const Npc>(L, 1) != nullptr);
	return 1;
}

int luaNpcSetMasterPos(lua_State* L)
{
	// npc:setMasterPos(pos[, radius])
	Npc* npc = getUserdata<Npc>(L, 1);
	if (!npc) {
		lua_pushnil(L);
		return 1;
	}

	const Position& pos = getPosition(L, 2);
	int32_t radius = getInteger<int32_t>(L, 3, 1);
	npc->setMasterPos(pos, radius);
	pushBoolean(L, true);
	return 1;
}

int luaNpcGetSpectators(lua_State* L)
{
	// npc:getSpectators()
	Npc* npc = getUserdata<Npc>(L, 1);
	if (!npc) {
		lua_pushnil(L);
		return 1;
	}

	const auto& spectators = npc->getSpectators();
	lua_createtable(L, spectators.size(), 0);

	int index = 0;
	for (const auto& spectatorPlayer : npc->getSpectators()) {
		pushUserdata<const Player>(L, spectatorPlayer);
		setMetatable(L, -1, "Player");
		lua_rawseti(L, -2, ++index);
	}
	return 1;
}
} // namespace

void LuaScriptInterface::registerNpc()
{
	// Npc
	registerClass("Npc", "Creature", luaNpcCreate);
	registerMetaMethod("Npc", "__eq", LuaScriptInterface::luaUserdataCompare);

	registerMethod("Npc", "isNpc", luaNpcIsNpc);

	registerMethod("Npc", "setMasterPos", luaNpcSetMasterPos);

	registerMethod("Npc", "getSpectators", luaNpcGetSpectators);
}
