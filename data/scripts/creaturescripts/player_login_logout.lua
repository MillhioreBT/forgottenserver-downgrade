local loginMessage = CreatureEvent("loginMessage")

function loginMessage.onLogin(player)
	print(string.format("\27[32m%s%s\27[0m", player:getName(),
	                       " has logged in."))

	-- Notify player about the rewards in their reward chest
	local rewardChest = player:getRewardChest()
	local rewardContainerCount = 0
	for _, item in ipairs(rewardChest:getItems()) do
		if item:getId() == ITEM_REWARD_CONTAINER then
			rewardContainerCount = rewardContainerCount + 1
		end
	end
	if rewardContainerCount > 0 then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, string.format("You have %d reward%s in your reward chest.", rewardContainerCount, rewardContainerCount > 1 and "s" or ""))
	end

	return true
end

loginMessage:register()

local logoutMessage = CreatureEvent("logoutMessage")

function logoutMessage.onLogout(player)
	print(string.format("\27[31m%s%s\27[0m", player:getName(),
	                       " has logged out."))
	return true
end

logoutMessage:register()
