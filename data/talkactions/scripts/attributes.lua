local function setAttribute(player, thing, attribute, value)
	local attributeId = Game.getItemAttributeByName(attribute)
	if attributeId == ITEM_ATTRIBUTE_NONE then return "Invalid attribute name." end

	if not thing:setAttribute(attribute, value) then return "Could not set attribute." end

	player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE,
	                       string.format("Attribute %s set to: %s", attribute, thing:getAttribute(attributeId)))
	thing:getPosition():sendMagicEffect(CONST_ME_MAGIC_GREEN)
	return true
end

function onSay(player, words, param)
	local position = player:getPosition()
	position:getNextPosition(player:getDirection())

	local tile = Tile(position)
	if not tile then
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "There is no tile in front of you.")
		return false
	end

	local thing = tile:getTopVisibleThing(player)
	if not thing then
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "There is an empty tile in front of you.")
		return false
	end

	local attribute, value, extra = unpack(param:splitTrimmed(","))
	if attribute == "" then
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, string.format("Usage: %s attribute, value.", words))
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
				player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "Could not set custom attribute.")
				return false
			end

			player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE,
			                       string.format("Custom attribute %s set to: %s", value, item:getCustomAttribute(value)))
			position:sendMagicEffect(CONST_ME_MAGIC_GREEN)
		else
			player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, response)
		end
	else
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "Thing in front of you is not supported.")
	end
	return false
end
