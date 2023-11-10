// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "items.h"

#include "movement.h"
#include "pugicast.h"
#include "script.h"
#include "spells.h"
#include "weapons.h"

extern MoveEvents* g_moveEvents;
extern Weapons* g_weapons;
extern Scripts* g_scripts;

Items::Items()
{
	items.reserve(30000);
	nameToItems.reserve(30000);
}

void Items::clear()
{
	items.clear();
	clientIdToServerIdMap.clear();
	nameToItems.clear();
	currencyItems.clear();
	inventory.clear();
}

bool Items::reload()
{
	clear();
	loadFromOtb("data/items/items.otb");

	g_scripts->loadScripts("items", false, true);
	g_moveEvents->reload();
	g_weapons->reload();
	g_weapons->loadDefaults();
	return true;
}

constexpr auto OTBI = OTB::Identifier{{'O', 'T', 'B', 'I'}};

bool Items::loadFromOtb(const std::string& file)
{
	OTB::Loader loader{file, OTBI};

	auto& root = loader.parseTree();

	PropStream props;
	if (loader.getProps(root, props)) {
		// 4 byte flags
		// attributes
		// 0x01 = version data
		uint32_t flags;
		if (!props.read<uint32_t>(flags)) {
			return false;
		}

		uint8_t attr;
		if (!props.read<uint8_t>(attr)) {
			return false;
		}

		if (attr == ROOT_ATTR_VERSION) {
			uint16_t datalen;
			if (!props.read<uint16_t>(datalen)) {
				return false;
			}

			if (datalen != sizeof(VERSIONINFO)) {
				return false;
			}

			VERSIONINFO vi;
			if (!props.read(vi)) {
				return false;
			}

			majorVersion = vi.dwMajorVersion; // items otb format file version
			minorVersion = vi.dwMinorVersion; // client version
			buildNumber = vi.dwBuildNumber;   // revision
		}
	}

	if (majorVersion == 0xFFFFFFFF) {
		std::cout << "[Warning - Items::loadFromOtb] items.otb using generic client version." << std::endl;
	} else if (majorVersion != 3) {
		std::cout << "Old version detected, a newer version of items.otb is required." << std::endl;
		return false;
	} else if (minorVersion < CLIENT_VERSION_860_OLD) {
		std::cout << "A newer version of items.otb is required." << std::endl;
		return false;
	}

	for (auto& itemNode : root.children) {
		PropStream stream;
		if (!loader.getProps(itemNode, stream)) {
			return false;
		}

		uint32_t flags;
		if (!stream.read<uint32_t>(flags)) {
			return false;
		}

		uint16_t serverId = 0;
		uint16_t clientId = 0;
		uint16_t speed = 0;
		uint16_t wareId = 0;
		uint8_t lightLevel = 0;
		uint8_t lightColor = 0;
		uint8_t alwaysOnTopOrder = 0;

		uint8_t attrib;
		while (stream.read<uint8_t>(attrib)) {
			uint16_t datalen;
			if (!stream.read<uint16_t>(datalen)) {
				return false;
			}

			switch (attrib) {
				case ITEM_ATTR_SERVERID: {
					if (datalen != sizeof(uint16_t)) {
						return false;
					}

					if (!stream.read<uint16_t>(serverId)) {
						return false;
					}
					break;
				}

				case ITEM_ATTR_CLIENTID: {
					if (datalen != sizeof(uint16_t)) {
						return false;
					}

					if (!stream.read<uint16_t>(clientId)) {
						return false;
					}
					break;
				}

				case ITEM_ATTR_SPEED: {
					if (datalen != sizeof(uint16_t)) {
						return false;
					}

					if (!stream.read<uint16_t>(speed)) {
						return false;
					}
					break;
				}

				case ITEM_ATTR_LIGHT2: {
					if (datalen != sizeof(lightBlock2)) {
						return false;
					}

					lightBlock2 lb2;
					if (!stream.read(lb2)) {
						return false;
					}

					lightLevel = static_cast<uint8_t>(lb2.lightLevel);
					lightColor = static_cast<uint8_t>(lb2.lightColor);
					break;
				}

				case ITEM_ATTR_TOPORDER: {
					if (datalen != sizeof(uint8_t)) {
						return false;
					}

					if (!stream.read<uint8_t>(alwaysOnTopOrder)) {
						return false;
					}
					break;
				}

				case ITEM_ATTR_WAREID: {
					if (datalen != sizeof(uint16_t)) {
						return false;
					}

					if (!stream.read<uint16_t>(wareId)) {
						return false;
					}
					break;
				}

				default: {
					// skip unknown attributes
					if (!stream.skip(datalen)) {
						return false;
					}
					break;
				}
			}
		}

		clientIdToServerIdMap.emplace(clientId, serverId);

		// store the found item
		if (serverId >= items.size()) {
			items.resize(serverId + 1);
		}
		ItemType& iType = items[serverId];

		iType.group = static_cast<itemgroup_t>(itemNode.type);
		switch (itemNode.type) {
			case ITEM_GROUP_CONTAINER:
				iType.type = ITEM_TYPE_CONTAINER;
				break;
			case ITEM_GROUP_DOOR:
				// not used
				iType.type = ITEM_TYPE_DOOR;
				break;
			case ITEM_GROUP_MAGICFIELD:
				// not used
				iType.type = ITEM_TYPE_MAGICFIELD;
				break;
			case ITEM_GROUP_TELEPORT:
				// not used
				iType.type = ITEM_TYPE_TELEPORT;
				break;
			case ITEM_GROUP_NONE:
			case ITEM_GROUP_GROUND:
			case ITEM_GROUP_SPLASH:
			case ITEM_GROUP_FLUID:
			case ITEM_GROUP_CHARGES:
			case ITEM_GROUP_DEPRECATED:
				break;
			default:
				return false;
		}

		iType.blockSolid = hasBitSet(FLAG_BLOCK_SOLID, flags);
		iType.blockProjectile = hasBitSet(FLAG_BLOCK_PROJECTILE, flags);
		iType.blockPathFind = hasBitSet(FLAG_BLOCK_PATHFIND, flags);
		iType.hasHeight = hasBitSet(FLAG_HAS_HEIGHT, flags);
		iType.useable = hasBitSet(FLAG_USEABLE, flags);
		iType.pickupable = hasBitSet(FLAG_PICKUPABLE, flags);
		iType.moveable = hasBitSet(FLAG_MOVEABLE, flags);
		iType.stackable = hasBitSet(FLAG_STACKABLE, flags);

		iType.alwaysOnTop = hasBitSet(FLAG_ALWAYSONTOP, flags);
		iType.isVertical = hasBitSet(FLAG_VERTICAL, flags);
		iType.isHorizontal = hasBitSet(FLAG_HORIZONTAL, flags);
		iType.isHangable = hasBitSet(FLAG_HANGABLE, flags);
		iType.allowDistRead = hasBitSet(FLAG_ALLOWDISTREAD, flags);
		iType.rotatable = hasBitSet(FLAG_ROTATABLE, flags);
		iType.canReadText = hasBitSet(FLAG_READABLE, flags);
		iType.lookThrough = hasBitSet(FLAG_LOOKTHROUGH, flags);
		// iType.walkStack = !hasBitSet(FLAG_FULLTILE, flags);
		iType.forceUse = hasBitSet(FLAG_FORCEUSE, flags);

		iType.id = serverId;
		iType.clientId = clientId;
		iType.speed = speed;
		iType.lightLevel = lightLevel;
		iType.lightColor = lightColor;
		iType.wareId = wareId;
		iType.alwaysOnTopOrder = alwaysOnTopOrder;
	}

	items.shrink_to_fit();
	return true;
}

