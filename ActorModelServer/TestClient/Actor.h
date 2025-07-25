#pragma once

using ActorId = unsigned long long;
class NetBuffer;

class Actor
{
public:
	explicit Actor(ActorId inActorId);
	virtual ~Actor() = default;

public:
	virtual void OnRecv(NetBuffer& buffer) = 0;

private:
	ActorId actorId;
};