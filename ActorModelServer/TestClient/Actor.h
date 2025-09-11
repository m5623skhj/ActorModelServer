#pragma once

using ActorId = unsigned long long;

class Actor
{
public:
	explicit Actor(ActorId inActorId);
	virtual ~Actor() = default;

public:
	ActorId GetActorId() const { return actorId; }

private:
	ActorId actorId;
};