void Items::buildInventoryList()
{
	inventory.reserve(items.size());
	for (const auto& type : items) {
		if (type.weaponType != WEAPON_NONE || type.ammoType != AMMO_NONE || type.attack != 0 || type.defense != 0 ||
		    type.extraDefense != 0 || type.armor != 0 || type.slotPosition & SLOTP_NECKLACE ||
		    type.slotPosition & SLOTP_RING || type.slotPosition & SLOTP_AMMO || type.slotPosition & SLOTP_FEET ||
		    type.slotPosition & SLOTP_HEAD || type.slotPosition & SLOTP_ARMOR || type.slotPosition & SLOTP_LEGS) {
			inventory.push_back(type.clientId);
		}
	}
	inventory.shrink_to_fit();
	std::sort(inventory.begin(), inventory.end());
}

ItemType& Items::getItemType(size_t id)
{
	if (id < items.size()) {
		return items[id];
	}
	return items.front();
}

const ItemType& Items::getItemType(size_t id) const
{
	if (id < items.size()) {
		return items[id];
	}
	return items.front();
}

const ItemType& Items::getItemIdByClientId(uint16_t spriteId) const
{
	if (spriteId >= 100) {
		if (uint16_t serverId = clientIdToServerIdMap.getServerId(spriteId)) {
			return getItemType(serverId);
		}
	}
	return items.front();
}

