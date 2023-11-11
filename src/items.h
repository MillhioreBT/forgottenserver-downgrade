// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#ifndef FS_ITEMS_H
#define FS_ITEMS_H

#include "const.h"
#include "enums.h"
#include "itemloader.h"
#include "position.h"

#include <sol/sol.hpp>

enum SlotPositionBits : uint32_t
{
	SLOTP_WHEREEVER = 0xFFFFFFFF,
	SLOTP_HEAD = 1 << 0,
	SLOTP_NECKLACE = 1 << 1,
	SLOTP_BACKPACK = 1 << 2,
	SLOTP_ARMOR = 1 << 3,
	SLOTP_RIGHT = 1 << 4,
	SLOTP_LEFT = 1 << 5,
	SLOTP_LEGS = 1 << 6,
	SLOTP_FEET = 1 << 7,
	SLOTP_RING = 1 << 8,
	SLOTP_AMMO = 1 << 9,
	SLOTP_DEPOT = 1 << 10,
	SLOTP_TWO_HAND = 1 << 11,
	SLOTP_HAND = (SLOTP_LEFT | SLOTP_RIGHT)
};

enum ItemTypes_t
{
	ITEM_TYPE_NONE,
	ITEM_TYPE_DEPOT,
	ITEM_TYPE_MAILBOX,
	ITEM_TYPE_TRASHHOLDER,
	ITEM_TYPE_CONTAINER,
	ITEM_TYPE_DOOR,
	ITEM_TYPE_MAGICFIELD,
	ITEM_TYPE_TELEPORT,
	ITEM_TYPE_BED,
	ITEM_TYPE_KEY,
	ITEM_TYPE_RUNE,
	ITEM_TYPE_LAST
};

struct Abilities
{
	uint32_t healthGain = 0;
	uint32_t healthTicks = 0;
	uint32_t manaGain = 0;
	uint32_t manaTicks = 0;

	uint32_t conditionImmunities = 0;
	uint32_t conditionSuppressions = 0;

	// stats modifiers
	std::array<int32_t, STAT_LAST + 1> stats = {0};
	std::array<int32_t, STAT_LAST + 1> statsPercent = {0};

	// extra skill modifiers
	std::array<int32_t, SKILL_LAST + 1> skills = {0};
	std::array<int32_t, SPECIALSKILL_LAST + 1> specialSkills = {0};

	int32_t speed = 0;

	// field damage abilities modifiers
	std::array<int16_t, COMBAT_COUNT> fieldAbsorbPercent = {0};

	// damage abilities modifiers
	std::array<int16_t, COMBAT_COUNT> absorbPercent = {0};

	// elemental damage
	uint16_t elementDamage = 0;
	CombatType_t elementType = COMBAT_NONE;

	bool manaShield = false;
	bool invisible = false;
	bool regeneration = false;
};

class ConditionDamage;

class ItemType
{
public:
	ItemType() = default;

	// non-copyable
	ItemType(const ItemType& other) = delete;
	ItemType& operator=(const ItemType& other) = delete;

	ItemType(ItemType&& other) = default;
	ItemType& operator=(ItemType&& other) = default;

	bool isGroundTile() const { return group == ITEM_GROUP_GROUND; }
	bool isContainer() const { return group == ITEM_GROUP_CONTAINER; }
	bool isSplash() const { return group == ITEM_GROUP_SPLASH; }
	bool isFluidContainer() const { return group == ITEM_GROUP_FLUID; }

	bool isDoor() const { return (type == ITEM_TYPE_DOOR); }
	bool isMagicField() const { return (type == ITEM_TYPE_MAGICFIELD); }
	bool isTeleport() const { return (type == ITEM_TYPE_TELEPORT); }
	bool isKey() const { return (type == ITEM_TYPE_KEY); }
	bool isDepot() const { return (type == ITEM_TYPE_DEPOT); }
	bool isMailbox() const { return (type == ITEM_TYPE_MAILBOX); }
	bool isTrashHolder() const { return (type == ITEM_TYPE_TRASHHOLDER); }
	bool isBed() const { return (type == ITEM_TYPE_BED); }
	bool isRune() const { return (type == ITEM_TYPE_RUNE); }
	bool isPickupable() const { return pickupable; }
	bool isUseable() const { return (useable); }
	bool hasSubType() const { return (isFluidContainer() || isSplash() || stackable || charges != 0); }

	Abilities& getAbilities()
	{
		if (!abilities) {
			abilities.reset(new Abilities());
		}
		return *abilities;
	}

	std::string_view getPluralName() const
	{
		if (showCount == 0) {
			return name;
		}

		return pluralName;
	}

	itemgroup_t group = ITEM_GROUP_NONE;
	ItemTypes_t type = ITEM_TYPE_NONE;
	uint16_t id = 0;
	uint16_t clientId = 0;
	bool stackable = false;

	std::string name;
	std::string article;
	std::string pluralName;
	std::string description;
	std::string runeSpellName;
	std::string vocationString;

