// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "protocolgame.h"

#include "actions.h"
#include "ban.h"
#include "configmanager.h"
#include "game.h"
#include "iologindata.h"
#include "outputmessage.h"
#include "player.h"
#include "scheduler.h"

extern CreatureEvents* g_creatureEvents;
extern Chat* g_chat;

namespace {

using WaitList = std::deque<std::pair<int64_t, uint32_t>>; // (timeout, player guid)

WaitList priorityWaitList, waitList;

std::tuple<WaitList&, WaitList::iterator, WaitList::size_type> findClient(const Player& player)
{
	const auto fn = [&](const WaitList::value_type& it) { return it.second == player.getGUID(); };

	auto it = std::find_if(priorityWaitList.begin(), priorityWaitList.end(), fn);
	if (it != priorityWaitList.end()) {
		return std::make_tuple(std::ref(priorityWaitList), it, std::distance(it, priorityWaitList.end()) + 1);
	}

	it = std::find_if(waitList.begin(), waitList.end(), fn);
	if (it != waitList.end()) {
		return std::make_tuple(std::ref(waitList), it, priorityWaitList.size() + std::distance(it, waitList.end()) + 1);
	}

	return std::make_tuple(std::ref(waitList), waitList.end(), priorityWaitList.size() + waitList.size());
}

uint8_t getWaitTime(std::size_t slot)
{
	if (slot < 5) {
		return 5;
	} else if (slot < 10) {
		return 10;
	} else if (slot < 20) {
		return 20;
	} else if (slot < 50) {
		return 60;
	} else {
		return 120;
	}
}

int64_t getTimeout(std::size_t slot)
{
	// timeout is set to 15 seconds longer than expected retry attempt
	return getWaitTime(slot) + 15;
}

void cleanupList(WaitList& list)
{
	int64_t time = OTSYS_TIME();

	auto it = list.begin();
	while (it != list.end()) {
		if (it->first <= time) {
			it = list.erase(it);
		} else {
			++it;
		}
	}
}

std::size_t clientLogin(const Player& player)
{
	// Currentslot = position in wait list, 0 for direct access
	if (player.hasFlag(PlayerFlag_CanAlwaysLogin) || player.getAccountType() >= ACCOUNT_TYPE_GAMEMASTER) {
		return 0;
	}

	cleanupList(priorityWaitList);
	cleanupList(waitList);

	const uint32_t maxPlayers = static_cast<uint32_t>(getInteger(ConfigManager::MAX_PLAYERS));
	if (maxPlayers == 0 || (priorityWaitList.empty() && waitList.empty() && g_game.getPlayersOnline() < maxPlayers)) {
		return 0;
	}

	auto result = findClient(player);
	if (std::get<1>(result) != std::get<0>(result).end()) {
		auto currentSlot = std::get<2>(result);
		// If server has capacity for this client, let him in even though his current slot might be higher than 0.
		if ((g_game.getPlayersOnline() + currentSlot) <= maxPlayers) {
			std::get<0>(result).erase(std::get<1>(result));
			return 0;
		}

		// let them wait a bit longer
		std::get<1>(result)->second = OTSYS_TIME() + (getTimeout(currentSlot) * 1000);
		return currentSlot;
	}

	auto currentSlot = priorityWaitList.size();
	if (player.isPremium()) {
		priorityWaitList.emplace_back(OTSYS_TIME() + (getTimeout(++currentSlot) * 1000), player.getGUID());
	} else {
		currentSlot += waitList.size();
		waitList.emplace_back(OTSYS_TIME() + (getTimeout(++currentSlot) * 1000), player.getGUID());
	}
	return currentSlot;
}

} // namespace

void ProtocolGame::release()
{
	// dispatcher thread
	if (player && player->client == shared_from_this()) {
		player->client.reset();
		player->decrementReferenceCounter();
		player = nullptr;
	}

	OutputMessagePool::getInstance().removeProtocolFromAutosend(shared_from_this());
	Protocol::release();
}

void ProtocolGame::login(uint32_t characterId, uint32_t accountId, OperatingSystem_t operatingSystem)
{
	// dispatcher thread
	Player* foundPlayer = g_game.getPlayerByGUID(characterId);
	const bool isAccountManager =
	    getBoolean(ConfigManager::ACCOUNT_MANAGER) && characterId == ACCOUNT_MANAGER_PLAYER_ID;
	if (!foundPlayer || getBoolean(ConfigManager::ALLOW_CLONES) || isAccountManager) {
		player = new Player(getThis());
		player->setGUID(characterId);

		player->incrementReferenceCounter();
		player->setID();

		if (!IOLoginData::preloadPlayer(player)) {
			disconnectClient("Your character could not be loaded.");
			return;
		}

		if (IOBan::isPlayerNamelocked(player->getGUID())) {
			disconnectClient("Your character has been namelocked.");
			return;
		}

		if (g_game.getGameState() == GAME_STATE_CLOSING && !player->hasFlag(PlayerFlag_CanAlwaysLogin)) {
			disconnectClient("The game is just going down.\nPlease try again later.");
			return;
		}

		if (g_game.getGameState() == GAME_STATE_CLOSED && !player->hasFlag(PlayerFlag_CanAlwaysLogin)) {
			disconnectClient("Server is currently closed.\nPlease try again later.");
			return;
		}

		if (getBoolean(ConfigManager::ONE_PLAYER_ON_ACCOUNT) && !isAccountManager &&
		    player->getAccountType() < ACCOUNT_TYPE_GAMEMASTER && g_game.getPlayerByAccount(player->getAccount())) {
			disconnectClient("You may only login with one character\nof your account at the same time.");
			return;
		}

		if (!player->hasFlag(PlayerFlag_CannotBeBanned)) {
			BanInfo banInfo;
			if (IOBan::isAccountBanned(accountId, banInfo)) {
				if (banInfo.reason.empty()) {
					banInfo.reason = "(none)";
				}

				if (banInfo.expiresAt > 0) {
					disconnectClient(
					    fmt::format("Your account has been banned until {:s} by {:s}.\n\nReason specified:\n{:s}",
					                formatDateShort(banInfo.expiresAt), banInfo.bannedBy, banInfo.reason));
				} else {
					disconnectClient(
					    fmt::format("Your account has been permanently banned by {:s}.\n\nReason specified:\n{:s}",
					                banInfo.bannedBy, banInfo.reason));
				}
				return;
			}
		}

		if (std::size_t currentSlot = clientLogin(*player)) {
			uint8_t retryTime = getWaitTime(currentSlot);
			auto output = OutputMessagePool::getOutputMessage();
			output->addByte(0x16);
			output->addString(
			    fmt::format("Too many players online.\nYou are at place {:d} on the waiting list.", currentSlot));
			output->addByte(retryTime);
			send(output);
			disconnect();
			return;
		}

		if (!IOLoginData::loadPlayerById(player, player->getGUID())) {
			disconnectClient("Your character could not be loaded.");
			return;
		}

		player->setOperatingSystem(operatingSystem);

		if (isAccountManager) {
			player->accountNumber = accountId;
		}

		if (!g_game.placeCreature(player, player->getLoginPosition())) {
			if (!g_game.placeCreature(player, player->getTemplePosition(), false, true)) {
				disconnectClient("Temple position is wrong. Contact the administrator.");
				return;
			}
		}

		if (operatingSystem >= CLIENTOS_OTCLIENT_LINUX) {
			player->registerCreatureEvent("ExtendedOpcode");
		}

		player->lastIP = player->getIP();
		player->lastLoginSaved = std::max<time_t>(time(nullptr), player->lastLoginSaved + 1);
		acceptPackets = true;
	} else {
		if (eventConnect != 0 || !getBoolean(ConfigManager::REPLACE_KICK_ON_LOGIN)) {
			// Already trying to connect
			disconnectClient("You are already logged in.");
			return;
		}

		if (foundPlayer->client) {
			foundPlayer->disconnect();
			foundPlayer->isConnecting = true;

			eventConnect = g_scheduler.addEvent(
			    createSchedulerTask(1000, [=, thisPtr = getThis(), playerID = foundPlayer->getID()]() {
				    thisPtr->connect(playerID, operatingSystem);
			    }));
		} else {
			connect(foundPlayer->getID(), operatingSystem);
		}
	}
	OutputMessagePool::getInstance().addProtocolToAutosend(shared_from_this());
}

void ProtocolGame::connect(uint32_t playerId, OperatingSystem_t operatingSystem)
{
	eventConnect = 0;

	Player* foundPlayer = g_game.getPlayerByID(playerId);
	if (!foundPlayer || foundPlayer->client) {
		disconnectClient("You are already logged in.");
		return;
	}

	if (isConnectionExpired()) {
		// ProtocolGame::release() has been called at this point and the Connection object
		// no longer exists, so we return to prevent leakage of the Player.
		return;
	}

	player = foundPlayer;
	player->incrementReferenceCounter();

	player->clearModalWindows();
	g_chat->removeUserFromAllChannels(*player);
	player->setOperatingSystem(operatingSystem);
	player->isConnecting = false;

	player->client = getThis();
	sendAddCreature(player, player->getPosition(), 0);
	player->lastIP = player->getIP();
	player->lastLoginSaved = std::max<time_t>(time(nullptr), player->lastLoginSaved + 1);
	player->resetIdleTime();
	acceptPackets = true;
}

void ProtocolGame::logout(bool displayEffect, bool forced)
{
	// dispatcher thread
	if (!player) {
		return;
	}

	if (!player->isRemoved()) {
		if (!forced) {
			if (player->getAccountType() < ACCOUNT_TYPE_GOD) {
				if (player->getTile()->hasFlag(TILESTATE_NOLOGOUT)) {
					player->sendCancelMessage(RETURNVALUE_YOUCANNOTLOGOUTHERE);
					return;
				}

				if (!player->getTile()->hasFlag(TILESTATE_PROTECTIONZONE) && player->hasCondition(CONDITION_INFIGHT)) {
					player->sendCancelMessage(RETURNVALUE_YOUMAYNOTLOGOUTDURINGAFIGHT);
					return;
				}
			}

			// scripting event - onLogout
			if (!g_creatureEvents->playerLogout(player)) {
				// Let the script handle the error message
				return;
			}
		}

		if (displayEffect && !player->isDead() && !player->isInGhostMode()) {
			g_game.addMagicEffect(player->getPosition(), CONST_ME_POFF);
		}
	}

	disconnect();

	g_game.removeCreature(player);
}

