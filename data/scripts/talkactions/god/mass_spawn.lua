local massSpawn = TalkAction("/mass")

function massSpawn.onSay(player, words, param)
    -- Usage: /mass monsterName, amount
    local split = param:split(",")
    if #split < 2 then
        player:sendCancelMessage("Insufficient parameters. Usage: /mass monsterName, amount")
        return false
    end

    local monsterName = split[1]:trim()
    local amount = tonumber(split[2]:trim())

    if not amount then
        player:sendCancelMessage("Invalid amount.")
        return false
    end

    local monsterType = MonsterType(monsterName)
    if not monsterType then
        player:sendCancelMessage("Monster not found: " .. monsterName)
        return false
    end

    if amount > 5000 then
        player:sendCancelMessage("Amount too high. Limit is 5000.")
        return false
    end

    local playerPos = player:getPosition()
    local spawnedCount = 0
    local range = 50 -- Radius to spawn monsters around the player

    for i = 1, amount do
        local randomPos = Position(
            playerPos.x + math.random(-range, range),
            playerPos.y + math.random(-range, range),
            playerPos.z
        )
        
        -- Try to find a valid tile
        local tile = Tile(randomPos)
        if tile and not tile:hasFlag(TILESTATE_BLOCKSOLID) and not tile:hasFlag(TILESTATE_PROTECTIONZONE) then
             Game.createMonster(monsterName, randomPos, true, true)
             spawnedCount = spawnedCount + 1
        end
    end

    player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "Spawned " .. spawnedCount .. " " .. monsterName .. "(s) around you.")
    return false
end

massSpawn:separator(" ")
massSpawn:register()
