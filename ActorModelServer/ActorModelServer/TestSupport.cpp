#include "PreCompile.h"
#include "TestSupport.h"
#include "NetServerSerializeBuffer.h"
#include "CoreType.h"
#include "Session.h"

namespace CoreTestSupport
{
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

        session.SendMessage(message);
        session.ProcessMessageForTest();
    }
}
