#include "PreCompile.h"
#include "TestSupport.h"
#include "NetServerSerializeBuffer.h"
#include "CoreType.h"
#include "Session.h"

namespace CoreTestSupport
{
    void TestSessionInterface::RegisterSession(Session* session)
    {
        if (session == nullptr)
        {
            return;
        }

        testSessionList.push_back(session);
    }

    void TestSessionInterface::InjectPacketForTest(Session& session, const unsigned int packetId, IPacket& packet)
    {
        NetBuffer* testBuffer = Session::InjectPacketForTest(packet);
        const auto msgOpt = session.CreateMessageFromPacket(*testBuffer);

		NetBuffer::Free(testBuffer);
        if (not msgOpt.has_value())
        {
            std::cerr << "[TestSessionInterface] CreateMessageFromPacket failed.\n";
            return;
        }

		const auto message = msgOpt.value();
        try
        {
            msgOpt.value()();
        }
        catch (const std::exception& e)
        {
            std::cerr << "[TestSessionInterface] Handler threw exception : " << e.what() << '\n';
        }
    }

	void TestSessionInterface::Tick()
	{
		for (const auto& session : testSessionList)
		{
			session->PreTimerForTest();
		}

    	for (const auto& session : testSessionList)
		{
			session->OnTimerForTest();
		}

		for (const auto& session : testSessionList)
		{
			session->PostTimerForTest();
		}
	}
}
