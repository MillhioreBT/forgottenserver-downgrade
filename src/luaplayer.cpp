// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "player.h"

#include "chat.h"
#include "game.h"
#include "iologindata.h"
#include "luascript.h"
#include "spells.h"
#include "vocation.h"

extern Chat* g_chat;
extern Game g_game;
extern Vocations g_vocations;
extern Spells* g_spells;

namespace {
using namespace Lua;

// Player
int luaPlayerCreate(lua_State* L)
{
	// Player(id or guid or name or userdata)
	Player* player;
	if (isInteger(L, 2)) {
		uint32_t id = getInteger<uint32_t>(L, 2);
		if (id >= 0x10000000 && id <= Player::playerAutoID) {
			player = g_game.getPlayerByID(id);
		} else {
			player = g_game.getPlayerByGUID(id);
		}
	} else if (isString(L, 2)) {
		ReturnValue ret = g_game.getPlayerByNameWildcard(getString(L, 2), player);
		if (ret != RETURNVALUE_NOERROR) {
			lua_pushnil(L);
			lua_pushinteger(L, ret);
			return 2;
		}
	} else if (isUserdata(L, 2)) {
		if (getUserdataType(L, 2) != LuaData_Player) {
			lua_pushnil(L);
			return 1;
		}
		player = getUserdata<Player>(L, 2);
	} else {
		player = nullptr;
	}

	if (player) {
		pushUserdata<Player>(L, player);
		setMetatable(L, -1, "Player");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerIsPlayer(lua_State* L)
{
	// player:isPlayer()
	pushBoolean(L, getUserdata<const Player>(L, 1) != nullptr);
	return 1;
}

int luaPlayerIsAccountManager(lua_State* L)
{
	// player:isAccountManager()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		pushBoolean(L, player->isAccountManager());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetGuid(lua_State* L)
{
	// player:getGuid()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getGUID());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetIp(lua_State* L)
{
	// player:getIp()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getIP());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetAccountId(lua_State* L)
{
	// player:getAccountId()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getAccount());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetLastLoginSaved(lua_State* L)
{
	// player:getLastLoginSaved()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getLastLoginSaved());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetLastLogout(lua_State* L)
{
	// player:getLastLogout()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getLastLogout());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetAccountType(lua_State* L)
{
	// player:getAccountType()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getAccountType());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetAccountType(lua_State* L)
{
	// player:setAccountType(accountType)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		player->setAccountType(getInteger<AccountType_t>(L, 2));
		IOLoginData::setAccountType(player->getAccount(), player->getAccountType());
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetTibiaCoins(lua_State* L)
{
	// player:getTibiaCoins()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, IOLoginData::getTibiaCoins(player->getAccount()));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetTibiaCoins(lua_State* L)
{
	// player:setTibiaCoins(tibiaCoins)
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		IOLoginData::updateTibiaCoins(player->getAccount(), getInteger<uint64_t>(L, 2));
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetCapacity(lua_State* L)
{
	// player:getCapacity()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getCapacity());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetCapacity(lua_State* L)
{
	// player:setCapacity(capacity)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		player->setCapacity(getInteger<uint32_t>(L, 2));
		player->sendStats();
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetFreeCapacity(lua_State* L)
{
	// player:getFreeCapacity()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getFreeCapacity());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetDepotChest(lua_State* L)
{
	// player:getDepotChest(depotId[, autoCreate = false])
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	uint32_t depotId = getInteger<uint32_t>(L, 2);
	bool autoCreate = getBoolean(L, 3, false);
	DepotChest* depotChest = player->getDepotChest(depotId, autoCreate);
	if (depotChest) {
		player->setLastDepotId(static_cast<uint16_t>(depotId)); // FIXME: workaround for #2251
		pushUserdata<Item>(L, depotChest);
		setItemMetatable(L, -1, depotChest);
	} else {
		pushBoolean(L, false);
	}
	return 1;
}

int luaPlayerGetSkullTime(lua_State* L)
{
	// player:getSkullTime()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getSkullTicks());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetSkullTime(lua_State* L)
{
	// player:setSkullTime(skullTime)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		player->setSkullTicks(getInteger<int64_t>(L, 2));
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetDeathPenalty(lua_State* L)
{
	// player:getDeathPenalty()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushnumber(L, player->getLostPercent() * 100);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetExperience(lua_State* L)
{
	// player:getExperience()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getExperience());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddExperience(lua_State* L)
{
	// player:addExperience(experience[, sendText = false])
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint64_t experience = getInteger<uint64_t>(L, 2);
		bool sendText = getBoolean(L, 3, false);
		player->addExperience(nullptr, experience, sendText);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerRemoveExperience(lua_State* L)
{
	// player:removeExperience(experience[, sendText = false])
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint64_t experience = getInteger<uint64_t>(L, 2);
		bool sendText = getBoolean(L, 3, false);
		player->removeExperience(experience, sendText);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetLevel(lua_State* L)
{
	// player:getLevel()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getLevel());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetMagicLevel(lua_State* L)
{
	// player:getMagicLevel()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getMagicLevel());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetBaseMagicLevel(lua_State* L)
{
	// player:getBaseMagicLevel()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getBaseMagicLevel());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetMana(lua_State* L)
{
	// player:getMana()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getMana());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetMana(lua_State* L)
{
	// player:setMana(mana)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	player->setMana(getInteger<uint32_t>(L, 2));
	player->sendStats();
	pushBoolean(L, true);
	return 1;
}

int luaPlayerAddMana(lua_State* L)
{
	// player:addMana(manaChange[, animationOnLoss = false])
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	int32_t manaChange = getInteger<int32_t>(L, 2);
	bool animationOnLoss = getBoolean(L, 3, false);
	if (!animationOnLoss && manaChange < 0) {
		player->changeMana(manaChange);
	} else {
		CombatDamage damage;
		damage.primary.value = manaChange;
		damage.origin = ORIGIN_NONE;
		g_game.combatChangeMana(nullptr, player, damage);
	}
	pushBoolean(L, true);
	return 1;
}

int luaPlayerGetMaxMana(lua_State* L)
{
	// player:getMaxMana()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getMaxMana());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetMaxMana(lua_State* L)
{
	// player:setMaxMana(maxMana)
	Player* player = getPlayer(L, 1);
	if (player) {
		player->setMaxMana(getInteger<uint32_t>(L, 2));
		player->setMana(player->getMana());
		player->sendStats();
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetManaSpent(lua_State* L)
{
	// player:getManaSpent()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getSpentMana());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddManaSpent(lua_State* L)
{
	// player:addManaSpent(amount[, artificial = true])
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint64_t amount = getInteger<uint64_t>(L, 2);
		bool artificial = getBoolean(L, 3, true);
		player->addManaSpent(amount, artificial);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerRemoveManaSpent(lua_State* L)
{
	// player:removeManaSpent(amount[, notify = true])
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		player->removeManaSpent(getInteger<uint64_t>(L, 2), getBoolean(L, 3, true));
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetBaseMaxHealth(lua_State* L)
{
	// player:getBaseMaxHealth()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getBaseMaxHealth());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetBaseMaxMana(lua_State* L)
{
	// player:getBaseMaxMana()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getBaseMaxMana());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetSkillLevel(lua_State* L)
{
	// player:getSkillLevel(skillType)
	skills_t skillType = getInteger<skills_t>(L, 2);
	const Player* player = getUserdata<const Player>(L, 1);
	if (player && skillType <= SKILL_LAST) {
		lua_pushinteger(L, player->getBaseSkill(skillType));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetEffectiveSkillLevel(lua_State* L)
{
	// player:getEffectiveSkillLevel(skillType)
	skills_t skillType = getInteger<skills_t>(L, 2);
	const Player* player = getUserdata<const Player>(L, 1);
	if (player && skillType <= SKILL_LAST) {
		lua_pushinteger(L, player->getSkillLevel(skillType));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetSkillPercent(lua_State* L)
{
	// player:getSkillPercent(skillType)
	skills_t skillType = getInteger<skills_t>(L, 2);
	const Player* player = getUserdata<const Player>(L, 1);
	if (player && skillType <= SKILL_LAST) {
		lua_pushinteger(L, player->getSkillPercent(skillType));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetSkillTries(lua_State* L)
{
	// player:getSkillTries(skillType)
	skills_t skillType = getInteger<skills_t>(L, 2);
	const Player* player = getUserdata<const Player>(L, 1);
	if (player && skillType <= SKILL_LAST) {
		lua_pushinteger(L, player->getSkillTries(skillType));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddSkillTries(lua_State* L)
{
	// player:addSkillTries(skillType, tries[, artificial = false])
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		skills_t skillType = getInteger<skills_t>(L, 2);
		uint64_t tries = getInteger<uint64_t>(L, 3);
		bool artificial = getBoolean(L, 4, true);
		player->addSkillAdvance(skillType, tries, artificial);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerRemoveSkillTries(lua_State* L)
{
	// player:removeSkillTries(skillType, tries[, notify = true])
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		skills_t skillType = getInteger<skills_t>(L, 2);
		uint64_t tries = getInteger<uint64_t>(L, 3);
		player->removeSkillTries(skillType, tries, getBoolean(L, 4, true));
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetSpecialSkill(lua_State* L)
{
	// player:getSpecialSkill(specialSkillType)
	SpecialSkills_t specialSkillType = getInteger<SpecialSkills_t>(L, 2);
	const Player* player = getUserdata<const Player>(L, 1);
	if (player && specialSkillType <= SPECIALSKILL_LAST) {
		lua_pushinteger(L, player->getSpecialSkill(specialSkillType));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddSpecialSkill(lua_State* L)
{
	// player:addSpecialSkill(specialSkillType, value)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	SpecialSkills_t specialSkillType = getInteger<SpecialSkills_t>(L, 2);
	if (specialSkillType > SPECIALSKILL_LAST) {
		lua_pushnil(L);
		return 1;
	}

	player->setVarSpecialSkill(specialSkillType, getInteger<int32_t>(L, 3));
	player->sendSkills();
	pushBoolean(L, true);
	return 1;
}

int luaPlayerGetItemCount(lua_State* L)
{
	// player:getItemCount(itemId[, subType = -1])
	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	uint16_t itemId;
	if (isInteger(L, 2)) {
		itemId = getInteger<uint16_t>(L, 2);
	} else {
		itemId = Item::items.getItemIdByName(getString(L, 2));
		if (itemId == 0) {
			lua_pushnil(L);
			return 1;
		}
	}

	int32_t subType = getInteger<int32_t>(L, 3, -1);
	lua_pushinteger(L, player->getItemTypeCount(itemId, subType));
	return 1;
}

int luaPlayerGetItemById(lua_State* L)
{
	// player:getItemById(itemId, deepSearch[, subType = -1])
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	uint16_t itemId;
	if (isInteger(L, 2)) {
		itemId = getInteger<uint16_t>(L, 2);
	} else {
		itemId = Item::items.getItemIdByName(getString(L, 2));
		if (itemId == 0) {
			lua_pushnil(L);
			return 1;
		}
	}
	bool deepSearch = getBoolean(L, 3);
	int32_t subType = getInteger<int32_t>(L, 4, -1);

	Item* item = g_game.findItemOfType(player, itemId, deepSearch, subType);
	if (item) {
		pushUserdata<Item>(L, item);
		setItemMetatable(L, -1, item);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetVocation(lua_State* L)
{
	// player:getVocation()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		pushUserdata<Vocation>(L, player->getVocation());
		setMetatable(L, -1, "Vocation");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetVocation(lua_State* L)
{
	// player:setVocation(id or name or userdata)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	Vocation* vocation;
	if (isInteger(L, 2)) {
		vocation = g_vocations.getVocation(getInteger<uint16_t>(L, 2));
	} else if (isString(L, 2)) {
		if (auto id = g_vocations.getVocationId(getString(L, 2))) {
			vocation = g_vocations.getVocation(id.value());
		} else {
			vocation = nullptr;
		}
	} else if (isUserdata(L, 2)) {
		vocation = getUserdata<Vocation>(L, 2);
	} else {
		vocation = nullptr;
	}

	if (!vocation) {
		pushBoolean(L, false);
		return 1;
	}

	player->setVocation(vocation->getId());
	pushBoolean(L, true);
	return 1;
}

int luaPlayerGetSex(lua_State* L)
{
	// player:getSex()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getSex());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetSex(lua_State* L)
{
	// player:setSex(newSex)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		PlayerSex_t newSex = getInteger<PlayerSex_t>(L, 2);
		player->setSex(newSex);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetTown(lua_State* L)
{
	// player:getTown()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		pushUserdata<Town>(L, player->getTown());
		setMetatable(L, -1, "Town");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetTown(lua_State* L)
{
	// player:setTown(town)
	Town* town = getUserdata<Town>(L, 2);
	if (!town) {
		pushBoolean(L, false);
		return 1;
	}

	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		player->setTown(town);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetGuild(lua_State* L)
{
	// player:getGuild()
	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	Guild* guild = player->getGuild();
	if (!guild) {
		lua_pushnil(L);
		return 1;
	}

	pushUserdata<Guild>(L, guild);
	setMetatable(L, -1, "Guild");
	return 1;
}

int luaPlayerSetGuild(lua_State* L)
{
	// player:setGuild(guild)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	player->setGuild(getUserdata<Guild>(L, 2));
	pushBoolean(L, true);
	return 1;
}

int luaPlayerGetGuildLevel(lua_State* L)
{
	// player:getGuildLevel()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player && player->getGuild()) {
		lua_pushinteger(L, player->getGuildRank()->level);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetGuildLevel(lua_State* L)
{
	// player:setGuildLevel(level)
	Player* player = getUserdata<Player>(L, 1);
	if (!player || !player->getGuild()) {
		lua_pushnil(L);
		return 1;
	}

	uint8_t level = getInteger<uint8_t>(L, 2);
	GuildRank_ptr rank = player->getGuild()->getRankByLevel(level);
	if (!rank) {
		pushBoolean(L, false);
	} else {
		player->setGuildRank(rank);
		pushBoolean(L, true);
	}

	return 1;
}

int luaPlayerGetGuildNick(lua_State* L)
{
	// player:getGuildNick()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		pushString(L, player->getGuildNick());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetGuildNick(lua_State* L)
{
	// player:setGuildNick(nick)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		const std::string& nick = getString(L, 2);
		player->setGuildNick(nick);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetGroup(lua_State* L)
{
	// player:getGroup()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		pushUserdata<Group>(L, player->getGroup());
		setMetatable(L, -1, "Group");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetGroup(lua_State* L)
{
	// player:setGroup(group)
	Group* group = getUserdata<Group>(L, 2);
	if (!group) {
		pushBoolean(L, false);
		return 1;
	}

	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		player->setGroup(group);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetStamina(lua_State* L)
{
	// player:getStamina()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getStaminaMinutes());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetStamina(lua_State* L)
{
	// player:setStamina(stamina)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint16_t stamina = getInteger<uint16_t>(L, 2);
		player->setStaminaMinutes(stamina);
		player->sendStats();
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetSoul(lua_State* L)
{
	// player:getSoul()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getSoul());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddSoul(lua_State* L)
{
	// player:addSoul(soulChange)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		int32_t soulChange = getInteger<int32_t>(L, 2);
		player->changeSoul(soulChange);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetMaxSoul(lua_State* L)
{
	// player:getMaxSoul()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		if (auto vocation = player->getVocation()) {
			lua_pushinteger(L, vocation->getSoulMax());
		} else {
			pushBoolean(L, false);
		}
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetBankBalance(lua_State* L)
{
	// player:getBankBalance()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getBankBalance());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetBankBalance(lua_State* L)
{
	// player:setBankBalance(bankBalance)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	int64_t balance = getInteger<int64_t>(L, 2);
	if (balance < 0) {
		reportErrorFunc(L, "Invalid bank balance value.");
		lua_pushnil(L);
		return 1;
	}

	player->setBankBalance(balance);
	pushBoolean(L, true);
	return 1;
}

int luaPlayerAddItem(lua_State* L)
{
	// player:addItem(itemId[, count = 1[, canDropOnMap = true[, subType = 1[, slot = CONST_SLOT_WHEREEVER]]]])
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		pushBoolean(L, false);
		return 1;
	}

	uint16_t itemId;
	if (isInteger(L, 2)) {
		itemId = getInteger<uint16_t>(L, 2);
	} else {
		itemId = Item::items.getItemIdByName(getString(L, 2));
		if (itemId == 0) {
			lua_pushnil(L);
			return 1;
		}
	}

	int32_t count = getInteger<int32_t>(L, 3, 1);
	int32_t subType = getInteger<int32_t>(L, 5, 1);

	const ItemType& it = Item::items[itemId];

	int32_t itemCount = 1;
	int parameters = lua_gettop(L);
	if (parameters >= 5) {
		itemCount = std::max<int32_t>(1, count);
	} else if (it.hasSubType()) {
		if (it.stackable) {
			itemCount = std::ceil(count / static_cast<float>(it.stackSize));
		}

		subType = count;
	} else {
		itemCount = std::max<int32_t>(1, count);
	}

	bool hasTable = itemCount > 1;
	if (hasTable) {
		lua_newtable(L);
	} else if (itemCount == 0) {
		lua_pushnil(L);
		return 1;
	}

	bool canDropOnMap = getBoolean(L, 4, true);
	slots_t slot = getInteger<slots_t>(L, 6, CONST_SLOT_WHEREEVER);
	for (int32_t i = 1; i <= itemCount; ++i) {
		int32_t stackCount = subType;
		if (it.stackable) {
			stackCount = std::min<int32_t>(stackCount, it.stackSize);
			subType -= stackCount;
		}

		Item* item = Item::CreateItem(itemId, static_cast<uint16_t>(stackCount));
		if (!item) {
			if (!hasTable) {
				lua_pushnil(L);
			}
			return 1;
		}

		ReturnValue ret = g_game.internalPlayerAddItem(player, item, canDropOnMap, slot);
		if (ret != RETURNVALUE_NOERROR) {
			delete item;
			if (!hasTable) {
				lua_pushnil(L);
			}
			return 1;
		}

		if (hasTable) {
			lua_pushinteger(L, i);
			pushUserdata<Item>(L, item);
			setItemMetatable(L, -1, item);
			lua_settable(L, -3);
		} else {
			pushUserdata<Item>(L, item);
			setItemMetatable(L, -1, item);
		}
	}
	return 1;
}

int luaPlayerAddItemEx(lua_State* L)
{
	// player:addItemEx(item[, canDropOnMap = false[, index = INDEX_WHEREEVER[, flags = 0]]])
	// player:addItemEx(item[, canDropOnMap = true[, slot = CONST_SLOT_WHEREEVER]])
	Item* item = getUserdata<Item>(L, 2);
	if (!item) {
		reportErrorFunc(L, LuaScriptInterface::getErrorDesc(LuaErrorCode::ITEM_NOT_FOUND));
		pushBoolean(L, false);
		return 1;
	}

	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	if (item->getParent() != VirtualCylinder::virtualCylinder) {
		reportErrorFunc(L, "Item already has a parent");
		pushBoolean(L, false);
		return 1;
	}

	bool canDropOnMap = getBoolean(L, 3, false);
	ReturnValue returnValue;
	if (canDropOnMap) {
		slots_t slot = getInteger<slots_t>(L, 4, CONST_SLOT_WHEREEVER);
		returnValue = g_game.internalPlayerAddItem(player, item, true, slot);
	} else {
		int32_t index = getInteger<int32_t>(L, 4, INDEX_WHEREEVER);
		uint32_t flags = getInteger<uint32_t>(L, 5, 0);
		returnValue = g_game.internalAddItem(player, item, index, flags);
	}

	if (returnValue == RETURNVALUE_NOERROR) {
		ScriptEnvironment::removeTempItem(item);
	}
	lua_pushinteger(L, returnValue);
	return 1;
}

int luaPlayerRemoveItem(lua_State* L)
{
	// player:removeItem(itemId, count[, subType = -1[, ignoreEquipped = false]])
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	uint16_t itemId;
	if (isInteger(L, 2)) {
		itemId = getInteger<uint16_t>(L, 2);
	} else {
		itemId = Item::items.getItemIdByName(getString(L, 2));
		if (itemId == 0) {
			lua_pushnil(L);
			return 1;
		}
	}

	uint32_t count = getInteger<uint32_t>(L, 3);
	int32_t subType = getInteger<int32_t>(L, 4, -1);
	bool ignoreEquipped = getBoolean(L, 5, false);
	pushBoolean(L, player->removeItemOfType(itemId, count, subType, ignoreEquipped));
	return 1;
}

int luaPlayerGetMoney(lua_State* L)
{
	// player:getMoney()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getMoney());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddMoney(lua_State* L)
{
	// player:addMoney(money)
	uint64_t money = getInteger<uint64_t>(L, 2);
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		g_game.addMoney(player, money);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerRemoveMoney(lua_State* L)
{
	// player:removeMoney(money)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint64_t money = getInteger<uint64_t>(L, 2);
		pushBoolean(L, g_game.removeMoney(player, money));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerShowTextDialog(lua_State* L)
{
	// player:showTextDialog(id or name or userdata[, text[, canWrite[, length]]])
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	bool canWrite = getBoolean(L, 4, false);
	int32_t length = getInteger<int32_t>(L, 5, -1);
	std::string text;

	int parameters = lua_gettop(L);
	if (parameters >= 3) {
		text = getString(L, 3);
	}

	Item* item;
	if (isInteger(L, 2)) {
		item = Item::CreateItem(getInteger<uint16_t>(L, 2));
	} else if (isString(L, 2)) {
		item = Item::CreateItem(Item::items.getItemIdByName(getString(L, 2)));
	} else if (isUserdata(L, 2)) {
		if (getUserdataType(L, 2) != LuaData_Item) {
			pushBoolean(L, false);
			return 1;
		}

		item = getUserdata<Item>(L, 2);
	} else {
		item = nullptr;
	}

	if (!item) {
		reportErrorFunc(L, LuaScriptInterface::getErrorDesc(LuaErrorCode::ITEM_NOT_FOUND));
		pushBoolean(L, false);
		return 1;
	}

	if (length < 0) {
		length = Item::items[item->getID()].maxTextLen;
	}

	if (!text.empty()) {
		item->setText(text);
		length = std::max<int32_t>(text.size(), length);
	}

	item->setParent(player);
	player->incrementWindowTextId();
	player->setRawWriteItem(item);
	player->setMaxWriteLen(static_cast<uint16_t>(length));
	player->sendTextWindow(item, static_cast<uint16_t>(length), canWrite);
	lua_pushinteger(L, player->getWindowTextId());
	return 1;
}

int luaPlayerSendTextMessage(lua_State* L)
{
	// player:sendTextMessage(type, text[, position, primaryValue = 0, primaryColor = TEXTCOLOR_NONE[, secondaryValue =
	// 0, secondaryColor = TEXTCOLOR_NONE]]) player:sendTextMessage(type, text, channelId)

	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	int parameters = lua_gettop(L);

	TextMessage message(getInteger<MessageClasses>(L, 2), getString(L, 3));
	if (parameters == 4) {
		uint16_t channelId = getInteger<uint16_t>(L, 4);
		ChatChannel* channel = g_chat->getChannel(*player, channelId);
		if (!channel || !channel->hasUser(*player)) {
			pushBoolean(L, false);
			return 1;
		}
		message.channelId = channelId;
	} else {
		if (parameters >= 6) {
			message.position = getPosition(L, 4);
			message.primary.value = getInteger<int32_t>(L, 5);
			message.primary.color = getInteger<TextColor_t>(L, 6);
		}

		if (parameters >= 8) {
			message.secondary.value = getInteger<int32_t>(L, 7);
			message.secondary.color = getInteger<TextColor_t>(L, 8);
		}
	}

	player->sendTextMessage(message);
	pushBoolean(L, true);
	return 1;
}

int luaPlayerSendChannelMessage(lua_State* L)
{
	// player:sendChannelMessage(author, text, type, channelId)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	uint16_t channelId = getInteger<uint16_t>(L, 5);
	SpeakClasses type = getInteger<SpeakClasses>(L, 4);
	const std::string& text = getString(L, 3);
	const std::string& author = getString(L, 2);
	player->sendChannelMessage(author, text, type, channelId);
	pushBoolean(L, true);
	return 1;
}

int luaPlayerSendPrivateMessage(lua_State* L)
{
	// player:sendPrivateMessage(speaker, text[, type])
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	const Player* speaker = getUserdata<const Player>(L, 2);
	const std::string& text = getString(L, 3);
	SpeakClasses type = getInteger<SpeakClasses>(L, 4, TALKTYPE_PRIVATE);
	player->sendPrivateMessage(speaker, type, text);
	pushBoolean(L, true);
	return 1;
}

int luaPlayerChannelSay(lua_State* L)
{
	// player:channelSay(speaker, type, text, channelId)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	Creature* speaker = getCreature(L, 2);
	SpeakClasses type = getInteger<SpeakClasses>(L, 3);
	const std::string& text = getString(L, 4);
	uint16_t channelId = getInteger<uint16_t>(L, 5);
	player->sendToChannel(speaker, type, text, channelId);
	pushBoolean(L, true);
	return 1;
}

int luaPlayerOpenChannel(lua_State* L)
{
	// player:openChannel(channelId)
	uint16_t channelId = getInteger<uint16_t>(L, 2);
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		g_game.playerOpenChannel(player->getID(), channelId);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetSlotItem(lua_State* L)
{
	// player:getSlotItem(slot)
	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	uint32_t slot = getInteger<uint32_t>(L, 2);
	Thing* thing = player->getThing(slot);
	if (!thing) {
		lua_pushnil(L);
		return 1;
	}

	Item* item = thing->getItem();
	if (item) {
		pushUserdata<Item>(L, item);
		setItemMetatable(L, -1, item);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetParty(lua_State* L)
{
	// player:getParty()
	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	Party* party = player->getParty();
	if (party) {
		pushUserdata<Party>(L, party);
		setMetatable(L, -1, "Party");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddOutfit(lua_State* L)
{
	// player:addOutfit(lookType)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		player->addOutfit(getInteger<uint16_t>(L, 2), 0);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddOutfitAddon(lua_State* L)
{
	// player:addOutfitAddon(lookType, addon)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint16_t lookType = getInteger<uint16_t>(L, 2);
		uint8_t addon = getInteger<uint8_t>(L, 3);
		player->addOutfit(lookType, addon);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerRemoveOutfit(lua_State* L)
{
	// player:removeOutfit(lookType)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint16_t lookType = getInteger<uint16_t>(L, 2);
		pushBoolean(L, player->removeOutfit(lookType));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerRemoveOutfitAddon(lua_State* L)
{
	// player:removeOutfitAddon(lookType, addon)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint16_t lookType = getInteger<uint16_t>(L, 2);
		uint8_t addon = getInteger<uint8_t>(L, 3);
		pushBoolean(L, player->removeOutfitAddon(lookType, addon));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerHasOutfit(lua_State* L)
{
	// player:hasOutfit(lookType[, addon = 0])
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint16_t lookType = getInteger<uint16_t>(L, 2);
		uint8_t addon = getInteger<uint8_t>(L, 3, 0);
		pushBoolean(L, player->hasOutfit(lookType, addon));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerCanWearOutfit(lua_State* L)
{
	// player:canWearOutfit(lookType[, addon = 0])
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		uint16_t lookType = getInteger<uint16_t>(L, 2);
		uint8_t addon = getInteger<uint8_t>(L, 3, 0);
		pushBoolean(L, player->canWear(lookType, addon));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSendOutfitWindow(lua_State* L)
{
	// player:sendOutfitWindow()
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		player->sendOutfitWindow();
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetPremiumEndsAt(lua_State* L)
{
	// player:getPremiumEndsAt()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getPremiumEndsAt());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSetPremiumEndsAt(lua_State* L)
{
	// player:setPremiumEndsAt(timestamp)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	time_t timestamp = getInteger<time_t>(L, 2);

	player->setPremiumTime(timestamp);
	IOLoginData::updatePremiumTime(player->getAccount(), timestamp);
	pushBoolean(L, true);
	return 1;
}

int luaPlayerHasBlessing(lua_State* L)
{
	// player:hasBlessing(blessing)
	auto blessing = getBlessingId(L, 2);
	const Player* player = getUserdata<const Player>(L, 1);
	if (player && blessing) {
		pushBoolean(L, player->hasBlessing(blessing.value()));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddBlessing(lua_State* L)
{
	// player:addBlessing(blessing)
	auto blessing = getBlessingId(L, 2);
	Player* player = getUserdata<Player>(L, 1);
	if (!player || !blessing) {
		lua_pushnil(L);
		return 1;
	}

	if (player->hasBlessing(blessing.value())) {
		pushBoolean(L, false);
		return 1;
	}

	player->addBlessing(blessing.value());
	pushBoolean(L, true);
	return 1;
}

int luaPlayerRemoveBlessing(lua_State* L)
{
	// player:removeBlessing(blessing)
	auto blessing = getBlessingId(L, 2);
	Player* player = getUserdata<Player>(L, 1);
	if (!player || !blessing) {
		lua_pushnil(L);
		return 1;
	}

	if (!player->hasBlessing(blessing.value())) {
		pushBoolean(L, false);
		return 1;
	}

	player->removeBlessing(blessing.value());
	pushBoolean(L, true);
	return 1;
}

int luaPlayerCanLearnSpell(lua_State* L)
{
	// player:canLearnSpell(spellName)
	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	const std::string& spellName = getString(L, 2);
	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if (!spell) {
		reportErrorFunc(L, "Spell \"" + spellName + "\" not found");
		pushBoolean(L, false);
		return 1;
	}

	if (player->hasFlag(PlayerFlag_IgnoreSpellCheck)) {
		pushBoolean(L, true);
		return 1;
	}

	if (player->getLevel() < spell->getLevel()) {
		pushBoolean(L, false);
	} else if (player->getMagicLevel() < spell->getMagicLevel()) {
		pushBoolean(L, false);
	} else if (!spell->hasVocationSpellMap(player->getVocationId())) {
		pushBoolean(L, false);
	} else {
		pushBoolean(L, true);
	}
	return 1;
}

int luaPlayerLearnSpell(lua_State* L)
{
	// player:learnSpell(spellName)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		const std::string& spellName = getString(L, 2);
		player->learnInstantSpell(spellName);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerForgetSpell(lua_State* L)
{
	// player:forgetSpell(spellName)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		const std::string& spellName = getString(L, 2);
		player->forgetInstantSpell(spellName);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerHasLearnedSpell(lua_State* L)
{
	// player:hasLearnedSpell(spellName)
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		const std::string& spellName = getString(L, 2);
		pushBoolean(L, player->hasLearnedInstantSpell(spellName));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSendTutorial(lua_State* L)
{
	// player:sendTutorial(tutorialId)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		uint8_t tutorialId = getInteger<uint8_t>(L, 2);
		player->sendTutorial(tutorialId);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerAddMapMark(lua_State* L)
{
	// player:addMapMark(position, type, description)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		const Position& position = getPosition(L, 2);
		uint8_t type = getInteger<uint8_t>(L, 3);
		const std::string& description = getString(L, 4);
		player->sendAddMarker(position, type, description);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSave(lua_State* L)
{
	// player:save()
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		player->setLoginPosition(player->getPosition());
		pushBoolean(L, IOLoginData::savePlayer(player));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerPopupFYI(lua_State* L)
{
	// player:popupFYI(message)
	Player* player = getUserdata<Player>(L, 1);
	if (player) {
		const std::string& message = getString(L, 2);
		player->sendFYIBox(message);
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerIsPzLocked(lua_State* L)
{
	// player:isPzLocked()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		pushBoolean(L, player->isPzLocked());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetClient(lua_State* L)
{
	// player:getClient()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_createtable(L, 0, 2);
		setField(L, "version", player->getProtocolVersion());
		setField(L, "os", player->getOperatingSystem());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetHouse(lua_State* L)
{
	// player:getHouse()
	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	House* house = g_game.map.houses.getHouseByPlayerId(player->getGUID());
	if (house) {
		pushUserdata<House>(L, house);
		setMetatable(L, -1, "House");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerSendHouseWindow(lua_State* L)
{
	// player:sendHouseWindow(house, listId)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	House* house = getUserdata<House>(L, 2);
	if (!house) {
		lua_pushnil(L);
		return 1;
	}

	uint32_t listId = getInteger<uint32_t>(L, 3);
	player->sendHouseWindow(house, listId);
	pushBoolean(L, true);
	return 1;
}

int luaPlayerSetEditHouse(lua_State* L)
{
	// player:setEditHouse(house, listId)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	House* house = getUserdata<House>(L, 2);
	if (!house) {
		lua_pushnil(L);
		return 1;
	}

	uint32_t listId = getInteger<uint32_t>(L, 3);
	player->setEditHouse(house, listId);
	pushBoolean(L, true);
	return 1;
}

int luaPlayerSetGhostMode(lua_State* L)
{
	// player:setGhostMode(enabled[, magicEffect = CONST_ME_TELEPORT])
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	bool enabled = getBoolean(L, 2);
	if (player->isInGhostMode() == enabled) {
		pushBoolean(L, true);
		return 1;
	}

	MagicEffectClasses magicEffect = getInteger<MagicEffectClasses>(L, 3, CONST_ME_TELEPORT);

	player->switchGhostMode();

	Tile* tile = player->getTile();
	const Position& position = player->getPosition();

	SpectatorVec spectators;
	g_game.map.getSpectators(spectators, position, true, true);
	for (Creature* spectator : spectators) {
		assert(dynamic_cast<Player*>(spectator) != nullptr);
		Player* tmpPlayer = static_cast<Player*>(spectator);
		if (tmpPlayer != player && !tmpPlayer->isAccessPlayer()) {
			if (enabled) {
				tmpPlayer->sendRemoveTileThing(position, tile->getClientIndexOfCreature(tmpPlayer, player));
			} else {
				tmpPlayer->sendCreatureAppear(player, position, magicEffect);
			}
		} else {
			tmpPlayer->sendCreatureChangeVisible(player, !enabled);
		}
	}

	if (player->isInGhostMode()) {
		for (const auto& it : g_game.getPlayers()) {
			if (!it.second->isAccessPlayer()) {
				it.second->notifyStatusChange(player, VIPSTATUS_OFFLINE);
			}
		}
		IOLoginData::updateOnlineStatus(player->getGUID(), false);
	} else {
		for (const auto& it : g_game.getPlayers()) {
			if (!it.second->isAccessPlayer()) {
				it.second->notifyStatusChange(player, VIPSTATUS_ONLINE);
			}
		}
		IOLoginData::updateOnlineStatus(player->getGUID(), true);
	}
	pushBoolean(L, true);
	return 1;
}

int luaPlayerGetContainerId(lua_State* L)
{
	// player:getContainerId(container)
	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	Container* container = getUserdata<Container>(L, 2);
	if (container) {
		lua_pushinteger(L, player->getContainerID(container));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetContainerById(lua_State* L)
{
	// player:getContainerById(id)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	Container* container = player->getContainerByID(getInteger<uint8_t>(L, 2));
	if (container) {
		pushUserdata<Container>(L, container);
		setMetatable(L, -1, "Container");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetContainerIndex(lua_State* L)
{
	// player:getContainerIndex(id)
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getContainerIndex(getInteger<uint8_t>(L, 2)));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetInstantSpells(lua_State* L)
{
	// player:getInstantSpells()
	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	std::vector<const InstantSpell*> spells;
	for (auto& spell : g_spells->getInstantSpells()) {
		if (spell.second.canCast(player)) {
			spells.push_back(&spell.second);
		}
	}

	lua_createtable(L, spells.size(), 0);

	int index = 0;
	for (auto spell : spells) {
		pushInstantSpell(L, *spell);
		lua_rawseti(L, -2, ++index);
	}
	return 1;
}

int luaPlayerCanCast(lua_State* L)
{
	// player:canCast(spell)
	const Player* player = getUserdata<const Player>(L, 1);
	InstantSpell* spell = getUserdata<InstantSpell>(L, 2);
	if (player && spell) {
		pushBoolean(L, spell->canCast(player));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerHasChaseMode(lua_State* L)
{
	// player:hasChaseMode()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		pushBoolean(L, player->getChaseMode());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerHasSecureMode(lua_State* L)
{
	// player:hasSecureMode()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		pushBoolean(L, player->getSecureMode());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetFightMode(lua_State* L)
{
	// player:getFightMode()
	const Player* player = getUserdata<const Player>(L, 1);
	if (player) {
		lua_pushinteger(L, player->getFightMode());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaPlayerGetIdleTime(lua_State* L)
{
	// player:getIdleTime()
	const Player* player = getUserdata<const Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	lua_pushinteger(L, player->getIdleTime());
	return 1;
}

int luaPlayerResetIdleTime(lua_State* L)
{
	// player:resetIdleTime()
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	player->resetIdleTime();
	pushBoolean(L, true);
	return 1;
}

int luaPlayerOpenContainer(lua_State* L)
{
	// player:openContainer(container)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	Container* container = getUserdata<Container>(L, 2);
	if (!container) {
		lua_pushnil(L);
		return 1;
	}

	if (player->getContainerID(container) == -1) {
		player->addContainer(player->getOpenContainers().size(), container);
		player->onSendContainer(container);
		pushBoolean(L, true);
		return 1;
	}

	pushBoolean(L, false);
	return 1;
}

int luaPlayerCloseContainer(lua_State* L)
{
	// player:closeContainer(container)
	Player* player = getUserdata<Player>(L, 1);
	if (!player) {
		lua_pushnil(L);
		return 1;
	}

	const Container* container = getUserdata<const Container>(L, 2);
	if (!container) {
		lua_pushnil(L);
		return 1;
	}

	int8_t oldContainerId = player->getContainerID(container);
	if (oldContainerId != -1) {
		player->onCloseContainer(container);
		player->closeContainer(oldContainerId);
		pushBoolean(L, true);
		return 1;
	}

	pushBoolean(L, false);
	return 1;
}

// OfflinePlayer
int luaOfflinePlayerCreate(lua_State* L)
{
	// OfflinePlayer(guid or name)
	auto player = std::make_unique<Player>(nullptr);
	if (const uint32_t guid = getInteger<uint32_t>(L, 2)) {
		if (g_game.getPlayerByGUID(guid) || !IOLoginData::loadPlayerById(player.get(), guid)) {
			lua_pushnil(L);
			return 1;
		}
	} else if (const std::string& name = getString(L, 2); !name.empty()) {
		if (g_game.getPlayerByName(name) || !IOLoginData::loadPlayerByName(player.get(), name)) {
			lua_pushnil(L);
			return 1;
		}
	}

	if (player) {
		pushUserdata<Player>(L, player.release());
		setMetatable(L, -1, "OfflinePlayer");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaOfflinePlayerRemove(lua_State* L)
{
	// offlinePlayer:__close() or offlinePlayer:__gc()
	Player** playerPtr = getRawUserdata<Player>(L, 1);
	if (auto player = *playerPtr) {
		delete player;
		*playerPtr = nullptr;
	}
	return 0;
}
} // namespace

void LuaScriptInterface::registerPlayer()
{
	// Player
	registerClass("Player", "Creature", luaPlayerCreate);
	registerMetaMethod("Player", "__eq", LuaScriptInterface::luaUserdataCompare);

	registerMethod("Player", "isPlayer", luaPlayerIsPlayer);
	registerMethod("Player", "isAccountManager", luaPlayerIsAccountManager);

	registerMethod("Player", "getGuid", luaPlayerGetGuid);
	registerMethod("Player", "getIp", luaPlayerGetIp);
	registerMethod("Player", "getAccountId", luaPlayerGetAccountId);
	registerMethod("Player", "getLastLoginSaved", luaPlayerGetLastLoginSaved);
	registerMethod("Player", "getLastLogout", luaPlayerGetLastLogout);

	registerMethod("Player", "getAccountType", luaPlayerGetAccountType);
	registerMethod("Player", "setAccountType", luaPlayerSetAccountType);

	registerMethod("Player", "getTibiaCoins", luaPlayerGetTibiaCoins);
	registerMethod("Player", "setTibiaCoins", luaPlayerSetTibiaCoins);

	registerMethod("Player", "getCapacity", luaPlayerGetCapacity);
	registerMethod("Player", "setCapacity", luaPlayerSetCapacity);

	registerMethod("Player", "getFreeCapacity", luaPlayerGetFreeCapacity);

	registerMethod("Player", "getDepotChest", luaPlayerGetDepotChest);

	registerMethod("Player", "getSkullTime", luaPlayerGetSkullTime);
	registerMethod("Player", "setSkullTime", luaPlayerSetSkullTime);
	registerMethod("Player", "getDeathPenalty", luaPlayerGetDeathPenalty);

	registerMethod("Player", "getExperience", luaPlayerGetExperience);
	registerMethod("Player", "addExperience", luaPlayerAddExperience);
	registerMethod("Player", "removeExperience", luaPlayerRemoveExperience);
	registerMethod("Player", "getLevel", luaPlayerGetLevel);

	registerMethod("Player", "getMagicLevel", luaPlayerGetMagicLevel);
	registerMethod("Player", "getBaseMagicLevel", luaPlayerGetBaseMagicLevel);
	registerMethod("Player", "getMana", luaPlayerGetMana);
	registerMethod("Player", "setMana", luaPlayerSetMana);
	registerMethod("Player", "addMana", luaPlayerAddMana);
	registerMethod("Player", "getMaxMana", luaPlayerGetMaxMana);
	registerMethod("Player", "setMaxMana", luaPlayerSetMaxMana);
	registerMethod("Player", "getManaSpent", luaPlayerGetManaSpent);
	registerMethod("Player", "addManaSpent", luaPlayerAddManaSpent);
	registerMethod("Player", "removeManaSpent", luaPlayerRemoveManaSpent);

	registerMethod("Player", "getBaseMaxHealth", luaPlayerGetBaseMaxHealth);
	registerMethod("Player", "getBaseMaxMana", luaPlayerGetBaseMaxMana);

	registerMethod("Player", "getSkillLevel", luaPlayerGetSkillLevel);
	registerMethod("Player", "getEffectiveSkillLevel", luaPlayerGetEffectiveSkillLevel);
	registerMethod("Player", "getSkillPercent", luaPlayerGetSkillPercent);
	registerMethod("Player", "getSkillTries", luaPlayerGetSkillTries);
	registerMethod("Player", "addSkillTries", luaPlayerAddSkillTries);
	registerMethod("Player", "removeSkillTries", luaPlayerRemoveSkillTries);
	registerMethod("Player", "getSpecialSkill", luaPlayerGetSpecialSkill);
	registerMethod("Player", "addSpecialSkill", luaPlayerAddSpecialSkill);

	registerMethod("Player", "getItemCount", luaPlayerGetItemCount);
	registerMethod("Player", "getItemById", luaPlayerGetItemById);

	registerMethod("Player", "getVocation", luaPlayerGetVocation);
	registerMethod("Player", "setVocation", luaPlayerSetVocation);

	registerMethod("Player", "getSex", luaPlayerGetSex);
	registerMethod("Player", "setSex", luaPlayerSetSex);

	registerMethod("Player", "getTown", luaPlayerGetTown);
	registerMethod("Player", "setTown", luaPlayerSetTown);

	registerMethod("Player", "getGuild", luaPlayerGetGuild);
	registerMethod("Player", "setGuild", luaPlayerSetGuild);

	registerMethod("Player", "getGuildLevel", luaPlayerGetGuildLevel);
	registerMethod("Player", "setGuildLevel", luaPlayerSetGuildLevel);

	registerMethod("Player", "getGuildNick", luaPlayerGetGuildNick);
	registerMethod("Player", "setGuildNick", luaPlayerSetGuildNick);

	registerMethod("Player", "getGroup", luaPlayerGetGroup);
	registerMethod("Player", "setGroup", luaPlayerSetGroup);

	registerMethod("Player", "getStamina", luaPlayerGetStamina);
	registerMethod("Player", "setStamina", luaPlayerSetStamina);

	registerMethod("Player", "getSoul", luaPlayerGetSoul);
	registerMethod("Player", "addSoul", luaPlayerAddSoul);
	registerMethod("Player", "getMaxSoul", luaPlayerGetMaxSoul);

	registerMethod("Player", "getBankBalance", luaPlayerGetBankBalance);
	registerMethod("Player", "setBankBalance", luaPlayerSetBankBalance);

	registerMethod("Player", "addItem", luaPlayerAddItem);
	registerMethod("Player", "addItemEx", luaPlayerAddItemEx);
	registerMethod("Player", "removeItem", luaPlayerRemoveItem);

	registerMethod("Player", "getMoney", luaPlayerGetMoney);
	registerMethod("Player", "addMoney", luaPlayerAddMoney);
	registerMethod("Player", "removeMoney", luaPlayerRemoveMoney);

	registerMethod("Player", "showTextDialog", luaPlayerShowTextDialog);

	registerMethod("Player", "sendTextMessage", luaPlayerSendTextMessage);
	registerMethod("Player", "sendChannelMessage", luaPlayerSendChannelMessage);
	registerMethod("Player", "sendPrivateMessage", luaPlayerSendPrivateMessage);
	registerMethod("Player", "channelSay", luaPlayerChannelSay);
	registerMethod("Player", "openChannel", luaPlayerOpenChannel);

	registerMethod("Player", "getSlotItem", luaPlayerGetSlotItem);

	registerMethod("Player", "getParty", luaPlayerGetParty);

	registerMethod("Player", "addOutfit", luaPlayerAddOutfit);
	registerMethod("Player", "addOutfitAddon", luaPlayerAddOutfitAddon);
	registerMethod("Player", "removeOutfit", luaPlayerRemoveOutfit);
	registerMethod("Player", "removeOutfitAddon", luaPlayerRemoveOutfitAddon);
	registerMethod("Player", "hasOutfit", luaPlayerHasOutfit);
	registerMethod("Player", "canWearOutfit", luaPlayerCanWearOutfit);
	registerMethod("Player", "sendOutfitWindow", luaPlayerSendOutfitWindow);

	registerMethod("Player", "getPremiumEndsAt", luaPlayerGetPremiumEndsAt);
	registerMethod("Player", "setPremiumEndsAt", luaPlayerSetPremiumEndsAt);

	registerMethod("Player", "hasBlessing", luaPlayerHasBlessing);
	registerMethod("Player", "addBlessing", luaPlayerAddBlessing);
	registerMethod("Player", "removeBlessing", luaPlayerRemoveBlessing);

	registerMethod("Player", "canLearnSpell", luaPlayerCanLearnSpell);
	registerMethod("Player", "learnSpell", luaPlayerLearnSpell);
	registerMethod("Player", "forgetSpell", luaPlayerForgetSpell);
	registerMethod("Player", "hasLearnedSpell", luaPlayerHasLearnedSpell);

	registerMethod("Player", "sendTutorial", luaPlayerSendTutorial);
	registerMethod("Player", "addMapMark", luaPlayerAddMapMark);

	registerMethod("Player", "save", luaPlayerSave);
	registerMethod("Player", "popupFYI", luaPlayerPopupFYI);

	registerMethod("Player", "isPzLocked", luaPlayerIsPzLocked);

	registerMethod("Player", "getClient", luaPlayerGetClient);

	registerMethod("Player", "getHouse", luaPlayerGetHouse);
	registerMethod("Player", "sendHouseWindow", luaPlayerSendHouseWindow);
	registerMethod("Player", "setEditHouse", luaPlayerSetEditHouse);

	registerMethod("Player", "setGhostMode", luaPlayerSetGhostMode);

	registerMethod("Player", "getContainerId", luaPlayerGetContainerId);
	registerMethod("Player", "getContainerById", luaPlayerGetContainerById);
	registerMethod("Player", "getContainerIndex", luaPlayerGetContainerIndex);

	registerMethod("Player", "getInstantSpells", luaPlayerGetInstantSpells);
	registerMethod("Player", "canCast", luaPlayerCanCast);

	registerMethod("Player", "hasChaseMode", luaPlayerHasChaseMode);
	registerMethod("Player", "hasSecureMode", luaPlayerHasSecureMode);
	registerMethod("Player", "getFightMode", luaPlayerGetFightMode);

	registerMethod("Player", "getIdleTime", luaPlayerGetIdleTime);
	registerMethod("Player", "resetIdleTime", luaPlayerResetIdleTime);

	registerMethod("Player", "openContainer", luaPlayerOpenContainer);
	registerMethod("Player", "closeContainer", luaPlayerCloseContainer);

	// OfflinePlayer
	registerClass("OfflinePlayer", "Player", luaOfflinePlayerCreate);
	registerMetaMethod("OfflinePlayer", "__gc", luaOfflinePlayerRemove);
	registerMetaMethod("OfflinePlayer", "__close", luaOfflinePlayerRemove);
}
