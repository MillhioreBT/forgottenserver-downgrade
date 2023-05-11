local f = io.open("luascript.cpp", "r")
if f then
	local lines = {}
	for line in io.lines("luascript.cpp") do
		lines[#lines + 1] = line
	end

	f:close()

	local founds = {}

	for _, line in pairs(lines) do
		for word in line:gmatch("(registerEnum%(.-%))") do
			print(word .. ";")
		end
	end
end
