#pragma once
#include "Actor.h"

using PlayerIdType = unsigned long long;

class Player : public Actor
{
public:
	explicit Player(const ActorId inActorId) : Actor(inActorId) {}
	~Player() override = default;

public:
	[[nodiscard]]
	PlayerIdType GetPlayerId() const { return playerId; }
	void SetPlayerId(const PlayerIdType inPlayerId) { playerId = inPlayerId; }

	void OnActorCreated() override;
	void OnActorDestroyed() override;

private:
	PlayerIdType playerId{};
};