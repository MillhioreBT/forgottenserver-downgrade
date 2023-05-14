local tests = {
	npc = 1,
	monster = 2,
	item = 3,
}

local activeTest = tests.npc

local monsterName = "Rat"
local npcName = "Banker"
local itemId = 2148

local function runTest(playerPos)
	for i = 1, 5000 do
		if activeTest == tests.npc then
			local npc = Game.createNpc(npcName, playerPos)
			npc:remove()
		elseif activeTest == tests.monster then
			local monster = Game.createMonster(monsterName, playerPos)
			monster:remove()
		elseif activeTest == tests.item then
			local item = Game.createItem(itemId, 100, playerPos)
			item:remove()
		end
	end
end

function onSay(player, words, param)
	local playerPos = player:getPosition()

	local start = os.clock()
	runTest(playerPos)
	local reloadStart = os.clock()
	Game.reload(RELOAD_TYPE_TALKACTIONS)
	local testEnd = os.clock()
	print(
		"Test #1: Test: "
			.. reloadStart - start
			.. " - Reload: "
			.. testEnd - reloadStart
			.. " - Total: "
			.. testEnd - start
	)

	start = os.clock()
	runTest(playerPos)
	reloadStart = os.clock()
	Game.reload(RELOAD_TYPE_TALKACTIONS)
	testEnd = os.clock()
	print(
		"Test #2: Test: "
			.. reloadStart - start
			.. " - Reload: "
			.. testEnd - reloadStart
			.. " - Total: "
			.. testEnd - start
	)

	start = os.clock()
	runTest(playerPos)
	reloadStart = os.clock()
	Game.reload(RELOAD_TYPE_TALKACTIONS)
	testEnd = os.clock()
	print(
		"Test #3: Test: "
			.. reloadStart - start
			.. " - Reload: "
			.. testEnd - reloadStart
			.. " - Total: "
			.. testEnd - start
	)

	print(("-"):rep(50))
	return false
end
