-- #region Configuration
local ACCOUNT_MANAGER_ACCOUNT_ID = 1
local ACCOUNT_MANAGER_STORAGE = {}
local ACCOUNT_MANAGER_LOGOUT_DELAY = 1 * 60 -- 1 minute

local ACCOUNT_MANAGER_LOGINS = {}
local ACCOUNT_MANAGER_LOGIN_TRIES = 5
local ACCOUNT_MANAGER_LOGIN_TIMEOUT = 30 -- 30 seconds
local ACCOUNT_MANAGER_BAN_TIME = 1 * 60 -- 10 minutes

local CREATE_ACCOUNT_TABLE = {}
local CREATE_ACCOUNT_EXHAUST = 30 * 60 -- 30 minutes

local ACCOUNT_NAME_MIN_LENGTH = 3
local ACCOUNT_NAME_MAX_LENGTH = 20
local ACCOUNT_NAME_WITH_NUMBERS = false
local ACCOUNT_NAME_WITH_SPECIAL_CHARACTERS = false

local PASSWORD_MIN_LENGTH = 6
local PASSWORD_MAX_LENGTH = 30
local PASSWORD_WITH_NUMBERS = true
local PASSWORD_WITH_CAPITAL_LETTERS = true
local PASSWORD_WITH_SPECIAL_CHARACTERS = false
local PASSWORD_WITH_SPACES = false

local CREATE_CHARACTER_TABLE = {}
local CREATE_CHARACTER_EXHAUST = 3 * 60 -- 3 minutes

local CHARACTER_NAME_FORMAT = "[a-zA-Z%s]+"
local CHARACTER_NAME_MIN_LENGTH = 3
local CHARACTER_NAME_MAX_LENGTH = 20
local CHARACTER_NAME_MAX_WORDS = 3

local CHARACTER_DEFAULT = {
	TOWN = 1,
	POSX = 1000,
	POSY = 1000,
	POSZ = 7,
	LEVEL = 8,
	HEALTH = 185,
	MANA = 35,
	CAPACITY = 420,
	SKILL = 10,
	STAMINA = 2520,
	SOUL = 100,
	MAGIC_LEVEL = 0,
	OUTFIT_MALE = 128,
	OUTFIT_FEMALE = 136,
	OUTFIT_ADDONS = 0,
	OUTFIT_HEAD = 0,
	OUTFIT_BODY = 0,
	OUTFIT_LEGS = 0,
	OUTFIT_FEET = 0
}
-- #endregion

---@param player Player
---@return function
local function sender(player)
	return function(text, ...)
		player:say(string.format(text, ...), TALKTYPE_PRIVATE_NP, false, player,
		           player:getPosition())
	end
end

---@param player Player
---@return table
local function stater(player)
	local playerId = player:getId()
	return setmetatable({}, {
		__index = function(_, key)
			return ACCOUNT_MANAGER_STORAGE[playerId] and
				       ACCOUNT_MANAGER_STORAGE[playerId][key]
		end,
		__newindex = function(_, key, value)
			if not ACCOUNT_MANAGER_STORAGE[playerId] then
				ACCOUNT_MANAGER_STORAGE[playerId] = {}
			end

			ACCOUNT_MANAGER_STORAGE[playerId][key] = value
		end
	})
end

---@param accountName string
---@return number
local function getAccountIdByAccountName(accountName)
	local dbResult = db.storeQuery("SELECT `id` FROM `accounts` WHERE `name` = " ..
		                               db.escapeString(accountName) .. " LIMIT 1;")
	if dbResult then
		local accountId = result.getNumber(dbResult, "id")
		result.free(dbResult)
		return accountId
	end
	return 0
end

---@param account table
---@param IP number
---@return string, number?
local function CreateAccount(account, IP)
	if getAccountIdByAccountName(account.accountName) ~= 0 then
		return "already exists"
	end

	local timeNow = os.time()
	if CREATE_ACCOUNT_TABLE[IP] then
		if CREATE_ACCOUNT_TABLE[IP] > timeNow then return "cooldown" end
		CREATE_ACCOUNT_TABLE[IP] = nil
	end

	local dbResult = db.query(string.format(
		                          "INSERT INTO `accounts` (`name`, `password`, `creation`) VALUES (%s, HEX(%s), %d);",
		                          db.escapeString(account.accountName),
		                          db.escapeString(transformToSHA1(account.password)),
		                          timeNow))
	if dbResult then
		local resultId = db.storeQuery("SELECT `id` FROM `accounts` WHERE `name` = " ..
			                         db.escapeString(account.accountName) .. " LIMIT 1;")
		if not resultId then error("CreateAccount - can't get account id") end

		local accountId = result.getNumber(resultId, "id")
		result.free(resultId)
		if not CREATE_ACCOUNT_TABLE[IP] then
			CREATE_ACCOUNT_TABLE[IP] = timeNow + CREATE_ACCOUNT_EXHAUST
		end
		return "success", accountId
	end
	return "failed"
