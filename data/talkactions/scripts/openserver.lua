function onSay(player, words, param)
	Game.setGameState(GAME_STATE_NORMAL)
	player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "Server is now open.")
	return false
end
