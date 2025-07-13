#pragma once

enum class PACKET_ID : unsigned int
{
	INVALID_PACKET = 0,
	PING = 1,
	PONG = 2,
};

#define GET_PACKET_SIZE() virtual int GetPacketSize() override { return sizeof(*this) - 8; }
#define GET_PACKET_ID(packetId) virtual PACKET_ID GetPacketId() const override { return static_cast<PACKET_ID>(packetId); }

#pragma pack(push, 1)
class IPacket
{
public:
	IPacket() = default;
	virtual ~IPacket() = default;

	[[nodiscard]]
	virtual PACKET_ID GetPacketId() const = 0;
	[[nodiscard]]
	virtual int GetPacketSize() = 0;
};

class Ping : public IPacket
{
public:
	Ping() = default;
	~Ping() override;
	GET_PACKET_ID(PACKET_ID::PING)
	GET_PACKET_SIZE()
};

inline Ping::~Ping()
{
}

class Pong : public IPacket
{
public:
	Pong() = default;
	~Pong() override = default;
	GET_PACKET_ID(PACKET_ID::PONG)
	GET_PACKET_SIZE()
};

#pragma pack(pop)