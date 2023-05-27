local accountTitles = {
	[ACCOUNT_TYPE_NORMAL] = "Access Player",
	[ACCOUNT_TYPE_TUTOR] = "Tutor",
	[ACCOUNT_TYPE_SENIORTUTOR] = "Senior Tutor",
	[ACCOUNT_TYPE_GAMEMASTER] = "Gamemaster",
	[ACCOUNT_TYPE_GOD] = "God"
}

local commands = TalkAction("!commands", "/commands")

function commands.onSay(player)
	local talkactions = Game.getTalkactions()
	if #talkactions == 0 then
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE,
		                       "There are no talkactions.")
		return false
	end

	local isPlayerAccess = player:getGroup():getAccess()
	local accountType = player:getAccountType()

	local accessCommands = {}
	local normalCommands = {}

	for _, talkaction in ipairs(talkactions) do
		if talkaction:access() then
			if isPlayerAccess then accessCommands[#accessCommands + 1] = talkaction end
		else
			normalCommands[#normalCommands + 1] = talkaction
		end
	end

	if isPlayerAccess then
		table.sort(accessCommands,
		           function(a, b) return a:accountType() > b:accountType() end)
	end

	table.sort(normalCommands,
	           function(a, b) return a:accountType() > b:accountType() end)

	local message = {"Commands:"}

	if isPlayerAccess then
		local alreadyTitle = {}
		for _, talkaction in ipairs(accessCommands) do
			local talkAccType = talkaction:accountType()
			if accountType >= talkAccType then
				local title = accountTitles[talkAccType]
				if not alreadyTitle[title] then
					message[#message + 1] = string.format("\n%s:", title)
					alreadyTitle[title] = true
				end
				message[#message + 1] = string.format("%s", talkaction:getWords())
			end
		end
	end

	if #normalCommands > 0 then
		message[#message + 1] = "\nNormal Player:"
		for _, talkaction in ipairs(normalCommands) do
			message[#message + 1] = string.format("%s", talkaction:getWords())
		end
	end

	player:showTextDialog(1950, table.concat(message, "\n"))
	return false
end

commands:register()