end

---@param playerName string
---@return number
local function getPlayerIdByPlayerName(playerName)
	local dbResult = db.storeQuery("SELECT `id` FROM `players` WHERE `name` = " ..
		                               db.escapeString(playerName) .. " LIMIT 1;")
	if dbResult then
		local playerId = result.getNumber(dbResult, "id")
		result.free(dbResult)
		return playerId
	end
	return 0
end

---@param character table
---@param IP number
---@return string
local function CreateCharacter(character, IP)
	if getPlayerIdByPlayerName(character.name) ~= 0 then return "already exists" end

	local timeNow = os.time()
	if CREATE_CHARACTER_TABLE[IP] then
		if CREATE_CHARACTER_TABLE[IP] > timeNow then return "cooldown" end
		CREATE_CHARACTER_TABLE[IP] = nil
	end

	local lookType = character.sex == PLAYERSEX_FEMALE and
		                 CHARACTER_DEFAULT.OUTFIT_FEMALE or
		                 CHARACTER_DEFAULT.OUTFIT_MALE
	local dbResult = db.query(string.format(
		                          "INSERT INTO `players` (`name`, `account_id`, `vocation`, `health`, `healthmax`, `lookbody`, `lookfeet`, `lookhead`, `looklegs`, `looktype`, `lookaddons`, `maglevel`, `mana`, `manamax`, `sex`, `town_id`, `posx`, `posy`, `posz`, `cap`, `lastip`, `stamina`, `skill_fist`, `skill_club`, `skill_sword`, `skill_axe`, `skill_dist`, `skill_shielding`, `skill_fishing`) VALUES (%s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
		                          db.escapeString(character.name),
		                          character.accountId, character.vocationId,
		                          CHARACTER_DEFAULT.HEALTH, CHARACTER_DEFAULT.HEALTH,
		                          CHARACTER_DEFAULT.OUTFIT_BODY,
		                          CHARACTER_DEFAULT.OUTFIT_FEET,
		                          CHARACTER_DEFAULT.OUTFIT_HEAD,
		                          CHARACTER_DEFAULT.OUTFIT_LEGS, lookType,
		                          CHARACTER_DEFAULT.OUTFIT_ADDONS,
		                          CHARACTER_DEFAULT.MAGIC_LEVEL,
		                          CHARACTER_DEFAULT.MANA, CHARACTER_DEFAULT.MANA,
		                          character.sex, CHARACTER_DEFAULT.TOWN,
		                          CHARACTER_DEFAULT.POSX, CHARACTER_DEFAULT.POSY,
		                          CHARACTER_DEFAULT.POSZ, CHARACTER_DEFAULT.CAPACITY,
		                          IP, CHARACTER_DEFAULT.STAMINA,
		                          CHARACTER_DEFAULT.SKILL, CHARACTER_DEFAULT.SKILL,
		                          CHARACTER_DEFAULT.SKILL, CHARACTER_DEFAULT.SKILL,
		                          CHARACTER_DEFAULT.SKILL, CHARACTER_DEFAULT.SKILL,
		                          CHARACTER_DEFAULT.SKILL))
	if dbResult then
		if not CREATE_CHARACTER_TABLE[IP] then
			CREATE_CHARACTER_TABLE[IP] = timeNow + CREATE_CHARACTER_EXHAUST
		end
		return "success"
	end
	return "failed"
end

---@param password string
---@return string
local function validatePassword(password)
	local passwordLength = password:len()
	if passwordLength < PASSWORD_MIN_LENGTH then return "too short" end
	if passwordLength > PASSWORD_MAX_LENGTH then return "too long" end
	if PASSWORD_WITH_NUMBERS and not password:match("%d+") then
		return "without numbers"
	elseif PASSWORD_WITH_CAPITAL_LETTERS and not password:match("%u+") then
		return "without capital letters"
	elseif PASSWORD_WITH_SPECIAL_CHARACTERS and not password:match("%W+") then
		return "without special characters"
	elseif PASSWORD_WITH_SPACES and not password:match("%s+") then
		return "without spaces"
	end
	return "valid"
end

---@param accountName string
---@return string
local function validateAccountName(accountName)
	local accountNameLength = accountName:len()
	if accountNameLength < ACCOUNT_NAME_MIN_LENGTH then return "too short" end
	if accountNameLength > ACCOUNT_NAME_MAX_LENGTH then return "too long" end
	if not ACCOUNT_NAME_WITH_NUMBERS and accountName:match("%d+") then
		return "with numbers"
	elseif not ACCOUNT_NAME_WITH_SPECIAL_CHARACTERS and accountName:match("%W+") then
		return "with special characters"
	end
	return "valid"
