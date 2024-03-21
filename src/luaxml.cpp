// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "luascript.h"
#include "tools.h"

namespace {
using namespace Lua;

// XML
int luaCreateXmlDocument(lua_State* L)
{
	// XMLDocument(filename)
	std::string filename = getString(L, 2);
	if (filename.empty()) {
		lua_pushnil(L);
		return 1;
	}

	auto doc = std::make_unique<XMLDocument>();
	if (auto result = doc->load_file(filename.c_str())) {
		pushUserdata<XMLDocument>(L, doc.release());
		setMetatable(L, -1, "XMLDocument");
	} else {
		printXMLError("Error - luaCreateXmlDocument", filename, result);
		lua_pushnil(L);
	}
	return 1;
}

int luaDeleteXmlDocument(lua_State* L)
{
	// doc:delete() or doc:__gc() or doc:__close()
	XMLDocument** document = getRawUserdata<XMLDocument>(L, 1);
	if (document && *document) {
		delete *document;
		*document = nullptr;
	}
	return 1;
}

int luaXmlDocumentChild(lua_State* L)
{
	// doc:child(name)
	XMLDocument* document = getUserdata<XMLDocument>(L, 1);
	if (!document) {
		lua_pushnil(L);
		return 1;
	}

	std::string name = getString(L, 2);
	if (name.empty()) {
		lua_pushnil(L);
		return 1;
	}

	auto node = std::make_unique<XMLNode>(document->child(name.c_str()));
	pushUserdata<XMLNode>(L, node.release());
	setMetatable(L, -1, "XMLNode");
	return 1;
}

int luaDeleteXmlNode(lua_State* L)
{
	// node:delete() or node:__gc() or node:__close()
	XMLNode** node = getRawUserdata<XMLNode>(L, 1);
	if (node && *node) {
		delete *node;
		*node = nullptr;
	}
	return 1;
}

int luaXmlNodeAttribute(lua_State* L)
{
	// node:attribute(name)
	XMLNode* node = getUserdata<XMLNode>(L, 1);
	if (!node) {
		lua_pushnil(L);
		return 1;
	}

	std::string name = getString(L, 2);
	if (name.empty()) {
		lua_pushnil(L);
		return 1;
	}

	pugi::xml_attribute attribute = node->attribute(name.c_str());
	if (attribute) {
		pushString(L, attribute.value());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int luaXmlNodeName(lua_State* L)
{
	// node:name()
	XMLNode* node = getUserdata<XMLNode>(L, 1);
	if (!node) {
		lua_pushnil(L);
		return 1;
	}

	pushString(L, node->name());
	return 1;
}

int luaXmlNodeFirstChild(lua_State* L)
{
	// node:firstChild()
	XMLNode* node = getUserdata<XMLNode>(L, 1);
	if (!node) {
		lua_pushnil(L);
		return 1;
	}

	auto firstChild = node->first_child();
	if (!firstChild) {
		lua_pushnil(L);
		return 1;
	}

	auto newNode = std::make_unique<XMLNode>(std::move(firstChild));
	pushUserdata<XMLNode>(L, newNode.release());
	setMetatable(L, -1, "XMLNode");
	return 1;
}

int luaXmlNodeNextSibling(lua_State* L)
{
	// node:nextSibling()
	XMLNode* node = getUserdata<XMLNode>(L, 1);
	if (!node) {
		lua_pushnil(L);
		return 1;
	}

	auto nextSibling = node->next_sibling();
	if (!nextSibling) {
		lua_pushnil(L);
		return 1;
	}

	auto newNode = std::make_unique<XMLNode>(std::move(nextSibling));
	pushUserdata<XMLNode>(L, newNode.release());
	setMetatable(L, -1, "XMLNode");
	return 1;
}
} // namespace

void LuaScriptInterface::registerXML()
{
	// XML
	registerClass("XMLDocument", "", luaCreateXmlDocument);
	registerMetaMethod("XMLDocument", "__gc", luaDeleteXmlDocument);
	registerMetaMethod("XMLDocument", "__close", luaDeleteXmlDocument);

	registerMethod("XMLDocument", "delete", luaDeleteXmlDocument);

	registerMethod("XMLDocument", "child", luaXmlDocumentChild);

	registerClass("XMLNode", "");
	registerMetaMethod("XMLNode", "__gc", luaDeleteXmlNode);
	registerMetaMethod("XMLNode", "__close", luaDeleteXmlNode);

	registerMethod("XMLNode", "delete", luaDeleteXmlNode);

	registerMethod("XMLNode", "attribute", luaXmlNodeAttribute);
	registerMethod("XMLNode", "name", luaXmlNodeName);
	registerMethod("XMLNode", "firstChild", luaXmlNodeFirstChild);
	registerMethod("XMLNode", "nextSibling", luaXmlNodeNextSibling);
}
