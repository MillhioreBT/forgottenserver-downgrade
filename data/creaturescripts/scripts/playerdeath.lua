local deathListEnabled = true
local maxDeathRecords = 5
local playerDeathQuery =
	"INSERT INTO `player_deaths` (`player_id`, `time`, `level`, `killed_by`, `is_player`, `mostdamage_by`, `mostdamage_is_player`, `unjustified`, `mostdamage_unjustified`) VALUES (%d, %d, %d, %s, %d, %s, %d, %d, %d)"

local format = string.format

---@param killer Creature
---@return boolean, string
local function getKiller(killer)
	if not killer then return false, "field item" end

	if killer:isPlayer() then return true, killer:getName() end

	local master = killer:getMaster()
	if master and master ~= killer and master:isPlayer() then return true, master:getName() end

	return false, killer:getName()
end

---@param playerId integer
---@param playerName string
---@param killerId integer
---@param playerGuid integer
---@param byPlayer boolean
---@param killerName string
---@param playerGuildId integer
---@param killerGuildId integer
---@param timeNow integer
---@return nil
local function playerDeathSuccess(playerId, playerName, killerId, playerGuid, byPlayer, killerName, playerGuildId, killerGuildId,
                                  timeNow)
	local resultId = db.storeQuery("SELECT `player_id` FROM `player_deaths` WHERE `player_id` = " .. playerGuid)
	if not resultId then return end

	local deathRecords = 0
	local tmpResultId = true
	while tmpResultId do
		tmpResultId = result.next(resultId)
		deathRecords = deathRecords + 1
	end

	result.free(resultId)

	local limit = deathRecords - maxDeathRecords
	if limit > 0 then
		db.asyncQuery(format("DELETE FROM `player_deaths` WHERE `player_id` = %d ORDER BY `time` LIMIT %d", playerGuid, limit))
	end

	if byPlayer then
		if playerGuildId ~= 0 then
			if killerGuildId ~= 0 and playerGuildId ~= killerGuildId and isInWar(playerId, killerId) then
				resultId = db.storeQuery(format(
					                         "SELECT `id` FROM `guild_wars` WHERE `status` = 1 AND ((`guild1` = %d AND `guild2` = %d) OR (`guild1` = %d AND `guild2` = %d))",
					                         killerGuildId, playerGuildId, playerGuildId, killerGuildId))

				local warId = nil
				if resultId then
					warId = result.getNumber(resultId, "id")
					result.free(resultId)
				end

				if warId then
					db.asyncQuery(format(
						              "INSERT INTO `guildwar_kills` (`killer`, `target`, `killerguild`, `targetguild`, `time`, `warid`) VALUES (%s, %s, %d, %d, %d, %d)",
						              db.escapeString(killerName), db.escapeString(playerName), killerGuildId, playerGuildId, timeNow, warId))
				end
			end
		end
	end
end

function onDeath(player, corpse, killer, mostDamageKiller, lastHitUnjustified, mostDamageUnjustified)
	local playerId = player:getId()
	nextUseStaminaTime[playerId] = nil

	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "You are dead.")
	if not deathListEnabled then return end

	local timeNow = os.time()
	local byPlayer, killerName = getKiller(killer)
	local byPlayerMostDamage, killerNameMostDamage = getKiller(mostDamageKiller)
	local playerGuid = player:getGuid()
	local playerName = player:getName()
	local playerGuild = player:getGuild()
	local playerGuildId = playerGuild and playerGuild:getId() or 0
	local killerGuild = byPlayer and killer:getGuild() or nil
	local killerGuildId = killerGuild and killerGuild:getId() or 0
	local killerId = byPlayer and killer:getId() or 0
	db.asyncQuery(format(playerDeathQuery, playerGuid, timeNow, player:getLevel(), db.escapeString(killerName), byPlayer and 1 or 0,
	                     db.escapeString(killerNameMostDamage), byPlayerMostDamage and 1 or 0, lastHitUnjustified and 1 or 0,
	                     mostDamageUnjustified and 1 or 0), function(success)
		if success then
			playerDeathSuccess(playerId, playerName, killerId, playerGuid, byPlayer, killerName, playerGuildId, killerGuildId, timeNow)
		end
	end)
end
