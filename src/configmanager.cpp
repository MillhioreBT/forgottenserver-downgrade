// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found
// in the LICENSE file.

#include "otpch.h"

#include "configmanager.h"

#include "game.h"
#include "monster.h"
#include "pugicast.h"

#ifdef _WIN32
#include "lua.hpp"
#elif defined(__linux__)
#include "lua5.4/lua.hpp"
#endif

#if LUA_VERSION_NUM >= 502
#undef lua_strlen
#define lua_strlen lua_rawlen
#endif

extern Game g_game;

namespace {

template <typename T>
auto getEnv(const char* envVar, T&& defaultValue)
{
	if (auto value = std::getenv(envVar)) {
		return pugi::cast<std::decay_t<T>>(value);
	}
	return defaultValue;
}

std::string getGlobalString(lua_State* L, const char* identifier, const char* defaultValue)
{
	lua_getglobal(L, identifier);
	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return defaultValue;
	}

	size_t len = lua_strlen(L, -1);
	std::string ret{lua_tostring(L, -1), len};
	lua_pop(L, 1);
	return ret;
}

int64_t getGlobalInteger(lua_State* L, const char* identifier, const int64_t defaultValue = 0)
{
	lua_getglobal(L, identifier);
	if (!lua_isinteger(L, -1)) {
		lua_pop(L, 1);
		return defaultValue;
	}

	int64_t val = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return val;
}

bool getGlobalBoolean(lua_State* L, const char* identifier, const bool defaultValue)
{
	lua_getglobal(L, identifier);
	if (!lua_isboolean(L, -1)) {
		if (!lua_isstring(L, -1)) {
			lua_pop(L, 1);
			return defaultValue;
		}

		size_t len = lua_strlen(L, -1);
		std::string ret{lua_tostring(L, -1), len};
		lua_pop(L, 1);
		return booleanString(ret);
	}

	int val = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return val != 0;
}

} // namespace

ConfigManager::ConfigManager() { strings[ConfigKeysString::CONFIG_FILE] = "config.lua"; }

namespace {

ExperienceStages loadLuaStages(lua_State* L)
{
	ExperienceStages stages;

	lua_getglobal(L, "experienceStages");
	if (!lua_istable(L, -1)) {
		return {};
	}

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		const auto tableIndex = lua_gettop(L);
		auto minLevel = Lua::getField<uint32_t>(L, tableIndex, "minlevel", 1);
		auto maxLevel =
		    Lua::getField<uint32_t>(L, tableIndex, "maxlevel", std::numeric_limits<uint32_t>::max());
		auto multiplier = Lua::getField<float>(L, tableIndex, "multiplier", 1);
		stages.emplace_back(minLevel, maxLevel, multiplier);
		lua_pop(L, 4);
	}
	lua_pop(L, 1);

	std::sort(stages.begin(), stages.end());
	return stages;
}

ExperienceStages loadXMLStages()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/XML/stages.xml");
	if (!result) {
		printXMLError("Error - loadXMLStages", "data/XML/stages.xml", result);
		return {};
	}

	ExperienceStages stages;
	for (const auto& stageNode : doc.child("stages").children()) {
		if (caseInsensitiveEqual(stageNode.name(), "config")) {
			if (!stageNode.attribute("enabled").as_bool()) {
				return {};
			}
		} else {
			uint32_t minLevel = 1, maxLevel = std::numeric_limits<uint32_t>::max(), multiplier = 1;

			if (auto minLevelAttribute = stageNode.attribute("minlevel")) {
				minLevel = pugi::cast<uint32_t>(minLevelAttribute.value());
			}

			if (auto maxLevelAttribute = stageNode.attribute("maxlevel")) {
				maxLevel = pugi::cast<uint32_t>(maxLevelAttribute.value());
			}

			if (auto multiplierAttribute = stageNode.attribute("multiplier")) {
				multiplier = pugi::cast<uint32_t>(multiplierAttribute.value());
			}

			stages.emplace_back(minLevel, maxLevel, multiplier);
		}
	}

	std::sort(stages.begin(), stages.end());
	return stages;
}

} // namespace

