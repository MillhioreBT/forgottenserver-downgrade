local loginMessage = CreatureEvent("loginMessage")

function loginMessage.onLogin(player)
    io.write(player:getName(), " has logged in.\n")
    return true
end

loginMessage:register()

local logoutMessage = CreatureEvent("logoutMessage")

function logoutMessage.onLogout(player)
    io.write(player:getName(), " has logged out.\n")
    return true
end

logoutMessage:register()
