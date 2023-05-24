local function hasPendingReport(name, targetName, reportType)
	local f = io.open(string.format("data/reports/players/%s-%s-%d.txt", name,
	                                targetName, reportType), "r")
	if f then
		f:close()
		return true
	else
		return false
	end
end

local event = Event()

event.onReportRuleViolation = function(self, targetName, reportType,
                                       reportReason, comment, translation)
	local name = self:getName()
	if hasPendingReport(name, targetName, reportType) then
		self:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Your report is being processed.")
		return
	end

	local file = io.open(string.format("data/reports/players/%s-%s-%d.txt", name,
	                                   targetName, reportType), "a")
	if not file then
		self:sendTextMessage(MESSAGE_EVENT_ADVANCE,
		                     "There was an error when processing your report, please contact a gamemaster.")
		return
	end

	file:write("------------------------------\n")
	file:write("Reported by: " .. name .. "\n")
	file:write("Target: " .. targetName .. "\n")
	file:write("Type: " .. reportType .. "\n")
	file:write("Reason: " .. reportReason .. "\n")
	file:write("Comment: " .. comment .. "\n")
	if reportType ~= REPORT_TYPE_BOT then
		file:write("Translation: " .. translation .. "\n")
	end
	file:write("------------------------------\n")
	file:close()
	self:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format(
		                     "Thank you for reporting %s. Your report will be processed by %s team as soon as possible.",
		                     targetName,
		                     configManager.getString(configKeys.SERVER_NAME)))
end

event:register()
