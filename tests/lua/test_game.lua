test("convertIpToString reads network byte order", function()
	assertEqual(Game.convertIpToString(0x0100007F), "127.0.0.1")
	assertEqual(Game.convertIpToString(0xFFFFFFFF), "255.255.255.255")
	assertEqual(Game.convertIpToString(0), "0.0.0.0")
end)

test("getReverseDirection points back", function()
	assertEqual(Game.getReverseDirection(DIRECTION_NORTH), DIRECTION_SOUTH)
	assertEqual(Game.getReverseDirection(DIRECTION_EAST), DIRECTION_WEST)
	assertEqual(Game.getReverseDirection(DIRECTION_NORTHWEST), DIRECTION_SOUTHEAST)
	assertEqual(Game.getReverseDirection(DIRECTION_SOUTHWEST), DIRECTION_NORTHEAST)

	for direction = DIRECTION_NORTH, DIRECTION_NORTHEAST do
		assertEqual(Game.getReverseDirection(Game.getReverseDirection(direction)), direction)
	end
end)

test("reversed directions lead back to the start", function()
	local start = Position(100, 100, 7)
	for direction = DIRECTION_NORTH, DIRECTION_NORTHEAST do
		local position = Position(start.x, start.y, start.z)
		position:getNextPosition(direction)
		position:getNextPosition(Game.getReverseDirection(direction))
		assertEqual(position, start)
	end
end)

test("getSkillType maps weapons to their skill", function()
	assertEqual(Game.getSkillType(WEAPON_SWORD), SKILL_SWORD)
	assertEqual(Game.getSkillType(WEAPON_CLUB), SKILL_CLUB)
	assertEqual(Game.getSkillType(WEAPON_AXE), SKILL_AXE)
	assertEqual(Game.getSkillType(WEAPON_DISTANCE), SKILL_DISTANCE)
	assertEqual(Game.getSkillType(WEAPON_SHIELD), SKILL_SHIELD)
	assertEqual(Game.getSkillType(WEAPON_NONE), SKILL_FIST)
end)

test("getCountdownString starts at the largest unit", function()
	assertEqual(Game.getCountdownString(45), "45s")
	assertEqual(Game.getCountdownString(3661), "1h 1m 1s")
	assertEqual(Game.getCountdownString(90061), "1d 1h 1m 1s")
	assertEqual(Game.getCountdownString(0), "0s")
end)

test("getCountdownString long version spells out the units", function()
	assertEqual(Game.getCountdownString(3661, true), "1 hour 1 minute 1 second")
	assertEqual(Game.getCountdownString(7200, true), "2 hours 0 minutes 0 seconds")
end)

test("getCountdownString can hide the zero units", function()
	assertEqual(Game.getCountdownString(7200, true, true), "2 hours")
	assertEqual(Game.getCountdownString(86460, false, true), "1d 1m")
end)

test("getCountdownString of a negative duration is expired", function()
	assertEqual(Game.getCountdownString(-1), "expired")
end)
