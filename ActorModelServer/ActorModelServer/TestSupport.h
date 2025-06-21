#pragma once

class IPacket;
class Session;

namespace CoreTestSupport
{
	class TestSessionInterface
	{
	public:
		static void InjectPacket(Session& session, const unsigned int packetId, IPacket& packet)
;
	};
}
