local event = Event()

function event.onRotateItem(player, item)
	local newId = item:getType():getRotateTo()
	if newId ~= 0 then item:transform(newId) end
end

event:register()