	std::unique_ptr<Abilities> abilities;
	std::unique_ptr<ConditionDamage> conditionDamage;

	uint32_t attackSpeed = 0;
	uint32_t weight = 0;
	uint32_t levelDoor = 0;
	uint32_t decayTimeMin = 0;
	uint32_t decayTimeMax = 0;
	uint32_t wieldInfo = 0;
	uint32_t minReqLevel = 0;
	uint32_t minReqMagicLevel = 0;
	uint32_t charges = 0;
	int32_t maxHitChance = -1;
	int32_t decayTo = -1;
	int32_t attack = 0;
	int32_t defense = 0;
	int32_t extraDefense = 0;
	int32_t armor = 0;
	uint16_t rotateTo = 0;
	int32_t runeMagLevel = 0;
	int32_t runeLevel = 0;
	uint64_t worth = 0;

	CombatType_t combatType = COMBAT_NONE;

	uint16_t transformToOnUse[2] = {0, 0};
	uint16_t transformToFree = 0;
	uint16_t destroyTo = 0;
	uint16_t maxTextLen = 0;
	uint16_t writeOnceItemId = 0;
	uint16_t transformEquipTo = 0;
	uint16_t transformDeEquipTo = 0;
	uint16_t maxItems = 8;
	uint16_t slotPosition = SLOTP_HAND;
	uint16_t speed = 0;
	uint16_t wareId = 0;

	MagicEffectClasses magicEffect = CONST_ME_NONE;
	Direction bedPartnerDir = DIRECTION_NONE;
	WeaponType_t weaponType = WEAPON_NONE;
	Ammo_t ammoType = AMMO_NONE;
	ShootType_t shootType = CONST_ANI_NONE;
	RaceType_t corpseType = RACE_NONE;
	FluidTypes_t fluidSource = FLUID_NONE;

	uint8_t stackSize = 100;
	uint8_t floorChange = 0;
	uint8_t alwaysOnTopOrder = 0;
	uint8_t lightLevel = 0;
	uint8_t lightColor = 0;
	uint8_t shootRange = 1;
	int8_t hitChance = 0;

	bool storeItem = false;
	bool forceUse = false;
	bool forceSerialize = false;
	bool hasHeight = false;
	bool walkStack = true;
	bool blockSolid = false;
	bool blockPickupable = false;
	bool blockProjectile = false;
	bool blockPathFind = false;
	bool ignoreBlocking = false;
	bool showDuration = false;
	bool showCharges = false;
	bool showAttributes = false;
	bool replaceable = true;
	bool pickupable = false;
	bool rotatable = false;
	bool useable = false;
	bool moveable = false;
	bool alwaysOnTop = false;
	bool canReadText = false;
	bool canWriteText = false;
	bool isVertical = false;
	bool isHorizontal = false;
	bool isHangable = false;
	bool allowDistRead = false;
	bool lookThrough = false;
	bool stopTime = false;
	bool showCount = true;
};

class Items
{
public:
	using NameMap = std::unordered_map<std::string, uint16_t>;
	using InventoryVector = std::vector<uint16_t>;

	using CurrencyMap = std::map<uint64_t, uint16_t, std::greater<uint64_t>>;

	Items();

	// non-copyable
	Items(const Items&) = delete;
	Items& operator=(const Items&) = delete;

	bool reload();
	void clear();

	bool loadFromOtb(const std::string& file);

	const ItemType& operator[](size_t id) const { return getItemType(id); }
	const ItemType& getItemType(size_t id) const;
	ItemType& getItemType(size_t id);
	const ItemType& getItemIdByClientId(uint16_t spriteId) const;

	uint16_t getItemIdByName(const std::string& name);

	uint32_t majorVersion = 0;
	uint32_t minorVersion = 0;
	uint32_t buildNumber = 0;

	void loadFromLua(lua_State* L);
	void parseLuaItem(const sol::table& itemTable, uint16_t id);

	void buildInventoryList();
	const InventoryVector& getInventory() const { return inventory; }

	size_t size() const { return items.size(); }

	NameMap nameToItems;
	CurrencyMap currencyItems;

private:
	std::vector<ItemType> items;
	InventoryVector inventory;
	class ClientIdToServerIdMap
	{
	public:
		ClientIdToServerIdMap() { vec.reserve(30000); }

		void emplace(uint16_t clientId, uint16_t serverId)
		{
			if (clientId >= vec.size()) {
				vec.resize(clientId + 1, 0);
			}
			if (vec[clientId] == 0) {
				vec[clientId] = serverId;
			}
		}

		uint16_t getServerId(uint16_t clientId) const
		{
			uint16_t serverId = 0;
			if (clientId < vec.size()) {
				serverId = vec[clientId];
			}
			return serverId;
		}

		void clear() { vec.clear(); }

	private:
		std::vector<uint16_t> vec;
	} clientIdToServerIdMap;
};
#endif
