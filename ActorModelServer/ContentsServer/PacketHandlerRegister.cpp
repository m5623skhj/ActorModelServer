#include "PreCompile.h"
#include "Player.h"

Player::Player(const SOCKET& inSock)
	: Session(inSock)
{
	RegisterAllPacketHandler();
}

void Player::RegisterAllPacketHandler()
{
	RegisterPacketHandler(PACKET_ID::PING, &Player::HandlePing);
}
