local quests = {}
local missions = {}

Quest = {}
Quest.__index = Quest

function Quest:register()
	self.id = #quests + 1
	for _, mission in pairs(self.missions) do
		mission.id = #missions + 1
		mission.questId = self.id
		missions[mission.id] = setmetatable(mission, Mission)
	end

	quests[self.id] = self
	return true
end

function Quest:isStarted(player)
	return player:getStorageValue(self.storageId, 0) >= self.storageValue
end

function Quest:isCompleted(player)
	for _, mission in pairs(self.missions) do
		if not mission:isCompleted(player) then return false end
	end
	return true
end

function Quest:getMissions(player)
	local playerMissions = {}
	for _, mission in pairs(self.missions) do
		if mission:isStarted(player) then playerMissions[#playerMissions + 1] = mission end
	end
	return playerMissions
end

Mission = {}
Mission.__index = Mission

function Mission:isStarted(player)
	local value = player:getStorageValue(self.storageId, 0)
	if value >= self.startValue then
		if self.ignoreEndValue or value <= self.endValue then return true end
	end
	return false
end

function Mission:isCompleted(player)
	if self.ignoreEndValue then
		return player:getStorageValue(self.storageId, 0) >= self.endValue
	end
	return player:getStorageValue(self.storageId, 0) == self.endValue
end

function Mission:getName(player)
	if self:isCompleted(player) then return string.format("%s (Completed)", self.name) end
	return self.name
end

function Mission:getDescription(player)
	local descriptionType = type(self.description)
	if descriptionType == "function" then return self.description(player) end

	local value = player:getStorageValue(self.storageId, 0)
	if descriptionType == "string" then
		local description = self.description:gsub("|STATE|", value)
		description = self.description:gsub("\\n", "\n")
		return description
	end

	if descriptionType == "table" then
		if self.ignoreEndValue then
			for current = self.endValue, self.startValue, -1 do
				if value >= current then return self.description[current] end
			end
		else
			for current = self.endValue, self.startValue, -1 do
				if value == current then return self.description[current] end
			end
		end
	end

	return "An error has occurred, please contact a gamemaster."
end

function Game.getQuests() return quests end
function Game.getMissions() return missions end

function Game.getQuestById(id) return quests[id] end
function Game.getMissionById(id) return missions[id] end

function Game.clearQuests()
	quests = {}
	missions = {}
	return true
end

function Game.createQuest(name, quest)
	if not isScriptsInterface() then return end

	if type(quest) == "table" then
		setmetatable(quest, Quest)
		quest.id = -1
		quest.name = name
		if type(quest.missions) ~= "table" then quest.missions = {} end

		return quest
	end

	quest = setmetatable({}, Quest)
	quest.id = -1
	quest.name = name
	quest.storageId = 0
	quest.storageValue = 0
	quest.missions = {}
	return quest
end

function Game.isQuestStorage(key, value, oldValue)
	for _, quest in pairs(quests) do
		if quest.storageId == key and quest.storageValue == value then return true end
	end

	for _, mission in pairs(missions) do
		if mission.storageId == key and value >= mission.startValue and value <=
			mission.endValue then
			return not mission.description or oldValue < mission.startValue or oldValue >
				       mission.endValue
		end
	end
	return false
end

function Player:getQuests()
	local playerQuests = {}
	for _, quest in pairs(quests) do
		if quest:isStarted(self) then playerQuests[#playerQuests + 1] = quest end
	end
	return playerQuests
end

function Player:sendQuestLog()
	local msg<close> = NetworkMessage()
	msg:addByte(0xF0)
	local quests = self:getQuests()
	msg:addU16(#quests)

	for _, quest in pairs(quests) do
		msg:addU16(quest.id)
		msg:addString(quest.name)
		msg:addByte(quest:isCompleted(self) and 1 or 0)
	end

	msg:sendToPlayer(self)
	return true
end

function Player:sendQuestLine(quest)
	local msg<close> = NetworkMessage()
	msg:addByte(0xF1)
	msg:addU16(quest.id)
	local missions = quest:getMissions(self)
	msg:addByte(#missions)

	for _, mission in pairs(missions) do
		msg:addString(mission:getName(self))
		msg:addString(mission:getDescription(self))
	end

	msg:sendToPlayer(self)
	return true
end
