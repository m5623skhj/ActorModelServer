#include "PreCompile.h"
#include "Player.h"
#include "Client.h"

#include "../ContentsServer/Protocol.h"

void Player::OnActorCreated()
{
	Ping ping;
	Client::GetInst().SendPacket(ping);
}
