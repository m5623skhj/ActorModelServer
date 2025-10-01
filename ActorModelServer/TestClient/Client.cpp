#include "PreCompile.h"
#include "Client.h"
#include "../ContentsServer/Protocol.h"

Client& Client::GetInst()
{
	static Client instance;
	return instance;
}

bool Client::StartClient(const std::wstring& optionFilePath)
{
	logicThread = std::jthread(&Client::RunLogicThread, this);
	return Start(optionFilePath);
}

void Client::StopClient()
{
	Stop();
	if (logicThread.joinable())
	{
		logicThread.join();
	}
}

void Client::SendPacket(IPacket& packet)
{
	NetBuffer& buffer = *NetBuffer::Alloc();
	PACKET_ID packetId = packet.GetPacketId();
	buffer << packetId;
	char* targetPtr = reinterpret_cast<char*>(&packet + sizeof(char*));
	buffer.WriteBuffer(targetPtr, packet.GetPacketSize());

	SimpleClient::SendPacket(&buffer);
}

void Client::OnConnected()
{
	
}

void Client::OnDisconnected()
{
	
}

void Client::RunLogicThread()
{
	while (not IsNeedStop())
	{
		if (const int recvBufferSize = GetRecvBufferSize(); recvBufferSize > 0)
		{
			for (int i = 0; i < recvBufferSize; ++i)
			{
				NetBuffer* buffer = GetRecvBuffer();
				if (buffer == nullptr)
				{
					continue;
				}

				ProcessLogic(*buffer);
				NetBuffer::Free(buffer);
			}
		}

		Sleep(33);
	}
}