#pragma once
#include "Session.h"

using PlayerIdType = unsigned long long;

class Player final : public Session
{
public:
	Player() = delete;
	Player(SessionIdType inSessionIdType, const SOCKET& inSock, ThreadIdType inThreadId);
	~Player() override = default;

public:
	void OnActorCreated() override;
	void OnActorDestroyed() override;

private:
	void RegisterAllPacketHandler();

#pragma region Packet Handlers
public:
	void HandlePing();

#pragma endregion Packet Handlers

#pragma region Mediator Handlers
public:
	void PrepareItemForTrade(int64_t itemId);
	void PrepareMoneyForTrade(int64_t money);

#pragma endregion Mediator Handlers

#pragma region Message Handlers
public:
	void HandleTestMessage(int32_t value, PlayerIdType requestPlayerId);

#pragma endregion Message Handlers

public:
	[[nodiscard]]
	PlayerIdType GetPlayerId() const { return playerId; }
	void SetPlayerId(const PlayerIdType inPlayerId) { playerId = inPlayerId; }

private:
	PlayerIdType playerId{};
};
