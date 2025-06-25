#include "PreCompile.h"
#include "TestSupport.h"
#include "NetServerSerializeBuffer.h"
#include "Session.h"
#include "NonNetworkActor.h"

#if ENABLE_TEST_SUPPORT

namespace CoreTestSupport
{
    void TestInterface::RegisterSession(Session* session)
    {
        if (session != nullptr)
        {
            testSessionList.push_back(session);
        }
    }

    void TestInterface::UnregisterSession(const Session* session)
    {
        if (session == nullptr)
        {
            return;
        }

        if (const auto itor = std::ranges::find(testSessionList, session); itor != testSessionList.end())
        {
            testSessionList.erase(itor);
        }
    }

    void TestInterface::RegisterNonNetworkActor(NonNetworkActor* actor)
    {
        if (actor != nullptr)
        {
            testNonNetworkActorList.push_back(actor);
        }
    }

    void TestInterface::UnregisterNonNetworkActor(NonNetworkActor* actor)
    {
        if (actor == nullptr)
        {
            return;
        }

        if (const auto itor = std::ranges::find(testNonNetworkActorList, actor); itor != testNonNetworkActorList.end())
        {
            testNonNetworkActorList.erase(itor);
        }
    }

    void TestInterface::SendPacketForTest(Session& session, IPacket& packet)
    {
        NetBuffer* testBuffer = Session::InjectPacketForTest(packet);
        const auto msgOpt = session.CreateMessageFromPacket(*testBuffer);

		NetBuffer::Free(testBuffer);
        if (not msgOpt.has_value())
        {
            std::cout << "[TestInterface] CreateMessageFromPacket failed.\n";
            return;
        }

        try
        {
            msgOpt.value()();
        }
        catch (const std::exception& e)
        {
            std::cout << "[TestInterface] Handler threw exception : " << e.what() << '\n';
        }
    }

	void TestInterface::Tick()
	{
        PreTimerForTest();
		OnTimerForTest();
		PostTimerForTest();
	}

	void TestInterface::TearDown()
	{
		testSessionList.clear();
		testNonNetworkActorList.clear();
	}

	void TestInterface::PreTimerForTest()
	{
		for (const auto& session : testSessionList)
		{
			session->PreTimerForTest();
		}
		for (const auto& actor : testNonNetworkActorList)
		{
			actor->PreTimerForTest();
		}
	}

	void TestInterface::OnTimerForTest()
	{
		for (const auto& session : testSessionList)
		{
			session->OnTimerForTest();
		}
		for (const auto& actor : testNonNetworkActorList)
		{
			actor->OnTimerForTest();
		}
	}

	void TestInterface::PostTimerForTest()
	{
		for (const auto& session : testSessionList)
		{
			session->PostTimerForTest();
		}
		for (const auto& actor : testNonNetworkActorList)
		{
			actor->PostTimerForTest();
		}
	}
}

#endif