bool ConfigManager::load()
{
	lua_State* L = luaL_newstate();
	if (!L) {
		throw std::runtime_error("Failed to allocate memory");
	}

	luaL_openlibs(L);

	if (luaL_dofile(L, getString(ConfigKeysString::CONFIG_FILE).data())) {
		std::cout << "[Error - ConfigManager::load] " << lua_tostring(L, -1) << std::endl;
		lua_close(L);
		return false;
	}

	// parse config
	if (!loaded) { // info that must be loaded one time (unless we reset the modules involved)
		booleans[ConfigKeysBoolean::BIND_ONLY_GLOBAL_ADDRESS] = getGlobalBoolean(L, "bindOnlyGlobalAddress", false);
		booleans[ConfigKeysBoolean::OPTIMIZE_DATABASE] = getGlobalBoolean(L, "startupDatabaseOptimization", true);

		if (strings[ConfigKeysString::IP] == "") {
			strings[ConfigKeysString::IP] = getGlobalString(L, "ip", "127.0.0.1");
		}

		strings[ConfigKeysString::MAP_NAME] = getGlobalString(L, "mapName", "forgotten");
		strings[ConfigKeysString::MAP_AUTHOR] = getGlobalString(L, "mapAuthor", "Unknown");
		strings[ConfigKeysString::HOUSE_RENT_PERIOD] = getGlobalString(L, "houseRentPeriod", "never");

		strings[ConfigKeysString::MYSQL_HOST] = getGlobalString(L, "mysqlHost", getEnv("MYSQL_HOST", "127.0.0.1"));
		strings[ConfigKeysString::MYSQL_USER] =
		    getGlobalString(L, "mysqlUser", getEnv("MYSQL_USER", "forgottenserver"));
		strings[ConfigKeysString::MYSQL_PASS] = getGlobalString(L, "mysqlPass", getEnv("MYSQL_PASSWORD", ""));
		strings[ConfigKeysString::MYSQL_DB] =
		    getGlobalString(L, "mysqlDatabase", getEnv("MYSQL_DATABASE", "forgottenserver"));
		strings[ConfigKeysString::MYSQL_SOCK] = getGlobalString(L, "mysqlSock", getEnv("MYSQL_SOCK", ""));

		integers[ConfigKeysInteger::SQL_PORT] = getGlobalInteger(L, "mysqlPort", getEnv<uint16_t>("MYSQL_PORT", 3306));

		if (integers[ConfigKeysInteger::GAME_PORT] == 0) {
			integers[ConfigKeysInteger::GAME_PORT] = getGlobalInteger(L, "gameProtocolPort", 7172);
		}

		if (integers[ConfigKeysInteger::LOGIN_PORT] == 0) {
			integers[ConfigKeysInteger::LOGIN_PORT] = getGlobalInteger(L, "loginProtocolPort", 7171);
		}

		integers[ConfigKeysInteger::STATUS_PORT] = getGlobalInteger(L, "statusProtocolPort", 7171);

		integers[ConfigKeysInteger::MARKET_OFFER_DURATION] =
		    getGlobalInteger(L, "marketOfferDuration", 30 * 24 * 60 * 60);
	}

	booleans[ConfigKeysBoolean::ALLOW_CHANGEOUTFIT] = getGlobalBoolean(L, "allowChangeOutfit", true);
	booleans[ConfigKeysBoolean::ONE_PLAYER_ON_ACCOUNT] = getGlobalBoolean(L, "onePlayerOnlinePerAccount", true);
	booleans[ConfigKeysBoolean::AIMBOT_HOTKEY_ENABLED] = getGlobalBoolean(L, "hotkeyAimbotEnabled", true);
	booleans[ConfigKeysBoolean::REMOVE_RUNE_CHARGES] = getGlobalBoolean(L, "removeChargesFromRunes", true);
	booleans[ConfigKeysBoolean::REMOVE_WEAPON_AMMO] = getGlobalBoolean(L, "removeWeaponAmmunition", true);
	booleans[ConfigKeysBoolean::REMOVE_WEAPON_CHARGES] = getGlobalBoolean(L, "removeWeaponCharges", true);
	booleans[ConfigKeysBoolean::REMOVE_POTION_CHARGES] = getGlobalBoolean(L, "removeChargesFromPotions", true);
	booleans[ConfigKeysBoolean::EXPERIENCE_FROM_PLAYERS] = getGlobalBoolean(L, "experienceByKillingPlayers", false);
	booleans[ConfigKeysBoolean::FREE_PREMIUM] = getGlobalBoolean(L, "freePremium", false);
	booleans[ConfigKeysBoolean::REPLACE_KICK_ON_LOGIN] = getGlobalBoolean(L, "replaceKickOnLogin", true);
	booleans[ConfigKeysBoolean::ALLOW_CLONES] = getGlobalBoolean(L, "allowClones", false);
	booleans[ConfigKeysBoolean::ALLOW_WALKTHROUGH] = getGlobalBoolean(L, "allowWalkthrough", true);
	booleans[ConfigKeysBoolean::MARKET_PREMIUM] = getGlobalBoolean(L, "premiumToCreateMarketOffer", true);
	booleans[ConfigKeysBoolean::EMOTE_SPELLS] = getGlobalBoolean(L, "emoteSpells", false);
	booleans[ConfigKeysBoolean::STAMINA_SYSTEM] = getGlobalBoolean(L, "staminaSystem", true);
	booleans[ConfigKeysBoolean::WARN_UNSAFE_SCRIPTS] = getGlobalBoolean(L, "warnUnsafeScripts", true);
	booleans[ConfigKeysBoolean::CONVERT_UNSAFE_SCRIPTS] = getGlobalBoolean(L, "convertUnsafeScripts", true);
	booleans[ConfigKeysBoolean::CLASSIC_EQUIPMENT_SLOTS] = getGlobalBoolean(L, "classicEquipmentSlots", false);
	booleans[ConfigKeysBoolean::CLASSIC_ATTACK_SPEED] = getGlobalBoolean(L, "classicAttackSpeed", false);
	booleans[ConfigKeysBoolean::SCRIPTS_CONSOLE_LOGS] = getGlobalBoolean(L, "showScriptsLogInConsole", true);
	booleans[ConfigKeysBoolean::SERVER_SAVE_NOTIFY_MESSAGE] = getGlobalBoolean(L, "serverSaveNotifyMessage", true);
	booleans[ConfigKeysBoolean::SERVER_SAVE_CLEAN_MAP] = getGlobalBoolean(L, "serverSaveCleanMap", false);
	booleans[ConfigKeysBoolean::SERVER_SAVE_CLOSE] = getGlobalBoolean(L, "serverSaveClose", false);
	booleans[ConfigKeysBoolean::SERVER_SAVE_SHUTDOWN] = getGlobalBoolean(L, "serverSaveShutdown", true);
	booleans[ConfigKeysBoolean::ONLINE_OFFLINE_CHARLIST] = getGlobalBoolean(L, "showOnlineStatusInCharlist", false);
	booleans[ConfigKeysBoolean::YELL_ALLOW_PREMIUM] = getGlobalBoolean(L, "yellAlwaysAllowPremium", false);
	booleans[ConfigKeysBoolean::PREMIUM_TO_SEND_PRIVATE] = getGlobalBoolean(L, "premiumToSendPrivate", false);
	booleans[ConfigKeysBoolean::FORCE_MONSTERTYPE_LOAD] = getGlobalBoolean(L, "forceMonsterTypesOnLoad", true);
	booleans[ConfigKeysBoolean::DEFAULT_WORLD_LIGHT] = getGlobalBoolean(L, "defaultWorldLight", true);
	booleans[ConfigKeysBoolean::HOUSE_OWNED_BY_ACCOUNT] = getGlobalBoolean(L, "houseOwnedByAccount", false);
	booleans[ConfigKeysBoolean::CLEAN_PROTECTION_ZONES] = getGlobalBoolean(L, "cleanProtectionZones", false);
	booleans[ConfigKeysBoolean::HOUSE_DOOR_SHOW_PRICE] = getGlobalBoolean(L, "houseDoorShowPrice", true);
	booleans[ConfigKeysBoolean::ONLY_INVITED_CAN_MOVE_HOUSE_ITEMS] =
	    getGlobalBoolean(L, "onlyInvitedCanMoveHouseItems", true);
	booleans[ConfigKeysBoolean::REMOVE_ON_DESPAWN] = getGlobalBoolean(L, "removeOnDespawn", true);
	booleans[ConfigKeysBoolean::MONSTER_OVERSPAWN] = getGlobalBoolean(L, "monsterOverspawn", false);
	booleans[ConfigKeysBoolean::ACCOUNT_MANAGER] = getGlobalBoolean(L, "accountManager", true);

	strings[ConfigKeysString::DEFAULT_PRIORITY] = getGlobalString(L, "defaultPriority", "high");
	strings[ConfigKeysString::SERVER_NAME] = getGlobalString(L, "serverName", "");
	strings[ConfigKeysString::OWNER_NAME] = getGlobalString(L, "ownerName", "");
	strings[ConfigKeysString::OWNER_EMAIL] = getGlobalString(L, "ownerEmail", "");
	strings[ConfigKeysString::URL] = getGlobalString(L, "url", "");
	strings[ConfigKeysString::LOCATION] = getGlobalString(L, "location", "");
	strings[ConfigKeysString::MOTD] = getGlobalString(L, "motd", "");
	strings[ConfigKeysString::WORLD_TYPE] = getGlobalString(L, "worldType", "pvp");

	Monster::despawnRange = getGlobalInteger(L, "deSpawnRange", 2);
	Monster::despawnRadius = getGlobalInteger(L, "deSpawnRadius", 50);

	integers[ConfigKeysInteger::MAX_PLAYERS] = getGlobalInteger(L, "maxPlayers");
	integers[ConfigKeysInteger::PZ_LOCKED] = getGlobalInteger(L, "pzLocked", 60000);
	integers[ConfigKeysInteger::DEFAULT_DESPAWNRANGE] = Monster::despawnRange;
	integers[ConfigKeysInteger::DEFAULT_DESPAWNRADIUS] = Monster::despawnRadius;
	integers[ConfigKeysInteger::DEFAULT_WALKTOSPAWNRADIUS] = getGlobalInteger(L, "walkToSpawnRadius", 15);
	integers[ConfigKeysInteger::RATE_EXPERIENCE] = getGlobalInteger(L, "rateExp", 5);
	integers[ConfigKeysInteger::RATE_SKILL] = getGlobalInteger(L, "rateSkill", 3);
	integers[ConfigKeysInteger::RATE_LOOT] = getGlobalInteger(L, "rateLoot", 2);
	integers[ConfigKeysInteger::RATE_MAGIC] = getGlobalInteger(L, "rateMagic", 3);
	integers[ConfigKeysInteger::RATE_SPAWN] = getGlobalInteger(L, "rateSpawn", 1);
	integers[ConfigKeysInteger::HOUSE_PRICE] = getGlobalInteger(L, "housePriceEachSQM", 1000);
	integers[ConfigKeysInteger::KILLS_TO_RED] = getGlobalInteger(L, "killsToRedSkull", 3);
	integers[ConfigKeysInteger::KILLS_TO_BLACK] = getGlobalInteger(L, "killsToBlackSkull", 6);
	integers[ConfigKeysInteger::ACTIONS_DELAY_INTERVAL] = getGlobalInteger(L, "timeBetweenActions", 200);
	integers[ConfigKeysInteger::EX_ACTIONS_DELAY_INTERVAL] = getGlobalInteger(L, "timeBetweenExActions", 1000);
	integers[ConfigKeysInteger::MAX_MESSAGEBUFFER] = getGlobalInteger(L, "maxMessageBuffer", 4);
	integers[ConfigKeysInteger::KICK_AFTER_MINUTES] = getGlobalInteger(L, "kickIdlePlayerAfterMinutes", 15);
	integers[ConfigKeysInteger::PROTECTION_LEVEL] = getGlobalInteger(L, "protectionLevel", 1);
	integers[ConfigKeysInteger::DEATH_LOSE_PERCENT] = getGlobalInteger(L, "deathLosePercent", -1);
	integers[ConfigKeysInteger::STATUSQUERY_TIMEOUT] = getGlobalInteger(L, "statusTimeout", 5000);
	integers[ConfigKeysInteger::FRAG_TIME] = getGlobalInteger(L, "timeToDecreaseFrags", 24 * 60 * 60);
	integers[ConfigKeysInteger::WHITE_SKULL_TIME] = getGlobalInteger(L, "whiteSkullTime", 15 * 60);
	integers[ConfigKeysInteger::STAIRHOP_DELAY] = getGlobalInteger(L, "stairJumpExhaustion", 2000);
	integers[ConfigKeysInteger::EXP_FROM_PLAYERS_LEVEL_RANGE] = getGlobalInteger(L, "expFromPlayersLevelRange", 75);
	integers[ConfigKeysInteger::MAX_PACKETS_PER_SECOND] = getGlobalInteger(L, "maxPacketsPerSecond", 25);
	integers[ConfigKeysInteger::SERVER_SAVE_NOTIFY_DURATION] = getGlobalInteger(L, "serverSaveNotifyDuration", 5);
	integers[ConfigKeysInteger::YELL_MINIMUM_LEVEL] = getGlobalInteger(L, "yellMinimumLevel", 2);
	integers[ConfigKeysInteger::MINIMUM_LEVEL_TO_SEND_PRIVATE] = getGlobalInteger(L, "minimumLevelToSendPrivate", 1);
	integers[ConfigKeysInteger::VIP_FREE_LIMIT] = getGlobalInteger(L, "vipFreeLimit", 20);
	integers[ConfigKeysInteger::VIP_PREMIUM_LIMIT] = getGlobalInteger(L, "vipPremiumLimit", 100);
	integers[ConfigKeysInteger::DEPOT_FREE_LIMIT] = getGlobalInteger(L, "depotFreeLimit", 2000);
	integers[ConfigKeysInteger::DEPOT_PREMIUM_LIMIT] = getGlobalInteger(L, "depotPremiumLimit", 15000);
	integers[ConfigKeysInteger::STAMINA_REGEN_MINUTE] = getGlobalInteger(L, "timeToRegenMinuteStamina", 3 * 60);
	integers[ConfigKeysInteger::STAMINA_REGEN_PREMIUM] = getGlobalInteger(L, "timeToRegenMinutePremiumStamina", 6 * 60);
	integers[ConfigKeysInteger::HEALTH_GAIN_COLOUR] = getGlobalInteger(L, "healthGainColour", TEXTCOLOR_MAYABLUE);
	integers[ConfigKeysInteger::MANA_GAIN_COLOUR] = getGlobalInteger(L, "manaGainColour", TEXTCOLOR_BLUE);
	integers[ConfigKeysInteger::MANA_LOSS_COLOUR] = getGlobalInteger(L, "manaLossColour", TEXTCOLOR_BLUE);

	expStages = loadXMLStages();
	if (expStages.empty()) {
		expStages = loadLuaStages(L);
	} else {
		std::cout << "[Warning - ConfigManager::load] XML stages are deprecated, "
		             "consider moving to config.lua."
		          << std::endl;
	}
	expStages.shrink_to_fit();

	loaded = true;
	lua_close(L);

	return true;
}

