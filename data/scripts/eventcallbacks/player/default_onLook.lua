local fmt = string.format

local event = Event()

event.onLook = function(player, thing, position, distance, description)
	description = "You see " .. thing:getDescription(distance)
	if player:getGroup():getAccess() then
		local item = thing:getItem()
		if item then
			description = fmt("%s\nItem ID: %d", description, item:getId())

			local actionId = item:getActionId()
			if actionId ~= 0 then description = fmt("%s, Action ID: %d", description, actionId) end

			local uniqueId = item:getAttribute(ITEM_ATTRIBUTE_UNIQUEID)
			if uniqueId > 0 and uniqueId < 65536 then
				description = fmt("%s, Unique ID: %d", description, uniqueId)
			end

			local itemType = item:getType()

			local transformEquipId = itemType:getTransformEquipId()
			local transformDeEquipId = itemType:getTransformDeEquipId()
			if transformEquipId ~= 0 then
				description = fmt("%s\nTransforms to: %d (onEquip)", description, transformEquipId)
			elseif transformDeEquipId ~= 0 then
				description =
					fmt("%s\nTransforms to: %d (onDeEquip)", description, transformDeEquipId)
			end

			local decayId = itemType:getDecayId()
			if decayId ~= -1 then description = fmt("%s\nDecays to: %d", description, decayId) end
		else
			local thingCreature = thing:getCreature()
			local thingPlayer = thing:getPlayer()
			local thingMonster = thing:getMonster()
			if thingCreature then
				local str = "%s\nHealth: %d / %d"

				if thingPlayer then
					local thinPlayerMana = thingPlayer:getMana()
					if thinPlayerMana > 0 then
						str = fmt("%s, Mana: %d / %d", str, thinPlayerMana, thingPlayer:getMaxMana())
					end
				elseif thingMonster then
					local raceId = thingMonster:getType():raceId()
					if raceId ~= 0 then str = fmt("%s\nRaceId: %d", str, raceId) end
				end

				description = fmt(str, description, thingCreature:getHealth(),
				                  thingCreature:getMaxHealth()) .. "."
			end

			local thingPosition = thing:getPosition()
			if thingPosition then
				description = fmt("%s\nPosition: %d, %d, %d", description, thingPosition.x,
				                  thingPosition.y, thingPosition.z)
			end

			if thingPlayer then
				description = fmt("%s\nIP: %s.", description,
				                  Game.convertIpToString(thingPlayer:getIp()))
				if thingPlayer:getGroup():getAccess() then
					description = fmt("%s\nVocation: %s.", description,
					                  thingPlayer:getVocation():getName())
				end
			end
		end
	end
	return description
end

event:register()
