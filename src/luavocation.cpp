// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "luascript.h"
#include "vocation.h"

extern Vocations g_vocations;

namespace {
using namespace Lua;

// Vocation
int luaVocationCreate(lua_State* L)
{
	// Vocation(id or name)
	uint16_t id;
	if (isInteger(L, 2)) {
		id = getInteger<uint16_t>(L, 2);
	} else if (auto vocationId = g_vocations.getVocationId(getString(L, 2))) {
		id = vocationId.value();
	} else {
		lua_pushnil(L);
		return 1;
	}

	Vocation* vocation = g_vocations.getVocation(id);
	if (vocation) {
		pushUserdata<Vocation>(L, vocation);
		setMetatable(L, -1, "Vocation");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetId(lua_State* L)
{
	// vocation:getId()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getId());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetClientId(lua_State* L)
{
	// vocation:getClientId()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getClientId());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetName(lua_State* L)
{
	// vocation:getName()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		pushString(L, vocation->getVocName());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetDescription(lua_State* L)
{
	// vocation:getDescription()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		pushString(L, vocation->getVocDescription());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetRequiredSkillTries(lua_State* L)
{
	// vocation:getRequiredSkillTries(skillType, skillLevel)
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		skills_t skillType = getInteger<skills_t>(L, 2);
		uint16_t skillLevel = getInteger<uint16_t>(L, 3);
		lua_pushinteger(L, vocation->getReqSkillTries(skillType, skillLevel));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetRequiredManaSpent(lua_State* L)
{
	// vocation:getRequiredManaSpent(magicLevel)
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		uint32_t magicLevel = getInteger<uint32_t>(L, 2);
		lua_pushinteger(L, vocation->getReqMana(magicLevel));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetCapacityGain(lua_State* L)
{
	// vocation:getCapacityGain()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getCapGain());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetHealthGain(lua_State* L)
{
	// vocation:getHealthGain()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getHPGain());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetHealthGainTicks(lua_State* L)
{
	// vocation:getHealthGainTicks()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getHealthGainTicks());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetHealthGainAmount(lua_State* L)
{
	// vocation:getHealthGainAmount()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getHealthGainAmount());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetManaGain(lua_State* L)
{
	// vocation:getManaGain()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getManaGain());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetManaGainTicks(lua_State* L)
{
	// vocation:getManaGainTicks()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getManaGainTicks());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetManaGainAmount(lua_State* L)
{
	// vocation:getManaGainAmount()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getManaGainAmount());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetMaxSoul(lua_State* L)
{
	// vocation:getMaxSoul()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getSoulMax());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetSoulGainTicks(lua_State* L)
{
	// vocation:getSoulGainTicks()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getSoulGainTicks());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetAttackSpeed(lua_State* L)
{
	// vocation:getAttackSpeed()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getAttackSpeed());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetBaseSpeed(lua_State* L)
{
	// vocation:getBaseSpeed()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		lua_pushinteger(L, vocation->getBaseSpeed());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetDemotion(lua_State* L)
{
	// vocation:getDemotion()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (!vocation) {
		lua_pushnil(L);
		return 1;
	}

	uint16_t fromId = static_cast<uint16_t>(vocation->getFromVocation());
	if (fromId == VOCATION_NONE) {
		lua_pushnil(L);
		return 1;
	}

	Vocation* demotedVocation = g_vocations.getVocation(fromId);
	if (demotedVocation && demotedVocation != vocation) {
		pushUserdata<Vocation>(L, demotedVocation);
		setMetatable(L, -1, "Vocation");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationGetPromotion(lua_State* L)
{
	// vocation:getPromotion()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (!vocation) {
		lua_pushnil(L);
		return 1;
	}

	uint16_t promotedId = g_vocations.getPromotedVocation(vocation->getId());
	if (promotedId == VOCATION_NONE) {
		lua_pushnil(L);
		return 1;
	}

	Vocation* promotedVocation = g_vocations.getVocation(promotedId);
	if (promotedVocation && promotedVocation != vocation) {
		pushUserdata<Vocation>(L, promotedVocation);
		setMetatable(L, -1, "Vocation");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaVocationAllowsPvp(lua_State* L)
{
	// vocation:allowsPvp()
	const Vocation* vocation = getUserdata<const Vocation>(L, 1);
	if (vocation) {
		pushBoolean(L, vocation->allowsPvp());
	} else {
		lua_pushnil(L);
	}
	return 1;
}
} // namespace

void LuaScriptInterface::registerVocation()
{
	// Vocation
	registerClass("Vocation", "", luaVocationCreate);
	registerMetaMethod("Vocation", "__eq", LuaScriptInterface::luaUserdataCompare);

	registerMethod("Vocation", "getId", luaVocationGetId);
	registerMethod("Vocation", "getClientId", luaVocationGetClientId);
	registerMethod("Vocation", "getName", luaVocationGetName);
	registerMethod("Vocation", "getDescription", luaVocationGetDescription);

	registerMethod("Vocation", "getRequiredSkillTries", luaVocationGetRequiredSkillTries);
	registerMethod("Vocation", "getRequiredManaSpent", luaVocationGetRequiredManaSpent);

	registerMethod("Vocation", "getCapacityGain", luaVocationGetCapacityGain);

	registerMethod("Vocation", "getHealthGain", luaVocationGetHealthGain);
	registerMethod("Vocation", "getHealthGainTicks", luaVocationGetHealthGainTicks);
	registerMethod("Vocation", "getHealthGainAmount", luaVocationGetHealthGainAmount);

	registerMethod("Vocation", "getManaGain", luaVocationGetManaGain);
	registerMethod("Vocation", "getManaGainTicks", luaVocationGetManaGainTicks);
	registerMethod("Vocation", "getManaGainAmount", luaVocationGetManaGainAmount);

	registerMethod("Vocation", "getMaxSoul", luaVocationGetMaxSoul);
	registerMethod("Vocation", "getSoulGainTicks", luaVocationGetSoulGainTicks);

	registerMethod("Vocation", "getAttackSpeed", luaVocationGetAttackSpeed);
	registerMethod("Vocation", "getBaseSpeed", luaVocationGetBaseSpeed);

	registerMethod("Vocation", "getDemotion", luaVocationGetDemotion);
	registerMethod("Vocation", "getPromotion", luaVocationGetPromotion);

	registerMethod("Vocation", "allowsPvp", luaVocationAllowsPvp);
}
