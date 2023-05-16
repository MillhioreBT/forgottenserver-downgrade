function onSay(player, words, param)
	player:setHiddenHealth(not player:isHealthHidden())
	return false
end
