local loginMessage = CreatureEvent("loginMessage")

function loginMessage.onLogin(player)
	print(string.format("\27[32m%s%s\27[0m", player:getName(),
	                       " has logged in."))
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
