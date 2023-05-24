local CHAT_ID<const> = 7

function canJoin(player) return player:getAccountType() >= ACCOUNT_TYPE_GOD end

local environment = setmetatable({}, {
	__index = function(self, key)
		local value = rawget(self, key)
		if value ~= nil then return value end

		return _G[key]
	end
})

function onSpeak(player, messageType, message)
	if player:getAccountType() < ACCOUNT_TYPE_GOD then return false end

	environment.print = function(...)
		local args = table.pack(...)
		for i = 1, args.n do args[i] = tostring(args[i]) end

		return player:sendChannelMessage("Print:", table.concat(args, "\t"),
		                                 TALKTYPE_CHANNEL_O, CHAT_ID)
	end

	environment.player = player
	environment.tile = player:getTile()
	environment.pos = player:getPosition()

	local result, err = load(message, "Lua Channel", "t", environment)
	local error<const> = function(err)
		local _, last = err:find("(:%d+:)")
		player:sendChannelMessage("Error:", err:sub(last + 1), TALKTYPE_CHANNEL_R1,
		                          CHAT_ID)
		print(string.format("[Error - Lua Channel]: Author: %s, Error: %s",
		                    player:getName(), err))
	end

	if not result then
		error(err)
	else
		player:sendChannelMessage("Execute:", message, messageType, CHAT_ID)
		result, err = pcall(result)
		if not result then
			error(err)
		else
			local results = table.pack(result)
			for i = 1, results.n do results[i] = tostring(results[i]) end

			if results.n ~= 0 then
				player:sendChannelMessage("Result:", table.concat(results, "\t"),
				                          TALKTYPE_CHANNEL_O, CHAT_ID)
			end
		end
	end

	environment.player = nil
	environment.tile = nil
	environment.pos = nil
	return false
end
