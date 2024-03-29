function Game.broadcastMessage(message, messageType)
	if not messageType then messageType = MESSAGE_STATUS_WARNING end

	for _, player in ipairs(Game.getPlayers()) do
		player:sendTextMessage(messageType, message)
	end
end

function Game.convertIpToString(ip)
	return string.format("%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF,
	                     (ip >> 16) & 0xFF, ip >> 24)
end

function Game.getReverseDirection(direction)
	if direction == WEST then
		return EAST
	elseif direction == EAST then
		return WEST
	elseif direction == NORTH then
		return SOUTH
	elseif direction == SOUTH then
		return NORTH
	elseif direction == NORTHWEST then
		return SOUTHEAST
	elseif direction == NORTHEAST then
		return SOUTHWEST
	elseif direction == SOUTHWEST then
		return NORTHEAST
	elseif direction == SOUTHEAST then
		return NORTHWEST
	end
	return NORTH
end

function Game.getSkillType(weaponType)
	if weaponType == WEAPON_CLUB then
		return SKILL_CLUB
	elseif weaponType == WEAPON_SWORD then
		return SKILL_SWORD
	elseif weaponType == WEAPON_AXE then
		return SKILL_AXE
	elseif weaponType == WEAPON_DISTANCE then
		return SKILL_DISTANCE
	elseif weaponType == WEAPON_SHIELD then
		return SKILL_SHIELD
	end
	return SKILL_FIST
end

if not globalStorageTable then globalStorageTable = {} end

function Game.getStorageValue(key) return globalStorageTable[key] or -1 end

function Game.setStorageValue(key, value) globalStorageTable[key] = value end

do
	local cdShort = {"d", "h", "m", "s"}
	local cdLong = {" day", " hour", " minute", " second"}
	local function getTimeUnitGrammar(amount, unitID, isLong)
		return isLong and
			       string.format("%s%s", cdLong[unitID], amount ~= 1 and "s" or "") or
			       cdShort[unitID]
	end

	function Game.getCountdownString(duration, longVersion, hideZero)
		if duration < 0 then return "expired" end

		local days = math.floor(duration / 86400)
		local hours = math.floor((duration % 86400) / 3600)
		local minutes = math.floor((duration % 3600) / 60)
		local seconds = math.floor(duration % 60)

		local response = {}
		if hideZero then
			if days > 0 then
				response[#response + 1] = days .. getTimeUnitGrammar(days, 1, longVersion)
			end

			if hours > 0 then
				response[#response + 1] = hours .. getTimeUnitGrammar(hours, 2, longVersion)
			end

			if minutes > 0 then
				response[#response + 1] = minutes ..
					                          getTimeUnitGrammar(minutes, 3, longVersion)
			end

			if seconds > 0 then
				response[#response + 1] = seconds ..
					                          getTimeUnitGrammar(seconds, 4, longVersion)
			end
		else
			if days > 0 then
				response[#response + 1] = days .. getTimeUnitGrammar(days, 1, longVersion)
				response[#response + 1] = hours .. getTimeUnitGrammar(hours, 2, longVersion)
				response[#response + 1] = minutes ..
					                          getTimeUnitGrammar(minutes, 3, longVersion)
				response[#response + 1] = seconds ..
					                          getTimeUnitGrammar(seconds, 4, longVersion)
			elseif hours > 0 then
				response[#response + 1] = hours .. getTimeUnitGrammar(hours, 2, longVersion)
				response[#response + 1] = minutes ..
					                          getTimeUnitGrammar(minutes, 3, longVersion)
				response[#response + 1] = seconds ..
					                          getTimeUnitGrammar(seconds, 4, longVersion)
			elseif minutes > 0 then
				response[#response + 1] = minutes ..
					                          getTimeUnitGrammar(minutes, 3, longVersion)
				response[#response + 1] = seconds ..
					                          getTimeUnitGrammar(seconds, 4, longVersion)
			elseif seconds >= 0 then
				response[#response + 1] = seconds ..
					                          getTimeUnitGrammar(seconds, 4, longVersion)
			end
		end

		return table.concat(response, " ")
	end
end

do
	local worldLightLevel = 0
	local worldLightColor = 0

	function Game.getWorldLight() return worldLightLevel, worldLightColor end

	function Game.setWorldLight(color, level)
		if not configManager.getBoolean(configKeys.DEFAULT_WORLD_LIGHT) then return end

		local previousColor = worldLightColor
		local previousLevel = worldLightLevel
		worldLightColor = color
		worldLightLevel = level

		if worldLightColor ~= previousColor or worldLightLevel ~= previousLevel then
			for _, player in ipairs(Game.getPlayers()) do
				player:sendWorldLight(worldLightColor, worldLightLevel)
			end
		end
	end
end

do
	local worldTime = 0

	function Game.getWorldTime() return worldTime end

	function Game.setWorldTime(time)
		worldTime = time

		-- quarter-hourly update to client clock near the minimap
		if worldTime % 15 == 0 then
			for _, player in ipairs(Game.getPlayers()) do
				player:sendWorldTime(worldTime)
			end
		end
	end
end
