test("positions add and subtract coordinate by coordinate", function()
	assertEqual(Position(100, 200, 7) + Position(1, -2, 0), Position(101, 198, 7))
	assertEqual(Position(100, 200, 7) - Position(1, 2, 1), Position(99, 198, 6))
	assertEqual(Position(100, 200, 7) + {x = 5}, Position(105, 200, 7))
end)

test("positions are equal when x, y and z are", function()
	assertTrue(Position(1, 2, 3) == Position(1, 2, 3))
	assertFalse(Position(1, 2, 3) == Position(1, 2, 4))
end)

test("positions print their coordinates", function()
	assertEqual(tostring(Position(32369, 32241, 7)), "Position(32369, 32241, 7)")
	assertEqual(Position(1, 2, 3) .. Position(4, 5, 6), "Position(1, 2, 3)Position(4, 5, 6)")
end)

test("getDistance is the largest offset on any axis", function()
	local from = Position(100, 100, 7)
	assertEqual(from:getDistance(Position(103, 98, 7)), 3)
	assertEqual(from:getDistance(Position(100, 100, 7)), 0)
	assertEqual(from:getDistance(Position(101, 101, 3)), 4)
end)

test("getNextPosition moves the position itself", function()
	local position = Position(100, 100, 7)
	local returned = position:getNextPosition(DIRECTION_NORTHEAST)
	assertEqual(position, Position(101, 99, 7))
	assertTrue(rawequal(returned, position))

	position:getNextPosition(DIRECTION_SOUTH, 3)
	assertEqual(position, Position(101, 102, 7))
end)

test("getNextPosition with an unknown direction stays put", function()
	local position = Position(100, 100, 7)
	position:getNextPosition(DIRECTION_NONE)
	assertEqual(position, Position(100, 100, 7))
end)

test("every direction offset is one step", function()
	for direction = DIRECTION_NORTH, DIRECTION_NORTHEAST do
		local offset = Position.directionOffset[direction]
		assertEqual(math.max(math.abs(offset.x), math.abs(offset.y)), 1)
	end
end)

test("isInRange accepts the corners in any order", function()
	local inside = Position(105, 105, 7)
	assertTrue(inside:isInRange(Position(100, 100, 7), Position(110, 110, 7)))
	assertTrue(inside:isInRange(Position(110, 110, 7), Position(100, 100, 7)))
	assertTrue(Position(110, 100, 7):isInRange(Position(100, 100, 7), Position(110, 110, 7)))
	assertFalse(Position(111, 105, 7):isInRange(Position(100, 100, 7), Position(110, 110, 7)))
	assertFalse(inside:isInRange(Position(100, 100, 6), Position(110, 110, 6)))
end)

test("getDistanceBetween adds 15 across floors", function()
	assertEqual(getDistanceBetween(Position(100, 100, 7), Position(104, 101, 7)), 4)
	assertEqual(getDistanceBetween(Position(100, 100, 7), Position(104, 101, 6)), 19)
end)
