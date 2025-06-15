#include "PreCompile.h"
#include "Player.h"

Player::Player(const SessionIdType inSessionIdType, const SOCKET& inSock, const ThreadIdType inThreadId)
	: Session(inSessionIdType, inSock, inThreadId)
{
	RegisterAllPacketHandler();
}

void Player::RegisterAllPacketHandler()
{
	RegisterPacketHandler(PACKET_ID::PING, &Player::HandlePing);
}
