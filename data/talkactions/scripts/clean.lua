function onSay(player, words, param)
	local itemCount = cleanMap()
	if itemCount > 0 then
		player:sendTextMessage(MESSAGE_STATUS_WARNING,
		                       "Cleaned " .. itemCount .. " item" ..
			                       (itemCount > 1 and "s" or "") .. " from the map.")
	end
	return false
end
