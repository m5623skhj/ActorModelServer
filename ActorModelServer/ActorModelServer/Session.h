#pragma once
#include "Actor.h"

using SessionIdType = unsigned long long;

class Session : public Actor
{
public:
	Session() = delete;
	explicit Session(SessionIdType inSessionIdType);
	~Session() override = default;

public:
	SessionIdType GetSessionId() { return sessionId; }

private:
	SessionIdType sessionId{};
};