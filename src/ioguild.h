// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#ifndef FS_IOGUILD_H
#define FS_IOGUILD_H

class Guild;
using GuildWarVector = std::vector<uint32_t>;

class IOGuild
{
public:
	static Guild* loadGuild(uint32_t guildId);
	static uint32_t getGuildIdByName(std::string_view name);
	static void getWarList(uint32_t guildId, GuildWarVector& guildWarVector);
};

#endif
