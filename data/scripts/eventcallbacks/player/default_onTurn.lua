local tryFloors = {1, 1, -3, -1}
local exhaustTime = 100 -- Miliseconds

local function getNextStep(pos, direction)
	local nextPos = Position(pos)
	nextPos:getNextPosition(direction)
	local nextTile = Tile(nextPos)
	if not nextTile then
		for _, try in ipairs(tryFloors) do
			nextPos.z = nextPos.z + try
			nextTile = Tile(nextPos)
			if nextTile then
				return nextPos
			end
		end
	end
	return nextPos
end

local exhaustTable = {}

local event = Event()

function event.onTurn(player, direction)
	if not player:getGroup():getAccess() then
		return true
	end

	local pos = player:getPosition()
	local nextPos = getNextStep(pos, direction)
	if nextPos == pos then
		return true
	end

	local playerGuid = player:getGuid()
	local exhaust = exhaustTable[playerGuid]
	if exhaust then
		if exhaust > os.mtime() then
			return true
		end
		exhaustTable[playerGuid] = nil
	end

	player:teleportTo(nextPos, true)
	exhaustTable[playerGuid] = os.mtime() + exhaustTime
	return true
end

event:register()