uint16_t Items::getItemIdByName(const std::string& name)
{
	if (name.empty()) {
		return 0;
	}

	auto result = nameToItems.find(boost::algorithm::to_lower_copy<std::string>(name));
	if (result == nameToItems.end()) return 0;

	return result->second;
}

void Items::loadFromLua(lua_State* L)
{
	auto table = sol::stack::get<sol::table>(L, 1);
	if (!table.valid()) {
		fmt::print(">>> Items::loadFromLua: Invalid items table\n");
		return;
	}

	for (const auto& pair : table) {
		auto item = pair.second.as<sol::table>();
		if (!item.valid()) {
			fmt::print("[Warning - Items::loadFromLua] Invalid item\n");
			continue;
		}

		if (auto id = item["id"]; id.valid()) {
			parseLuaItem(item, id.get<uint16_t>());
			continue;
		}

		auto fromIdField = item["fromId"];
		auto toIdField = item["toId"];

		if (!fromIdField.valid() && !toIdField.valid()) {
			fmt::print("[Warning - Items::loadFromLua] Invalid item\n");
			continue;
		}

		auto id = fromIdField.get<uint16_t>();
		auto toId = toIdField.get<uint16_t>();
		while (id <= toId) {
			parseLuaItem(item, id++);
		}
	}

	buildInventoryList();
}

