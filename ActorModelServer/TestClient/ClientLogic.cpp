#include "PreCompile.h"
#include "Client.h"
#include "../ContentsServer/Protocol.h"

void Client::ProcessLogic(NetBuffer& buffer)
{
	PACKET_ID packetId;
	buffer >> packetId;

	switch (packetId)
	{
	case PACKET_ID::PONG:
		{
			Pong(buffer);
		}
		break;
	default:
	{
		std::cout << "Unknown packet received. Packet ID: " << static_cast<unsigned int>(packetId) << '\n';
	}
	break;
	}
}

void Client::Pong(NetBuffer& packet)
{
	UNREFERENCED_PARAMETER(packet);
}