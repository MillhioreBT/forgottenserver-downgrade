local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_SOUND_PURPLE)
combat:setArea(createCombatArea(AREA_SQUAREWAVE6))

local parameters = {
	{key = CONDITION_PARAM_TICKS, value = 8 * 1000},
	{key = CONDITION_PARAM_SKILL_SHIELDPERCENT, value = 85}
}

local function callback(creature, target)
	target:addAttributeCondition(parameters)
end

combat:setCallback(CallBackParam.TARGETCREATURE, callback)

function onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

combat:setCallback(CallBackParam.TARGETCREATURE, callback)