#include "PreCompile.h"
#include "Client.h"
#include "../ContentsServer/Protocol.h"
#include "NetServerSerializeBuffer.h"

#include "Logger.h"
#include "LogExtension.h"
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
		case PACKET_ID::ADD_ACTOR:
		{
			AddActor(buffer);
		}
		break;
		default:
		{
			const std::string logString = "[Client] Unknown packet received. Packet ID: " + std::to_string(static_cast<unsigned int>(packetId));
			LOG_ERROR(logString);
		}
		break;
	}
}