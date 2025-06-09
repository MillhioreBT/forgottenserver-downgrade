// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found
// in the LICENSE file.

#include "otpch.h"

#include "configmanager.h"

#include "game.h"
#include "lua.hpp"
#include "monster.h"
#include "pugicast.h"

#if LUA_VERSION_NUM >= 502
#undef lua_strlen
#define lua_strlen lua_rawlen
#endif

extern Game g_game;

namespace {

std::array<std::string, ConfigManager::String::LAST_STRING> strings = {};
std::array<int64_t, ConfigManager::Integer::LAST_INTEGER> integers = {};
std::array<bool, ConfigManager::Boolean::LAST_BOOLEAN> booleans = {};

using ExperienceStages = std::vector<std::tuple<uint32_t, uint32_t, float>>;
ExperienceStages expStages;

using OTCFeatures = std::vector<uint8_t>;
OTCFeatures otcFeatures;

bool loaded = false;

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
		auto maxLevel = Lua::getField<uint32_t>(L, tableIndex, "maxlevel", std::numeric_limits<uint32_t>::max());
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

OTCFeatures loadLuaOTCFeatures(lua_State* L)
{
	OTCFeatures features;

	lua_getglobal(L, "OTCFeatures");
	if (!lua_istable(L, -1)) {
		return {};
	}

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		const auto feature = static_cast<uint8_t>(lua_tointeger(L, -1));
		features.push_back(feature);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return features;
}

} // namespace

