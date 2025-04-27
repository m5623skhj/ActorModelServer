#pragma once
#include "CoreType.h"
#include <unordered_map>

using PacketId = unsigned int;

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

private:
};