bool ConfigManager::getBoolean(ConfigKeysBoolean what) const
{
	if (what >= ConfigKeysBoolean::LAST) {
		std::cout << "[Warning - ConfigManager::getBoolean] Accessing invalid index: " << static_cast<int>(what)
		          << std::endl;
		return false;
	}
	return booleans[what];
}

std::string_view ConfigManager::getString(ConfigKeysString what) const
{
	if (what >= ConfigKeysString::LAST) {
		std::cout << "[Warning - ConfigManager::getString] Accessing invalid index: " << static_cast<int>(what)
		          << std::endl;
		return "";
	}
	return strings[what];
}

int64_t ConfigManager::getInteger(ConfigKeysInteger what) const
{
	if (what >= ConfigKeysInteger::LAST) {
		std::cout << "[Warning - ConfigManager::getInteger] Accessing invalid index: " << static_cast<int>(what)
		          << std::endl;
		return 0;
	}
	return integers[what];
}

float ConfigManager::getExperienceStage(const uint32_t level) const
{
	auto it = std::find_if(expStages.begin(), expStages.end(), [level](auto&& stage) {
		auto&& [minLevel, maxLevel, _] = stage;
		return level >= minLevel && level <= maxLevel;
	});

	if (it == expStages.end()) {
		return getInteger(ConfigKeysInteger::RATE_EXPERIENCE);
	}

	return std::get<2>(*it);
}

bool ConfigManager::setBoolean(ConfigKeysBoolean what, const bool value)
{
	if (what >= ConfigKeysBoolean::LAST) {
		std::cout << "[Warning - ConfigManager::setBoolean] Accessing invalid index: " << static_cast<int>(what)
		          << std::endl;
		return false;
	}

	booleans[what] = value;
	return true;
}

bool ConfigManager::setString(ConfigKeysString what, std::string_view value)
{
	if (what >= ConfigKeysString::LAST) {
		std::cout << "[Warning - ConfigManager::setString] Accessing invalid index: " << static_cast<int>(what)
		          << std::endl;
		return false;
	}

	strings[what] = value;
	return true;
}

bool ConfigManager::setInteger(ConfigKeysInteger what, const int64_t value)
{
	if (what >= ConfigKeysInteger::LAST) {
		std::cout << "[Warning - ConfigManager::setInteger] Accessing invalid index: " << static_cast<int>(what)
		          << std::endl;
		return false;
	}

	integers[what] = value;
	return true;
}
