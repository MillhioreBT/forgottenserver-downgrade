local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setArea(createCombatArea(AREA_SQUARE1X1))

local function callback(creature, target)
	return doChallengeCreature(creature, target)
end

combat:setCallback(CallBackParam.TARGETCREATURE, callback)

function onCastSpell(creature, variant) return combat:execute(creature, variant) end
