function Combat:getPositions(creature, variant)
	local positions = {}
	local function callback(creature, position) positions[#positions + 1] = position end

	self:setCallback(CallBackParam.TARGETTILE, callback)
	self:execute(creature, variant)
	return positions
end

function Combat:getTargets(creature, variant)
	local targets = {}
	local function callback(creature, target) targets[#targets + 1] = target end

	self:setCallback(CallBackParam.TARGETCREATURE, callback)
	self:execute(creature, variant)
	return targets
end
