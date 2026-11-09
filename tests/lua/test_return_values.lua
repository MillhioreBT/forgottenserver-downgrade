test("the conjure, spear, raid and move ReturnValues are registered", function()
	local names = {
		"RETURNVALUE_CANNOTCONJUREITEMHERE", "RETURNVALUE_YOUNEEDTOSPLITYOURSPEARS",
		"RETURNVALUE_NOSUCHRAIDEXISTS", "RETURNVALUE_ANOTHERRAIDISALREADYEXECUTING",
		"RETURNVALUE_ITEMCANNOTBEMOVEDTHERE", "RETURNVALUE_CREATURENOTMOVEABLE"
	}
	for _, name in ipairs(names) do
		assertEqual(name .. " " .. type(_G[name]), name .. " number")
	end
end)
