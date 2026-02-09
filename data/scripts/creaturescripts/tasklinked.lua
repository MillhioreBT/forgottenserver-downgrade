local killEvent = CreatureEvent("TaskSystemOnKill")
function killEvent.onKill(player, target)
    if not player or not target:isMonster() then
        return true
    end

    local monsterName = target:getName()
    local actionId = TaskLinkedSystem.monsterToTask[monsterName:lower()]
    
    if not actionId then
        return true
    end
    
    local taskNumber = TaskLinkedSystem.getTaskNumber(actionId)
    local task = TaskLinkedSystem.tasks[actionId]
    local storage = PlayerStorageKeys.TaskLinked.storageBase + taskNumber
    local currentKills = math.max(0, player:getStorageValue(storage) or 0)
    local requiredKills = task.kills

    if currentKills >= requiredKills then
        return true
    end

    if not TaskLinkedSystem.isTaskActive(player, taskNumber) then
        return true
    end
    
    if not TaskLinkedSystem.canStartTask(player, taskNumber) then
        local prevTask = TaskLinkedSystem.getPreviousTask(taskNumber)
        if prevTask then
            player:sendTextMessage(MESSAGE_STATUS_WARNING, string.format("You need to complete the all task first!", prevTask.race))
        end
        return true
    end
    
    player:setStorageValue(storage, currentKills + 1)
    
    if (currentKills + 1) < requiredKills then
        player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, string.format("[Task System] You killed a %s (%s Task)", monsterName, task.race))
        player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, string.format("[%s Task] Progress: %d/%d kills", task.race, currentKills + 1, requiredKills))
    end
    
    if (currentKills + 1) == requiredKills then
        local reward = task.reward
        local expReward = tonumber(reward.exp) or 0
        if expReward > 0 then
            player:addExperience(expReward, true)
        end
        for _, item in ipairs(reward.items) do
            player:addItem(item.id, item.count)
        end
        
        player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s Task completed! You earned %d experience and items.", task.race, expReward))
        
        local allTasksCompleted = true
        for i = 1, TaskLinkedSystem.rooms[1].requiredTasks do
            if (player:getStorageValue(PlayerStorageKeys.TaskLinked.storageBase + i) or 0) < TaskLinkedSystem.tasks[49000 + i].kills then
                allTasksCompleted = false
                break
            end
        end
        
        if allTasksCompleted then
            player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "You completed all tasks! The final reward chest is now available.")
        end
    end
    
    return true
end
killEvent:register()

local leverAction = Action()
function leverAction.onUse(player, item, fromPos, target, toPos, isHotkey)
    local actionId = item:getActionId()
    local task = TaskLinkedSystem.tasks[actionId]
    
    if not task then
        player:sendTextMessage(MESSAGE_STATUS_WARNING, "This lever is not configured for any task.")
        return true
    end
    
    local taskNumber = TaskLinkedSystem.getTaskNumber(actionId)
    local storage = PlayerStorageKeys.TaskLinked.storageBase + taskNumber
    local kills = math.max(0, player:getStorageValue(storage) or 0)
    local requiredKills = task.kills
    
    if kills >= requiredKills then
        player:sendTextMessage(MESSAGE_STATUS_WARNING, "You already completed this task! Move to the next one.")
        return true
    end

    if not TaskLinkedSystem.canStartTask(player, taskNumber) then
        local prevTask = TaskLinkedSystem.getPreviousTask(taskNumber)
        if prevTask then
            player:sendTextMessage(MESSAGE_STATUS_WARNING, string.format("You need to complete the %s task first!", prevTask.race))
        else
            player:sendTextMessage(MESSAGE_STATUS_WARNING, "You need to complete the previous task first!")
        end
        return true
    end
    
    if not TaskLinkedSystem.isTaskActive(player, taskNumber) then
        TaskLinkedSystem.activateTask(player, taskNumber)
        player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s Task activated! You can now hunt %s monsters.", task.race, table.concat(task.monsters, ", ")))
    end
    
    local message = {
        string.format("== %s Task ==", task.race),
        "Monsters: "..table.concat(task.monsters, ", "),
        string.format("Progress: %d/%d kills", kills, requiredKills),
        string.format("Reward: %d experience", task.reward.exp),
        "Items:"
    }
    
    for _, item in ipairs(task.reward.items) do
        table.insert(message, string.format("- %d %s", item.count, ItemType(item.id):getName()))
    end
    
    local status
    if kills >= requiredKills then
        status = "COMPLETED"
    elseif TaskLinkedSystem.isTaskActive(player, taskNumber) then
        status = "ACTIVE"
    else
        status = "INACTIVE"
    end
    
    table.insert(message, string.format("Status: %s", status))
    
    player:showTextDialog(1950, table.concat(message, "\n"))
    return true
end

for actionId, _ in pairs(TaskLinkedSystem.tasks) do
    leverAction:aid(actionId)
end
leverAction:register()

local chestAction = Action()
function chestAction.onUse(player, item, fromPos, target, toPos, isHotkey)
    local totalTasks = 0
    for _ in pairs(TaskLinkedSystem.tasks) do
        totalTasks = totalTasks + 1
    end
    
    local completedTasks = 0
    for actionId, task in pairs(TaskLinkedSystem.tasks) do
        local taskNumber = TaskLinkedSystem.getTaskNumber(actionId)
        local storage = PlayerStorageKeys.TaskLinked.storageBase + taskNumber
        local kills = math.max(0, player:getStorageValue(storage) or 0)
        
        if kills >= task.kills then
            completedTasks = completedTasks + 1
        end
    end
    
    if completedTasks < totalTasks then
        player:sendTextMessage(MESSAGE_STATUS_WARNING, string.format("You need to complete all %d tasks before accessing this chest! (%d/%d completed)", totalTasks, completedTasks, totalTasks))
        return true
    end
    
    local chestStorage = PlayerStorageKeys.TaskLinked.roomStorageBase + 1
    if (player:getStorageValue(chestStorage) or 0) >= 1 then
        player:sendTextMessage(MESSAGE_STATUS_WARNING, "You already received the final reward!")
        return true
    end
    
    local finalReward = TaskLinkedSystem.rooms[1].finalReward
    if finalReward then
        local expReward = tonumber(finalReward.exp) or 0
        if expReward > 0 then
            player:addExperience(expReward, true)
        end
        
        for _, item in ipairs(finalReward.items) do
            player:addItem(item.id, item.count)
        end
        
        player:setStorageValue(chestStorage, 1)
        player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("Congratulations! You completed all %d tasks and received %d experience points!", completedTasks, expReward))
    else
        player:sendTextMessage(MESSAGE_STATUS_WARNING, "No final reward configured for your completion level.")
    end
    
    return true
end
chestAction:aid(PlayerStorageKeys.TaskLinked.chestActionId)
chestAction:register()

local loginEvent = CreatureEvent("TaskSystemLogin")
function loginEvent.onLogin(player)
    player:registerEvent("TaskSystemOnKill")
    return true
end
loginEvent:register()

local count = 0
for _ in pairs(TaskLinkedSystem.tasks) do count = count + 1 end
-- Task System loaded silently to avoid interfering with startup formatting