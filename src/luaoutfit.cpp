// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "luascript.h"
#include "outfit.h"

namespace {
using namespace Lua;

// Outfit
int luaOutfitCreate(lua_State* L)
{
	// Outfit(looktype)
	const Outfit* outfit = Outfits::getInstance().getOutfitByLookType(getInteger<uint16_t>(L, 2));
	if (outfit) {
		pushOutfit(L, outfit);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaOutfitCompare(lua_State* L)
{
	// outfit == outfitEx
	Outfit outfitEx = getOutfitClass(L, 2);
	Outfit outfit = getOutfitClass(L, 1);
	pushBoolean(L, outfit == outfitEx);
	return 1;
}
} // namespace

void LuaScriptInterface::registerOutfit()
{
	// Outfit
	registerClass("Outfit", "", luaOutfitCreate);
	registerMetaMethod("Outfit", "__eq", luaOutfitCompare);
}
