#pragma once
#include "Actor.h"

class ServerCore;

using SessionIdType = unsigned long long;

class Session : public Actor
{
public:
	friend ServerCore;

public:
	Session() = delete;
	explicit Session(SessionIdType inSessionIdType, const SOCKET& inSock);
	~Session() override = default;

public:
	void OnConnected();
	void OnDisconnected();
	void OnRecv();

public:
	SessionIdType GetSessionId() { return sessionId; }

private:
	SOCKET sock{};
	SessionIdType sessionId{};
	bool ioCancel{};

	std::atomic_bool isUsingSession{};
};