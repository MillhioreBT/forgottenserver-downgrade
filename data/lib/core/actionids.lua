actionIds = {
	sandHole = 100, -- hidden sand hole
	pickHole = 105, -- hidden mud hole
	levelDoor = 1000, -- level door
	citizenship = 30020, -- citizenship teleport
	citizenshipLast = 30050 -- citizenship teleport last
}

-- Check duplicates actionIds
do
	local duplicates = {}
	for name, id in pairs(actionIds) do
		if duplicates[id] then error("Duplicate actionId: " .. id) end
		duplicates[id] = name
	end

	local __index = function(self, key)
		local aid = actionIds[key]
		if not aid then debugPrint("Invalid actionId: " .. key) end
		return aid
	end

	setmetatable(actionIds, {__index = __index})
end
