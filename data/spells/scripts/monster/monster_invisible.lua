local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)

local condition = Condition(CONDITION_INVISIBLE)
condition:setParameter(CONDITION_PARAM_TICKS, 10000)
combat:addCondition(condition)

function onCastSpell(creature, variant)
	local position = creature:getPosition()
	if position:isInRange(Position(1047, 1015, 1), Position(1069, 1027, 13)) then
		return true
	end
	return combat:execute(creature, variant)
end
