// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "luascript.h"
#include "script.h"
#include "talkaction.h"

extern TalkActions* g_talkActions;
extern Scripts* g_scripts;

namespace {
using namespace Lua;

// TalkAction
int luaCreateTalkaction(lua_State* L)
{
	// TalkAction(words)
	if (LuaScriptInterface::getScriptEnv()->getScriptInterface() != &g_scripts->getScriptInterface()) {
		reportErrorFunc(L, "TalkActions can only be registered in the Scripts interface.");
		lua_pushnil(L);
		return 1;
	}

	TalkAction* talk = new TalkAction(LuaScriptInterface::getScriptEnv()->getScriptInterface());
	for (int i = 2; i <= lua_gettop(L); i++) {
		talk->setWords(getString(L, i));
	}
	talk->fromLua = true;
	pushUserdata<TalkAction>(L, talk);
	setMetatable(L, -1, "TalkAction");
	return 1;
}

int luaTalkactionOnSay(lua_State* L)
{
	// talkAction:onSay(callback)
	TalkAction* talk = getUserdata<TalkAction>(L, 1);
	if (talk) {
		if (!talk->loadCallback()) {
			pushBoolean(L, false);
			return 1;
		}
		pushBoolean(L, true);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaTalkactionRegister(lua_State* L)
{
	// talkAction:register()
	TalkAction** talkPtr = getRawUserdata<TalkAction>(L, 1);
	if (auto talk = *talkPtr) {
		if (!talk->isScripted()) {
			pushBoolean(L, false);
			return 1;
		}

		pushBoolean(L, g_talkActions->registerLuaEvent(talk));
		*talkPtr = nullptr;
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaTalkactionGetWords(lua_State* L)
{
	// talkAction:words()
	TalkAction* talk = getUserdata<TalkAction>(L, 1);
	if (talk) {
		pushString(L, talk->getWords());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaTalkactionSeparator(lua_State* L)
{
	// get: talkAction:separator() set: talkAction:separator(sep)
	TalkAction* talk = getUserdata<TalkAction>(L, 1);
	if (talk) {
		if (lua_gettop(L) == 1) {
			pushString(L, talk->getSeparator());
		} else {
			talk->setSeparator(getString(L, 2));
			pushBoolean(L, true);
		}
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaTalkactionAccess(lua_State* L)
{
	// get: talkAction:access() set: talkAction:access(needAccess = false)
	TalkAction* talk = getUserdata<TalkAction>(L, 1);
	if (talk) {
		if (lua_gettop(L) == 1) {
			pushBoolean(L, talk->getNeedAccess());
		} else {
			talk->setNeedAccess(getBoolean(L, 2));
			pushBoolean(L, true);
		}
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaTalkactionAccountType(lua_State* L)
{
	// get: talkAction:accountType() set: talkAction:accountType(AccountType_t = ACCOUNT_TYPE_NORMAL)
	TalkAction* talk = getUserdata<TalkAction>(L, 1);
	if (talk) {
		if (lua_gettop(L) == 1) {
			lua_pushinteger(L, talk->getRequiredAccountType());
		} else {
			talk->setRequiredAccountType(getInteger<AccountType_t>(L, 2));
			pushBoolean(L, true);
		}
	} else {
		lua_pushnil(L);
	}
	return 1;
}
} // namespace

void LuaScriptInterface::registerTalkActions()
{
	// TalkAction
	registerClass("TalkAction", "", luaCreateTalkaction);

	registerMethod("TalkAction", "onSay", luaTalkactionOnSay);
	registerMethod("TalkAction", "register", luaTalkactionRegister);
	registerMethod("TalkAction", "getWords", luaTalkactionGetWords);
	registerMethod("TalkAction", "separator", luaTalkactionSeparator);
	registerMethod("TalkAction", "access", luaTalkactionAccess);
	registerMethod("TalkAction", "accountType", luaTalkactionAccountType);
}
