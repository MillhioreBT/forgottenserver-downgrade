local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_ENERGYHIT)
combat:setArea(createCombatArea(AREA_BEAM5, AREADIAGONAL_BEAM5))

local function callback(player, level, magicLevel)
	local min = (level / 5) + (magicLevel * 1.8) + 11
	local max = (level / 5) + (magicLevel * 3) + 19
	return -min, -max
end

combat:setCallback(CallBackParam.LEVELMAGICVALUE, callback)

function onCastSpell(creature, variant) return combat:execute(creature, variant) end
