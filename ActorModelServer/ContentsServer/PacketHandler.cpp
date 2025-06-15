#include "PreCompile.h"
#include "Player.h"

void Player::HandlePing()
{
	Pong pong;

	SendPacket(pong);
}