--[[
Reserved storage ranges:
- 300000 to 301000+ reserved for achievements
- 20000 to 21000+ reserved for achievement progress
- 10000000 to 20000000 reserved for outfits and mounts on source
]] --
PlayerStorageKeys = {
	annihilatorReward = 30015,
	promotion = 30018,
	delayLargeSeaShell = 30019,
	firstRod = 30020,
	delayWallMirror = 30021,
	madSheepSummon = 30023,
	crateUsable = 30024,
	achievementsBase = 300000,
	achievementsCounter = 20000
}

GlobalStorageKeys = {}

-- Check duplicates player storage keys
do
	local duplicates = {}
	for name, id in pairs(PlayerStorageKeys) do
		if duplicates[id] then error("Duplicate keyStorage: " .. id) end
		duplicates[id] = name
	end

	local __index = function(self, key)
		local keyStorage = PlayerStorageKeys[key]
		if not keyStorage then debugPrint("Invalid keyStorage: " .. key) end
		return keyStorage
	end

	setmetatable(PlayerStorageKeys, {__index = __index})
end

-- Check duplicates global storage keys
do
	local duplicates = {}
	for name, id in pairs(GlobalStorageKeys) do
		if duplicates[id] then error("Duplicate keyStorage: " .. id) end
		duplicates[id] = name
	end

	local __index = function(self, key)
		local keyStorage = GlobalStorageKeys[key]
		if not keyStorage then debugPrint("Invalid keyStorage: " .. key) end
		return keyStorage
	end

	setmetatable(GlobalStorageKeys, {__index = __index})
end
