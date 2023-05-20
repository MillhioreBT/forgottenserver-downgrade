// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#ifndef FS_CONFIGMANAGER_H
#define FS_CONFIGMANAGER_H

using ExperienceStages = std::vector<std::tuple<uint32_t, uint32_t, float>>;

enum class ConfigKeysBoolean
{
	ALLOW_CHANGEOUTFIT,
	ONE_PLAYER_ON_ACCOUNT,
	AIMBOT_HOTKEY_ENABLED,
	REMOVE_RUNE_CHARGES,
	REMOVE_WEAPON_AMMO,
	REMOVE_WEAPON_CHARGES,
	REMOVE_POTION_CHARGES,
	EXPERIENCE_FROM_PLAYERS,
	FREE_PREMIUM,
	REPLACE_KICK_ON_LOGIN,
	ALLOW_CLONES,
	ALLOW_WALKTHROUGH,
	BIND_ONLY_GLOBAL_ADDRESS,
	OPTIMIZE_DATABASE,
	MARKET_PREMIUM,
	EMOTE_SPELLS,
	STAMINA_SYSTEM,
	WARN_UNSAFE_SCRIPTS,
	CONVERT_UNSAFE_SCRIPTS,
	CLASSIC_EQUIPMENT_SLOTS,
	CLASSIC_ATTACK_SPEED,
	SCRIPTS_CONSOLE_LOGS,
	SERVER_SAVE_NOTIFY_MESSAGE,
	SERVER_SAVE_CLEAN_MAP,
	SERVER_SAVE_CLOSE,
	SERVER_SAVE_SHUTDOWN,
	ONLINE_OFFLINE_CHARLIST,
	YELL_ALLOW_PREMIUM,
	PREMIUM_TO_SEND_PRIVATE,
	FORCE_MONSTERTYPE_LOAD,
	DEFAULT_WORLD_LIGHT,
	HOUSE_OWNED_BY_ACCOUNT,
	CLEAN_PROTECTION_ZONES,
	HOUSE_DOOR_SHOW_PRICE,
	ONLY_INVITED_CAN_MOVE_HOUSE_ITEMS,
	REMOVE_ON_DESPAWN,
	MONSTER_OVERSPAWN,
	ACCOUNT_MANAGER,

	LAST /* this must be the last one */
};

enum class ConfigKeysString
{
	MAP_NAME,
	HOUSE_RENT_PERIOD,
	SERVER_NAME,
	OWNER_NAME,
	OWNER_EMAIL,
	URL,
	LOCATION,
	IP,
	MOTD,
	WORLD_TYPE,
	MYSQL_HOST,
	MYSQL_USER,
	MYSQL_PASS,
	MYSQL_DB,
	MYSQL_SOCK,
	DEFAULT_PRIORITY,
	MAP_AUTHOR,
	CONFIG_FILE,

	LAST /* this must be the last one */
};

enum class ConfigKeysInteger
{
	SQL_PORT,
	MAX_PLAYERS,
	PZ_LOCKED,
	DEFAULT_DESPAWNRANGE,
	DEFAULT_DESPAWNRADIUS,
	DEFAULT_WALKTOSPAWNRADIUS,
	RATE_EXPERIENCE,
	RATE_SKILL,
	RATE_LOOT,
	RATE_MAGIC,
	RATE_SPAWN,
	HOUSE_PRICE,
	KILLS_TO_RED,
	KILLS_TO_BLACK,
	MAX_MESSAGEBUFFER,
	ACTIONS_DELAY_INTERVAL,
	EX_ACTIONS_DELAY_INTERVAL,
	KICK_AFTER_MINUTES,
	PROTECTION_LEVEL,
	DEATH_LOSE_PERCENT,
	STATUSQUERY_TIMEOUT,
	FRAG_TIME,
	WHITE_SKULL_TIME,
	GAME_PORT,
	LOGIN_PORT,
	STATUS_PORT,
	STAIRHOP_DELAY,
	MARKET_OFFER_DURATION,
	EXP_FROM_PLAYERS_LEVEL_RANGE,
	MAX_PACKETS_PER_SECOND,
	SERVER_SAVE_NOTIFY_DURATION,
	YELL_MINIMUM_LEVEL,
	MINIMUM_LEVEL_TO_SEND_PRIVATE,
	VIP_FREE_LIMIT,
	VIP_PREMIUM_LIMIT,
	DEPOT_FREE_LIMIT,
	DEPOT_PREMIUM_LIMIT,
	STAMINA_REGEN_MINUTE,
	STAMINA_REGEN_PREMIUM,
	HEALTH_GAIN_COLOUR,
	MANA_GAIN_COLOUR,
	MANA_LOSS_COLOUR,

	LAST /* this must be the last one */
};

template <typename T, typename E>
struct ConfigValues
{
	using Container = std::array<T, static_cast<int>(E::LAST)>;

public:
	T& operator[](E key) { return values[static_cast<int>(key)]; }
	const T& operator[](E key) const { return values[static_cast<int>(key)]; }

private:
	Container values;
};

class ConfigManager
{
public:
	ConfigManager();

	bool load();

	bool getBoolean(ConfigKeysBoolean what) const;
	std::string_view getString(ConfigKeysString what) const;
	int64_t getInteger(ConfigKeysInteger what) const;
	float getExperienceStage(const uint32_t level) const;

	bool setBoolean(ConfigKeysBoolean what, const bool value);
	bool setString(ConfigKeysString what, std::string_view value);
	bool setInteger(ConfigKeysInteger what, const int64_t value);

	auto operator[](ConfigKeysBoolean what) const { return getBoolean(what); }
	auto operator[](ConfigKeysString what) const { return getString(what); }
	auto operator[](ConfigKeysInteger what) const { return getInteger(what); }

private:
	ConfigValues<bool, ConfigKeysBoolean> booleans;
	ConfigValues<std::string, ConfigKeysString> strings;
	ConfigValues<int64_t, ConfigKeysInteger> integers;

	ExperienceStages expStages = {};

	bool loaded = false;
};

#endif // FS_CONFIGMANAGER_H
