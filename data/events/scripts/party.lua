function Party:onJoin(player)
	if hasEvent.onJoin then
		return Event.onJoin(self, player)
	end
	return true
end

function Party:onLeave(player)
	if hasEvent.onLeave then
		return Event.onLeave(self, player)
	end
	return true
end

function Party:onDisband()
	if hasEvent.onDisband then
		return Event.onDisband(self)
	end
	return true
end

function Party:onShareExperience(exp)
	if hasEvent.onShareExperience then
		return Event.onShareExperience(self, exp, exp)
	end
	return exp
end
