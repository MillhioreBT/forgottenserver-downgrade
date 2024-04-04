local event = GlobalEvent("WorldTime")

-- 1h realtime = 1day worldtime
-- 2.5s realtime = 1min worldtime
-- worldTime is calculated in minutes

function event.onTime(interval)
	local currentTime = os.time()
	local sec = os.date("%S", currentTime)
	local min = os.date("%M", currentTime)
	local worldTime = math.floor((sec + (min * 60)) / 2.5)
	Game.setWorldTime(worldTime)
	return true
end

event:interval(2500) -- 2.5 seconds
event:register()