void ProtocolGame::onRecvFirstMessage(NetworkMessage& msg)
{
	if (g_game.getGameState() == GAME_STATE_SHUTDOWN) {
		disconnect();
		return;
	}

	OperatingSystem_t operatingSystem = static_cast<OperatingSystem_t>(msg.get<uint16_t>());
	version = msg.get<uint16_t>();

	if (!Protocol::RSA_decrypt(msg)) {
		disconnect();
		return;
	}

	xtea::key key;
	key[0] = msg.get<uint32_t>();
	key[1] = msg.get<uint32_t>();
	key[2] = msg.get<uint32_t>();
	key[3] = msg.get<uint32_t>();
	enableXTEAEncryption();
	setXTEAKey(std::move(key));

	msg.skipBytes(1); // gamemaster flag

	auto accountName = msg.getString();
	auto characterName = msg.getString();
	auto password = msg.getString();

	bool accountManager = getBoolean(ConfigManager::ACCOUNT_MANAGER);
	if (accountManager && accountName.empty() && password.empty()) {
		accountName = ACCOUNT_MANAGER_ACCOUNT_NAME;
		password = ACCOUNT_MANAGER_ACCOUNT_PASSWORD;
	}

	if (accountName.empty()) {
		disconnectClient("You must enter your account name.");
		return;
	}

	uint32_t timeStamp = msg.get<uint32_t>();
	uint8_t randNumber = msg.getByte();
	if (challengeTimestamp != timeStamp || challengeRandom != randNumber) {
		disconnect();
		return;
	}

	// OTCv8 detect
	const auto otcv8StrLen = msg.get<uint16_t>();
	if (otcv8StrLen == OTCV8_LENGTH && msg.getString(OTCV8_LENGTH) == OTCV8_NAME) {
		isOTCv8 = msg.get<uint16_t>() != 0;
	}

	if (isOTCv8 || operatingSystem >= CLIENTOS_OTCLIENT_LINUX) {
		if (isOTCv8) {
			sendOTCv8Features();
		}
		NetworkMessage opcodeMessage;
		opcodeMessage.addByte(0x32);
		opcodeMessage.addByte(0x00);
		opcodeMessage.add<uint16_t>(0x00);
		writeToOutputBuffer(opcodeMessage);
	}

	if (version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX) {
		disconnectClient(fmt::format("Only clients with protocol {:s} allowed!", CLIENT_VERSION_STR));
		return;
	}

	if (g_game.getGameState() == GAME_STATE_STARTUP) {
		disconnectClient("Gameworld is starting up. Please wait.");
		return;
	}

	if (g_game.getGameState() == GAME_STATE_MAINTAIN) {
		disconnectClient("Gameworld is under maintenance. Please re-connect in a while.");
		return;
	}

	BanInfo banInfo;
	if (IOBan::isIpBanned(getIP(), banInfo)) {
		if (banInfo.reason.empty()) {
			banInfo.reason = "(none)";
		}

		disconnectClient(fmt::format("Your IP has been banned until {:s} by {:s}.\n\nReason specified:\n{:s}",
		                             formatDateShort(banInfo.expiresAt), banInfo.bannedBy, banInfo.reason));
		return;
	}

	auto [accountId, characterId] = IOLoginData::gameworldAuthentication(accountName, password, characterName);
	if (accountManager && characterName == ACCOUNT_MANAGER_PLAYER_NAME) {
		if (accountId == 0) {
			std::tie(accountId, characterId) =
			    IOLoginData::getAccountIdByAccountName(accountName, password, characterName);
		}
	}

	if (accountId == 0) {
		disconnectClient("Account name or password is not correct.");
		return;
	}

	g_dispatcher.addTask([=, thisPtr = getThis()]() { thisPtr->login(characterId, accountId, operatingSystem); });
}

void ProtocolGame::onConnect()
{
	auto output = OutputMessagePool::getOutputMessage();
	static std::random_device rd;
	static std::ranlux24 generator(rd());
	static std::uniform_int_distribution<uint16_t> randNumber(0x00, 0xFF);

	// Skip checksum
	output->skipBytes(sizeof(uint32_t));

	// Packet length & type
	output->add<uint16_t>(0x0006);
	output->addByte(0x1F);

	// Add timestamp & random number
	challengeTimestamp = static_cast<uint32_t>(time(nullptr));
	output->add<uint32_t>(challengeTimestamp);

	challengeRandom = randNumber(generator);
	output->addByte(challengeRandom);

	// Go back and write checksum
	output->skipBytes(-12);
	output->add<uint32_t>(adlerChecksum(output->getOutputBuffer() + sizeof(uint32_t), 8));

	send(output);
}

void ProtocolGame::disconnectClient(std::string_view message) const
{
	auto output = OutputMessagePool::getOutputMessage();
	output->addByte(0x14);
	output->addString(message);
	send(output);
	disconnect();
}

void ProtocolGame::writeToOutputBuffer(const NetworkMessage& msg)
{
	auto out = getOutputBuffer(msg.getLength());
	out->append(msg);
}

void ProtocolGame::parsePacket(NetworkMessage& msg)
{
	if (!acceptPackets || g_game.getGameState() == GAME_STATE_SHUTDOWN || msg.getLength() == 0) {
		return;
	}

	uint8_t recvbyte = msg.getByte();

	if (!player) {
		if (recvbyte == 0x0F) {
			disconnect();
		}

		return;
	}

	// a dead player can not performs actions
	if (player->isRemoved() || player->isDead()) {
		if (recvbyte == 0x0F) {
			disconnect();
			return;
		}

		if (recvbyte != 0x14) {
			return;
		}
	}

	// Account Manager
	if (player->isAccountManager()) {
		switch (recvbyte) {
			case 0x14:
				g_dispatcher.addTask([thisPtr = getThis()]() { thisPtr->logout(true, false); });
				break;
			case 0x1E:
				g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerReceivePing(playerID); });
				break;
			case 0x32:
				parseExtendedOpcode(msg);
				break; // otclient extended opcode
			case 0x64:
			case 0x65:
			case 0x66:
			case 0x67:
			case 0x68:
			case 0x69:
			case 0x6A:
			case 0x6B:
			case 0x6C:
			case 0x6D:
				g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerCancelMove(playerID); });
				break;
			case 0x89:
				parseTextWindow(msg);
				break;
			case 0x8A:
				parseHouseWindow(msg);
				break;
			case 0x8C:
				parseLookAt(msg);
				break;
			case 0x8E: /* join aggression */
				break;
			case 0x96:
				parseSay(msg);
				break;
			case 0xC9: /* update tile */
				break;
			default:
				break;
		}

		if (msg.isOverrun()) {
			disconnect();
		}
		return;
	}

	switch (recvbyte) {
		case 0x14:
			g_dispatcher.addTask([thisPtr = getThis()]() { thisPtr->logout(true, false); });
			break;
		case 0x1E:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerReceivePing(playerID); });
			break;
		case 0x32:
			parseExtendedOpcode(msg);
			break; // otclient extended opcode
		case 0x64:
			parseAutoWalk(msg);
			break;
		case 0x65:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerMove(playerID, DIRECTION_NORTH); });
			break;
		case 0x66:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerMove(playerID, DIRECTION_EAST); });
			break;
		case 0x67:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerMove(playerID, DIRECTION_SOUTH); });
			break;
		case 0x68:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerMove(playerID, DIRECTION_WEST); });
			break;
		case 0x69:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerStopAutoWalk(playerID); });
			break;
		case 0x6A:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerMove(playerID, DIRECTION_NORTHEAST); });
			break;
		case 0x6B:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerMove(playerID, DIRECTION_SOUTHEAST); });
			break;
		case 0x6C:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerMove(playerID, DIRECTION_SOUTHWEST); });
			break;
		case 0x6D:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerMove(playerID, DIRECTION_NORTHWEST); });
			break;
		case 0x6F:
			g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
			                     [playerID = player->getID()]() { g_game.playerTurn(playerID, DIRECTION_NORTH); });
			break;
		case 0x70:
			g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
			                     [playerID = player->getID()]() { g_game.playerTurn(playerID, DIRECTION_EAST); });
			break;
		case 0x71:
			g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
			                     [playerID = player->getID()]() { g_game.playerTurn(playerID, DIRECTION_SOUTH); });
			break;
		case 0x72:
			g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
			                     [playerID = player->getID()]() { g_game.playerTurn(playerID, DIRECTION_WEST); });
			break;
		case 0x78:
			parseThrow(msg);
			break;
		case 0x79:
			parseLookInShop(msg);
			break;
		case 0x7A:
			parsePlayerPurchase(msg);
			break;
		case 0x7B:
			parsePlayerSale(msg);
			break;
		case 0x7C:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerCloseShop(playerID); });
			break;
		case 0x7D:
			parseRequestTrade(msg);
			break;
		case 0x7E:
			parseLookInTrade(msg);
			break;
		case 0x7F:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerAcceptTrade(playerID); });
			break;
		case 0x80:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerCloseTrade(playerID); });
			break;
		case 0x82:
			parseUseItem(msg);
			break;
		case 0x83:
			parseUseItemEx(msg);
			break;
		case 0x84:
			parseUseWithCreature(msg);
			break;
		case 0x85:
			parseRotateItem(msg);
			break;
		case 0x87:
			parseCloseContainer(msg);
			break;
		case 0x88:
			parseUpArrowContainer(msg);
			break;
		case 0x89:
			parseTextWindow(msg);
			break;
		case 0x8A:
			parseHouseWindow(msg);
			break;
		case 0x8C:
			parseLookAt(msg);
			break;
		case 0x8D:
			parseLookInBattleList(msg);
			break;
		case 0x8E: /* join aggression */
			break;
		case 0x96:
			parseSay(msg);
			break;
		case 0x97:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerRequestChannels(playerID); });
			break;
		case 0x98:
			parseOpenChannel(msg);
			break;
		case 0x99:
			parseCloseChannel(msg);
			break;
		case 0x9A:
			parseOpenPrivateChannel(msg);
			break;
		case 0x9E:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerCloseNpcChannel(playerID); });
			break;
		case 0xA0:
			parseFightModes(msg);
			break;
		case 0xA1:
			parseAttack(msg);
			break;
		case 0xA2:
			parseFollow(msg);
			break;
		case 0xA3:
			parseInviteToParty(msg);
			break;
		case 0xA4:
			parseJoinParty(msg);
			break;
		case 0xA5:
			parseRevokePartyInvite(msg);
			break;
		case 0xA6:
			parsePassPartyLeadership(msg);
			break;
		case 0xA7:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerLeaveParty(playerID); });
			break;
		case 0xA8:
			parseEnableSharedPartyExperience(msg);
			break;
		case 0xAA:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerCreatePrivateChannel(playerID); });
			break;
		case 0xAB:
			parseChannelInvite(msg);
			break;
		case 0xAC:
			parseChannelExclude(msg);
			break;
		case 0xBE:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerCancelAttackAndFollow(playerID); });
			break;
		case 0xC9: /* update tile */
			break;
		case 0xCA:
			parseUpdateContainer(msg);
			break;
		case 0xD2:
			g_dispatcher.addTask([playerID = player->getID()]() { g_game.playerRequestOutfit(playerID); });
			break;
		case 0xD3:
			parseSetOutfit(msg);
			break;
		case 0xDC:
			parseAddVip(msg);
			break;
		case 0xDD:
			parseRemoveVip(msg);
			break;
		case 0xE6:
			parseBugReport(msg);
			break;
		case 0xE7: /* thank you */
			break;
		case 0xF2:
			parseRuleViolationReport(msg);
			break;
		case 0xF3: /* get object info */
			break;
		case 0xF9:
			parseModalWindowAnswer(msg);
			break;

		default:
			g_dispatcher.addTask([=, playerID = player->getID(), msg = new NetworkMessage(msg)]() {
				g_game.parsePlayerNetworkMessage(playerID, recvbyte, msg);
			});
			break;
	}

	if (msg.isOverrun()) {
		disconnect();
	}
}

