#pragma once
#include <vector>

class IPacket;
class Session;

namespace CoreTestSupport
{
	class TestSessionInterface
	{
	public:
		static void RegisterSession(Session* session);
		static void InjectPacketForTest(Session& session, const unsigned int packetId, IPacket& packet);
		static void Tick();

	private:
		static inline std::vector<Session*> testSessionList;
	};
}
