function Tile:getParent() return nil end

function Tile:getCreature() return nil end

function Tile:getPlayer() return nil end

function Tile:getMonster() return nil end

function Tile:getNpc() return nil end

function Tile:getItem() return nil end

function Tile:getContainer() return nil end

function Tile:getTeleport() return nil end

function Tile:isCreature() return false end

function Tile:isPlayer() return false end

function Tile:isItem() return false end

function Tile:isMonster() return false end

function Tile:isNpc() return false end

function Tile:isTeleport() return false end

function Tile:isTile() return true end

function Tile:isContainer() return false end

function Tile:isHouse() return self:getHouse() ~= nil end

function Tile:relocateTo(toPosition)
	if self:getPosition() == toPosition or not Tile(toPosition) then return false end

	for i = self:getThingCount() - 1, 0, -1 do
		local thing = self:getThing(i)
		if thing then
			local item = thing:getItem()
			if item then
				if item:getFluidType() ~= 0 then
					item:remove()
				elseif ItemType(item:getId()):isMovable() then
					item:moveTo(toPosition)
				end
			else
				local creature = thing:getCreature()
				if creature then creature:teleportTo(toPosition) end
			end
		end
	end

	return true
end

function Tile:isWalkable()
	local ground = self:getGround()
	if not ground or ground:hasProperty(CONST_PROP_BLOCKSOLID) then return false end

	local items = self:getItems()
	for i = 1, self:getItemCount() do
		local item = items[i]
		local itemType = item:getType()
		if itemType:getType() ~= ITEM_TYPE_MAGICFIELD and not itemType:isMovable() and
			item:hasProperty(CONST_PROP_BLOCKSOLID) then return false end
	end
	return true
end

function Tile:getTopPlayer()
	local creature = self:getTopCreature()
	return creature and creature:getPlayer()
end
