local cast = TalkAction("!cast")

local function sendCastStartedMessages(player, password)
	player:sendChannelMessage("", "========================================", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	player:sendChannelMessage("", "       LIVE CAST STARTED!", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	player:sendChannelMessage("", "========================================", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	if password:len() > 0 then
		player:sendChannelMessage("", "Password: " .. password, TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	else
		player:sendChannelMessage("", "Password: None (public cast)", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	end
	player:sendChannelMessage("", "----------------------------------------", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	player:sendChannelMessage("", "Commands:", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	player:sendChannelMessage("", "  !cast on - Start live casting (public, with bonus)", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	player:sendChannelMessage("", "  !cast off - Stop live casting", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	player:sendChannelMessage("", "  !cast password, <pass> - Start live casting with password (no bonus)", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	player:sendChannelMessage("", "  /commands - View all cast commands", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
	player:sendChannelMessage("", "========================================", TALKTYPE_CHANNEL_O, CHANNEL_CAST)
end

function cast.onSay(player, words, param)
	local raw = (param or "")
	local sparam = raw:trim()
	local lparam = sparam:lower()

	if lparam == "on" or lparam == "start" then
		if player:isLiveCasting() then
			player:stopLiveCasting()
		end

		player:startLiveCasting("")
		sendCastStartedMessages(player, "")
		return false
	end

	if lparam == "off" or lparam == "stop" then
		if player:isLiveCasting() then
			player:stopLiveCasting()
			player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "[Cast] You have stopped live casting.")
		else
			player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "[Cast] You are not live casting.")
		end
		return false
	end

	local pass = sparam:match("^[Pp]assword%s*[, ]%s*(.+)") or sparam:match("^[Pp]ass%s*[, ]%s*(.+)") or sparam
	pass = (pass or ""):trim()
	pass = pass:gsub('%W','')
	if pass:len() > 30 then
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "[Cast] Invalid password. Maximum 30 characters allowed.")
		return false
	end

	if player:isLiveCasting() then
		player:stopLiveCasting()
	end

	player:startLiveCasting(pass)
	sendCastStartedMessages(player, pass)
	return false
end

cast:separator(" ")
cast:register()
