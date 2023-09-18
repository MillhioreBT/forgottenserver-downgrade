function onSay(player, words, param, type)
	local cliport = player:getCondition(CONDITION_CLIPORT, CONDITIONID_DEFAULT)
	if cliport then
		player:removeCondition(CONDITION_CLIPORT, CONDITIONID_DEFAULT)

		player:sendCancelMessage("You are no longer in cliport mode.")
	else
		cliport = Condition(CONDITION_CLIPORT, CONDITIONID_DEFAULT)
		cliport:setParameter(CONDITION_PARAM_TICKS, -1)
		player:addCondition(cliport)

		player:sendCancelMessage("You are now in cliport mode.")

		local specs = nil
		if player:isInGhostMode() then
			local spectators = Game.getSpectators(player:getPosition(), true, true)
			specs = {}
			for _, spectator in ipairs(spectators) do
				if spectator:getGroup():getAccess() or spectator:getAccountType() >=
					ACCOUNT_TYPE_GOD then specs[#specs + 1] = spectator end
			end
		end

		player:getPosition():sendMagicEffect(CONST_ME_TELEPORT, specs)
	end
	return false
end
