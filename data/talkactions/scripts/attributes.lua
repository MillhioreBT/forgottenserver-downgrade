local function message(player, msg, ...)
	player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, string.format(msg, ...))
end

local function setAttribute(player, thing, attribute, value)
	local attributeId = Game.getItemAttributeByName(attribute)
	if attributeId == ITEM_ATTRIBUTE_NONE then return "Invalid attribute name." end

	if not thing:setAttribute(attribute, value) then return "Could not set attribute." end

	message(player, "Attribute %s set to: %s", attribute, thing:getAttribute(attributeId))
	thing:getPosition():sendMagicEffect(CONST_ME_MAGIC_GREEN)
	return true
end

---@type {[string]: fun(player: Player, creature: Creature, value: string): boolean}
local creatureAttrs = {
	health = function(player, creature, value)
		creature:setHealth(math.floor(tonumber(value) or 0), player)
		return true
	end,

	addSummon = function(player, creature, value)
		local summon = Game.createMonster(value, creature:getPosition())
		if not summon then return false end

		creature:addSummon(summon)
		return true
	end,

	event = function (player, creature, value)
		creature:registerEvent(value)
		return true
	end
}

function onSay(player, words, param)
	local position = player:getPosition()
	position:getNextPosition(player:getDirection())

	local tile = Tile(position)
	if not tile then
		message(player, "There is no tile in front of you.")
		return false
	end

	local thing = tile:getTopVisibleThing(player)
	if not thing then
		message(player, "There is an empty tile in front of you.")
		return false
	end

	---@type string, string, string
	local attribute, value, extra = unpack(param:splitTrimmed(","))
	if attribute == "" then
		message(player, "Usage: %s attribute, value.", words)
		return false
	end

	local item = thing:getItem()
	if item then
		local response = setAttribute(player, item, attribute, value)
		if response == true then return true end

		if attribute == "custom" then
			if not extra then
				player:sendCancelMessage("You need to specify a custom attribute.")
				return false
			end

			if not item:setCustomAttribute(value, extra) then
				message(player, "Could not set custom attribute.")
				return false
			end

			message(player, "Custom attribute %s set to: %s", value, item:getCustomAttribute(value))
			position:sendMagicEffect(CONST_ME_MAGIC_GREEN)
		else
			message(player, response)
		end
		return false
	end

	local creature = thing:getCreature()
	if creature then
		local creatureAttr = creatureAttrs[attribute]
		if not creatureAttr then
			message(player, "Invalid attribute name.")
			return false
		end

		local response = creatureAttr(player, creature, value)
		if not response then
			message(player, "Could not set attribute.")
			return false
		end

		position:sendMagicEffect(CONST_ME_MAGIC_GREEN)
		return true
	end

	player:sendCancelMessage("You can only use this command on items or creatures.")
	return false
end
