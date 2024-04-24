// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "game.h"
#include "luascript.h"

extern Game g_game;

namespace {
using namespace Lua;

// Position
int luaPositionCreate(lua_State* L)
{
	// Position([x = 0[, y = 0[, z = 0[, stackpos = 0]]]])
	// Position([position])
	if (lua_gettop(L) <= 1) {
		pushPosition(L, Position());
		return 1;
	}

	int32_t stackpos;
	if (isTable(L, 2)) {
		const Position& position = getPosition(L, 2, stackpos);
		pushPosition(L, position, stackpos);
	} else {
		uint16_t x = getInteger<uint16_t>(L, 2, 0);
		uint16_t y = getInteger<uint16_t>(L, 3, 0);
		uint8_t z = getInteger<uint8_t>(L, 4, 0);
		stackpos = getInteger<int32_t>(L, 5, 0);

		pushPosition(L, Position(x, y, z), stackpos);
	}
	return 1;
}

int luaPositionCompare(lua_State* L)
{
	// position == positionEx
	const Position& positionEx = getPosition(L, 2);
	const Position& position = getPosition(L, 1);
	pushBoolean(L, position == positionEx);
	return 1;
}

int luaPositionIsSightClear(lua_State* L)
{
	// position:isSightClear(positionEx[, sameFloor = true])
	bool sameFloor = getBoolean(L, 3, true);
	const Position& positionEx = getPosition(L, 2);
	const Position& position = getPosition(L, 1);
	pushBoolean(L, g_game.isSightClear(position, positionEx, sameFloor));
	return 1;
}

int luaPositionSendMagicEffect(lua_State* L)
{
	// position:sendMagicEffect(magicEffect[, players = {}])
	SpectatorVec spectators;
	if (lua_gettop(L) >= 3) {
		getSpectators<Player>(L, 3, spectators);
	}

	MagicEffectClasses magicEffect = getInteger<MagicEffectClasses>(L, 2);
	const Position& position = getPosition(L, 1);
	if (!spectators.empty()) {
		Game::addMagicEffect(spectators, position, magicEffect);
	} else {
		g_game.addMagicEffect(position, magicEffect);
	}

	pushBoolean(L, true);
	return 1;
}

int luaPositionSendDistanceEffect(lua_State* L)
{
	// position:sendDistanceEffect(positionEx, distanceEffect[, players = {}])
	SpectatorVec spectators;
	if (lua_gettop(L) >= 4) {
		getSpectators<Player>(L, 4, spectators);
	}

	ShootType_t distanceEffect = getInteger<ShootType_t>(L, 3);
	const Position& positionEx = getPosition(L, 2);
	const Position& position = getPosition(L, 1);
	if (!spectators.empty()) {
		Game::addDistanceEffect(spectators, position, positionEx, distanceEffect);
	} else {
		g_game.addDistanceEffect(position, positionEx, distanceEffect);
	}

	pushBoolean(L, true);
	return 1;
}
} // namespace

void LuaScriptInterface::registerPosition()
{
	// Position
	registerClass("Position", "", luaPositionCreate);
	registerMetaMethod("Position", "__eq", luaPositionCompare);

	registerMethod("Position", "isSightClear", luaPositionIsSightClear);

	registerMethod("Position", "sendMagicEffect", luaPositionSendMagicEffect);
	registerMethod("Position", "sendDistanceEffect", luaPositionSendDistanceEffect);
}