end

---@param characterName string
---@return string
local function validateCharacterName(characterName)
	local characterNameLength = characterName:len()
	if characterNameLength < CHARACTER_NAME_MIN_LENGTH then return "too short" end
	if characterNameLength > CHARACTER_NAME_MAX_LENGTH then return "too long" end
	if CHARACTER_NAME_FORMAT then
		local characterNameFormated = characterName:match(CHARACTER_NAME_FORMAT)
		if characterNameFormated ~= characterName then return "invalid" end
		if characterNameFormated:titleCase() ~= characterNameFormated then
			return "without title case"
		end
	end

	local words = characterName:split(" ")
	if #words > CHARACTER_NAME_MAX_WORDS then return "with too many words" end

	for _, word in ipairs(words) do
		local wordLength = word:len()
		if wordLength < CHARACTER_NAME_MIN_LENGTH then
			return "with words too short"
		elseif wordLength > CHARACTER_NAME_MAX_LENGTH then
			return "with words too long"
		end
	end
	return "valid"
end

---@param seconds number
---@return string
local function getTimeFormatted(seconds)
	local hours = math.floor(seconds / 3600)
	local minutes = math.floor((seconds - (hours * 3600)) / 60)
	seconds = seconds - (hours * 3600) - (minutes * 60)
	local str = {}
	if hours > 0 then str[#str + 1] = hours .. " hours" end
	if minutes > 0 then str[#str + 1] = minutes .. " minutes" end
	if seconds > 0 then str[#str + 1] = seconds .. " seconds" end
	return table.concat(str, " and ")
end

local event = Event()

function event.onAccountManager(player, text)
	local send = sender(player)
	local state = stater(player)
	local accountId = state.accountId or player:getAccountId()
	local isAccountManager = accountId == ACCOUNT_MANAGER_ACCOUNT_ID
	local command = text:trim():lower()

	if not state.talk then
		if command == "account" then
			if isAccountManager then
				send("What would you like your password to be?")
				state.talk = "create account"
				state.createAccount = {}
				return
			end

			send("Type {character} to create a new character.")
			return
		elseif command == "character" then
			if isAccountManager then return end

			send("What would you like as your character name?")
			state.talk = "create character"
			state.createCharacter = {}
		elseif command == "recover" then
			if isAccountManager then return end
		end
		return
	end

	-- Create Account
	if state.talk == "create account" then
		local createAccount = state.createAccount
		if not createAccount.password then
			local validation = validatePassword(text)
			if validation == "valid" then
				createAccount.password = text
				send("%s is it? {yes} or {no}?", text)
			else
				send("Your password is " .. validation .. ".")
			end
			return
		elseif not createAccount.confirmPassword then
			if text == "yes" then
				createAccount.confirmPassword = true
				send("What would you like your account name to be?")
			elseif text == "no" then
				send("What would you like your password to be then?")
				createAccount.password = nil
			else
				send("Type {yes} or {no}.")
			end
			return
		elseif not createAccount.accountName then
			local validation = validateAccountName(text)
			if validation == "valid" then
				createAccount.accountName = text
				send("%s is it? {yes} or {no}?", text)
			else
				send("Your account name is " .. validation .. ".")
			end
			return
		elseif not createAccount.confirmAccountName then
			if text == "yes" then
				if getAccountIdByAccountName(createAccount.accountName) ~= 0 then
					send("Account with that name already exists, please choose another one.")
					createAccount.accountName = nil
					return
				end

				createAccount.confirmAccountName = true
				local validation, id = CreateAccount(createAccount, player:getIp())
				if validation == "success" then
					send(
						"Your account has been created, you may manage it now, but please remember your account name {%s} and password {%s}!",
						createAccount.accountName, createAccount.password)
					send("Please type {account} to manage your account.")
					state.talk = nil
					state.accountId = id
					state.createAccount = nil
				elseif validation == "cooldown" then
					send("You must wait {%s} before creating another account.",
					     getTimeFormatted(CREATE_ACCOUNT_EXHAUST))
				else
					send("Your account could not be created, please try again.")
					state.createAccount = nil
				end
			elseif text == "no" then
				send("What would you like your account name to be then?")
				createAccount.accountName = nil
			else
				send("Type {yes} or {no}.")
			end
			return
		end
	end

	-- Create Character
	if state.talk == "create character" then
		local createCharacter = state.createCharacter
		if not createCharacter.name then
			local validation = validateCharacterName(text)
			if validation == "valid" then
				createCharacter.name = text
				send("{%s}, are you sure? {yes} or {no}", text)
			else
				send("Your character name is " .. validation .. ".")
			end
			return
		elseif not createCharacter.confirmName then
			if text == "yes" then
				send("Would you like to be a {male} or a {female}?")
				createCharacter.confirmName = true
			elseif text == "no" then
				send("What would you like as your character name then?")
				createCharacter.name = nil
			else
				send("Type {yes} or {no}.")
			end
			return
		elseif not createCharacter.sex then
			if text == "female" or text == "male" then
				createCharacter.sex = text == "female" and PLAYERSEX_FEMALE or
					                      PLAYERSEX_MALE
				send("A {%s}, are you sure? {yes} or {no}", text)
				return
			else
				send("Please type a sex, {male} or a {female}?")
				return
			end
		elseif not createCharacter.confirmSex then
			if text == "yes" then
				send(
					"What would you like to be... {sorcerer}, {druid}, {paladin} or {knight}.")
				createCharacter.confirmSex = true
			elseif text == "no" then
				send()
			else
			end
			return
		elseif not createCharacter.vocationId then
			if text == "sorcerer" then
				createCharacter.vocationId = 1
			elseif text == "druid" then
				createCharacter.vocationId = 2
			elseif text == "paladin" then
				createCharacter.vocationId = 3
			elseif text == "knight" then
				createCharacter.vocationId = 4
			else
				send("Please type a vocation, {sorcerer}, {druid}, {paladin} or {knight}.")
				return
			end

			send("So you would like to be a {%s}, {yes} or {no}?", text)
			return
		elseif not createCharacter.confirmVocation then
			if text == "yes" then
				createCharacter.accountId = accountId
				local character = CreateCharacter(createCharacter, player:getIp())
				if character == "success" then
					send("Your character {%s} has been created.", createCharacter.name)
				elseif character == "cooldown" then
					send("You must wait {%s} before creating another character.",
					     getTimeFormatted(CREATE_CHARACTER_EXHAUST))
				else
					send("Your character could not be created, please try again.")
				end
				state.talk = nil
				state.createCharacter = nil
			elseif text == "no" then
				send(
					"What would you like to be then... {sorcerer}, {druid}, {paladin} or {knight}.")
				createCharacter.vocationId = nil
			else
				send("Type {yes} or {no}.")
			end
			return
		end
	end
end

event:register()

---@param player Player
---@param playerId number
local function logoutEvent(player, playerId)
	ACCOUNT_MANAGER_STORAGE[playerId] = nil
	if player then player:remove() end
end

---@param player Player
---@param days integer
---@param reason string
---@return boolean
local function banIp(player, days, reason)
	local ip = player:getIp()
	local resultId = db.storeQuery("SELECT 1 FROM `ip_bans` WHERE `ip` = " .. ip)
	if resultId then
		result.free(resultId)
		return false
	end

	local timeNow = os.time()
	db.query(string.format(
		         "INSERT INTO `ip_bans` (`ip`, `reason`, `banned_at`, `expires_at`, `banned_by`) VALUES (%d, %s, %d, %d, %d)",
		         ip, db.escapeString(reason), timeNow, timeNow + (days * 86400),
		         player:getGuid()))
	return true
end

local login = CreatureEvent("Account Manager Login")

function login.onLogin(player)
	if player:isAccountManager() then
		local playerIp = player:getIp()
		-- Protect
		local tries = ACCOUNT_MANAGER_LOGINS[playerIp]
		if tries and tries >= ACCOUNT_MANAGER_LOGIN_TRIES then
			banIp(player, ACCOUNT_MANAGER_BAN_TIME, "Too many login attempts.")
			return false
		end

		ACCOUNT_MANAGER_LOGINS[playerIp] = tries and tries + 1 or 1
		-- Decrease Tries
		addEvent(function(playerIp)
			ACCOUNT_MANAGER_LOGINS[playerIp] = ACCOUNT_MANAGER_LOGINS[playerIp] - 1
			if ACCOUNT_MANAGER_LOGINS[playerIp] <= 0 then
				ACCOUNT_MANAGER_LOGINS[playerIp] = nil
			end
		end, ACCOUNT_MANAGER_LOGIN_TIMEOUT * 1000, playerIp)

		-- Logout Event
		addEvent(logoutEvent, ACCOUNT_MANAGER_LOGOUT_DELAY * 1000, player,
		         player:getIp())

		-- Initial Message
		local send = sender(player)
		if player:getAccountId() == ACCOUNT_MANAGER_ACCOUNT_ID then
			send(
				"Hello, type {account} to create an account or {recover} to recover an account.")
		else
			send("Hello, type {character} to create a new character.")
		end
	end
	return true
end

login:register()
