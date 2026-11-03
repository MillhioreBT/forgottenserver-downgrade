local tests = {}

function test(name, body) tests[#tests + 1] = {name = name, body = body} end

local function describe(value)
	if type(value) == "string" then return string.format("%q", value) end
	return tostring(value)
end

function assertEqual(actual, expected)
	if actual ~= expected then
		error(string.format("expected %s, got %s", describe(expected), describe(actual)), 2)
	end
end

function assertTrue(value)
	if value ~= true then error("expected true, got " .. describe(value), 2) end
end

function assertFalse(value)
	if value ~= false then error("expected false, got " .. describe(value), 2) end
end

function runTests()
	local failures = {}
	for _, case in ipairs(tests) do
		local ok, message = pcall(case.body)
		if not ok then failures[#failures + 1] = case.name .. ": " .. tostring(message) end
	end
	return #tests, failures
end
