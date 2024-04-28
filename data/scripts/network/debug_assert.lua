local debugAssert = PacketHandler(0xE8)

function debugAssert.onReceive(player, msg)
	if player:hasDebugAssert() then return end

	local assertLine = msg:getString()
	local date = msg:getString()
	local description = msg:getString()
	local comment = msg:getString()
	Game.saveDebugAssert(player:getGuid(), assertLine, date, description, comment)
end

debugAssert:register()
