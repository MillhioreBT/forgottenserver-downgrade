// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#ifndef FS_WILDCARDTREE_H
#define FS_WILDCARDTREE_H

#include "enums.h"

class WildcardTreeNode
{
public:
	explicit WildcardTreeNode(bool breakpoint) : breakpoint(breakpoint) {}
	WildcardTreeNode(WildcardTreeNode&& other) = default;

	// non-copyable
	WildcardTreeNode(const WildcardTreeNode&) = delete;
	WildcardTreeNode& operator=(const WildcardTreeNode&) = delete;

	WildcardTreeNode* getChild(char ch);
	const WildcardTreeNode* getChild(char ch) const;
	WildcardTreeNode* addChild(char ch, bool breakpoint);

	void insert(std::string_view str);
	void remove(std::string_view str);

	ReturnValue findOne(std::string_view query, std::string& result) const;

private:
	std::map<char, WildcardTreeNode> children;
	bool breakpoint;
};

#endif
