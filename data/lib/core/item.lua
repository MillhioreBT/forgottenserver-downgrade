function Item:getType() return ItemType(self:getId()) end

function Item:getContainer() return Container(self:getUniqueId()) end

function Item:getTeleport() return Teleport(self:getUniqueId()) end

function Item:isContainer() return false end

function Item:isCreature() return false end

function Item:getCreature() return nil end

function Item:getPlayer() return nil end

function Item:isMonster() return false end

function Item:isNpc() return false end

function Item:isPlayer() return false end

function Item:isTeleport() return false end

function Item:isPodium() return false end

function Item:isTile() return false end

function Item:isItemType() return false end

function Item:getItem() return self end

function Item:getAttack()
	if self:hasAttribute(ITEM_ATTRIBUTE_ATTACK) then return self:getAttribute(ITEM_ATTRIBUTE_ATTACK) --[[@as integer]] end
	return self:getType():getAttack()
end

function Item:getDefense()
	if self:hasAttribute(ITEM_ATTRIBUTE_DEFENSE) then return self:getAttribute(ITEM_ATTRIBUTE_DEFENSE) --[[@as integer]] end
	return self:getType():getDefense()
end

function Item:getExtraDefense()
	if self:hasAttribute(ITEM_ATTRIBUTE_EXTRADEFENSE) then
		return self:getAttribute(ITEM_ATTRIBUTE_EXTRADEFENSE) --[[@as integer]]
	end
	return self:getType():getExtraDefense()
end

function Item:getArmor()
	if self:hasAttribute(ITEM_ATTRIBUTE_ARMOR) then return self:getAttribute(ITEM_ATTRIBUTE_ARMOR) --[[@as integer]] end
	return self:getType():getArmor()
end

function Item:getAttackSpeed()
	if self:hasAttribute(ITEM_ATTRIBUTE_ATTACK_SPEED) then
		return self:getAttribute(ITEM_ATTRIBUTE_ATTACK_SPEED) --[[@as integer]]
	end
	return self:getType():getAttackSpeed()
end

function Item:getHitChance()
	if self:hasAttribute(ITEM_ATTRIBUTE_HITCHANCE) then
		return self:getAttribute(ITEM_ATTRIBUTE_HITCHANCE) --[[@as integer]]
	end
	return self:getType():getHitChance()
end

function Item:getShootRange()
	if self:hasAttribute(ITEM_ATTRIBUTE_SHOOTRANGE) then
		return self:getAttribute(ITEM_ATTRIBUTE_SHOOTRANGE) --[[@as integer]]
	end
	return self:getType():getShootRange()
end

function Item:getDuration()
	if self:hasAttribute(ITEM_ATTRIBUTE_DURATION) then
		return self:getAttribute(ITEM_ATTRIBUTE_DURATION) --[[@as integer]]
	end
	return self:getType():getDuration()
end

function Item:getText()
	if self:hasAttribute(ITEM_ATTRIBUTE_TEXT) then return self:getAttribute(ITEM_ATTRIBUTE_TEXT) --[[@as string]] end
	return ""
end

function Item:getDate()
	if self:hasAttribute(ITEM_ATTRIBUTE_DATE) then return self:getAttribute(ITEM_ATTRIBUTE_DATE) --[[@as string]] end
	return ""
end

function Item:getWriter()
	if self:hasAttribute(ITEM_ATTRIBUTE_WRITER) then return self:getAttribute(ITEM_ATTRIBUTE_WRITER) --[[@as string]] end
	return ""
end

function Item:setDuration(duration)
	if duration > 0 then
		return self:setAttribute(ITEM_ATTRIBUTE_DURATION, duration)
	else
		return self:removeAttribute(ITEM_ATTRIBUTE_DURATION)
	end
end

function Item:setText(text)
	if text ~= "" then
		return self:setAttribute(ITEM_ATTRIBUTE_TEXT, text)
	else
		return self:removeAttribute(ITEM_ATTRIBUTE_TEXT)
	end
