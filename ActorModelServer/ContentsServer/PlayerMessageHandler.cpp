#include "PreCompile.h"
#include "Player.h"
#include "Logger.h"
#include "LogExtension.h"

void Player::HandleTestMessage(const int32_t value, const PlayerIdType requestPlayerId)
{
	std::string logString = "[Player] HandleTestMessage called from PlayerId: " + std::to_string(requestPlayerId) + " with value: " + std::to_string(value);
	LOG_DEBUG(logString);
}