bool ConfigManager::load()
{
	lua_State* L = luaL_newstate();
	if (!L) {
		throw std::runtime_error("Failed to allocate memory");
	}

	luaL_openlibs(L);

	strings[CONFIG_FILE] = "config.lua";
	if (luaL_dofile(L, getString(String::CONFIG_FILE).data())) {
		std::cout << "[Error - ConfigManager::load] " << lua_tostring(L, -1) << std::endl;
		lua_close(L);
		return false;
	}

	// parse config
	if (!loaded) { // info that must be loaded one time (unless we reset the modules involved)
		booleans[Boolean::BIND_ONLY_GLOBAL_ADDRESS] = getGlobalBoolean(L, "bindOnlyGlobalAddress", false);
		booleans[Boolean::OPTIMIZE_DATABASE] = getGlobalBoolean(L, "startupDatabaseOptimization", true);

		if (strings[String::IP] == "") {
			strings[String::IP] = getGlobalString(L, "ip", "127.0.0.1");
		}

		strings[String::MAP_NAME] = getGlobalString(L, "mapName", "forgotten");
		strings[String::MAP_AUTHOR] = getGlobalString(L, "mapAuthor", "Unknown");
		strings[String::HOUSE_RENT_PERIOD] = getGlobalString(L, "houseRentPeriod", "never");

		strings[String::MYSQL_HOST] = getGlobalString(L, "mysqlHost", getEnv("MYSQL_HOST", "127.0.0.1"));
		strings[String::MYSQL_USER] = getGlobalString(L, "mysqlUser", getEnv("MYSQL_USER", "forgottenserver"));
		strings[String::MYSQL_PASS] = getGlobalString(L, "mysqlPass", getEnv("MYSQL_PASSWORD", ""));
		strings[String::MYSQL_DB] = getGlobalString(L, "mysqlDatabase", getEnv("MYSQL_DATABASE", "forgottenserver"));
		strings[String::MYSQL_SOCK] = getGlobalString(L, "mysqlSock", getEnv("MYSQL_SOCK", ""));

		integers[Integer::SQL_PORT] = getGlobalInteger(L, "mysqlPort", getEnv<uint16_t>("MYSQL_PORT", 3306));

		if (integers[Integer::GAME_PORT] == 0) {
			integers[Integer::GAME_PORT] = getGlobalInteger(L, "gameProtocolPort", 7172);
		}

		if (integers[Integer::LOGIN_PORT] == 0) {
			integers[Integer::LOGIN_PORT] = getGlobalInteger(L, "loginProtocolPort", 7171);
		}

		integers[Integer::STATUS_PORT] = getGlobalInteger(L, "statusProtocolPort", 7171);

		integers[Integer::MARKET_OFFER_DURATION] = getGlobalInteger(L, "marketOfferDuration", 30 * 24 * 60 * 60);
	}

	booleans[Boolean::ALLOW_CHANGEOUTFIT] = getGlobalBoolean(L, "allowChangeOutfit", true);
	booleans[Boolean::ONE_PLAYER_ON_ACCOUNT] = getGlobalBoolean(L, "onePlayerOnlinePerAccount", true);
	booleans[Boolean::AIMBOT_HOTKEY_ENABLED] = getGlobalBoolean(L, "hotkeyAimbotEnabled", true);
	booleans[Boolean::REMOVE_RUNE_CHARGES] = getGlobalBoolean(L, "removeChargesFromRunes", true);
	booleans[Boolean::REMOVE_WEAPON_AMMO] = getGlobalBoolean(L, "removeWeaponAmmunition", true);
	booleans[Boolean::REMOVE_WEAPON_CHARGES] = getGlobalBoolean(L, "removeWeaponCharges", true);
	booleans[Boolean::REMOVE_POTION_CHARGES] = getGlobalBoolean(L, "removeChargesFromPotions", true);
	booleans[Boolean::EXPERIENCE_FROM_PLAYERS] = getGlobalBoolean(L, "experienceByKillingPlayers", false);
	booleans[Boolean::FREE_PREMIUM] = getGlobalBoolean(L, "freePremium", false);
	booleans[Boolean::REPLACE_KICK_ON_LOGIN] = getGlobalBoolean(L, "replaceKickOnLogin", true);
	booleans[Boolean::ALLOW_CLONES] = getGlobalBoolean(L, "allowClones", false);
	booleans[Boolean::ALLOW_WALKTHROUGH] = getGlobalBoolean(L, "allowWalkthrough", true);
	booleans[Boolean::MARKET_PREMIUM] = getGlobalBoolean(L, "premiumToCreateMarketOffer", true);
	booleans[Boolean::EMOTE_SPELLS] = getGlobalBoolean(L, "emoteSpells", false);
	booleans[Boolean::STAMINA_SYSTEM] = getGlobalBoolean(L, "staminaSystem", true);
	booleans[Boolean::WARN_UNSAFE_SCRIPTS] = getGlobalBoolean(L, "warnUnsafeScripts", true);
	booleans[Boolean::CONVERT_UNSAFE_SCRIPTS] = getGlobalBoolean(L, "convertUnsafeScripts", true);
	booleans[Boolean::CLASSIC_EQUIPMENT_SLOTS] = getGlobalBoolean(L, "classicEquipmentSlots", false);
	booleans[Boolean::CLASSIC_ATTACK_SPEED] = getGlobalBoolean(L, "classicAttackSpeed", false);
	booleans[Boolean::SCRIPTS_CONSOLE_LOGS] = getGlobalBoolean(L, "showScriptsLogInConsole", true);
	booleans[Boolean::SERVER_SAVE_NOTIFY_MESSAGE] = getGlobalBoolean(L, "serverSaveNotifyMessage", true);
	booleans[Boolean::SERVER_SAVE_CLEAN_MAP] = getGlobalBoolean(L, "serverSaveCleanMap", false);
	booleans[Boolean::SERVER_SAVE_CLOSE] = getGlobalBoolean(L, "serverSaveClose", false);
	booleans[Boolean::SERVER_SAVE_SHUTDOWN] = getGlobalBoolean(L, "serverSaveShutdown", true);
	booleans[Boolean::ONLINE_OFFLINE_CHARLIST] = getGlobalBoolean(L, "showOnlineStatusInCharlist", false);
	booleans[Boolean::YELL_ALLOW_PREMIUM] = getGlobalBoolean(L, "yellAlwaysAllowPremium", false);
	booleans[Boolean::PREMIUM_TO_SEND_PRIVATE] = getGlobalBoolean(L, "premiumToSendPrivate", false);
	booleans[Boolean::FORCE_MONSTERTYPE_LOAD] = getGlobalBoolean(L, "forceMonsterTypesOnLoad", true);
	booleans[Boolean::DEFAULT_WORLD_LIGHT] = getGlobalBoolean(L, "defaultWorldLight", true);
	booleans[Boolean::HOUSE_OWNED_BY_ACCOUNT] = getGlobalBoolean(L, "houseOwnedByAccount", false);
	booleans[Boolean::CLEAN_PROTECTION_ZONES] = getGlobalBoolean(L, "cleanProtectionZones", false);
	booleans[Boolean::HOUSE_DOOR_SHOW_PRICE] = getGlobalBoolean(L, "houseDoorShowPrice", true);
	booleans[Boolean::ONLY_INVITED_CAN_MOVE_HOUSE_ITEMS] = getGlobalBoolean(L, "onlyInvitedCanMoveHouseItems", true);
	booleans[Boolean::REMOVE_ON_DESPAWN] = getGlobalBoolean(L, "removeOnDespawn", true);
	booleans[Boolean::MONSTER_OVERSPAWN] = getGlobalBoolean(L, "monsterOverspawn", false);
	booleans[Boolean::ACCOUNT_MANAGER] = getGlobalBoolean(L, "accountManager", true);
	booleans[Boolean::MANASHIELD_BREAKABLE] = getGlobalBoolean(L, "useBreakableManaShield", false);

	strings[String::DEFAULT_PRIORITY] = getGlobalString(L, "defaultPriority", "high");
	strings[String::SERVER_NAME] = getGlobalString(L, "serverName", "");
	strings[String::OWNER_NAME] = getGlobalString(L, "ownerName", "");
	strings[String::OWNER_EMAIL] = getGlobalString(L, "ownerEmail", "");
	strings[String::URL] = getGlobalString(L, "url", "");
	strings[String::LOCATION] = getGlobalString(L, "location", "");
	strings[String::MOTD] = getGlobalString(L, "motd", "");
	strings[String::WORLD_TYPE] = getGlobalString(L, "worldType", "pvp");

	Monster::despawnRange = getGlobalInteger(L, "deSpawnRange", 2);
	Monster::despawnRadius = getGlobalInteger(L, "deSpawnRadius", 50);

	integers[Integer::MAX_PLAYERS] = getGlobalInteger(L, "maxPlayers");
	integers[Integer::PZ_LOCKED] = getGlobalInteger(L, "pzLocked", 60000);
	integers[Integer::DEFAULT_DESPAWNRANGE] = Monster::despawnRange;
	integers[Integer::DEFAULT_DESPAWNRADIUS] = Monster::despawnRadius;
	integers[Integer::DEFAULT_WALKTOSPAWNRADIUS] = getGlobalInteger(L, "walkToSpawnRadius", 15);
	integers[Integer::RATE_EXPERIENCE] = getGlobalInteger(L, "rateExp", 5);
	integers[Integer::RATE_SKILL] = getGlobalInteger(L, "rateSkill", 3);
	integers[Integer::RATE_LOOT] = getGlobalInteger(L, "rateLoot", 2);
	integers[Integer::RATE_MAGIC] = getGlobalInteger(L, "rateMagic", 3);
	integers[Integer::RATE_SPAWN] = getGlobalInteger(L, "rateSpawn", 1);
	integers[Integer::HOUSE_PRICE] = getGlobalInteger(L, "housePriceEachSQM", 1000);
	integers[Integer::KILLS_TO_RED] = getGlobalInteger(L, "killsToRedSkull", 3);
	integers[Integer::KILLS_TO_BLACK] = getGlobalInteger(L, "killsToBlackSkull", 6);
	integers[Integer::ACTIONS_DELAY_INTERVAL] = getGlobalInteger(L, "timeBetweenActions", 200);
	integers[Integer::EX_ACTIONS_DELAY_INTERVAL] = getGlobalInteger(L, "timeBetweenExActions", 1000);
	integers[Integer::MAX_MESSAGEBUFFER] = getGlobalInteger(L, "maxMessageBuffer", 4);
	integers[Integer::KICK_AFTER_MINUTES] = getGlobalInteger(L, "kickIdlePlayerAfterMinutes", 15);
	integers[Integer::PROTECTION_LEVEL] = getGlobalInteger(L, "protectionLevel", 1);
	integers[Integer::DEATH_LOSE_PERCENT] = getGlobalInteger(L, "deathLosePercent", -1);
	integers[Integer::STATUSQUERY_TIMEOUT] = getGlobalInteger(L, "statusTimeout", 5000);
	integers[Integer::FRAG_TIME] = getGlobalInteger(L, "timeToDecreaseFrags", 24 * 60 * 60);
	integers[Integer::WHITE_SKULL_TIME] = getGlobalInteger(L, "whiteSkullTime", 15 * 60);
	integers[Integer::STAIRHOP_DELAY] = getGlobalInteger(L, "stairJumpExhaustion", 2000);
	integers[Integer::EXP_FROM_PLAYERS_LEVEL_RANGE] = getGlobalInteger(L, "expFromPlayersLevelRange", 75);
	integers[Integer::MAX_PACKETS_PER_SECOND] = getGlobalInteger(L, "maxPacketsPerSecond", 25);
	integers[Integer::SERVER_SAVE_NOTIFY_DURATION] = getGlobalInteger(L, "serverSaveNotifyDuration", 5);
	integers[Integer::YELL_MINIMUM_LEVEL] = getGlobalInteger(L, "yellMinimumLevel", 2);
	integers[Integer::MINIMUM_LEVEL_TO_SEND_PRIVATE] = getGlobalInteger(L, "minimumLevelToSendPrivate", 1);
	integers[Integer::VIP_FREE_LIMIT] = getGlobalInteger(L, "vipFreeLimit", 20);
	integers[Integer::VIP_PREMIUM_LIMIT] = getGlobalInteger(L, "vipPremiumLimit", 100);
	integers[Integer::DEPOT_FREE_LIMIT] = getGlobalInteger(L, "depotFreeLimit", 2000);
	integers[Integer::DEPOT_PREMIUM_LIMIT] = getGlobalInteger(L, "depotPremiumLimit", 15000);
	integers[Integer::STAMINA_REGEN_MINUTE] = getGlobalInteger(L, "timeToRegenMinuteStamina", 3 * 60);
	integers[Integer::STAMINA_REGEN_PREMIUM] = getGlobalInteger(L, "timeToRegenMinutePremiumStamina", 6 * 60);
	integers[Integer::HEALTH_GAIN_COLOUR] = getGlobalInteger(L, "healthGainColour", TEXTCOLOR_MAYABLUE);
	integers[Integer::MANA_GAIN_COLOUR] = getGlobalInteger(L, "manaGainColour", TEXTCOLOR_BLUE);
	integers[Integer::MANA_LOSS_COLOUR] = getGlobalInteger(L, "manaLossColour", TEXTCOLOR_BLUE);
	integers[Integer::MAX_PROTOCOL_OUTFITS] = getGlobalInteger(L, "maxProtocolOutfits", 50);
	integers[Integer::MOVE_CREATURE_INTERVAL] = getGlobalInteger(L, "MOVE_CREATURE_INTERVAL", MOVE_CREATURE_INTERVAL);
	integers[Integer::RANGE_MOVE_CREATURE_INTERVAL] =
	    getGlobalInteger(L, "RANGE_MOVE_CREATURE_INTERVAL", RANGE_MOVE_CREATURE_INTERVAL);
	integers[Integer::RANGE_USE_WITH_CREATURE_INTERVAL] =
	    getGlobalInteger(L, "RANGE_USE_WITH_CREATURE_INTERVAL", RANGE_USE_WITH_CREATURE_INTERVAL);
	integers[Integer::RANGE_MOVE_ITEM_INTERVAL] =
	    getGlobalInteger(L, "RANGE_MOVE_ITEM_INTERVAL", RANGE_MOVE_ITEM_INTERVAL);
	integers[Integer::RANGE_USE_ITEM_INTERVAL] =
	    getGlobalInteger(L, "RANGE_USE_ITEM_INTERVAL", RANGE_USE_ITEM_INTERVAL);
	integers[Integer::RANGE_USE_ITEM_EX_INTERVAL] =
	    getGlobalInteger(L, "RANGE_USE_ITEM_EX_INTERVAL", RANGE_USE_ITEM_EX_INTERVAL);
	integers[Integer::RANGE_ROTATE_ITEM_INTERVAL] =
	    getGlobalInteger(L, "RANGE_ROTATE_ITEM_INTERVAL", RANGE_ROTATE_ITEM_INTERVAL);

	expStages = loadXMLStages();
	if (expStages.empty()) {
		expStages = loadLuaStages(L);
	} else {
		std::cout << "[Warning - ConfigManager::load] XML stages are deprecated, "
		             "consider moving to config.lua."
		          << std::endl;
	}
	expStages.shrink_to_fit();

	otcFeatures = loadLuaOTCFeatures(L);

	loaded = true;
	lua_close(L);

	return true;
}

