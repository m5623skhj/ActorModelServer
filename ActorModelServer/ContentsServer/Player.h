#pragma once
#include "Session.h"

class Player final : public Session
{
public:
	Player() = delete;
	Player(const SessionIdType inSessionIdType, const SOCKET& inSock, const ThreadIdType inThreadId);
	~Player() override = default;

private:
	void RegisterAllPacketHandler();

public:
	void HandlePing();
};
