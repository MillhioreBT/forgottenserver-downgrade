// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "talkaction.h"

#include "player.h"
#include "pugicast.h"

TalkActions::TalkActions() : scriptInterface("TalkAction Interface") { scriptInterface.initState(); }

TalkActions::~TalkActions() { clear(false); }

void TalkActions::clear(bool fromLua)
{
	for (auto it = talkActions.begin(); it != talkActions.end();) {
		if (fromLua == it->second.fromLua) {
			it = talkActions.erase(it);
		} else {
			++it;
		}
	}

	reInitState(fromLua);
}

LuaScriptInterface& TalkActions::getScriptInterface() { return scriptInterface; }

std::string_view TalkActions::getScriptBaseName() const { return "talkactions"; }

Event_ptr TalkActions::getEvent(std::string_view nodeName)
{
	if (!caseInsensitiveEqual(nodeName, "talkaction")) {
		return nullptr;
	}
	return Event_ptr(new TalkAction(&scriptInterface));
}

bool TalkActions::registerEvent(Event_ptr event, const pugi::xml_node&)
{
	TalkAction_ptr talkAction{static_cast<TalkAction*>(event.release())}; // event is guaranteed to be a TalkAction
	const auto& words = talkAction->stealWordsMap();

	for (const auto& word : words) {
		talkActions.emplace(word, *talkAction);
	}
	return true;
}

bool TalkActions::registerLuaEvent(TalkAction* event)
{
	TalkAction_ptr talkAction{event};
	const auto& words = talkAction->stealWordsMap();

	if (words.empty()) {
		std::cout << "[Warning - TalkActions::registerLuaEvent] Missing words for talk action." << std::endl;
		return false;
	}

	for (const auto& word : words) {
		talkActions.emplace(word, *talkAction);
	}
	return true;
}

TalkActionResult_t TalkActions::playerSaySpell(Player* player, SpeakClasses type, std::string_view words) const
{
	size_t wordsLength = words.length();
	for (auto it = talkActions.begin(); it != talkActions.end();) {
		std::string_view talkactionWords = it->first;
		if (!caseInsensitiveStartsWith(words, talkactionWords)) {
			++it;
			continue;
		}

		std::string param;
		if (wordsLength != talkactionWords.size()) {
			param = words.substr(talkactionWords.size());
			if (param.front() != ' ') {
				++it;
				continue;
			}
			boost::algorithm::trim_left(param);

			auto separator = it->second.getSeparator();
			if (separator != " ") {
				if (!param.empty()) {
					if (param != separator) {
						++it;
						continue;
					} else {
						param.erase(param.begin());
					}
				}
			}
		}

		if (it->second.getNeedAccess() && !player->isAccessPlayer()) {
			std::cout << player->getGroup()->access << std::endl;
			return TALKACTION_CONTINUE;
		}

		if (player->getAccountType() < it->second.getRequiredAccountType()) {
			std::cout << static_cast<uint16_t>(player->getAccountType()) << " - "
			          << static_cast<uint16_t>(it->second.getRequiredAccountType()) << std::endl;
			return TALKACTION_CONTINUE;
		}

		if (it->second.executeSay(player, words, param, type)) {
			return TALKACTION_CONTINUE;
		} else {
			return TALKACTION_BREAK;
		}
	}
	return TALKACTION_CONTINUE;
}

bool TalkAction::configureEvent(const pugi::xml_node& node)
{
	pugi::xml_attribute wordsAttribute = node.attribute("words");
	if (!wordsAttribute) {
		std::cout << "[Error - TalkAction::configureEvent] Missing words for talk action or spell" << std::endl;
		return false;
	}

	pugi::xml_attribute separatorAttribute = node.attribute("separator");
	if (separatorAttribute) {
		separator = pugi::cast<char>(separatorAttribute.value());
	}

	pugi::xml_attribute accessAttribute = node.attribute("access");
	if (accessAttribute) {
		needAccess = accessAttribute.as_bool();
	}

	pugi::xml_attribute accountTypeAttribute = node.attribute("accountType");
	if (accountTypeAttribute) {
		requiredAccountType = static_cast<AccountType_t>(pugi::cast<int32_t>(accountTypeAttribute.value()));
	}

	for (const auto& word : explodeString(wordsAttribute.as_string(), ";")) {
		setWords(word);
	}
	return true;
}

std::string_view TalkAction::getScriptEventName() const { return "onSay"; }

bool TalkAction::executeSay(Player* player, std::string_view words, std::string_view param, SpeakClasses type) const
{
	// onSay(player, words, param, type)
	if (!scriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - TalkAction::executeSay] Call stack overflow" << std::endl;
		return false;
	}

	ScriptEnvironment* env = scriptInterface->getScriptEnv();
	env->setScriptId(scriptId, scriptInterface);

	lua_State* L = scriptInterface->getLuaState();

	scriptInterface->pushFunction(scriptId);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushString(L, words);
	LuaScriptInterface::pushString(L, param);
	lua_pushinteger(L, type);

	return scriptInterface->callFunction(4);
}
