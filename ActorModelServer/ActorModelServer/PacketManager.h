#pragma once
#include "CoreType.h"
#include <unordered_map>
#include "../Common/Protocol.h"
#include <functional>
#include <any>
#include "Actor.h"
#include "NetServerSerializeBuffer.h"

class PacketManager
{
private:
	PacketManager() = default;
	~PacketManager() = default;

	PacketManager(const PacketManager&) = delete;
	PacketManager& operator=(const PacketManager&) = delete;

public:
	[[nodiscard]]
	static PacketManager& GetInst();

public:
	[[nodiscard]]
	Message GetMessageFromPacket(const PACKET_ID packetId) const
	{
		if (const auto it = packetHandlerMap.find(packetId); it != packetHandlerMap.end())
		{
			return it->second;
		}
		return nullptr;
	}

private:
	std::unordered_map<PACKET_ID, Message> packetHandlerMap;
};