void ProtocolGame::GetTileDescription(const Tile* tile, NetworkMessage& msg)
{
	int32_t count = 0;
	if (const auto ground = tile->getGround()) {
		msg.addItem(ground, isOTCv8);
		++count;
	}

	const bool isStacked = player->getPosition() == tile->getPosition();

	const TileItemVector* items = tile->getItemList();
	if (items) {
		for (auto it = items->getBeginTopItem(), end = items->getEndTopItem(); it != end; ++it) {
			msg.addItem(*it, isOTCv8);

			if (!isOTCv8) {
				if (++count == 9 && isStacked) {
					break;
				} else if (count == MAX_STACKPOS_THINGS) {
					return;
				}
			} else if (++count == MAX_STACKPOS_THINGS) {
				break;
			}
		}
	}

	const CreatureVector* creatures = tile->getCreatures();
	if (creatures) {
		for (auto it = creatures->rbegin(), end = creatures->rend(); it != end; ++it) {
			if (!isOTCv8 && count == 9 && isStacked) {
				auto [known, removedKnown] = isKnownCreature(player->getID());
				AddCreature(msg, player, known, removedKnown);
			} else {
				const Creature* creature = (*it);
				if (!player->canSeeCreature(creature)) {
					continue;
				}

				auto [known, removedKnown] = isKnownCreature(creature->getID());
				AddCreature(msg, creature, known, removedKnown);
			}

			if (++count == MAX_STACKPOS_THINGS && !isOTCv8) {
				return;
			}
		}
	}

	if (items && count < MAX_STACKPOS_THINGS) {
		for (auto it = items->getBeginDownItem(), end = items->getEndDownItem(); it != end; ++it) {
			msg.addItem(*it, isOTCv8);

			if (++count == MAX_STACKPOS_THINGS) {
				return;
			}
		}
	}
}

void ProtocolGame::GetMapDescription(int32_t x, int32_t y, int32_t z, int32_t width, int32_t height,
                                     NetworkMessage& msg)
{
	int32_t skip = -1;
	int32_t startz, endz, zstep;

	if (z > 7) {
		startz = z - 2;
		endz = std::min<int32_t>(MAP_MAX_LAYERS - 1, z + 2);
		zstep = 1;
	} else {
		startz = 7;
		endz = 0;
		zstep = -1;
	}

	for (int32_t nz = startz; nz != endz + zstep; nz += zstep) {
		GetFloorDescription(msg, x, y, nz, width, height, z - nz, skip);
	}

	if (skip >= 0) {
		msg.addByte(static_cast<uint8_t>(skip));
		msg.addByte(0xFF);
	}
}

void ProtocolGame::GetFloorDescription(NetworkMessage& msg, int32_t x, int32_t y, int32_t z, int32_t width,
                                       int32_t height, int32_t offset, int32_t& skip)
{
	for (int32_t nx = 0; nx < width; nx++) {
		for (int32_t ny = 0; ny < height; ny++) {
			Tile* tile = g_game.map.getTile(static_cast<uint16_t>(x + nx + offset),
			                                static_cast<uint16_t>(y + ny + offset), static_cast<uint8_t>(z));
			if (tile) {
				if (skip >= 0) {
					msg.addByte(static_cast<uint8_t>(skip));
					msg.addByte(0xFF);
				}

				skip = 0;
				GetTileDescription(tile, msg);
			} else if (skip == 0xFE) {
				msg.addByte(0xFF);
				msg.addByte(0xFF);
				skip = -1;
			} else {
				++skip;
			}
		}
	}
}

std::pair<bool, uint32_t> ProtocolGame::isKnownCreature(uint32_t id)
{
	auto result = knownCreatureSet.insert(id);
	if (!result.second) {
		return std::make_pair(true, 0);
	}

	if (knownCreatureSet.size() > 250) {
		// Look for a creature to remove
		for (const uint32_t& creatureId : knownCreatureSet) {
			Creature* creature = g_game.getCreatureByID(creatureId);
			if (!canSee(creature)) {
				knownCreatureSet.erase(creatureId);
				return std::make_pair(false, creatureId);
			}
		}

		// Bad situation. Let's just remove anyone.
		auto creatureId = knownCreatureSet.begin();
		if (*creatureId == id) {
			++creatureId;
		}

		knownCreatureSet.erase(creatureId);
		return std::make_pair(false, *creatureId);
	}
	return {};
}

bool ProtocolGame::canSee(const Creature* c) const
{
	if (!c || !player || c->isRemoved()) {
		return false;
	}

	if (!player->canSeeCreature(c)) {
		return false;
	}

	return canSee(c->getPosition());
}

bool ProtocolGame::canSee(const Position& pos) const { return canSee(pos.x, pos.y, pos.z); }

bool ProtocolGame::canSee(int32_t x, int32_t y, int32_t z) const
{
	if (!player) {
		return false;
	}

	const Position& myPos = player->getPosition();
	if (myPos.z <= 7) {
		// we are on ground level or above (7 -> 0)
		// view is from 7 -> 0
		if (z > 7) {
			return false;
		}
	} else { // if (myPos.z >= 8) {
		// we are underground (8 -> 15)
		// view is +/- 2 from the floor we stand on
		if (std::abs(myPos.getZ() - z) > 2) {
			return false;
		}
	}

	// negative offset means that the action taken place is on a lower floor than ourself
	int32_t offsetz = myPos.getZ() - z;
	if ((x >= myPos.getX() - Map::maxClientViewportX + offsetz) &&
	    (x <= myPos.getX() + (Map::maxClientViewportX + 1) + offsetz) &&
	    (y >= myPos.getY() - Map::maxClientViewportY + offsetz) &&
	    (y <= myPos.getY() + (Map::maxClientViewportY + 1) + offsetz)) {
		return true;
	}
	return false;
}

// Parse methods
void ProtocolGame::parseChannelInvite(NetworkMessage& msg)
{
	auto name = msg.getString();
	g_dispatcher.addTask(
	    [playerID = player->getID(), name = std::string{name}]() { g_game.playerChannelInvite(playerID, name); });
}

void ProtocolGame::parseChannelExclude(NetworkMessage& msg)
{
	auto name = msg.getString();
	g_dispatcher.addTask(
	    [=, playerID = player->getID(), name = std::string{name}]() { g_game.playerChannelExclude(playerID, name); });
}

void ProtocolGame::parseOpenChannel(NetworkMessage& msg)
{
	uint16_t channelId = msg.get<uint16_t>();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerOpenChannel(playerID, channelId); });
}

void ProtocolGame::parseCloseChannel(NetworkMessage& msg)
{
	uint16_t channelId = msg.get<uint16_t>();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerCloseChannel(playerID, channelId); });
}

void ProtocolGame::parseOpenPrivateChannel(NetworkMessage& msg)
{
	auto receiver = msg.getString();
	g_dispatcher.addTask([playerID = player->getID(), receiver = std::string{receiver}]() {
		g_game.playerOpenPrivateChannel(playerID, receiver);
	});
}