end

function Item:setDate(date)
	if date ~= 0 then
		return self:setAttribute(ITEM_ATTRIBUTE_DATE, date)
	else
		return self:removeAttribute(ITEM_ATTRIBUTE_DATE)
	end
end

function Item:setWriter(writer)
	if writer ~= "" then
		return self:setAttribute(ITEM_ATTRIBUTE_WRITER, writer)
	else
		return self:removeAttribute(ITEM_ATTRIBUTE_WRITER)
	end
end

local fmt = string.format
local concat = table.concat

do
	local StreamMeta = {}
	StreamMeta.__index = StreamMeta

	function StreamMeta:append(s, ...) self[#self + 1] = fmt(s, ...) end

	function StreamMeta:concat(s) return concat(self, s) end

	function StringStream() return setmetatable({}, StreamMeta) end

	local function internalItemGetNameDescription(it, item, subType, addArticle)
		subType = subType or (item and item:getSubType() or -1)
		local ss = StringStream()
		local obj = item or it
		local name = obj:getName()
		if name ~= "" then
			if it:isStackable() and subType > 1 then
				if it:hasShowCount() then ss:append("%d ", subType) end
				ss:append("%s", obj:getPluralName())
			else
				if addArticle and obj:getArticle() ~= "" then ss:append("%s ", obj:getArticle()) end
				ss:append("%s", obj:getName())
			end
		else
			ss:append("an item of type %d", obj:getId())
		end
		return ss:concat()
	end

	function Item:getNameDescription(subType, addArticle)
		return internalItemGetNameDescription(self:getType(), self, subType, addArticle)
	end

	function ItemType:getNameDescription(subType, addArticle)
		return internalItemGetNameDescription(self, nil, subType, addArticle)
	end
end

do
	-- Lua item descriptions created by Zbizu
	local housePriceVisible = configManager.getBoolean(configKeys.HOUSE_DOOR_SHOW_PRICE)
	local pricePerSQM = configManager.getNumber(configKeys.HOUSE_PRICE)

	local showAtkWeaponTypes = {WEAPON_CLUB, WEAPON_SWORD, WEAPON_AXE, WEAPON_DISTANCE}
	local showDefWeaponTypes = {WEAPON_CLUB, WEAPON_SWORD, WEAPON_AXE, WEAPON_DISTANCE, WEAPON_SHIELD}
	local suppressedConditionNames = {
		-- NOTE: these names are made up just to match dwarven ring attribute style
		[CONDITION_POISON] = "antidote",
		[CONDITION_FIRE] = "non-inflammable",
		[CONDITION_ENERGY] = "antistatic",
		[CONDITION_BLEEDING] = "antiseptic",
		[CONDITION_HASTE] = "heavy",
		[CONDITION_PARALYZE] = "slow immunity",
		[CONDITION_DRUNK] = "hard drinking",
		[CONDITION_DROWN] = "water breathing",
		[CONDITION_FREEZING] = "warm",
		[CONDITION_DAZZLED] = "dazzle immunity",
		[CONDITION_CURSED] = "curse immunity"
	}

	-- first argument: Item, itemType or item id
	local function internalItemGetDescription(item, lookDistance, subType, addArticle)
		-- optional, but true by default
		if addArticle == nil then addArticle = true end

		local isVirtual = false -- check if we're inspecting a real item or itemType only
		local itemType

		-- determine the kind of item data to process
		if tonumber(item) then
			-- number in function argument
			-- pull data from itemType instead
			isVirtual = true
			itemType = ItemType(item)
			if not itemType then return end

			-- polymorphism for item attributes (atk, def, etc)
			item = itemType
		elseif item:isItemType() then
			isVirtual = true
			itemType = item
		else
			itemType = item:getType()
		end

		-- use default values if not provided
		lookDistance = lookDistance or -1
		subType = subType or (not isVirtual and item:getSubType() or -1)

		-- possibility of picking the item up (will be reused later)
		local isPickupable = itemType:isMovable() and itemType:isPickupable()

		-- has uniqueId tag
		local isUnique = false
		if not isVirtual then
			local uid = item:getUniqueId()
			if uid > 100 and uid < 0xFFFF then isUnique = true end
		end

		-- read item name with article
		local itemName = item:getNameDescription(subType, addArticle)

		-- things that will go in parenthesis "(...)"
		local descriptions = {}

		-- spell words
		do
			local spellName = itemType:getRuneSpellName()
			if spellName then descriptions[#descriptions + 1] = fmt('"%s"', spellName) end
		end

		-- container capacity
		do
			-- show container capacity only on non-quest containers
			if not isUnique then
				local isContainer = itemType:isContainer()
				if isContainer then descriptions[#descriptions + 1] = fmt("Vol:%d", itemType:getCapacity()) end
			end
		end

		-- actionId (will be reused)
		local actionId = 0
		if not isVirtual then actionId = item:getActionId() end

		-- key
		do
			if not isVirtual and itemType:isKey() then
				descriptions[#descriptions + 1] = fmt("Key:%0.4d", actionId)
			end
		end

		-- weapon type (will be reused)
		local weaponType = itemType:getWeaponType()

		-- attack attributes
		do
			local attack = item:getAttack()

			-- bows and crossbows
			-- range, attack, hit%
			if itemType:isBow() then
				descriptions[#descriptions + 1] = fmt("Range:%d", item:getShootRange())

				if attack ~= 0 then descriptions[#descriptions + 1] = fmt("Atk+%d", attack) end

				local hitPercent = item:getHitChance()
				if hitPercent ~= 0 then descriptions[#descriptions + 1] = fmt("Hit%%+%d", hitPercent) end

				-- melee weapons and missiles
				-- atk x physical +y% element
			elseif table.contains(showAtkWeaponTypes, weaponType) then
				local atkString = fmt("Atk:%d", attack)
				local elementDmg = itemType:getElementDamage()
				if elementDmg ~= 0 then
					atkString = fmt("%s physical %+d %s", atkString, elementDmg,
					                getCombatName(itemType:getElementType()))
				end

				descriptions[#descriptions + 1] = atkString
			end
		end

		-- attack speed
		do
			local atkSpeed = item:getAttackSpeed()
			if atkSpeed ~= 0 then descriptions[#descriptions + 1] = fmt("AS:%0.2f/turn", 2000 / atkSpeed) end
		end

		-- defense attributes
		do
			local showDef = table.contains(showDefWeaponTypes, weaponType)
			local ammoType = itemType:getAmmoType()
			if showDef then
				local defAttrs = {}

				local defense = item:getDefense()
				if defense ~= 0 then
					if weaponType == WEAPON_DISTANCE then
						-- throwables
						if ammoType ~= AMMO_ARROW and ammoType ~= AMMO_BOLT then
							defAttrs[#defAttrs + 1] = defense
						end
					else
						defAttrs[#defAttrs + 1] = defense
					end
				end

				-- extra def
				local xD = item:getExtraDefense()
				if xD ~= 0 then defAttrs[#defAttrs + 1] = fmt("%+d", xD) end

				if #defAttrs > 0 then descriptions[#descriptions + 1] = fmt("Def:%s", concat(defAttrs, " ")) end
			end
		end

		-- armor
		do
			local arm = item:getArmor()
			if arm > 0 then descriptions[#descriptions + 1] = fmt("Arm:%d", arm) end
		end

		-- abilities (will be reused)
		local abilities = itemType:getAbilities()

		-- stats: hp/mp/soul/magic level
		do
			local stats = {}
			-- flat buffs
			for stat, value in pairs(abilities.stats) do
				stats[stat] = {name = getStatName(stat - 1)}
				if value ~= 0 then stats[stat].flat = value end
			end

			-- percent buffs
			for stat, value in pairs(abilities.statsPercent) do
				if value ~= 0 then stats[stat].percent = value end
			end

			-- display the buffs
			for _, statData in pairs(stats) do
				local displayValues = {}
				if statData.flat then displayValues[#displayValues + 1] = statData.flat end

				if statData.percent then displayValues[#displayValues + 1] = statData.percent end

				-- desired format examples:
				-- +5%
				-- +20 and 5%
				if #displayValues > 0 then
					displayValues[1] = fmt("%+d", displayValues[1])
					descriptions[#descriptions + 1] = fmt("%s %s", statData.name, concat(displayValues, " and "))
				end
			end
		end

		-- skill boosts
		do
			for skill, value in pairs(abilities.skills) do
				if value ~= 0 then
					descriptions[#descriptions + 1] = fmt("%s %+d", getSkillName(skill - 1), value)
				end
			end
		end

		-- element magic level
		do
			for element, value in pairs(abilities.specialMagicLevel) do
				if value ~= 0 then
					descriptions[#descriptions + 1] = fmt("%s magic level %+d", getCombatName(2^(element-1)), value)
				end
			end
		end

		-- special skills
		do
			for skill, value in pairs(abilities.specialSkills) do
				if value ~= 0 then
					-- add + symbol to special skill "amount" fields
					if skill - 1 < 6 and skill % 2 == 1 then
						value = fmt("%+d", value)
					elseif skill - 1 >= 6 then
						-- fatal, dodge, momentum coming from the item natively
						-- (stats coming from tier are near tier info)
						value = fmt("%0.2f", value / 100)
					end

					descriptions[#descriptions + 1] = fmt("%s %s%%", getSpecialSkillName(skill - 1), value)
				end
			end
		end

		-- protections
		do
			local protections = {}
			for element, value in pairs(abilities.absorbPercent) do
				if value ~= 0 then
					protections[#protections + 1] = fmt("%s %+d%%", getCombatName(2 ^ (element - 1)), value)
				end
			end

			if #protections > 0 then
				descriptions[#descriptions + 1] = fmt("protection %s", concat(protections, ", "))
			end
		end

		-- damage reflection
		-- to do

		-- magic shield (classic)
		if abilities.manaShield then descriptions[#descriptions + 1] = "magic shield" end

		-- magic shield capacity +flat and x%
		-- to do

		-- regeneration
		if abilities.manaGain > 0 or abilities.healthGain > 0 or abilities.regeneration then
			descriptions[#descriptions + 1] = "faster regeneration"
		end

		-- invisibility
		if abilities.invisible then descriptions[#descriptions + 1] = "invisibility" end

		-- condition immunities
		do
			local suppressions = abilities.conditionSuppressions
			for conditionId, conditionName in pairs(suppressedConditionNames) do
				if (abilities.conditionSuppressions & conditionId) ~= 0 then
					descriptions[#descriptions + 1] = conditionName
				end
			end
		end

		-- speed
		if abilities.speed ~= 0 then
			descriptions[#descriptions + 1] = fmt("speed %+d", math.floor(abilities.speed / 2))
		end

		-- collecting attributes finished
		-- build the output text
		local response = {itemName}

		-- item group (will be reused)
		local itemGroup = itemType:getGroup()

		-- fluid type
		do
			if (itemGroup == ITEM_GROUP_FLUID or itemGroup == ITEM_GROUP_SPLASH) and subType > 0 then
				local subTypeName = getSubTypeName(subType)
				response[#response + 1] = fmt(" of %s", (subTypeName ~= "" and subTypeName or "unknown"))
			end
		end

		-- door check (will be reused)
		local isDoor = itemType:isDoor()

		-- door info
		if isDoor then
			if not isVirtual then
				local minLevelDoor = itemType:getLevelDoor()
				if minLevelDoor ~= 0 then
					if actionId >= minLevelDoor then
						response[#response + 1] = fmt(" for level %d", actionId - minLevelDoor)
					end
				end
			end
		end

		-- primary attributes parenthesis
		if #descriptions > 0 then response[#response + 1] = fmt(" (%s)", concat(descriptions, ", ")) end

		-- charges and duration
		do
			local expireInfo = {}

			-- charges
			if itemType:hasShowCharges() then
				local charges = item:getCharges()
				expireInfo[#expireInfo + 1] = fmt("has %d charge%s left", charges, (charges ~= 1 and "s" or ""))
			end

			-- duration
			if itemType:hasShowDuration() then
				local currentDuration = item:getDuration()
				if isVirtual then currentDuration = currentDuration * 1000 end

				local maxDuration = itemType:getDuration() * 1000
				if maxDuration == 0 then
					local transferType = itemType:getTransformEquipId()
					if transferType ~= 0 then
						transferType = ItemType(transferType)
						maxDuration = transferType and transferType:getDuration() * 1000 or maxDuration
					end
				end

				if currentDuration == maxDuration then
					expireInfo[#expireInfo + 1] = "is brand-new"
				elseif currentDuration ~= 0 then
					expireInfo[#expireInfo + 1] = fmt("will expire in %s", Game.getCountdownString(
						                                  math.floor(currentDuration / 1000), true, true))
				end
			end

			if #expireInfo > 0 then response[#response + 1] = fmt(" that %s", concat(expireInfo, " and ")) end
		end

		-- dot after primary attributes info
		response[#response + 1] = "."

		-- empty fluid container suffix
		if itemGroup == ITEM_GROUP_FLUID and subType < 1 then response[#response + 1] = " It is empty." end

		-- house door
		if isDoor and not isVirtual then
			local tile = item:getTile()
			if tile then
				local house = tile:getHouse()
				if house then
					local houseName = house:getName()
					local houseOwnerName = house:getOwnerName()
					local isForSale = false

					if not houseOwnerName or houseOwnerName:len() == 0 then
						houseOwnerName = "Nobody"
						isForSale = true
					end

					response[#response + 1] = fmt(" It belongs to house '%s'. %s owns this house.", houseName,
					                              houseOwnerName)
					if housePriceVisible and isForSale and pricePerSQM > 0 then
						response[#response + 1] = fmt(" It costs %d gold coins.", pricePerSQM * house:getTileCount())
					end
				end
			end
		end

		-- item count (will be reused later)
		local count = isVirtual and 1 or item:getCount()

		-- wield info
		do
			if itemType:isRune() then
				local rune = Spell(itemType:getId())
				if rune then
					local runeLevel = rune:runeLevel()
					local runeMagLevel = rune:runeMagicLevel()
					local runeVocMap = rune:vocation()

					local hasLevel = runeLevel > 0
					local hasMLvl = runeMagLevel > 0
					local hasVoc = #runeVocMap > 0
					if hasLevel or hasMLvl or hasVoc then
						local vocAttrs = {}
						if not hasVoc then
							vocAttrs[#vocAttrs + 1] = "players"
						else
							for _, vocName in ipairs(runeVocMap) do
								local vocation = Vocation(vocName)
								if vocation and vocation:getPromotion() then vocAttrs[#vocAttrs + 1] = vocName end
							end
						end

						if #vocAttrs > 1 then vocAttrs[#vocAttrs - 1] = fmt("and %s", vocAttrs[#vocAttrs - 1]) end

						local levelInfo = {}
						if hasLevel then levelInfo[#levelInfo + 1] = fmt("level %d", runeLevel) end

						if hasMLvl then levelInfo[#levelInfo + 1] = fmt("magic level %d", runeMagLevel) end

						local levelStr = ""
						if #levelInfo > 0 then levelStr = fmt(" of %s or higher", concat(levelInfo, " and ")) end

						response[#response + 1] = fmt("\n%s can only be used properly by %s%s.",
						                              (count > 1 and "They" or "It"), concat(vocAttrs, ", "), levelStr)
					end
				end
			else
				local wieldInfo = itemType:getWieldInfo()
				if wieldInfo ~= 0 then
					local wieldAttrs = {}
					if (wieldInfo & WIELDINFO_PREMIUM) ~= 0 then wieldAttrs[#wieldAttrs + 1] = "premium" end

					local vocStr = itemType:getVocationString()
					if vocStr ~= "" then
						wieldAttrs[#wieldAttrs + 1] = vocStr
					else
						wieldAttrs[#wieldAttrs + 1] = "players"
					end

					local levelInfo = {}
					if (wieldInfo & WIELDINFO_LEVEL) ~= 0 then
						levelInfo[#levelInfo + 1] = fmt("level %d", itemType:getMinReqLevel())
					end

					if (wieldInfo & WIELDINFO_MAGLV) ~= 0 then
						levelInfo[#levelInfo + 1] = fmt("magic level %d", itemType:getMinReqMagicLevel())
					end

					if #levelInfo > 0 then
						wieldAttrs[#wieldAttrs + 1] = fmt("of %s or higher", concat(levelInfo, " and "))
					end

					response[#response + 1] = fmt("\n%s can only be wielded properly by %s.",
					                              (count > 1 and "They" or "It"), concat(wieldAttrs, " "))
				end
			end
		end

		if lookDistance <= 1 then
			local weight = item:getWeight()
			if isPickupable and not isUnique then
				response[#response + 1] = fmt("\n%s %0.2f oz.", (count == 1 or not itemType:hasShowCount()) and
					                              "It weighs" or "They weigh", weight / 100)
			end
		end

		-- item text
		if not isVirtual and itemType:hasAllowDistRead() then
			local text = item:getText()
			if text and text:len() > 0 then
				if lookDistance <= 4 then
					local writer = item:getAttribute(ITEM_ATTRIBUTE_WRITER)
					local writeDate = item:getAttribute(ITEM_ATTRIBUTE_DATE)
					local writeInfo = {}
					if writer and writer:len() > 0 then
						writeInfo[#writeInfo + 1] = fmt("\n%s wrote", writer)

						if writeDate and writeDate > 0 then
							writeInfo[#writeInfo + 1] = fmt(" on %s", os.date("%d %b %Y", writeDate))
						end
					end

					if #writeInfo > 0 then
						response[#response + 1] = fmt("%s:\n%s", concat(writeInfo, ""), text)
					else
						response[#response + 1] = fmt("\nYou read: %s", text)
					end
				else
					response[#response + 1] = "\nYou are too far away to read it."
				end
			else
				response[#response + 1] = "\nNothing is written on it."
			end
		end

		-- item description
		local isBed = itemType:isBed()
		if lookDistance <= 1 or (not isVirtual and (isDoor or isBed)) then
			-- custom item description
			local desc = not isVirtual and item:getSpecialDescription()

			-- native item description
			if not desc or desc == "" then desc = itemType:getDescription() end

			-- level door description
			if isDoor and lookDistance <= 1 and (not desc or desc == "") and itemType:getLevelDoor() ~= 0 then
				desc = "Only the worthy may pass."
			end

			if desc and desc:len() > 0 then
				if not (isBed and desc == "Nobody is sleeping there.") then
					response[#response + 1] = fmt("\n%s", desc)
				end
			end
		end

		-- turn response into a single string
		return concat(response, "")
	end

	function Item:getDescription(lookDistance, subType, addArticle)
		return internalItemGetDescription(self, lookDistance, subType, addArticle)
	end

	function ItemType:getItemDescription(lookDistance, subType, addArticle)
		return internalItemGetDescription(self, lookDistance, subType, addArticle)
	end
end
