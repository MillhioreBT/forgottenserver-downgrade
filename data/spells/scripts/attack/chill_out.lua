local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_ICEDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_ICEAREA)
combat:setArea(createCombatArea(AREA_WAVE4))

local function callback(player, level, magicLevel)
	level = math.min(level, 20)
	magicLevel = math.min(magicLevel, 20)
	local min = (level / 5) + (magicLevel * 0.3) + 2
	local max = (level / 5) + (magicLevel * 0.45) + 3
	return -min, -max
end

combat:setCallback(CallBackParam.LEVELMAGICVALUE, callback)

function onCastSpell(creature, variant) return combat:execute(creature, variant) end
