#pragma once
#include <vector>
#include "BuildConfig.h"

#if ENABLE_TEST_SUPPORT

class IPacket;
class Session;

namespace CoreTestSupport
{
	class TestSessionInterface
	{
	public:
		static void RegisterSession(Session* session);
		static void SendPacketForTest(Session& session, IPacket& packet);
		static void Tick();

	private:
		static inline std::vector<Session*> testSessionList;
	};
}

#endif