void ProtocolGame::parseAutoWalk(NetworkMessage& msg)
{
	uint8_t numdirs = msg.getByte();
	if (numdirs == 0 || (msg.getBufferPosition() + numdirs) != (msg.getLength() + 8)) {
		return;
	}

	msg.skipBytes(numdirs);

	std::vector<Direction> path;
	path.reserve(numdirs);

	for (uint8_t i = 0; i < numdirs; ++i) {
		uint8_t rawdir = msg.getPreviousByte();
		switch (rawdir) {
			case 1:
				path.push_back(DIRECTION_EAST);
				break;
			case 2:
				path.push_back(DIRECTION_NORTHEAST);
				break;
			case 3:
				path.push_back(DIRECTION_NORTH);
				break;
			case 4:
				path.push_back(DIRECTION_NORTHWEST);
				break;
			case 5:
				path.push_back(DIRECTION_WEST);
				break;
			case 6:
				path.push_back(DIRECTION_SOUTHWEST);
				break;
			case 7:
				path.push_back(DIRECTION_SOUTH);
				break;
			case 8:
				path.push_back(DIRECTION_SOUTHEAST);
				break;
			default:
				break;
		}
	}

	if (path.empty()) {
		return;
	}

	g_dispatcher.addTask(
	    [playerID = player->getID(), path = std::move(path)]() { g_game.playerAutoWalk(playerID, path); });
}

void ProtocolGame::parseSetOutfit(NetworkMessage& msg)
{
	Outfit_t newOutfit;
	newOutfit.lookType = msg.get<uint16_t>();
	newOutfit.lookHead = msg.getByte();
	newOutfit.lookBody = msg.getByte();
	newOutfit.lookLegs = msg.getByte();
	newOutfit.lookFeet = msg.getByte();
	newOutfit.lookAddons = msg.getByte();
	newOutfit.lookMount = isOTCv8 ? msg.get<uint16_t>() : 0;
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerChangeOutfit(playerID, newOutfit); });
}

void ProtocolGame::parseUseItem(NetworkMessage& msg)
{
	Position pos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t stackpos = msg.getByte();
	uint8_t index = msg.getByte();
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, playerID = player->getID()]() {
		g_game.playerUseItem(playerID, pos, stackpos, index, spriteId);
	});
}

void ProtocolGame::parseUseItemEx(NetworkMessage& msg)
{
	Position fromPos = msg.getPosition();
	uint16_t fromSpriteId = msg.get<uint16_t>();
	uint8_t fromStackPos = msg.getByte();
	Position toPos = msg.getPosition();
	uint16_t toSpriteId = msg.get<uint16_t>();
	uint8_t toStackPos = msg.getByte();
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, playerID = player->getID()]() {
		g_game.playerUseItemEx(playerID, fromPos, fromStackPos, fromSpriteId, toPos, toStackPos, toSpriteId);
	});
}

void ProtocolGame::parseUseWithCreature(NetworkMessage& msg)
{
	Position fromPos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t fromStackPos = msg.getByte();
	uint32_t creatureId = msg.get<uint32_t>();
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, playerID = player->getID()]() {
		g_game.playerUseWithCreature(playerID, fromPos, fromStackPos, creatureId, spriteId);
	});
}

void ProtocolGame::parseCloseContainer(NetworkMessage& msg)
{
	uint8_t cid = msg.getByte();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerCloseContainer(playerID, cid); });
}

void ProtocolGame::parseUpArrowContainer(NetworkMessage& msg)
{
	uint8_t cid = msg.getByte();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerMoveUpContainer(playerID, cid); });
}

void ProtocolGame::parseUpdateContainer(NetworkMessage& msg)
{
	uint8_t cid = msg.getByte();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerUpdateContainer(playerID, cid); });
}

void ProtocolGame::parseThrow(NetworkMessage& msg)
{
	Position fromPos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t fromStackpos = msg.getByte();
	Position toPos = msg.getPosition();
	uint8_t count = msg.getByte();

	if (toPos != fromPos) {
		g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, playerID = player->getID()]() {
			g_game.playerMoveThing(playerID, fromPos, spriteId, fromStackpos, toPos, count);
		});
	}
}

void ProtocolGame::parseLookAt(NetworkMessage& msg)
{
	Position pos = msg.getPosition();
	msg.skipBytes(2); // spriteId
	uint8_t stackpos = msg.getByte();
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, playerID = player->getID()]() { g_game.playerLookAt(playerID, pos, stackpos); });
}

void ProtocolGame::parseLookInBattleList(NetworkMessage& msg)
{
	uint32_t creatureId = msg.get<uint32_t>();
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, playerID = player->getID()]() { g_game.playerLookInBattleList(playerID, creatureId); });
}

void ProtocolGame::parseSay(NetworkMessage& msg)
{
	std::string_view receiver;
	uint16_t channelId;

	SpeakClasses type = static_cast<SpeakClasses>(msg.getByte());
	switch (type) {
		case TALKTYPE_PRIVATE:
		case TALKTYPE_PRIVATE_RED:
			receiver = msg.getString();
			channelId = 0;
			break;

		case TALKTYPE_CHANNEL_Y:
		case TALKTYPE_CHANNEL_R1:
		case TALKTYPE_CHANNEL_R2:
			channelId = msg.get<uint16_t>();
			break;

		default:
			channelId = 0;
			break;
	}

	auto text = msg.getString();
	if (text.length() > 255) {
		return;
	}

	g_dispatcher.addTask([=, playerID = player->getID(), receiver = std::string{receiver}, text = std::string{text}]() {
		g_game.playerSay(playerID, channelId, type, receiver, text);
	});
}

void ProtocolGame::parseFightModes(NetworkMessage& msg)
{
	uint8_t rawFightMode = msg.getByte();  // 1 - offensive, 2 - balanced, 3 - defensive
	uint8_t rawChaseMode = msg.getByte();  // 0 - stand while fighting, 1 - chase opponent
	uint8_t rawSecureMode = msg.getByte(); // 0 - can't attack unmarked, 1 - can attack unmarked

	fightMode_t fightMode;
	if (rawFightMode == 1) {
		fightMode = FIGHTMODE_ATTACK;
	} else if (rawFightMode == 2) {
		fightMode = FIGHTMODE_BALANCED;
	} else {
		fightMode = FIGHTMODE_DEFENSE;
	}

	g_dispatcher.addTask([=, playerID = player->getID()]() {
		g_game.playerSetFightModes(playerID, fightMode, rawChaseMode != 0, rawSecureMode != 0);
	});
}

void ProtocolGame::parseAttack(NetworkMessage& msg)
{
	uint32_t creatureId = msg.get<uint32_t>();
	// msg.get<uint32_t>(); creatureId (same as above)
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerSetAttackedCreature(playerID, creatureId); });
}

void ProtocolGame::parseFollow(NetworkMessage& msg)
{
	uint32_t creatureId = msg.get<uint32_t>();
	// msg.get<uint32_t>(); creatureId (same as above)
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerFollowCreature(playerID, creatureId); });
}

void ProtocolGame::parseTextWindow(NetworkMessage& msg)
{
	uint32_t windowTextID = msg.get<uint32_t>();
	auto newText = msg.getString();
	g_dispatcher.addTask([playerID = player->getID(), windowTextID, newText = std::string{newText}]() {
		g_game.playerWriteItem(playerID, windowTextID, newText);
	});
}

void ProtocolGame::parseHouseWindow(NetworkMessage& msg)
{
	uint8_t doorId = msg.getByte();
	uint32_t id = msg.get<uint32_t>();
	auto text = msg.getString();
	g_dispatcher.addTask([=, playerID = player->getID(), text = std::string{text}]() {
		g_game.playerUpdateHouseWindow(playerID, doorId, id, text);
	});
}

void ProtocolGame::parseLookInShop(NetworkMessage& msg)
{
	uint16_t id = msg.get<uint16_t>();
	uint8_t count = msg.getByte();
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION,
	                     [=, playerID = player->getID()]() { g_game.playerLookInShop(playerID, id, count); });
}

void ProtocolGame::parsePlayerPurchase(NetworkMessage& msg)
{
	uint16_t id = msg.get<uint16_t>();
	uint8_t count = msg.getByte();
	uint8_t amount = msg.getByte();
	bool ignoreCap = msg.getByte() != 0;
	bool inBackpacks = msg.getByte() != 0;
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, playerID = player->getID()]() {
		g_game.playerPurchaseItem(playerID, id, count, amount, ignoreCap, inBackpacks);
	});
}

void ProtocolGame::parsePlayerSale(NetworkMessage& msg)
{
	uint16_t id = msg.get<uint16_t>();
	uint8_t count = msg.getByte();
	uint8_t amount = msg.getByte();
	bool ignoreEquipped = msg.getByte() != 0;
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, playerID = player->getID()]() {
		g_game.playerSellItem(playerID, id, count, amount, ignoreEquipped);
	});
}

void ProtocolGame::parseRequestTrade(NetworkMessage& msg)
{
	Position pos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t stackpos = msg.getByte();
	uint32_t playerId = msg.get<uint32_t>();
	g_dispatcher.addTask(
	    [=, playerID = player->getID()]() { g_game.playerRequestTrade(playerID, pos, stackpos, playerId, spriteId); });
}

void ProtocolGame::parseLookInTrade(NetworkMessage& msg)
{
	bool counterOffer = (msg.getByte() == 0x01);
	uint8_t index = msg.getByte();
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, playerID = player->getID()]() {
		g_game.playerLookInTrade(playerID, counterOffer, index);
	});
}

void ProtocolGame::parseAddVip(NetworkMessage& msg)
{
	auto name = msg.getString();
	g_dispatcher.addTask(
	    [playerID = player->getID(), name = std::string{name}]() { g_game.playerRequestAddVip(playerID, name); });
}

void ProtocolGame::parseRemoveVip(NetworkMessage& msg)
{
	uint32_t guid = msg.get<uint32_t>();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerRequestRemoveVip(playerID, guid); });
}

void ProtocolGame::parseRotateItem(NetworkMessage& msg)
{
	Position pos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t stackpos = msg.getByte();
	g_dispatcher.addTask(DISPATCHER_TASK_EXPIRATION, [=, playerID = player->getID()]() {
		g_game.playerRotateItem(playerID, pos, stackpos, spriteId);
	});
}