bool ConfigManager::getBoolean(Boolean what)
{
	if (what >= Boolean::LAST_BOOLEAN) {
		std::cout << "[Warning - ConfigManager::getBoolean] Accessing invalid index: " << what << std::endl;
		return false;
	}
	return booleans[what];
}

std::string_view ConfigManager::getString(String what)
{
	if (what >= String::LAST_STRING) {
		std::cout << "[Warning - ConfigManager::getString] Accessing invalid index: " << what << std::endl;
		return "";
	}
	return strings[what];
}

int64_t ConfigManager::getInteger(Integer what)
{
	if (what >= Integer::LAST_INTEGER) {
		std::cout << "[Warning - ConfigManager::getInteger] Accessing invalid index: " << what << std::endl;
		return 0;
	}
	return integers[what];
}

float ConfigManager::getExperienceStage(uint32_t level)
{
	auto it = std::find_if(expStages.begin(), expStages.end(), [level](auto&& stage) {
		auto&& [minLevel, maxLevel, _] = stage;
		return level >= minLevel && level <= maxLevel;
	});

	if (it == expStages.end()) {
		return getInteger(Integer::RATE_EXPERIENCE);
	}

	return std::get<2>(*it);
}

bool ConfigManager::setBoolean(Boolean what, bool value)
{
	if (what >= Boolean::LAST_BOOLEAN) {
		std::cout << "[Warning - ConfigManager::setBoolean] Accessing invalid index: " << what << std::endl;
		return false;
	}

	booleans[what] = value;
	return true;
}

bool ConfigManager::setString(String what, std::string_view value)
{
	if (what >= String::LAST_STRING) {
		std::cout << "[Warning - ConfigManager::setString] Accessing invalid index: " << what << std::endl;
		return false;
	}

	strings[what] = value;
	return true;
}

bool ConfigManager::setInteger(Integer what, int64_t value)
{
	if (what >= Integer::LAST_INTEGER) {
		std::cout << "[Warning - ConfigManager::setInteger] Accessing invalid index: " << what << std::endl;
		return false;
	}

	integers[what] = value;
	return true;
}

const OTCFeatures& ConfigManager::getOTCFeatures() { return otcFeatures; }
