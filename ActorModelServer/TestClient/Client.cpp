#include "PreCompile.h"
#include "Client.h"

Client& Client::GetInst()
{
	static Client instance;
	return instance;
}

bool Client::Start(const std::wstring& optionFilePath)
{
	logicThread = std::jthread(&Client::RunLogicThread, this);
	return SimpleClient::Start(optionFilePath);
}

void Client::Stop()
{
	SimpleClient::Stop();
	if (logicThread.joinable())
	{
		logicThread.join();
	}
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