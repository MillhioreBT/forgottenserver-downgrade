// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#ifndef FS_BAN_H
#define FS_BAN_H

#include "connection.h"

struct BanInfo
{
	std::string bannedBy;
	std::string reason;
	time_t expiresAt;
};

struct ConnectBlock
{
	constexpr ConnectBlock(uint64_t lastAttempt, uint64_t blockTime, uint32_t count) :
	    lastAttempt(lastAttempt), blockTime(blockTime), count(count)
	{}

	uint64_t lastAttempt;
	uint64_t blockTime;
	uint32_t count;
};

using IpConnectMap = std::map<uint32_t, ConnectBlock>;

struct LoginFailures
{
	int64_t windowStart = 0;
	int64_t blockedUntil = 0;
	uint32_t count = 0;
};

class Ban
{
public:
	bool acceptConnection(const uint32_t clientIP);

	bool isLoginBlocked(const uint32_t clientIP);
	void onLoginFailed(const uint32_t clientIP);

private:
	static constexpr int64_t LOGIN_FAILURE_WINDOW = 5 * 60 * 1000;
	static constexpr int64_t LOGIN_BLOCK_TIME = 5 * 60 * 1000;

	IpConnectMap ipConnectMap;
	std::map<uint32_t, LoginFailures> loginFailures;
	std::recursive_mutex lock;
};

class IOBan
{
public:
	static bool isAccountBanned(uint32_t accountId, BanInfo& banInfo);
	static bool isIpBanned(const uint32_t clientIP, BanInfo& banInfo);
	static bool isPlayerNamelocked(uint32_t playerId);
};

#endif // FS_BAN_H
