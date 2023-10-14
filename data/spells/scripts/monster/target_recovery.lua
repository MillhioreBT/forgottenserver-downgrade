local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_HEALING)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)

function onCastSpell(creature, variant)
	if creature:getHealth() <= 100000 then
		creature:setHealth(100000)
		return true
	end
	return combat:execute(creature, variant)
end
