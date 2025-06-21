#pragma once
#include <vector>
#include "BuildConfig.h"

#if ENABLE_TEST_SUPPORT

class IPacket;
class Session;
class NonNetworkActor;

namespace CoreTestSupport
{
	class TestInterface
	{
	public:
		static void RegisterSession(Session* session);
		static void UnregisterSession(const Session* session);
		static void RegisterNonNetworkActor(NonNetworkActor* actor);
		static void UnregisterNonNetworkActor(NonNetworkActor* actor);
		static void SendPacketForTest(Session& session, IPacket& packet);
		static void Tick();

	private:
		static void PreTimerForTest();
		static void OnTimerForTest();
		static void PostTimerForTest();

	private:
		static inline std::vector<Session*> testSessionList;
		static inline std::vector<NonNetworkActor*> testNonNetworkActorList;
	};
}

#endif