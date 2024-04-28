function onLogout(player)
	local playerId = player:getId()
	nextUseStaminaTime[playerId] = nil
	return true
end
