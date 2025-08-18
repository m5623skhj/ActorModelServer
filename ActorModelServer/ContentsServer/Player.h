#pragma once
#include "Session.h"

class Player final : public Session
{
public:
	Player() = delete;
	Player(SessionIdType inSessionIdType, const SOCKET& inSock, ThreadIdType inThreadId);
	~Player() override = default;

private:
	void RegisterAllPacketHandler();

public:
	void HandlePing();
};
