// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "luavariant.h"

#include "luascript.h"
#include "thing.h"

namespace {
using namespace Lua;

// Variant
int luaVariantCreate(lua_State* L)
{
	// Variant(number or string or position or thing)
	LuaVariant variant;
	if (isUserdata(L, 2)) {
		if (Thing* thing = getThing(L, 2)) {
			variant.setTargetPosition(thing->getPosition());
		}
	} else if (isTable(L, 2)) {
		variant.setPosition(getPosition(L, 2));
	} else if (isInteger(L, 2)) {
		variant.setNumber(getInteger<uint32_t>(L, 2));
	} else if (isString(L, 2)) {
		variant.setString(getString(L, 2));
	}
	pushVariant(L, variant);
	return 1;
}

int luaVariantGetNumber(lua_State* L)
{
	// Variant:getNumber()
	const LuaVariant& variant = getVariant(L, 1);
	if (variant.isNumber()) {
		lua_pushinteger(L, variant.getNumber());
	} else {
		lua_pushinteger(L, 0);
	}
	return 1;
}

int luaVariantGetString(lua_State* L)
{
	// Variant:getString()
	const LuaVariant& variant = getVariant(L, 1);
	if (variant.isString()) {
		pushString(L, variant.getString());
	} else {
		pushString(L, "");
	}
	return 1;
}

int luaVariantGetPosition(lua_State* L)
{
	// Variant:getPosition()
	const LuaVariant& variant = getVariant(L, 1);
	if (variant.isPosition()) {
		pushPosition(L, variant.getPosition());
	} else if (variant.isTargetPosition()) {
		pushPosition(L, variant.getTargetPosition());
	} else {
		pushPosition(L, Position());
	}
	return 1;
}
} // namespace

void LuaScriptInterface::registerVariant()
{
	// Variant
	registerClass("Variant", "", luaVariantCreate);

	registerMethod("Variant", "getNumber", luaVariantGetNumber);
	registerMethod("Variant", "getString", luaVariantGetString);
	registerMethod("Variant", "getPosition", luaVariantGetPosition);
}