void Items::parseLuaItem(const sol::table& item, uint16_t id)
{
	if (id > 0 && id < 100) {
		ItemType& iType = items[id];
		iType.id = id;
	}

	ItemType& it = getItemType(id);
	if (it.id == 0) {
		return;
	}

	if (!it.name.empty()) {
		fmt::print("[Warning - Items::loadFromLua] Duplicated item id: {}\n", it.id);
		return;
	}

	it.name = item["name"].get<std::string>();
	if (!it.name.empty()) {
		auto lowerCaseName = boost::algorithm::to_lower_copy<std::string>(it.name);
		if (nameToItems.find(lowerCaseName) == nameToItems.end()) {
			nameToItems.emplace(std::move(lowerCaseName), id);
		}
	}

	if (auto article = item["article"]; article.valid()) {
		it.article = article.get<std::string>();
	}

	if (auto plural = item["plural"]; plural.valid()) {
		it.pluralName = plural.get<std::string>();
	}

	if (auto description = item["description"]; description.valid()) {
		it.description = description.get<std::string>();
	}

	if (auto runeSpellName = item["runeSpellName"]; runeSpellName.valid()) {
		it.runeSpellName = runeSpellName.get<std::string>();
	}

	if (auto weight = item["weight"]; weight.valid()) {
		it.weight = weight.get<uint32_t>();
	}

	if (auto type = item["type"]; type.valid()) {
		it.type = type.get<ItemTypes_t>();
		if (it.type == ITEM_TYPE_CONTAINER) {
			it.group = ITEM_GROUP_CONTAINER;
		}
	}

	if (auto showCount = item["showCount"]; showCount.valid()) {
		it.showCount = showCount.get<bool>();
	}

	if (auto armor = item["armor"]; armor.valid()) {
		it.armor = armor.get<int32_t>();
	}

	if (auto defense = item["defense"]; defense.valid()) {
		it.defense = defense.get<int32_t>();
	}

	if (auto extraDefense = item["extraDefense"]; extraDefense.valid()) {
		it.extraDefense = extraDefense.get<int32_t>();
	}

	if (auto attack = item["attack"]; attack.valid()) {
		it.attack = attack.get<int32_t>();
	}

	if (auto attackSpeed = item["attackSpeed"]; attackSpeed.valid()) {
		it.attackSpeed = attackSpeed.get<uint32_t>();
	}

	if (auto rotateTo = item["rotateTo"]; rotateTo.valid()) {
		it.rotateTo = rotateTo.get<uint8_t>();
	}

	if (auto moveable = item["moveable"]; moveable.valid()) {
		it.moveable = moveable.get<bool>();
	}

	if (auto blockProjectile = item["blockProjectile"]; blockProjectile.valid()) {
		it.blockProjectile = blockProjectile.get<bool>();
	}

	if (auto ignoreBlocking = item["ignoreBlocking"]; ignoreBlocking.valid()) {
		it.ignoreBlocking = ignoreBlocking.get<bool>();
	}

	if (auto pickupable = item["pickupable"]; pickupable.valid()) {
		it.pickupable = pickupable.get<bool>();
	}

	if (auto forceSerialize = item["forceSerialize"]; forceSerialize.valid()) {
		it.forceSerialize = forceSerialize.get<bool>();
	}

	if (auto floorChange = item["floorChange"]; floorChange.valid()) {
		it.floorChange = floorChange.get<uint8_t>();
	}

	if (auto corpseType = item["corpseType"]; corpseType.valid()) {
		it.corpseType = corpseType.get<RaceType_t>();
	}

	if (auto maxItems = item["containerSize"]; maxItems.valid()) {
		it.maxItems = maxItems.get<uint8_t>();
	}

	if (auto fluidSource = item["fluidSource"]; fluidSource.valid()) {
		it.fluidSource = fluidSource.get<FluidTypes_t>();
	}

	if (auto fluidContainer = item["fluidContainer"]; fluidContainer.valid()) {
		it.group = ITEM_GROUP_FLUID;
	}

	if (auto canReadText = item["readable"]; canReadText.valid()) {
		it.canReadText = canReadText.get<bool>();
	}

	if (auto canWriteText = item["writeable"]; canWriteText.valid()) {
		it.canWriteText = canWriteText.get<bool>();
	}

	if (auto maxTextLen = item["maxTextLen"]; maxTextLen.valid()) {
		it.maxTextLen = maxTextLen.get<uint16_t>();
	}

	if (auto writeOnceItemId = item["writeOnceItemId"]; writeOnceItemId.valid()) {
		it.writeOnceItemId = writeOnceItemId.get<uint16_t>();
	}

	if (auto weaponType = item["weaponType"]; weaponType.valid()) {
		it.weaponType = weaponType.get<WeaponType_t>();
	}

	if (auto slotPosition = item["slotPosition"]; slotPosition.valid()) {
		it.slotPosition = slotPosition.get<uint16_t>();
	}

	if (auto ammoType = item["ammoType"]; ammoType.valid()) {
		it.ammoType = ammoType.get<Ammo_t>();
	}

	if (auto shootType = item["shootType"]; shootType.valid()) {
		it.shootType = shootType.get<ShootType_t>();
	}

	if (auto magicEffect = item["magicEffect"]; magicEffect.valid()) {
		it.magicEffect = magicEffect.get<MagicEffectClasses>();
	}

	if (auto shootRange = item["shootRange"]; shootRange.valid()) {
		it.shootRange = shootRange.get<uint8_t>();
	}

	if (auto stopTime = item["stopDuration"]; stopTime.valid()) {
		it.stopTime = stopTime.get<bool>();
	}

	if (auto decayTo = item["decayTo"]; decayTo.valid()) {
		it.decayTo = decayTo.get<int32_t>();
	}

	if (auto transformEquipTo = item["transformEquipTo"]; transformEquipTo.valid()) {
		it.transformEquipTo = transformEquipTo.get<uint16_t>();
	}

	if (auto transformDeEquipTo = item["transformDeEquipTo"]; transformDeEquipTo.valid()) {
		it.transformDeEquipTo = transformDeEquipTo.get<uint16_t>();
	}

	if (auto duration = item["duration"]; duration.valid()) {
		it.decayTimeMin = duration.get<uint32_t>();
	} else {
		if (auto decayTimeMin = item["minDuration"]; decayTimeMin.valid()) {
			it.decayTimeMin = decayTimeMin.get<uint32_t>();
		}

		if (auto decayTimeMax = item["maxDuration"]; decayTimeMax.valid()) {
			it.decayTimeMax = decayTimeMax.get<uint32_t>();
		}
	}

	if (auto showDuration = item["showDuration"]; showDuration.valid()) {
		it.showDuration = showDuration.get<bool>();
	}

	if (auto charges = item["charges"]; charges.valid()) {
		it.charges = charges.get<uint32_t>();
	}

	if (auto showCharges = item["showCharges"]; showCharges.valid()) {
		it.showCharges = showCharges.get<bool>();
	}

	if (auto showAttributes = item["showAttributes"]; showAttributes.valid()) {
		it.showAttributes = showAttributes.get<bool>();
	}

	if (auto hitChance = item["hitChance"]; hitChance.valid()) {
		it.hitChance = std::min<int8_t>(100, std::max<int8_t>(127, hitChance.get<int8_t>()));
	}

	if (auto field = item["field"]; field.valid()) {
		it.group = ITEM_GROUP_MAGICFIELD;
		it.type = ITEM_TYPE_MAGICFIELD;

		CombatType_t combatType = COMBAT_NONE;
		if (auto combatTypeField = field["combatType"]; combatTypeField.valid()) {
			combatType = combatTypeField.get<CombatType_t>();
		}

		if (combatType != COMBAT_NONE) {
			ConditionDamage* conditionDamage = nullptr;

			if (combatType == COMBAT_PHYSICALDAMAGE) {
				conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_BLEEDING);
			} else if (combatType == COMBAT_ENERGYDAMAGE) {
				conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_ENERGY);
			} else if (combatType == COMBAT_EARTHDAMAGE) {
				conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_POISON);
			} else if (combatType == COMBAT_FIREDAMAGE) {
				conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_FIRE);
			} else if (combatType == COMBAT_DROWNDAMAGE) {
				conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_DROWN);
			} else {
				fmt::print("[Warning - Items::loadFromLua] Invalid combat type: {}\n", static_cast<int>(combatType));
			}

			it.combatType = combatType;
			it.conditionDamage.reset(conditionDamage);

			uint32_t ticks = 0;
			int32_t start = 0;
			int32_t count = 1;
			int32_t initDamage = -1;
			int32_t damage = 0;

			if (auto ticksField = field["ticks"]; ticksField.valid()) {
				ticks = ticksField.get<uint32_t>();
			}

			if (auto startField = field["start"]; startField.valid()) {
				start = std::max<int32_t>(0, startField.get<int32_t>());
			}

			if (auto countField = field["count"]; countField.valid()) {
				count = std::max<int32_t>(1, countField.get<int32_t>());
			}

			if (auto initDamageField = field["initDamage"]; initDamageField.valid()) {
				initDamage = initDamageField.get<int32_t>();
			}

			if (auto damageField = field["damage"]; damageField.valid()) {
				damage = -damageField.get<int32_t>();
				if (start > 0) {
					std::list<int32_t> damageList;
					ConditionDamage::generateDamageList(damage, start, damageList);
					for (int32_t damageValue : damageList) {
						conditionDamage->addDamage(1, ticks, -damageValue);
					}

					start = 0;
				} else {
					conditionDamage->addDamage(count, ticks, damage);
				}
			}

			// datapack compatibility, presume damage to be initialdamage if initialdamage is not declared.
			// initDamage = 0 (don't override initDamage with damage, don't set any initDamage)
			// initDamage = -1 (undefined, override initDamage with damage)
			if (initDamage > 0 || initDamage < -1) {
				conditionDamage->setInitDamage(-initDamage);
			} else if (initDamage == -1 && start != 0) {
				conditionDamage->setInitDamage(start);
			}

			conditionDamage->setParam(CONDITION_PARAM_FIELD, 1);

			if (conditionDamage->getTotalDamage() > 0) {
				conditionDamage->setParam(CONDITION_PARAM_FORCEUPDATE, 1);
			}
		}
	}

	if (auto maxHitChance = item["maxHitChance"]; maxHitChance.valid()) {
		it.maxHitChance = std::min<int8_t>(100, std::max<int8_t>(127, maxHitChance.get<int8_t>()));
	}

	if (auto replaceable = item["replaceable"]; replaceable.valid()) {
		it.replaceable = replaceable.get<bool>();
	}

	if (auto partnerDirection = item["partnerDirection"]; partnerDirection.valid()) {
		it.bedPartnerDir = partnerDirection.get<Direction>();
	}

	if (auto levelDoor = item["levelDoor"]; levelDoor.valid()) {
		it.levelDoor = levelDoor.get<uint32_t>();
	}

	if (auto maleSleeper = item["maleSleeper"]; maleSleeper.valid()) {
		auto value = maleSleeper.get<uint16_t>();
		it.transformToOnUse[PLAYERSEX_MALE] = value;
		ItemType& other = getItemType(value);
		if (other.transformToFree == 0) {
			other.transformToFree = it.id;
		}

		if (it.transformToOnUse[PLAYERSEX_FEMALE] == 0) {
			it.transformToOnUse[PLAYERSEX_FEMALE] = value;
		}
	}

	if (auto femaleSleeper = item["femaleSleeper"]; femaleSleeper.valid()) {
		auto value = femaleSleeper.get<uint16_t>();
		it.transformToOnUse[PLAYERSEX_FEMALE] = value;

		ItemType& other = getItemType(value);
		if (other.transformToFree == 0) {
			other.transformToFree = it.id;
		}

		if (it.transformToOnUse[PLAYERSEX_MALE] == 0) {
			it.transformToOnUse[PLAYERSEX_MALE] = value;
		}
	}

	if (auto transformTo = item["transformTo"]; transformTo.valid()) {
		it.transformToFree = transformTo.get<uint16_t>();
	}

	if (auto destroyTo = item["destroyTo"]; destroyTo.valid()) {
		it.destroyTo = destroyTo.get<uint16_t>();
	}

	if (auto walkStack = item["walkStack"]; walkStack.valid()) {
		it.walkStack = walkStack.get<bool>();
	}

	if (auto blocking = item["blocking"]; blocking.valid()) {
		it.blockSolid = blocking.get<bool>();
	}

	if (auto allowDistRead = item["allowDistRead"]; allowDistRead.valid()) {
		it.allowDistRead = allowDistRead.get<bool>();
	}

	if (auto worth = item["worth"]; worth.valid()) {
		auto value = worth.get<uint64_t>();
		if (currencyItems.find(value) != currencyItems.end()) {
			fmt::print("[Warning - Items::loadFromLua] Duplicated currency worth. Item {} redefine worth./n", it.id);
		} else {
			currencyItems.insert(CurrencyMap::value_type(worth, id));
			it.worth = value;
		}
	}

	if (auto stackSize = item["stackSize"]; stackSize.valid()) {
		auto value = stackSize.get<uint16_t>();
		if (value == 0 || value >= 255) {
			fmt::print("[Warning - Items::loadFromLua] Invalid stack size. Item {} has stack size {}.\n", it.id, value);
		} else {
			it.stackSize = static_cast<uint8_t>(value);
		}
	}

	auto& abilities = it.getAbilities();

	if (auto conditionSuppressions = item["conditionSuppressions"]; conditionSuppressions.valid()) {
		abilities.conditionSuppressions = conditionSuppressions.get<uint32_t>();
	}

	if (auto elementType = item["elementType"]; elementType.valid()) {
		abilities.elementType = elementType.get<CombatType_t>();
	}

	if (auto elementDamage = item["elementDamage"]; elementDamage.valid()) {
		abilities.elementDamage = elementDamage.get<uint16_t>();
	}

	if (auto invisible = item["invisible"]; invisible.valid()) {
		abilities.invisible = invisible.get<bool>();
	}

	if (auto speed = item["speed"]; speed.valid()) {
		abilities.speed = speed.get<int32_t>();
	}

	if (auto healthGain = item["healthGain"]; healthGain.valid()) {
		abilities.healthGain = healthGain.get<uint32_t>();
		abilities.regeneration = true;
	}

	if (auto healthTicks = item["healthTicks"]; healthTicks.valid()) {
		abilities.healthTicks = healthTicks.get<uint32_t>();
		abilities.regeneration = true;
	}

	if (auto manaGain = item["manaGain"]; manaGain.valid()) {
		abilities.manaGain = manaGain.get<uint32_t>();
		abilities.regeneration = true;
	}

	if (auto manaTicks = item["manaTicks"]; manaTicks.valid()) {
		abilities.manaTicks = manaTicks.get<uint32_t>();
		abilities.regeneration = true;
	}

	if (auto manaShield = item["manaShield"]; manaShield.valid()) {
		abilities.manaShield = manaShield.get<bool>();
	}

	if (auto skillFist = item["skillFist"]; skillFist.valid()) {
		abilities.skills[SKILL_FIST] = skillFist.get<int32_t>();
	}

	if (auto skillClub = item["skillClub"]; skillClub.valid()) {
		abilities.skills[SKILL_CLUB] = skillClub.get<int32_t>();
	}

	if (auto skillSword = item["skillSword"]; skillSword.valid()) {
		abilities.skills[SKILL_SWORD] = skillSword.get<int32_t>();
	}

	if (auto skillAxe = item["skillAxe"]; skillAxe.valid()) {
		abilities.skills[SKILL_AXE] = skillAxe.get<int32_t>();
	}

	if (auto skillDistance = item["skillDist"]; skillDistance.valid()) {
		abilities.skills[SKILL_DISTANCE] = skillDistance.get<int32_t>();
	}

	if (auto skillShield = item["skillShield"]; skillShield.valid()) {
		abilities.skills[SKILL_SHIELD] = skillShield.get<int32_t>();
	}

	if (auto skillFishing = item["skillFishing"]; skillFishing.valid()) {
		abilities.skills[SKILL_FISHING] = skillFishing.get<int32_t>();
	}

	if (auto criticalHitChance = item["criticalHitChance"]; criticalHitChance.valid()) {
		abilities.specialSkills[SPECIALSKILL_CRITICALHITCHANCE] = criticalHitChance.get<int32_t>();
	}

	if (auto criticalHitAmount = item["criticalHitAmount"]; criticalHitAmount.valid()) {
		abilities.specialSkills[SPECIALSKILL_CRITICALHITAMOUNT] = criticalHitAmount.get<int32_t>();
	}

	if (auto lifeLeechChance = item["lifeLeechChance"]; lifeLeechChance.valid()) {
		abilities.specialSkills[SPECIALSKILL_LIFELEECHCHANCE] = lifeLeechChance.get<int32_t>();
	}

	if (auto lifeLeechAmount = item["lifeLeechAmount"]; lifeLeechAmount.valid()) {
		abilities.specialSkills[SPECIALSKILL_LIFELEECHAMOUNT] = lifeLeechAmount.get<int32_t>();
	}

	if (auto manaLeechChance = item["manaLeechChance"]; manaLeechChance.valid()) {
		abilities.specialSkills[SPECIALSKILL_MANALEECHCHANCE] = manaLeechChance.get<int32_t>();
	}

	if (auto manaLeechAmount = item["manaLeechAmount"]; manaLeechAmount.valid()) {
		abilities.specialSkills[SPECIALSKILL_MANALEECHAMOUNT] = manaLeechAmount.get<int32_t>();
	}

	if (auto maxHealth = item["maxHealth"]; maxHealth.valid()) {
		abilities.stats[STAT_MAXHITPOINTS] = maxHealth.get<int32_t>();
	}

	if (auto maxHealthPercent = item["maxHealthPercent"]; maxHealthPercent.valid()) {
		abilities.statsPercent[STAT_MAXHITPOINTS] = maxHealthPercent.get<int32_t>();
	}

	if (auto maxMana = item["maxMana"]; maxMana.valid()) {
		abilities.stats[STAT_MAXMANAPOINTS] = maxMana.get<int32_t>();
	}

	if (auto maxManaPercent = item["maxManaPercent"]; maxManaPercent.valid()) {
		abilities.statsPercent[STAT_MAXMANAPOINTS] = maxManaPercent.get<int32_t>();
	}

	if (auto magicLevelPoints = item["magicLevelPoints"]; magicLevelPoints.valid()) {
		abilities.stats[STAT_MAGICPOINTS] = magicLevelPoints.get<int32_t>();
	}

	if (auto magicLevelPointsPercent = item["magicLevelPointsPercent"]; magicLevelPointsPercent.valid()) {
		abilities.statsPercent[STAT_MAGICPOINTS] = magicLevelPointsPercent.get<int32_t>();
	}

	if (auto absorbPercent = item["absorbPercent"]; absorbPercent.valid()) {
		const auto& absorbInfo = absorbPercent.get<sol::table>();
		for (const auto& pair : absorbInfo) {
			const auto& combatType = pair.first.as<CombatType_t>();
			const auto& value = pair.second.as<int16_t>();
			abilities.absorbPercent[combatTypeToIndex(combatType)] += value;
		}
	}

	if (auto fieldAbsorbPercent = item["fieldAbsorbPercent"]; fieldAbsorbPercent.valid()) {
		const auto& absorbInfo = fieldAbsorbPercent.get<sol::table>();
		for (const auto& pair : absorbInfo) {
			const auto& combatType = pair.first.as<CombatType_t>();
			const auto& value = pair.second.as<int16_t>();
			abilities.fieldAbsorbPercent[combatTypeToIndex(combatType)] += value;
		}
	}

	// check bed items
	if ((it.transformToFree != 0 || it.transformToOnUse[PLAYERSEX_FEMALE] != 0 ||
	     it.transformToOnUse[PLAYERSEX_MALE] != 0) &&
	    it.type != ITEM_TYPE_BED) {
		fmt::print("[Warning - Items::parseLuaItem] Item {} is not set as a bed-type\n", it.id);
	}
}
