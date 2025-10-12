#include <iostream>
#include "ServerCore.h"
#include "Player.h"
#include "Logger.h"
#include "LogExtension.h"

int main()
{
	auto playerFactoryFunc = [](SessionIdType sessionId, SOCKET sock, ThreadIdType threadId) {
		return std::make_shared<Player>(sessionId, sock, threadId);
	};

	if (not ServerCore::GetInst().StartServer(L"ServerOption.txt", std::move(playerFactoryFunc)))
	{
		LOG_ERROR("ServerCore::StartServer() failed");
		return 0;
	}

	while (ServerCore::GetInst().IsStop() == false)
	{
		system("cls");
		if (GetAsyncKeyState('Q') & 0x8000)
		{
			ServerCore::GetInst().StopServer();
			break;
		}

		std::cout << "StopServer : Q" << '\n';
		std::cout << "Current User Count: " << ServerCore::GetInst().GetUserCount() << '\n';
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	LOG_DEBUG("Server is stopping");
	return 0;
}