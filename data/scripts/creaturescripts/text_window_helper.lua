if not TextWindows then TextWindows = {} end

local textWindow = {}
textWindow.__index = textWindow

function TextWindow(window) return setmetatable(window, textWindow) end

function textWindow.setCallback(window, callback)
	window.callback = callback
	return true
end

function textWindow.setItemId(window, itemId)
	window.itemId = itemId
	return true
end

function textWindow.setText(window, text)
	window.text = text
	return true
end

function textWindow.setCanWrite(window, canWrite)
	window.canWrite = canWrite
	return true
end

function textWindow.setLength(window, length)
	window.length = length
	return true
end

function textWindow.sendToPlayer(window, player)
	player:registerEvent("TextWindowHelper")
	window.id = player:showTextDialog(window.itemId, window.text, window.canWrite,
	                                  window.length)
	TextWindows[player:getGuid()] = window
	return window.id
end

local textWindowHelper = CreatureEvent("TextWindowHelper")

function textWindowHelper.onTextEdit(player, item, text, windowTextId)
	player:unregisterEvent("TextWindowHelper")
	local playerGuid = player:getGuid()
	local window = TextWindows[playerGuid]
	TextWindows[playerGuid] = nil
	if window and window.id == windowTextId and window.callback then
		return window.callback(player, item, text)
	end
	return true
end

textWindowHelper:register()
