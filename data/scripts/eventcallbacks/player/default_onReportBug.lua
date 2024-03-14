local event = Event()

event.onReportBug = function(player, message)
	if player:getAccountType() == ACCOUNT_TYPE_NORMAL then return false end

	local position = player:getPosition()
	local category = BUG_CATEGORY_OTHER
	local name = player:getName()
	local file = io.open("data/reports/bugs/" .. name .. " report.txt", "a")

	if not file then
		player:sendTextMessage(MESSAGE_EVENT_DEFAULT, "There was an error when processing your report, please contact a gamemaster.")
		return true
	end

	file:write("------------------------------\n")
	file:write("Name: " .. name)
	if category == BUG_CATEGORY_MAP then
		file:write(" [Map position: " .. position.x .. ", " .. position.y .. ", " .. position.z .. "]")
	end
	local playerPosition = player:getPosition()
	file:write(" [Player Position: " .. playerPosition.x .. ", " .. playerPosition.y .. ", " .. playerPosition.z .. "]\n")
	file:write("Comment: " .. message .. "\n")
	file:close()

	player:sendTextMessage(MESSAGE_EVENT_DEFAULT,
	                       "Your report has been sent to " .. configManager.getString(configKeys.SERVER_NAME) .. ".")
	return true
end

event:register()
