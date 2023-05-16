local log = PacketHandler(0xF0)

function log.onReceive(player, msg) player:sendQuestLog() end

log:register()

local line = PacketHandler(0xF1)

function line.onReceive(player, msg)
	local quest = Game.getQuestById(msg:getU16())
	if quest then player:sendQuestLine(quest) end
end

line:register()