void ProtocolGame::parseRuleViolationReport(NetworkMessage& msg)
{
	uint8_t reportType = msg.getByte();
	uint8_t reportReason = msg.getByte();
	auto targetName = msg.getString();
	auto comment = msg.getString();
	std::string_view translation;
	if (reportType == REPORT_TYPE_NAME) {
		translation = msg.getString();
	} else if (reportType == REPORT_TYPE_STATEMENT) {
		translation = msg.getString();
		msg.get<uint32_t>(); // statement id, used to get whatever player have said, we don't log that.
	}

	g_dispatcher.addTask([=, playerID = player->getID(), targetName = std::string{targetName},
	                      comment = std::string{comment}, translation = std::string{translation}]() {
		g_game.playerReportRuleViolation(playerID, targetName, reportType, reportReason, comment, translation);
	});
}

void ProtocolGame::parseBugReport(NetworkMessage& msg)
{
	auto message = msg.getString();
	g_dispatcher.addTask([=, playerID = player->getID(), message = std::string{message}]() {
		g_game.playerReportBug(playerID, message);
	});
}

void ProtocolGame::parseInviteToParty(NetworkMessage& msg)
{
	uint32_t targetId = msg.get<uint32_t>();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerInviteToParty(playerID, targetId); });
}

void ProtocolGame::parseJoinParty(NetworkMessage& msg)
{
	uint32_t targetId = msg.get<uint32_t>();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerJoinParty(playerID, targetId); });
}

void ProtocolGame::parseRevokePartyInvite(NetworkMessage& msg)
{
	uint32_t targetId = msg.get<uint32_t>();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerRevokePartyInvitation(playerID, targetId); });
}

void ProtocolGame::parsePassPartyLeadership(NetworkMessage& msg)
{
	uint32_t targetId = msg.get<uint32_t>();
	g_dispatcher.addTask([=, playerID = player->getID()]() { g_game.playerPassPartyLeadership(playerID, targetId); });
}

void ProtocolGame::parseEnableSharedPartyExperience(NetworkMessage& msg)
{
	bool sharedExpActive = msg.getByte() == 1;
	g_dispatcher.addTask(
	    [=, playerID = player->getID()]() { g_game.playerEnableSharedPartyExperience(playerID, sharedExpActive); });
}

void ProtocolGame::parseModalWindowAnswer(NetworkMessage& msg)
{
	if (!isOTCv8) {
		return;
	}

	uint32_t id = msg.get<uint32_t>();
	uint8_t button = msg.getByte();
	uint8_t choice = msg.getByte();
	g_dispatcher.addTask(
	    [=, playerID = player->getID()]() { g_game.playerAnswerModalWindow(playerID, id, button, choice); });
}

