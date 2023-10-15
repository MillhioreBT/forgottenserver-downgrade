function onUpdateDatabase()	
	print("> Updating database to version 36 (rework on outfits(addons) & mount saving/loading)")

	db.query([[
		CREATE TABLE IF NOT EXISTS `player_outfits` (`player_id` int DEFAULT 0 NOT NULL,`outfit_id` smallint unsigned DEFAULT 0 NOT NULL,`addons` tinyint unsigned DEFAULT 0 NOT NULL,
		PRIMARY KEY (`player_id`,`outfit_id`),
		FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE)
		ENGINE=InnoDB DEFAULT CHARACTER SET=utf8;
	]])

	local outfitRange = 10001000

	local resultId = db.storeQuery(string.format("SELECT `player_id`, `value` FROM `player_storage` WHERE `key` >= %d AND `key` <= %d", outfitRange, outfitRange + 500))
	if resultId then
		repeat
			local playerId = result.getNumber(resultId, "player_id")
			local outfitId = bit.rshift(result.getNumber(resultId, "value"), 16)
			local addons = bit.band(result.getNumber(resultId, "value"), 0xFF)

			db.query(string.format("INSERT INTO `player_outfits` (`player_id`, `outfit_id`, `addons`) VALUES (%d, %d, %d)", playerId, outfitId, addons))
		until not result.next(resultId)
		result.free(resultId)
	end

	-- deleting all outfit & mount storages at once
	db.asyncQuery(string.format("DELETE FROM `player_storage` WHERE `key` >= %d AND `key` <= %d", outfitRange, outfitRange + 500))
	return true