function Game.broadcastMessage(message, messageType)
	if not messageType then messageType = MESSAGE_STATUS_WARNING end

	for _, player in ipairs(Game.getPlayers()) do player:sendTextMessage(messageType, message) end
end

function Game.convertIpToString(ip)
	return string.format("%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, ip >> 24)
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

do
	local cdShort = {"d", "h", "m", "s"}
	local cdLong = {" day", " hour", " minute", " second"}
	local function getTimeUnitGrammar(amount, unitID, isLong)
		return isLong and string.format("%s%s", cdLong[unitID], amount ~= 1 and "s" or "") or
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
			if days > 0 then response[#response + 1] = days .. getTimeUnitGrammar(days, 1, longVersion) end

			if hours > 0 then response[#response + 1] = hours .. getTimeUnitGrammar(hours, 2, longVersion) end

			if minutes > 0 then
				response[#response + 1] = minutes .. getTimeUnitGrammar(minutes, 3, longVersion)
			end

			if seconds > 0 then
				response[#response + 1] = seconds .. getTimeUnitGrammar(seconds, 4, longVersion)
			end
		else
			if days > 0 then
				response[#response + 1] = days .. getTimeUnitGrammar(days, 1, longVersion)
				response[#response + 1] = hours .. getTimeUnitGrammar(hours, 2, longVersion)
				response[#response + 1] = minutes .. getTimeUnitGrammar(minutes, 3, longVersion)
				response[#response + 1] = seconds .. getTimeUnitGrammar(seconds, 4, longVersion)
			elseif hours > 0 then
				response[#response + 1] = hours .. getTimeUnitGrammar(hours, 2, longVersion)
				response[#response + 1] = minutes .. getTimeUnitGrammar(minutes, 3, longVersion)
				response[#response + 1] = seconds .. getTimeUnitGrammar(seconds, 4, longVersion)
			elseif minutes > 0 then
				response[#response + 1] = minutes .. getTimeUnitGrammar(minutes, 3, longVersion)
				response[#response + 1] = seconds .. getTimeUnitGrammar(seconds, 4, longVersion)
			elseif seconds >= 0 then
				response[#response + 1] = seconds .. getTimeUnitGrammar(seconds, 4, longVersion)
			end
		end

		return table.concat(response, " ")
	end
end

function Game.saveDebugAssert(playerGuid, assertLine, date, description, comment)
	db.asyncQuery(
		"INSERT INTO `player_debugasserts` (`player_id`, `assert_line`, `date`, `description`, `comment`) VALUES(" ..
			playerGuid .. ", " .. db.escapeString(assertLine) .. ", " .. db.escapeString(date) .. ", " ..
			db.escapeString(description) .. ", " .. db.escapeString(comment) .. ")")
end

do
	local periodMultiplier = {
		[RENTPERIOD_DAILY] = 1,
		[RENTPERIOD_WEEKLY] = 7,
		[RENTPERIOD_MONTHLY] = 30,
		[RENTPERIOD_YEARLY] = 365
	}

	local day = 24 * 60 * 60

	local function getHousePaidUntil(rentPeriod)
		local multiplier = periodMultiplier[rentPeriod]
		if not multiplier then return 0 end
		return day * multiplier
	end

	local rentPeriods = {
		[RENTPERIOD_DAILY] = "daily",
		[RENTPERIOD_WEEKLY] = "weekly",
		[RENTPERIOD_MONTHLY] = "monthly",
		[RENTPERIOD_YEARLY] = "yearly"
	}

	function Game.getNameRentPeriodHouse(rentPeriod) return rentPeriods[rentPeriod] end

	local periodRentNames = {
		["daily"] = RENTPERIOD_DAILY,
		["weekly"] = RENTPERIOD_WEEKLY,
		["monthly"] = RENTPERIOD_MONTHLY,
		["yearly"] = RENTPERIOD_YEARLY
	}

	function Game.getRentPeriodHouse(s) return periodRentNames[s] or RENTPERIOD_NEVER end

	local function payRent(player, house, rent, rentPeriod, timeNow)
		local paidUntil = timeNow + getHousePaidUntil(rentPeriod)
		local nameRentPeriod = Game.getNameRentPeriodHouse(rentPeriod)
		local payRentWarnings = house:getPayRentWarnings()
		local houseName = house:getName()
		local playerBalance = player:getBankBalance()
		if playerBalance >= rent then
			player:setBankBalance(playerBalance - rent)
			house:setPaidUntil(paidUntil)
			house:setPayRentWarnings(0)
		elseif payRentWarnings < 7 then
			local daysLeft = 7 - payRentWarnings

			local stampedLetter = Game.createItem(ITEM_LETTER_STAMPED, 1)
			stampedLetter:setAttribute(ITEM_ATTRIBUTE_TEXT, string.format(
				                           "Warning! \nThe %s rent of %d gold for your house \"%s\" is payable. Have it within %d days or you will lose this house.",
				                           nameRentPeriod, rent, houseName, daysLeft))

			local playerInbox = player:getInbox()
			playerInbox:addItemEx(stampedLetter, INDEX_WHEREEVER, FLAG_NOLIMIT)
			house:setPayRentWarnings(payRentWarnings + 1)
		else
			house:setOwnerGuid(0)
		end

		player:save()
	end

	function Game.payHouses(rentPeriod)
		if rentPeriod == RENTPERIOD_NEVER then return end

		local currentTime = os.time()
		for _, house in ipairs(Game.getHouses()) do
			repeat
				local ownerGuid = house:getOwnerGuid()
				if ownerGuid == 0 then break end

				local rent = house:getRent()
				if rent == 0 or house:getPaidUntil() > currentTime then break end

				if not house:getTown() then break end

				local player = Player(ownerGuid)
				if player then
					payRent(player, house, rent, rentPeriod, currentTime)
				else
					local offlinePlayer<close> = OfflinePlayer(ownerGuid)
					if offlinePlayer then payRent(player, house, rent, rentPeriod, currentTime) end
				end
			until true
		end
	end
end
