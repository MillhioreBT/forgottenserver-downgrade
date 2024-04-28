local fmt = string.format

function onSay(player, words, param)

	local desc = {"Server Info:\n"}

	-- Global Rates
	desc[#desc + 1] = fmt("Exp rate: %s", Game.getExperienceStage(player:getLevel()))
	desc[#desc + 1] = fmt("Skill rate: %s", configManager.getNumber(configKeys.RATE_SKILL))
	desc[#desc + 1] = fmt("Magic rate: %s", configManager.getNumber(configKeys.RATE_MAGIC))
	desc[#desc + 1] = fmt("Loot rate: %s", configManager.getNumber(configKeys.RATE_LOOT))

	-- Player Rates
	desc[#desc + 1] = fmt("XP rate base: %+d%%", player:getExperienceRate(ExperienceRateType.BASE) - 100)
	desc[#desc + 1] = fmt("XP rate low level: %+d%%", player:getExperienceRate(ExperienceRateType.LOW_LEVEL) - 100)
	desc[#desc + 1] = fmt("XP rate bonus: %+d%%", player:getExperienceRate(ExperienceRateType.BONUS) - 100)
	desc[#desc + 1] = fmt("XP rate stamina: %+d%%", player:getExperienceRate(ExperienceRateType.STAMINA) - 100)

	player:popupFYI(table.concat(desc, "\n"))
	return false
end
