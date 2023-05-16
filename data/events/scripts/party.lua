function Party:onJoin(player)
	if hasEvent.onJoin then return Event.onJoin(self, player) end
	return true
end

function Party:onLeave(player)
	if hasEvent.onLeave then return Event.onLeave(self, player) end
	return true
end

function Party:onDisband()
	if hasEvent.onDisband then return Event.onDisband(self) end
	return true
end

function Party:onShareExperience(exp)
	if hasEvent.onShareExperience then
		return Event.onShareExperience(self, exp, exp)
	end
	return exp
end

function Party:onInvite(player)
	if hasEvent.onInvite then return Event.onInvite(self, player) end
	return true
end

function Party:onRevokeInvitation(player)
	if hasEvent.onRevokeInvitation then
		return Event.onRevokeInvitation(self, player)
	end
	return true
end

function Party:onPassLeadership(player)
	if hasEvent.onPassLeadership then return Event.onPassLeadership(self, player) end
	return true
end