// Send methods
void ProtocolGame::sendOpenPrivateChannel(std::string_view receiver)
{
	NetworkMessage msg;
	msg.addByte(0xAD);
	msg.addString(receiver);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureOutfit(const Creature* creature, const Outfit_t& outfit)
{
	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x8E);
	msg.add<uint32_t>(creature->getID());
	AddOutfit(msg, outfit);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureLight(const Creature* creature)
{
	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	AddCreatureLight(msg, creature);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureWalkthrough(const Creature* creature, bool walkthrough)
{
	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x92);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(walkthrough ? 0x00 : 0x01);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureShield(const Creature* creature)
{
	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x91);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(player->getPartyShield(creature->getPlayer()));
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureSkull(const Creature* creature)
{
	if (g_game.getWorldType() != WORLD_TYPE_PVP) {
		return;
	}

	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x90);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(player->getSkullClient(creature));
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureSquare(const Creature* creature, SquareColor_t color)
{
	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x86);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(color);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendTutorial(uint8_t tutorialId)
{
	NetworkMessage msg;
	msg.addByte(0xDC);
	msg.addByte(tutorialId);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendAddMarker(const Position& pos, uint8_t markType, std::string_view desc)
{
	NetworkMessage msg;
	msg.addByte(0xDD);
	msg.addPosition(pos);
	msg.addByte(markType);
	msg.addString(desc);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendReLoginWindow()
{
	NetworkMessage msg;
	msg.addByte(0x28);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendStats()
{
	NetworkMessage msg;
	AddPlayerStats(msg);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendTextMessage(const TextMessage& message)
{
	NetworkMessage msg;
	msg.addByte(0xB4);
	msg.addByte(message.type);
	msg.addString(message.text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendClosePrivate(uint16_t channelId)
{
	NetworkMessage msg;
	msg.addByte(0xB3);
	msg.add<uint16_t>(channelId);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatePrivateChannel(uint16_t channelId, std::string_view channelName)
{
	NetworkMessage msg;
	msg.addByte(0xB2);
	msg.add<uint16_t>(channelId);
	msg.addString(channelName);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendChannelsDialog()
{
	NetworkMessage msg;
	msg.addByte(0xAB);

	const ChannelList& list = g_chat->getChannelList(*player);
	msg.addByte(list.size());
	for (const ChatChannel* channel : list) {
		msg.add<uint16_t>(channel->getId());
		msg.addString(channel->getName());
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendChannel(uint16_t channelId, std::string_view channelName)
{
	NetworkMessage msg;
	msg.addByte(0xAC);
	msg.add<uint16_t>(channelId);
	msg.addString(channelName);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendChannelMessage(std::string_view author, std::string_view text, SpeakClasses type,
                                      uint16_t channel)
{
	NetworkMessage msg;
	msg.addByte(0xAA);
	msg.add<uint32_t>(0x00);
	msg.addString(author);
	msg.add<uint16_t>(0x00);
	msg.addByte(type);
	msg.add<uint16_t>(channel);
	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendIcons(uint16_t icons)
{
	NetworkMessage msg;
	msg.addByte(0xA2);
	msg.add<uint16_t>(icons);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendContainer(uint8_t cid, const Container* container, bool hasParent, uint16_t firstIndex)
{
	NetworkMessage msg;
	msg.addByte(0x6E);

	msg.addByte(cid);

	msg.addItem(container, isOTCv8);
	msg.addString(container->getName());

	msg.addByte(static_cast<uint8_t>(container->capacity()));

	msg.addByte(hasParent ? 0x01 : 0x00);

	msg.addByte(static_cast<uint8_t>(std::min<uint32_t>(0xFF, container->size())));

	uint32_t i = 0;
	const ItemDeque& itemList = container->getItemList();
	for (ItemDeque::const_iterator cit = itemList.begin() + firstIndex, end = itemList.end(); i < 0xFF && cit != end;
	     ++cit, ++i) {
		msg.addItem(*cit, isOTCv8);
	}
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendShop(const ShopInfoList& itemList)
{
	NetworkMessage msg;
	msg.addByte(0x7A);

	uint16_t itemsToSend = std::min<size_t>(itemList.size(), std::numeric_limits<uint16_t>::max());
	msg.addByte(itemsToSend);

	uint16_t i = 0;
	for (auto it = itemList.begin(); i < itemsToSend; ++it, ++i) {
		AddShopItem(msg, *it);
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCloseShop()
{
	NetworkMessage msg;
	msg.addByte(0x7C);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendSaleItemList(const std::list<ShopInfo>& shop)
{
	NetworkMessage msg;
	msg.addByte(0x7B);
	msg.add<uint32_t>(player->getMoney());

	std::map<uint16_t, uint32_t> saleMap;

	if (shop.size() <= 5) {
		// For very small shops it's not worth it to create the complete map
		for (const ShopInfo& shopInfo : shop) {
			if (shopInfo.sellPrice == 0) {
				continue;
			}

			int8_t subtype = -1;

			const ItemType& itemType = Item::items[shopInfo.itemId];
			if (itemType.hasSubType() && !itemType.stackable) {
				subtype = (shopInfo.subType == 0 ? -1 : shopInfo.subType);
			}

			uint32_t count = player->getItemTypeCount(shopInfo.itemId, subtype);
			if (count > 0) {
				saleMap[shopInfo.itemId] = count;
			}
		}
	} else {
		// Large shop, it's better to get a cached map of all item counts and use it
		// We need a temporary map since the finished map should only contain items
		// available in the shop
		std::map<uint32_t, uint32_t> tempSaleMap;
		player->getAllItemTypeCount(tempSaleMap);

		// We must still check manually for the special items that require subtype matches
		// (That is, fluids such as potions etc., actually these items are very few since
		// health potions now use their own ID)
		for (const ShopInfo& shopInfo : shop) {
			if (shopInfo.sellPrice == 0) {
				continue;
			}

			int8_t subtype = -1;

			const ItemType& itemType = Item::items[shopInfo.itemId];
			if (itemType.hasSubType() && !itemType.stackable) {
				subtype = (shopInfo.subType == 0 ? -1 : shopInfo.subType);
			}

			if (subtype != -1) {
				uint32_t count;
				if (itemType.isFluidContainer() || itemType.isSplash()) {
					count = player->getItemTypeCount(shopInfo.itemId, subtype); // This shop item requires extra checks
				} else {
					count = subtype;
				}

				if (count > 0) {
					saleMap[shopInfo.itemId] = count;
				}
			} else {
				std::map<uint32_t, uint32_t>::const_iterator findIt = tempSaleMap.find(shopInfo.itemId);
				if (findIt != tempSaleMap.end() && findIt->second > 0) {
					saleMap[shopInfo.itemId] = findIt->second;
				}
			}
		}
	}

	uint8_t itemsToSend = std::min<size_t>(saleMap.size(), std::numeric_limits<uint8_t>::max());
	msg.addByte(itemsToSend);

	uint8_t i = 0;
	for (std::map<uint16_t, uint32_t>::const_iterator it = saleMap.begin(); i < itemsToSend; ++it, ++i) {
		msg.addItemId(it->first, isOTCv8);
		msg.addByte(static_cast<uint8_t>(std::min<uint32_t>(it->second, std::numeric_limits<uint8_t>::max())));
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendTradeItemRequest(std::string_view traderName, const Item* item, bool ack)
{
	NetworkMessage msg;

	if (ack) {
		msg.addByte(0x7D);
	} else {
		msg.addByte(0x7E);
	}

	msg.addString(traderName);

	if (const Container* tradeContainer = item->getContainer()) {
		std::list<const Container*> listContainer{tradeContainer};
		std::list<const Item*> itemList{tradeContainer};
		while (!listContainer.empty()) {
			const Container* container = listContainer.front();
			listContainer.pop_front();

			for (Item* containerItem : container->getItemList()) {
				Container* tmpContainer = containerItem->getContainer();
				if (tmpContainer) {
					listContainer.push_back(tmpContainer);
				}
				itemList.push_back(containerItem);
			}
		}

		msg.addByte(itemList.size());
		for (const Item* listItem : itemList) {
			msg.addItem(listItem, isOTCv8);
		}
	} else {
		msg.addByte(0x01);
		msg.addItem(item, isOTCv8);
	}
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCloseTrade()
{
	NetworkMessage msg;
	msg.addByte(0x7F);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCloseContainer(uint8_t cid)
{
	NetworkMessage msg;
	msg.addByte(0x6F);
	msg.addByte(cid);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureTurn(const Creature* creature, uint32_t stackpos)
{
	if (stackpos >= MAX_STACKPOS_THINGS || !canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x6B);
	msg.addPosition(creature->getPosition());
	msg.addByte(static_cast<uint8_t>(stackpos));

	msg.add<uint16_t>(0x63);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(creature->getDirection());
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureSay(const Creature* creature, SpeakClasses type, std::string_view text,
                                   const Position* pos /* = nullptr*/)
{
	if (!creature) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0xAA);
	msg.add<uint32_t>(0x00);

	msg.addString(creature->getName());

	// Add level only for players
	if (const Player* speaker = creature->getPlayer()) {
		if (!speaker->isAccessPlayer() && !speaker->isAccountManager()) {
			msg.add<uint16_t>(static_cast<uint16_t>(speaker->getLevel()));
		} else {
			msg.add<uint16_t>(0x00);
		}
	} else {
		msg.add<uint16_t>(0x00);
	}

	msg.addByte(type);
	if (pos) {
		msg.addPosition(*pos);
	} else {
		msg.addPosition(creature->getPosition());
	}

	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendToChannel(const Creature* creature, SpeakClasses type, std::string_view text, uint16_t channelId)
{
	if (!creature) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0xAA);
	msg.add<uint32_t>(0x00);

	if (type == TALKTYPE_CHANNEL_R2) {
		msg.addString("");
		type = TALKTYPE_CHANNEL_R1;
	} else {
		msg.addString(creature->getName());
		// Add level only for players
		if (const Player* speaker = creature->getPlayer()) {
			if (!speaker->isAccessPlayer() && !speaker->isAccountManager()) {
				msg.add<uint16_t>(static_cast<uint16_t>(speaker->getLevel()));
			} else {
				msg.add<uint16_t>(0x00);
			}
		} else {
			msg.add<uint16_t>(0x00);
		}
	}

	msg.addByte(type);
	msg.add<uint16_t>(channelId);
	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendPrivateMessage(const Player* speaker, SpeakClasses type, std::string_view text)
{
	NetworkMessage msg;
	msg.addByte(0xAA);
	static uint32_t statementId = 0;
	msg.add<uint32_t>(++statementId);
	if (speaker) {
		msg.addString(speaker->getName());
		if (!speaker->isAccessPlayer() && !speaker->isAccountManager()) {
			msg.add<uint16_t>(static_cast<uint16_t>(speaker->getLevel()));
		} else {
			msg.add<uint32_t>(0x00);
		}
	} else {
		msg.add<uint32_t>(0x00);
	}
	msg.addByte(type);
	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCancelTarget()
{
	NetworkMessage msg;
	msg.addByte(0xA3);
	msg.add<uint32_t>(0x00);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendChangeSpeed(const Creature* creature, uint32_t speed)
{
	NetworkMessage msg;
	msg.addByte(0x8F);
	msg.add<uint32_t>(creature->getID());
	msg.add<uint16_t>(static_cast<uint16_t>(speed));
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCancelWalk()
{
	NetworkMessage msg;
	msg.addByte(0xB5);
	msg.addByte(player->getDirection());
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendSkills()
{
	NetworkMessage msg;
	AddPlayerSkills(msg);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendPing()
{
	NetworkMessage msg;
	msg.addByte(0x1E);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendDistanceShoot(const Position& from, const Position& to, uint8_t type)
{
	NetworkMessage msg;
	msg.addByte(0x85);
	msg.addPosition(from);
	msg.addPosition(to);
	msg.addByte(type);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendMagicEffect(const Position& pos, uint8_t type)
{
	if (!canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x83);
	msg.addPosition(pos);
	msg.addByte(type);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureHealth(const Creature* creature)
{
	NetworkMessage msg;
	msg.addByte(0x8C);
	msg.add<uint32_t>(creature->getID());

	if (creature->isHealthHidden()) {
		msg.addByte(0x00);
	} else {
		msg.addByte(std::ceil(
		    (static_cast<double>(creature->getHealth()) / std::max<int32_t>(creature->getMaxHealth(), 1)) * 100));
	}
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendFYIBox(std::string_view message)
{
	NetworkMessage msg;
	msg.addByte(0x15);
	msg.addString(message);
	writeToOutputBuffer(msg);
}

// tile
void ProtocolGame::sendMapDescription(const Position& pos)
{
	NetworkMessage msg;
	msg.addByte(0x64);
	msg.addPosition(player->getPosition());
	GetMapDescription(pos.x - Map::maxClientViewportX, pos.y - Map::maxClientViewportY, pos.z,
	                  (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, msg);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendAddTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	if (stackpos >= MAX_STACKPOS_THINGS || !canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x6A);
	msg.addPosition(pos);
	msg.addByte(static_cast<uint8_t>(stackpos));
	msg.addItem(item, isOTCv8);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	if (stackpos >= MAX_STACKPOS_THINGS || !canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x6B);
	msg.addPosition(pos);
	msg.addByte(static_cast<uint8_t>(stackpos));
	msg.addItem(item, isOTCv8);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendRemoveTileThing(const Position& pos, uint32_t stackpos)
{
	if (!canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	RemoveTileThing(msg, pos, stackpos);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdateTileCreature(const Position& pos, uint32_t stackpos, const Creature* creature)
{
	if (stackpos >= MAX_STACKPOS_THINGS || !canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x6B);
	msg.addPosition(pos);
	msg.addByte(static_cast<uint8_t>(stackpos));

	auto [known, removedKnown] = isKnownCreature(creature->getID());
	AddCreature(msg, creature, false, removedKnown);

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdateTile(const Tile* tile, const Position& pos)
{
	if (!canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x69);
	msg.addPosition(pos);

	if (tile) {
		GetTileDescription(tile, msg);
		msg.addByte(0x00);
		msg.addByte(0xFF);
	} else {
		msg.addByte(0x01);
		msg.addByte(0xFF);
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendFightModes()
{
	NetworkMessage msg;
	msg.addByte(0xA7);
	msg.addByte(player->fightMode);
	msg.addByte(player->chaseMode);
	msg.addByte(player->secureMode);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendAddCreature(const Creature* creature, const Position& pos, int32_t stackpos,
                                   MagicEffectClasses magicEffect /*= CONST_ME_NONE*/)
{
	if (!canSee(pos)) {
		return;
	}

	if (creature != player) {
		if (stackpos != -1 && stackpos < MAX_STACKPOS_THINGS) {
			NetworkMessage msg;
			msg.addByte(0x6A);
			msg.addPosition(pos);
			msg.addByte(static_cast<uint8_t>(stackpos));

			auto [known, removedKnown] = isKnownCreature(creature->getID());
			AddCreature(msg, creature, known, removedKnown);
			writeToOutputBuffer(msg);
		}

		if (magicEffect != CONST_ME_NONE) {
			sendMagicEffect(pos, magicEffect);
		}
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x0A);

	msg.add<uint32_t>(player->getID());
	msg.add<uint16_t>(0x32); // beat duration (50)

	// can report bugs?
	if (player->getAccountType() >= ACCOUNT_TYPE_TUTOR) {
		msg.addByte(0x01);
	} else {
		msg.addByte(0x00);
	}

	writeToOutputBuffer(msg);

	sendMapDescription(pos);

	if (magicEffect != CONST_ME_NONE) {
		sendMagicEffect(pos, magicEffect);
	}

	for (int i = CONST_SLOT_FIRST; i <= CONST_SLOT_LAST; ++i) {
		sendInventoryItem(static_cast<slots_t>(i), player->getInventoryItem(static_cast<slots_t>(i)));
	}

	sendStats();
	sendSkills();

	// player light level
	sendCreatureLight(creature);

	const std::forward_list<VIPEntry>& vipEntries = IOLoginData::getVIPEntries(player->getAccount());
	for (const VIPEntry& entry : vipEntries) {
		Player* vipPlayer = g_game.getPlayerByGUID(entry.guid);

		sendVIP(entry.guid, entry.name,
		        static_cast<VipStatus_t>((vipPlayer && (!vipPlayer->isInGhostMode() || player->isAccessPlayer()))));
	}

	player->sendIcons();
}

void ProtocolGame::sendMoveCreature(const Creature* creature, const Position& newPos, int32_t newStackPos,
                                    const Position& oldPos, int32_t oldStackPos, bool teleport)
{
	if (creature == player) {
		if (teleport || oldStackPos >= MAX_STACKPOS_THINGS) {
			sendRemoveTileThing(oldPos, oldStackPos);
			sendMapDescription(newPos);
		} else {
			NetworkMessage msg;
			if (oldPos.z == 7 && newPos.z >= 8) {
				RemoveTileThing(msg, oldPos, oldStackPos);
			} else {
				msg.addByte(0x6D);
				msg.addPosition(oldPos);
				msg.addByte(static_cast<uint8_t>(oldStackPos));
				msg.addPosition(newPos);
			}

			if (newPos.z > oldPos.z) {
				MoveDownCreature(msg, creature, newPos, oldPos);
			} else if (newPos.z < oldPos.z) {
				MoveUpCreature(msg, creature, newPos, oldPos);
			}

			if (!isOTCv8 && newStackPos >= MAX_STACKPOS_THINGS) {
				msg.addByte(0x64);
				msg.addPosition(player->getPosition());
				GetMapDescription(newPos.x - Map::maxClientViewportX, newPos.y - Map::maxClientViewportY, newPos.z,
				                  (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, msg);
			} else {
				if (oldPos.y > newPos.y) { // north, for old x
					msg.addByte(0x65);
					GetMapDescription(oldPos.x - Map::maxClientViewportX, newPos.y - Map::maxClientViewportY, newPos.z,
					                  (Map::maxClientViewportX * 2) + 2, 1, msg);
				} else if (oldPos.y < newPos.y) { // south, for old x
					msg.addByte(0x67);
					GetMapDescription(oldPos.x - Map::maxClientViewportX, newPos.y + (Map::maxClientViewportY + 1),
					                  newPos.z, (Map::maxClientViewportX * 2) + 2, 1, msg);
				}

				if (oldPos.x < newPos.x) { // east, [with new y]
					msg.addByte(0x66);
					GetMapDescription(newPos.x + (Map::maxClientViewportX + 1), newPos.y - Map::maxClientViewportY,
					                  newPos.z, 1, (Map::maxClientViewportY * 2) + 2, msg);
				} else if (oldPos.x > newPos.x) { // west, [with new y]
					msg.addByte(0x68);
					GetMapDescription(newPos.x - Map::maxClientViewportX, newPos.y - Map::maxClientViewportY, newPos.z,
					                  1, (Map::maxClientViewportY * 2) + 2, msg);
				}
			}
			writeToOutputBuffer(msg);
		}
	} else if (canSee(oldPos) && canSee(creature->getPosition())) {
		if (teleport || (oldPos.z == 7 && newPos.z >= 8) || oldStackPos >= MAX_STACKPOS_THINGS) {
			sendRemoveTileThing(oldPos, oldStackPos);
			sendAddCreature(creature, newPos, newStackPos);
		} else {
			NetworkMessage msg;
			msg.addByte(0x6D);
			msg.addPosition(oldPos);
			msg.addByte(static_cast<uint8_t>(oldStackPos));
			msg.addPosition(creature->getPosition());
			writeToOutputBuffer(msg);
		}
	} else if (canSee(oldPos)) {
		sendRemoveTileThing(oldPos, oldStackPos);
	} else if (canSee(creature->getPosition())) {
		sendAddCreature(creature, newPos, newStackPos);
	}
}

void ProtocolGame::sendInventoryItem(slots_t slot, const Item* item)
{
	NetworkMessage msg;
	if (item) {
		msg.addByte(0x78);
		msg.addByte(slot);
		msg.addItem(item, isOTCv8);
	} else {
		msg.addByte(0x79);
		msg.addByte(slot);
	}
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendModalWindow(const ModalWindow& modalWindow)
{
	if (!isOTCv8) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0xFA);

	msg.add<uint32_t>(modalWindow.id);
	msg.addString(modalWindow.title);
	msg.addString(modalWindow.message);

	msg.addByte(modalWindow.buttons.size());
	for (const auto& it : modalWindow.buttons) {
		msg.addString(it.first);
		msg.addByte(it.second);
	}

	msg.addByte(modalWindow.choices.size());
	for (const auto& it : modalWindow.choices) {
		msg.addString(it.first);
		msg.addByte(it.second);
	}

	msg.addByte(modalWindow.defaultEscapeButton);
	msg.addByte(modalWindow.defaultEnterButton);
	msg.addByte(modalWindow.priority ? 0x01 : 0x00);

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendAddContainerItem(uint8_t cid, const Item* item)
{
	NetworkMessage msg;
	msg.addByte(0x70);
	msg.addByte(cid);
	msg.addItem(item, isOTCv8);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdateContainerItem(uint8_t cid, uint16_t slot, const Item* item)
{
	NetworkMessage msg;
	msg.addByte(0x71);
	msg.addByte(cid);
	msg.addByte(slot);
	msg.addItem(item, isOTCv8);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendRemoveContainerItem(uint8_t cid, uint16_t slot)
{
	NetworkMessage msg;
	msg.addByte(0x72);
	msg.addByte(cid);
	msg.addByte(slot);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendTextWindow(uint32_t windowTextId, Item* item, uint16_t maxlen, bool canWrite)
{
	NetworkMessage msg;
	msg.addByte(0x96);
	msg.add<uint32_t>(windowTextId);
	msg.addItem(item, isOTCv8);

	if (canWrite) {
		msg.add<uint16_t>(maxlen);
		msg.addString(item->getText());
	} else {
		auto text = item->getText();
		msg.add<uint16_t>(text.size());
		msg.addString(text);
	}

	auto writer = item->getWriter();
	if (!writer.empty()) {
		msg.addString(writer);
	} else {
		msg.add<uint16_t>(0x00);
	}

	time_t writtenDate = item->getDate();
	if (writtenDate != 0) {
		msg.addString(formatDateShort(writtenDate));
	} else {
		msg.add<uint16_t>(0x00);
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendTextWindow(uint32_t windowTextId, uint16_t itemId, std::string_view text)
{
	NetworkMessage msg;
	msg.addByte(0x96);
	msg.add<uint32_t>(windowTextId);
	msg.addItem(itemId, 1, isOTCv8);
	msg.add<uint16_t>(text.size());
	msg.addString(text);
	msg.add<uint16_t>(0x00);
	msg.add<uint16_t>(0x00);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendHouseWindow(uint32_t windowTextId, std::string_view text)
{
	NetworkMessage msg;
	msg.addByte(0x97);
	msg.addByte(0x00);
	msg.add<uint32_t>(windowTextId);
	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendOutfitWindow()
{
	const auto& outfits = Outfits::getInstance().getOutfits(player->getSex());
	if (outfits.size() == 0) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0xC8);

	Outfit_t currentOutfit = player->getDefaultOutfit();
	if (currentOutfit.lookType == 0) {
		Outfit_t newOutfit;
		newOutfit.lookType = outfits.front()->lookType;
		currentOutfit = newOutfit;
	}

	Mount* currentMount = g_game.mounts.getMountByID(player->getCurrentMount());
	if (currentMount) {
		currentOutfit.lookMount = currentMount->clientId;
	}

	/*bool mounted;
	if (player->wasMounted) {
	    mounted = currentOutfit.lookMount != 0;
	} else {
	    mounted = player->isMounted();
	}*/

	AddOutfit(msg, currentOutfit);

	std::vector<ProtocolOutfit> protocolOutfits;
	if (player->isAccessPlayer()) {
		protocolOutfits.emplace_back("Gamemaster", 75, 0);
	}

	size_t maxProtocolOutfits = static_cast<size_t>(getInteger(ConfigManager::MAX_PROTOCOL_OUTFITS));
	if (isOTCv8) {
		maxProtocolOutfits = std::numeric_limits<uint8_t>::max();
	}

	for (const Outfit* outfit : outfits) {
		uint8_t addons;
		if (!player->getOutfitAddons(*outfit, addons)) {
			continue;
		}

		protocolOutfits.emplace_back(outfit->name, outfit->lookType, addons);
		if (protocolOutfits.size() == maxProtocolOutfits) {
			break;
		}
	}

	msg.addByte(protocolOutfits.size());
	for (const ProtocolOutfit& outfit : protocolOutfits) {
		msg.add<uint16_t>(outfit.lookType);
		msg.addString(outfit.name);
		msg.addByte(outfit.addons);
	}

	if (isOTCv8) {
		std::vector<const Mount*> mounts;
		for (const Mount& mount : g_game.mounts.getMounts()) {
			if (player->hasMount(&mount)) {
				mounts.push_back(&mount);
			}
		}

		msg.addByte(mounts.size());
		for (const Mount* mount : mounts) {
			msg.add<uint16_t>(mount->clientId);
			msg.addString(mount->name);
		}
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdatedVIPStatus(uint32_t guid, VipStatus_t newStatus)
{
	NetworkMessage msg;
	msg.addByte(newStatus == VIPSTATUS_ONLINE ? 0xD3 : 0xD4);
	msg.add<uint32_t>(guid);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendVIP(uint32_t guid, std::string_view name, VipStatus_t status)
{
	NetworkMessage msg;
	msg.addByte(0xD2);
	msg.add<uint32_t>(guid);
	msg.addString(name);
	msg.addByte(status);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendAnimatedText(std::string_view message, const Position& pos, TextColor_t color)
{
	if (!canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x84);
	msg.addPosition(pos);
	msg.addByte(color);
	msg.addString(message);
	writeToOutputBuffer(msg);
}

////////////// Add common messages
void ProtocolGame::AddCreature(NetworkMessage& msg, const Creature* creature, bool known, uint32_t remove)
{
	const Player* otherPlayer = creature->getPlayer();
	if (known) {
		msg.add<uint16_t>(0x62);
		msg.add<uint32_t>(creature->getID());
	} else {
		msg.add<uint16_t>(0x61);
		msg.add<uint32_t>(remove);
		msg.add<uint32_t>(creature->getID());
		msg.addString(creature->getName());
	}

	if (creature->isHealthHidden()) {
		msg.addByte(0x00);
	} else {
		msg.addByte(static_cast<uint8_t>(std::ceil(
		    (static_cast<double>(creature->getHealth()) / std::max<int32_t>(creature->getMaxHealth(), 1)) * 100)));
	}

	msg.addByte(creature->getDirection());

	if (!creature->isInGhostMode() && !creature->isInvisible()) {
		AddOutfit(msg, creature->getCurrentOutfit());
	} else {
		static Outfit_t outfit;
		AddOutfit(msg, outfit);
	}

	LightInfo lightInfo = creature->getCreatureLight();
	msg.addByte(player->isAccessPlayer() ? 0xFF : lightInfo.level);
	msg.addByte(lightInfo.color);

	msg.add<uint16_t>(static_cast<uint16_t>(creature->getStepSpeed()));

	msg.addByte(player->getSkullClient(creature));
	msg.addByte(player->getPartyShield(otherPlayer));

	if (!known) {
		msg.addByte(player->getGuildEmblem(otherPlayer));
	}

	msg.addByte(player->canWalkthroughEx(creature) ? 0x00 : 0x01);
}

void ProtocolGame::AddPlayerStats(NetworkMessage& msg)
{
	msg.addByte(0xA0);

	msg.add<uint16_t>(
	    static_cast<uint16_t>(std::min<uint32_t>(player->getHealth(), std::numeric_limits<uint16_t>::max())));
	msg.add<uint16_t>(
	    static_cast<uint16_t>(std::min<uint32_t>(player->getMaxHealth(), std::numeric_limits<uint16_t>::max())));

	msg.add<uint32_t>(player->hasFlag(PlayerFlag_HasInfiniteCapacity) ? 1000000 : player->getFreeCapacity());

	msg.add<uint32_t>(std::min<uint32_t>(player->getExperience(), std::numeric_limits<int32_t>::max()));

	msg.add<uint16_t>(static_cast<uint16_t>(player->getLevel()));
	msg.addByte(player->getLevelPercent());

	msg.add<uint16_t>(
	    static_cast<uint16_t>(std::min<uint32_t>(player->getMana(), std::numeric_limits<uint16_t>::max())));
	msg.add<uint16_t>(
	    static_cast<uint16_t>(std::min<uint32_t>(player->getMaxMana(), std::numeric_limits<uint16_t>::max())));

	msg.addByte(static_cast<uint8_t>(std::min<uint32_t>(player->getMagicLevel(), std::numeric_limits<uint8_t>::max())));
	if (isOTCv8) {
		msg.addByte(
		    static_cast<uint8_t>(std::min<uint32_t>(player->getBaseMagicLevel(), std::numeric_limits<uint8_t>::max())));
	}
	msg.addByte(static_cast<uint8_t>(player->getMagicLevelPercent()));

	msg.addByte(player->getSoul());

	msg.add<uint16_t>(player->getStaminaMinutes());

	if (isOTCv8) {
		msg.add<uint16_t>(player->getBaseSpeed() / 2);
	}

	/*msg.add<uint16_t>(player->getBaseSpeed() / 2);

	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	msg.add<uint16_t>(condition ? condition->getTicks() / 1000 : 0x00);

	msg.add<uint16_t>(player->getOfflineTrainingTime() / 60 / 1000);

	msg.add<uint16_t>(0); // xp boost time (seconds)
	msg.addByte(0); // enables exp boost in the store
	*/
}

void ProtocolGame::AddPlayerSkills(NetworkMessage& msg)
{
	msg.addByte(0xA1);

	if (!isOTCv8) {
		for (uint8_t i = SKILL_FIRST; i <= SKILL_LAST; ++i) {
			msg.addByte(
			    std::min<uint8_t>(static_cast<uint8_t>(player->getSkillLevel(i)), std::numeric_limits<uint8_t>::max()));
			msg.addByte(static_cast<uint8_t>(player->getSkillPercent(i)));
		}
	} else {
		for (uint8_t i = SKILL_FIRST; i <= SKILL_LAST; ++i) {
			msg.add<uint16_t>(std::min<uint16_t>(player->getSkillLevel(i), std::numeric_limits<uint16_t>::max()));
			msg.add<uint16_t>(player->getBaseSkill(i));
			msg.addByte(static_cast<uint8_t>(player->getSkillPercent(i)));
		}

		for (uint8_t i = SPECIALSKILL_FIRST; i <= SPECIALSKILL_LAST; ++i) {
			msg.add<uint16_t>(static_cast<uint16_t>(std::min<int32_t>(100, player->varSpecialSkills[i])));
			msg.add<uint16_t>(0);
		}
	}
}

void ProtocolGame::AddOutfit(NetworkMessage& msg, const Outfit_t& outfit)
{
	uint16_t lookType = outfit.lookType;
	if (!isOTCv8 && lookType >= 367) {
		lookType = 128;
	}

	msg.add<uint16_t>(lookType);

	if (lookType != 0) {
		msg.addByte(outfit.lookHead);
		msg.addByte(outfit.lookBody);
		msg.addByte(outfit.lookLegs);
		msg.addByte(outfit.lookFeet);
		msg.addByte(outfit.lookAddons);
	} else {
		msg.addItemId(outfit.lookTypeEx, isOTCv8);
	}

	if (isOTCv8) {
		msg.add<uint16_t>(outfit.lookMount);
	}
}

void ProtocolGame::AddWorldLight(NetworkMessage& msg, LightInfo lightInfo)
{
	msg.addByte(0x82);
	msg.addByte((player->isAccessPlayer() ? 0xFF : lightInfo.level));
	msg.addByte(lightInfo.color);
}

void ProtocolGame::AddCreatureLight(NetworkMessage& msg, const Creature* creature)
{
	LightInfo lightInfo = creature->getCreatureLight();

	msg.addByte(0x8D);
	msg.add<uint32_t>(creature->getID());
	msg.addByte((player->isAccessPlayer() ? 0xFF : lightInfo.level));
	msg.addByte(lightInfo.color);
}

// tile
void ProtocolGame::RemoveTileThing(NetworkMessage& msg, const Position& pos, uint32_t stackpos)
{
	if (stackpos >= MAX_STACKPOS_THINGS) {
		return;
	}

	msg.addByte(0x6C);
	msg.addPosition(pos);
	msg.addByte(static_cast<uint8_t>(stackpos));
}

void ProtocolGame::MoveUpCreature(NetworkMessage& msg, const Creature* creature, const Position& newPos,
                                  const Position& oldPos)
{
	if (creature != player) {
		return;
	}

	// floor change up
	msg.addByte(0xBE);

	// going to surface
	if (newPos.z == 7) {
		int32_t skip = -1;

		// floor 7 and 6 already set
		for (int i = 5; i >= 0; --i) {
			GetFloorDescription(msg, oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY, i,
			                    (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, 8 - i, skip);
		}
		if (skip >= 0) {
			msg.addByte(static_cast<uint8_t>(skip));
			msg.addByte(0xFF);
		}
	}
	// underground, going one floor up (still underground)
	else if (newPos.z > 7) {
		int32_t skip = -1;
		GetFloorDescription(msg, oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY,
		                    oldPos.getZ() - 3, (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, 3,
		                    skip);

		if (skip >= 0) {
			msg.addByte(static_cast<uint8_t>(skip));
			msg.addByte(0xFF);
		}
	}

	// moving up a floor up makes us out of sync
	// west
	msg.addByte(0x68);
	GetMapDescription(oldPos.x - Map::maxClientViewportX, oldPos.y - (Map::maxClientViewportY - 1), newPos.z, 1,
	                  (Map::maxClientViewportY * 2) + 2, msg);

	// north
	msg.addByte(0x65);
	GetMapDescription(oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY, newPos.z,
	                  (Map::maxClientViewportX * 2) + 2, 1, msg);
}

void ProtocolGame::MoveDownCreature(NetworkMessage& msg, const Creature* creature, const Position& newPos,
                                    const Position& oldPos)
{
	if (creature != player) {
		return;
	}

	// floor change down
	msg.addByte(0xBF);

	// going from surface to underground
	if (newPos.z == 8) {
		int32_t skip = -1;

		for (int i = 0; i < 3; ++i) {
			GetFloorDescription(msg, oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY,
			                    newPos.z + i, (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2,
			                    -i - 1, skip);
		}
		if (skip >= 0) {
			msg.addByte(static_cast<uint8_t>(skip));
			msg.addByte(0xFF);
		}
	}
	// going further down
	else if (newPos.z > oldPos.z && newPos.z > 8 && newPos.z < 14) {
		int32_t skip = -1;
		GetFloorDescription(msg, oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY, newPos.z + 2,
		                    (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, -3, skip);

		if (skip >= 0) {
			msg.addByte(static_cast<uint8_t>(skip));
			msg.addByte(0xFF);
		}
	}

	// moving down a floor makes us out of sync
	// east
	msg.addByte(0x66);
	GetMapDescription(oldPos.x + (Map::maxClientViewportX + 1), oldPos.y - (Map::maxClientViewportY + 1), newPos.z, 1,
	                  (Map::maxClientViewportY * 2) + 2, msg);

	// south
	msg.addByte(0x67);
	GetMapDescription(oldPos.x - Map::maxClientViewportX, oldPos.y + (Map::maxClientViewportY + 1), newPos.z,
	                  (Map::maxClientViewportX * 2) + 2, 1, msg);
}

void ProtocolGame::AddShopItem(NetworkMessage& msg, const ShopInfo& item)
{
	const ItemType& it = Item::items[item.itemId];
	msg.add<uint16_t>(it.clientId);

	if (it.isSplash() || it.isFluidContainer()) {
		msg.addByte(serverFluidToClient(static_cast<uint8_t>(item.subType)));
	} else {
		msg.addByte(0x00);
	}

	msg.addString(item.realName);
	msg.add<uint32_t>(it.weight);
	msg.add<uint32_t>(item.buyPrice);
	msg.add<uint32_t>(item.sellPrice);
}

void ProtocolGame::parseExtendedOpcode(NetworkMessage& msg)
{
	uint8_t opcode = msg.getByte();
	auto buffer = msg.getString();

	// process additional opcodes via lua script event
	g_dispatcher.addTask([=, playerID = player->getID(), buffer = std::string{buffer}]() {
		g_game.parsePlayerExtendedOpcode(playerID, opcode, buffer);
	});
}

void ProtocolGame::sendOTCv8Features()
{
	const auto& features = ConfigManager::getOTCFeatures();

	auto msg = getOutputBuffer(1024);
	msg->addByte(0x43);
	msg->add<uint16_t>(features.size());
	for (uint8_t feature : features) {
		msg->addByte(feature);
		msg->addByte(0x01);
	}

	send(std::move(getCurrentBuffer()));
}
