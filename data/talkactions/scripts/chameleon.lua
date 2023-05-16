local condition = Condition(CONDITION_OUTFIT, CONDITIONID_COMBAT)
condition:setTicks(-1)

function onSay(player, words, param)
	local itemType = ItemType(param)
	if itemType:getId() == 0 then
		itemType = ItemType(tonumber(param))
		if itemType:getId() == 0 then
			player:sendCancelMessage("There is no item with that id or name.")
			return false
		end
	end

	condition:setOutfit(itemType:getId())
	player:addCondition(condition)
	return false
end
