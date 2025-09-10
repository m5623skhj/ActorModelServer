#include "PreCompile.h"
#include "Player.h"

void Player::HandleTestMessage(const int32_t value, const PlayerIdType requestPlayerId)
{
	std::cout << "[Player] Received TestMessage from PlayerId: " << requestPlayerId << " with value: " << value << '\n';
}